/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dispatcher.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>


Dispatcher_Socket::Dispatcher_Socket
    (const std::string& dispatcher_share_name,
     const std::string& shadow_name_,
     const std::string& db_dir_,
     uint max_num_reading_processes)
  : socket("", max_num_reading_processes)
{
  signal(SIGPIPE, SIG_IGN);

  std::string db_dir = db_dir_;
  // get the absolute pathname of the current directory
  if (db_dir.substr(0, 1) != "/")
    db_dir = getcwd() + db_dir_;

  // initialize the socket for the server
  socket_name = db_dir + dispatcher_share_name;
  socket.open(socket_name);
}


Dispatcher_Socket::~Dispatcher_Socket()
{
  remove(socket_name.c_str());
}


void Dispatcher_Socket::look_for_a_new_connection(Connection_Per_Pid_Map& connection_per_pid)
{
  struct sockaddr_un sockaddr_un_dummy;
  uint sockaddr_un_dummy_size = sizeof(sockaddr_un_dummy);
  int socket_fd = accept(socket.descriptor(), (sockaddr*)&sockaddr_un_dummy,
			 (socklen_t*)&sockaddr_un_dummy_size);
  if (socket_fd == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EMFILE)
      throw File_Error
	    (errno, "(socket)", "Dispatcher_Server::6");
  }
  else
  {
    if (fcntl(socket_fd, F_SETFL, O_RDWR|O_NONBLOCK) == -1)
      throw File_Error
	    (errno, "(socket)", "Dispatcher_Server::7");
    started_connections.push_back(socket_fd);
  }

  // associate to a new connection the pid of the sender
  for (std::vector< int >::iterator it = started_connections.begin();
      it != started_connections.end(); ++it)
  {
    pid_t pid;
    int bytes_read = recv(*it, &pid, sizeof(pid_t), 0);
    if (bytes_read == -1)
      ;
    else
    {
      if (bytes_read != 0)
	connection_per_pid.set(pid, new Blocking_Client_Socket(*it));
      else
	close(*it);

      *it = started_connections.back();
      started_connections.pop_back();
      break;
    }
  }
}


int Global_Resource_Planner::probe(pid_t pid, const std::string& client_token, uint32 time_units, uint64 max_space)
{
  uint32 cur_time = time(0);

  if (rate_limit > 0 && !client_token.empty())
  {
    std::map< std::string, std::vector< Pending_Client > >::iterator pending_it =
        pending.insert(std::make_pair(client_token, std::vector< Pending_Client >())).first;
    Pending_Client* handle = 0;

    // Reuse an existing handle in pending or create one
    for (std::vector< Pending_Client >::iterator handle_it = pending_it->second.begin();
        handle_it != pending_it->second.end(); ++handle_it)
    {
      if (handle_it->pid == pid)
        handle = &*handle_it;
    }
    if (!handle)
    {
      if (pending_it->second.size() > 2*rate_limit)
        return Dispatcher::RATE_LIMITED;

      pending_it->second.push_back(Pending_Client(pid, cur_time));
      handle = &pending_it->second.back();
    }

    // Simple checks: is the query acceptable from a global point of view?
    if (global_available_time < global_used_time ||
        time_units > (global_available_time - global_used_time)/2 ||
        global_available_space < global_used_space ||
        max_space > (global_available_space - global_used_space)/2)
    {
      if (cur_time - handle->first_seen < 15)
        return 0;
      else
        return Dispatcher::QUERY_REJECTED;
    }

    // Enforce the rate limit: first count active requests ...
    uint32 token_count = 0;
    for (std::vector< Reader_Entry >::const_iterator it = active.begin(); it != active.end(); ++it)
    {
      if (it->client_token == client_token)
	++token_count;
    }
    if (token_count >= rate_limit)
    {
      if (cur_time - handle->first_seen < 15)
        return 0;
      else
      {
        *handle = pending_it->second.back();
        pending_it->second.pop_back();
        if (pending_it->second.empty())
          pending.erase(pending_it);
        return Dispatcher::RATE_LIMITED;
      }
    }

    // ... then the afterwards quota
    uint32 current_time = time(0);
    for (std::vector< Quota_Entry >::iterator it = afterwards.begin(); it != afterwards.end(); )
    {
      if (it->expiration_time < current_time)
      {
	*it = afterwards.back();
	afterwards.pop_back();
      }
      else
      {
	if (it->client_token == client_token)
	  ++token_count;
        ++it;
      }
    }
    if (token_count >= rate_limit)
    {
      if (cur_time - handle->first_seen < 15)
        return 0;
      else
      {
        *handle = pending_it->second.back();
        pending_it->second.pop_back();
        if (pending_it->second.empty())
          pending.erase(pending_it);
        return Dispatcher::RATE_LIMITED;
      }
    }

    // Remove found handle from pending
    if (handle)
    {
      *handle = pending_it->second.back();
      pending_it->second.pop_back();
      if (pending_it->second.empty())
        pending.erase(pending_it);
    }
  }
  else
  {
    // Simple checks: is the query acceptable from a global point of view?
    if (global_available_time < global_used_time ||
        time_units > (global_available_time - global_used_time)/2 ||
        global_available_space < global_used_space ||
        max_space > (global_available_space - global_used_space)/2)
      return 0;
  }

  active.push_back(Reader_Entry(pid, max_space, time_units, client_token, time(0)));

  global_used_space += max_space;
  global_used_time += time_units;
  return Dispatcher::REQUEST_READ_AND_IDX;
}


void Global_Resource_Planner::remove_entry(std::vector< Reader_Entry >::iterator& it)
{
  uint32 end_time = time(0);
  if (last_update_time < end_time && last_counted > 0)
  {
    if (end_time - last_update_time < 15)
    {
      for (uint32 i = last_update_time; i < end_time; ++i)
      {
	recent_average_used_space[i % 15] = last_used_space / last_counted;
	recent_average_used_time[i % 15] = last_used_time / last_counted;
      }
    }
    else
    {
      for (uint32 i = 0; i < 15; ++i)
      {
	recent_average_used_space[i] = last_used_space / last_counted;
	recent_average_used_time[i] = last_used_time / last_counted;
      }
    }

    average_used_space = 0;
    average_used_time = 0;
    for (uint32 i = 0; i < 15; ++i)
    {
      average_used_space += recent_average_used_space[i];
      average_used_time += recent_average_used_time[i];
    }
    average_used_space = average_used_space / 15;
    average_used_time = average_used_time / 15;

    last_used_space = 0;
    last_used_time = 0;
    last_counted = 0;
    last_update_time = end_time;
  }
  last_used_space += global_used_space;
  last_used_time += global_used_time;
  ++last_counted;

  // Adjust global counters
  global_used_space -= it->max_space;
  global_used_time -= it->max_time;

  if (rate_limit > 0 && !it->client_token.empty())
  {
    // Calculate afterwards blocking time
    uint32 penalty_time =
      std::max(global_available_space * (end_time - it->start_time + 1)
          / (global_available_space - average_used_space),
	  uint64(global_available_time) * (end_time - it->start_time + 1)
	  / (global_available_time - average_used_time))
      - (end_time - it->start_time + 1);
    afterwards.push_back(Quota_Entry(it->client_token, penalty_time + end_time));
  }

  // Really remove the element
  *it = active.back();
  active.pop_back();
}


void Global_Resource_Planner::remove(pid_t pid)
{
  bool was_active = false;

  for (std::vector< Reader_Entry >::iterator it = active.begin(); it != active.end(); ++it)
  {
    if (it->client_pid == pid)
    {
      remove_entry(it);
      was_active = true;
      break;
    }
  }

  if (!was_active)
  {
    for (std::map< std::string, std::vector< Pending_Client > >::iterator pending_it = pending.begin();
        pending_it != pending.end(); )
    {
      bool found = false;
      for (std::vector< Pending_Client >::iterator handle_it = pending_it->second.begin();
          handle_it != pending_it->second.end(); )
      {
        if (handle_it->pid == pid)
        {
          *handle_it = pending_it->second.back();
          pending_it->second.pop_back();
          found = true;
          break;
        }
        else
          ++handle_it;
      }

      if (found && pending_it->second.empty())
      {
        std::string client = pending_it->first;
        pending.erase(pending_it);
        pending_it = pending.lower_bound(client);
      }
      else
        ++pending_it;
    }
  }
}


void Global_Resource_Planner::purge(Connection_Per_Pid_Map& connection_per_pid)
{
  for (std::vector< Reader_Entry >::iterator it = active.begin(); it != active.end(); )
  {
    if (connection_per_pid.get(it->client_pid) == 0)
      remove_entry(it);
    else
      ++it;
  }

  for (std::map< std::string, std::vector< Pending_Client > >::iterator pending_it = pending.begin();
      pending_it != pending.end(); )
  {
    for (std::vector< Pending_Client >::iterator handle_it = pending_it->second.begin();
        handle_it != pending_it->second.end(); )
    {
      if (connection_per_pid.get(handle_it->pid) == 0)
      {
        *handle_it = pending_it->second.back();
        pending_it->second.pop_back();
      }
      else
        ++handle_it;
    }

    if (pending_it->second.empty())
    {
      std::string client = pending_it->first;
      pending.erase(pending_it);
      pending_it = pending.lower_bound(client);
    }
    else
      ++pending_it;
  }
}


Dispatcher::Dispatcher
    (std::string dispatcher_share_name_,
     std::string index_share_name,
     std::string shadow_name_,
     bool block_writing_after_rollback_,
     std::string db_dir_,
     uint max_num_reading_processes_,
     uint64 total_available_space_,
     uint64 total_available_time_units_,
     const std::vector< File_Properties* >& controlled_files_,
     Dispatcher_Logger* logger_,
     Dispatcher_Status_Output* status_output_)
    : socket(dispatcher_share_name_, shadow_name_, db_dir_, max_num_reading_processes_),
      transaction_insulator(db_dir_, controlled_files_),
      shadow_name(shadow_name_),
      dispatcher_share_name(dispatcher_share_name_),
      block_writing_after_rollback(block_writing_after_rollback_),
      logger(logger_),
      status_output(status_output_),
      pending_commit(false),
      writing_client(0),
      requests_started_counter(0),
      requests_finished_counter(0),
      global_resource_planner(total_available_time_units_, total_available_space_, 0)
{
  signal(SIGPIPE, SIG_IGN);

  if (shadow_name.substr(0, 1) != "/")
    shadow_name = getcwd() + shadow_name_;

  // open dispatcher_share
#ifdef __APPLE__
  dispatcher_shm_fd = shm_open
      (dispatcher_share_name.c_str(), O_RDWR|O_CREAT, S_666);
  if (dispatcher_shm_fd < 0)
    throw File_Error
        (errno, dispatcher_share_name, "Dispatcher_Server::APPLE::1");
#else
  dispatcher_shm_fd = shm_open
      (dispatcher_share_name.c_str(), O_RDWR|O_CREAT|O_TRUNC|O_EXCL, S_666);
  if (dispatcher_shm_fd < 0)
    throw File_Error
        (errno, dispatcher_share_name, "Dispatcher_Server::1");
  fchmod(dispatcher_shm_fd, S_666);
#endif

  std::string db_dir = transaction_insulator.db_dir();
  int foo = ftruncate(dispatcher_shm_fd,
		      SHM_SIZE + db_dir.size() + shadow_name.size()); foo = foo;
  dispatcher_shm_ptr = (uint8*)mmap
        (0, SHM_SIZE + db_dir.size() + shadow_name.size(),
         PROT_READ|PROT_WRITE, MAP_SHARED, dispatcher_shm_fd, 0);

  // copy db_dir and shadow_name
  *(uint32*)(dispatcher_shm_ptr + 3*sizeof(uint32)) = db_dir.size();
  memcpy((uint8*)dispatcher_shm_ptr + 4*sizeof(uint32), db_dir.data(), db_dir.size());
  *(uint32*)(dispatcher_shm_ptr + 4*sizeof(uint32) + db_dir.size())
      = shadow_name.size();
  memcpy((uint8*)dispatcher_shm_ptr + 5*sizeof(uint32) + db_dir.size(),
      shadow_name.data(), shadow_name.size());

  // Set command state to zero.
  *(uint32*)dispatcher_shm_ptr = 0;

  if (file_exists(shadow_name))
  {
    transaction_insulator.copy_shadows_to_mains();
    remove(shadow_name.c_str());
  }
  transaction_insulator.remove_shadows();
  remove((shadow_name + ".lock").c_str());
  transaction_insulator.set_current_footprints();
}


Dispatcher::~Dispatcher()
{
  munmap((void*)dispatcher_shm_ptr, SHM_SIZE + transaction_insulator.db_dir().size() + shadow_name.size());
  shm_unlink(dispatcher_share_name.c_str());
}


void Dispatcher::write_start(pid_t pid)
{
  // Lock the writing lock file for the client.
  try
  {
    Raw_File shadow_file(shadow_name + ".lock", O_RDWR|O_CREAT|O_EXCL, S_666, "write_start:1");

    transaction_insulator.copy_mains_to_shadows();
    transaction_insulator.write_index_of_empty_blocks();
    if (logger)
    {
      std::set< ::pid_t > registered = transaction_insulator.registered_pids();
      std::vector< Dispatcher_Logger::pid_t > registered_v;
      registered_v.assign(registered.begin(), registered.end());
      logger->write_start(pid, registered_v);
    }
  }
  catch (File_Error e)
  {
    if ((e.error_number == EEXIST) && (e.filename == (shadow_name + ".lock")))
    {
      pid_t locked_pid;
      std::ifstream lock((shadow_name + ".lock").c_str());
      lock>>locked_pid;
      if (locked_pid == pid)
        return;
      if (logger)
        logger->write_conflict(pid, locked_pid);
      else
        std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    return;
  }

  try
  {
    writing_client = pid;
    std::ofstream lock((shadow_name + ".lock").c_str());
    lock<<pid;
  }
  catch (...) {}
}


void Dispatcher::write_rollback(pid_t pid)
{
  writing_client = 0;
  if (logger)
    logger->write_rollback(pid);
  transaction_insulator.remove_shadows();
  remove((shadow_name + ".lock").c_str());
}


void Dispatcher::write_commit(pid_t pid)
{
  if (!processes_reading_idx.empty())
  {
    pending_commit = true;
    return;
  }
  pending_commit = false;
  writing_client = 0;

  if (logger)
    logger->write_commit(pid);
  try
  {
    Raw_File shadow_file(shadow_name, O_RDWR|O_CREAT|O_EXCL, S_666, "write_commit:1");

    transaction_insulator.copy_shadows_to_mains();
  }
  catch (File_Error e)
  {
    std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    return;
  }

  remove(shadow_name.c_str());
  transaction_insulator.remove_shadows();
  remove((shadow_name + ".lock").c_str());

  read_finished(pid);
  transaction_insulator.set_current_footprints();
}


void Dispatcher::request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space,
				      const std::string& client_token)
{
  if (logger)
    logger->request_read_and_idx(pid, max_allowed_time, max_allowed_space, client_token);
  ++requests_started_counter;

  transaction_insulator.request_read_and_idx(pid);

  processes_reading_idx.insert(pid);
}


void Dispatcher::read_idx_finished(pid_t pid)
{
  if (logger)
    logger->read_idx_finished(pid);
  processes_reading_idx.erase(pid);
}


void Dispatcher::read_finished(pid_t pid)
{
  if (logger)
    logger->read_finished(pid);
  ++requests_finished_counter;

  transaction_insulator.read_finished(pid);

  processes_reading_idx.erase(pid);
  disconnected.erase(pid);
  global_resource_planner.remove(pid);
}


void Dispatcher::read_aborted(pid_t pid)
{
  if (logger)
    logger->read_aborted(pid);

  if (!block_writing_after_rollback && (pid_t)writing_client == pid)
    write_rollback(pid);

  transaction_insulator.read_finished(pid);

  processes_reading_idx.erase(pid);
  disconnected.erase(pid);
  global_resource_planner.remove(pid);
}


void Dispatcher::standby_loop(uint64 milliseconds)
{
  uint32 counter = 0;
  uint32 idle_counter = 0;
  while ((milliseconds == 0) || (counter < milliseconds/100))
  {
    socket.look_for_a_new_connection(connection_per_pid);

    uint32 command = 0;
    uint32 client_pid = 0;
    connection_per_pid.poll_command_round_robin(command, client_pid);

    if (command == HANGUP)
      command = READ_ABORTED;

    if (command == 0)
    {
      ++counter;
      ++idle_counter;
      millisleep(idle_counter < 10 ? idle_counter*10 : 100);
      continue;
    }

    if (idle_counter > 0)
    {
      if (logger)
	logger->idle_counter(idle_counter);
      idle_counter = 0;
    }

    try
    {
      if (command == TERMINATE || command == OUTPUT_STATUS)
      {
	if (command == OUTPUT_STATUS)
	  output_status();

	connection_per_pid.get(client_pid)->send_result(command);
	connection_per_pid.set(client_pid, 0);

	if (command == TERMINATE)
	  break;
      }
      else if (command == WRITE_START || command == WRITE_ROLLBACK || command == WRITE_COMMIT)
      {
	if (command == WRITE_START)
	{
	  global_resource_planner.purge(connection_per_pid);
	  write_start(client_pid);
	}
	else if (command == WRITE_COMMIT)
	{
	  global_resource_planner.purge(connection_per_pid);
          if (logger)
            logger->write_commit(client_pid);
	  write_commit(client_pid);
	}
	else if (command == WRITE_ROLLBACK)
	  write_rollback(client_pid);

	connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == HANGUP || command == READ_ABORTED || command == READ_FINISHED)
      {
	if (command == READ_ABORTED)
	  read_aborted(client_pid);
	else if (command == READ_FINISHED)
	{
	  read_finished(client_pid);
	  connection_per_pid.get(client_pid)->send_result(command);
	}
	connection_per_pid.set(client_pid, 0);
      }
      else if (command == READ_IDX_FINISHED)
      {
	read_idx_finished(client_pid);
        if (connection_per_pid.get(client_pid) != 0)
	  connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == REQUEST_READ_AND_IDX)
      {
	std::vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(3);
	if (arguments.size() < 3)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}
        std::string client_token = connection_per_pid.get(client_pid)->get_argument();
        if (client_token.empty())
        {
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
        }
	uint32 max_allowed_time = arguments[0];
	uint64 max_allowed_space = (((uint64)arguments[2])<<32 | arguments[1]);

	if (pending_commit)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}

	command = global_resource_planner.probe(client_pid, client_token, max_allowed_time, max_allowed_space);
	if (command == REQUEST_READ_AND_IDX)
	  request_read_and_idx(client_pid, max_allowed_time, max_allowed_space, client_token);

	connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == PURGE)
      {
	std::vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(1);
	if (arguments.size() < 1)
	  continue;
	uint32 target_pid = arguments[0];

	read_aborted(target_pid);
        if (connection_per_pid.get(target_pid) != 0)
        {
	  connection_per_pid.get(target_pid)->send_result(READ_FINISHED);
	  connection_per_pid.set(target_pid, 0);
        }

	connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == QUERY_BY_TOKEN)
      {
        std::string target_token = connection_per_pid.get(client_pid)->get_argument();
        if (target_token.empty())
        {
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
        }

	pid_t target_pid = 0;
        for (std::vector< Reader_Entry >::const_iterator it = global_resource_planner.get_active().begin();
	    it != global_resource_planner.get_active().end(); ++it)
	{
	  if (it->client_token == target_token)
	    target_pid = it->client_pid;
	}

	connection_per_pid.get(client_pid)->send_result(target_pid);
      }
      else if (command == QUERY_MY_STATUS)
      {
        Blocking_Client_Socket* connection = connection_per_pid.get(client_pid);
        if (!connection)
          continue;

        std::string client_token = connection_per_pid.get(client_pid)->get_argument();
        if (client_token.empty())
        {
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
        }

        connection->send_data(global_resource_planner.get_rate_limit());

        for (std::vector< Reader_Entry >::const_iterator it = global_resource_planner.get_active().begin();
           it != global_resource_planner.get_active().end(); ++it)
        {
          if (it->client_token != client_token)
            continue;

          if (processes_reading_idx.find(it->client_pid) != processes_reading_idx.end())
            connection->send_data(REQUEST_READ_AND_IDX);
          else
            connection->send_data(READ_IDX_FINISHED);

          connection->send_data(it->client_pid);
          connection->send_data(it->max_time);
          connection->send_data(it->max_space >>32);
          connection->send_data(it->max_space & 0xffffffff);
          connection->send_data(it->start_time);
        }

        connection->send_data(0);

        for (std::vector< Quota_Entry >::const_iterator it = global_resource_planner.get_afterwards().begin();
            it != global_resource_planner.get_afterwards().end(); ++it)
        {
          if (it->client_token == client_token)
            connection->send_data(it->expiration_time);
        }

        connection->send_result(0);
      }
      else if (command == SET_GLOBAL_LIMITS)
      {
	std::vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(5);
	if (arguments.size() < 5)
	  continue;

	uint64 new_total_available_space = (((uint64)arguments[1])<<32 | arguments[0]);
	uint64 new_total_available_time_units = (((uint64)arguments[3])<<32 | arguments[2]);
        int rate_limit_ = arguments[4];

	if (new_total_available_space > 0)
	  global_resource_planner.set_total_available_space(new_total_available_space);
	if (new_total_available_time_units > 0)
	  global_resource_planner.set_total_available_time(new_total_available_time_units);
        if (rate_limit_ > -1)
          global_resource_planner.set_rate_limit(rate_limit_);

	connection_per_pid.get(client_pid)->send_result(command);
      }
    }
    catch (File_Error e)
    {
      std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';

      counter += 30;
      millisleep(3000);

      // Set command state to zero.
      *(uint32*)dispatcher_shm_ptr = 0;
    }
  }
}


void Dispatcher::output_status()
{
  try
  {
    std::set< ::pid_t > connected_processes;
    for (std::map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.base_map().begin();
        it != connection_per_pid.base_map().end(); ++it)
      connected_processes.insert(it->first);

    if (status_output)
      status_output->output_status(shadow_name + ".status",
          socket.num_started_connections(), connection_per_pid.base_map().size(),
          global_resource_planner.get_rate_limit(),
          global_resource_planner.get_total_available_space(),
          global_resource_planner.get_total_claimed_space(), global_resource_planner.get_average_claimed_space(),
          global_resource_planner.get_total_available_time(),
          global_resource_planner.get_total_claimed_time(), global_resource_planner.get_average_claimed_time(),
          requests_started_counter, requests_finished_counter,
          transaction_insulator.registered_pids(), processes_reading_idx, connected_processes,
          global_resource_planner.get_active(), global_resource_planner.get_afterwards());
  }
  catch (...) {}
}
