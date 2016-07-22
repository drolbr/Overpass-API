/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Template_DB.
*
* Template_DB is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Template_DB is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Template_DB.  If not, see <http://www.gnu.org/licenses/>.
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
{
  signal(SIGPIPE, SIG_IGN);
  
  std::string shadow_name = shadow_name_;
  std::string db_dir = db_dir_;
  // get the absolute pathname of the current directory
  if (db_dir.substr(0, 1) != "/")
    db_dir = getcwd() + db_dir_;
  if (shadow_name.substr(0, 1) != "/")
    shadow_name = getcwd() + shadow_name_;
  
  // initialize the socket for the server
  socket_name = db_dir + dispatcher_share_name;
  
  socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_descriptor == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Server::2");
  if (fcntl(socket_descriptor, F_SETFL, O_RDWR|O_NONBLOCK) == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Server::3");  
  struct sockaddr_un local;
  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, socket_name.c_str());
  if (bind(socket_descriptor, (struct sockaddr*)&local,
      sizeof(local.sun_family) + strlen(local.sun_path)) == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Server::4");
  if (chmod(socket_name.c_str(), S_666) == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Server::8");
  if (listen(socket_descriptor, max_num_reading_processes) == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Server::5");
}


Dispatcher_Socket::~Dispatcher_Socket()
{
  close(socket_descriptor);
  remove(socket_name.c_str());
}


void Dispatcher_Socket::look_for_a_new_connection(Connection_Per_Pid_Map& connection_per_pid)
{    
  struct sockaddr_un sockaddr_un_dummy;
  uint sockaddr_un_dummy_size = sizeof(sockaddr_un_dummy);
  int socket_fd = accept(socket_descriptor, (sockaddr*)&sockaddr_un_dummy,
			 (socklen_t*)&sockaddr_un_dummy_size);
  if (socket_fd == -1)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
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


int Global_Resource_Planner::probe(uint32 pid, uint32 client_token, uint32 time_units, uint64 max_space)
{
  if (rate_limit > 0 && client_token > 0)
  {
    uint32 token_count = 0;
    for (std::vector< Reader_Entry >::const_iterator it = active.begin(); it != active.end(); ++it)
    {
      if (it->client_token == client_token)
	++token_count;
    }
    if (token_count >= rate_limit)
      return Dispatcher::RATE_LIMITED;
    
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
      return Dispatcher::RATE_LIMITED;    
  }
  
  // Simple checks: is the query acceptable from a global point of view?
  if (time_units > (global_available_time - global_used_time)/2 ||
      max_space > (global_available_space - global_used_space)/2)
    return 0;
  
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
  
  if (rate_limit > 0 && it->client_token > 0)
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


void Global_Resource_Planner::remove(uint32 pid)
{
  for (std::vector< Reader_Entry >::iterator it = active.begin(); it != active.end(); ++it)
  {
    if (it->client_pid == pid)
    {
      remove_entry(it);
      break;
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
}


Dispatcher::Dispatcher
    (std::string dispatcher_share_name_,
     std::string index_share_name,
     std::string shadow_name_,
     std::string db_dir_,
     uint max_num_reading_processes_, uint purge_timeout_,
     uint64 total_available_space_,
     uint64 total_available_time_units_,
     const std::vector< File_Properties* >& controlled_files_,
     Dispatcher_Logger* logger_)
    : socket(dispatcher_share_name_, shadow_name_, db_dir_, max_num_reading_processes_),
      controlled_files(controlled_files_),
      data_footprints(controlled_files_.size()),
      map_footprints(controlled_files_.size()),
      shadow_name(shadow_name_), db_dir(db_dir_),
      dispatcher_share_name(dispatcher_share_name_),
      logger(logger_),
      pending_commit(false),
      requests_started_counter(0),
      requests_finished_counter(0),
      global_resource_planner(total_available_time_units_, total_available_space_, 0)
{
  signal(SIGPIPE, SIG_IGN);
  
  // get the absolute pathname of the current directory
  if (db_dir.substr(0, 1) != "/")
    db_dir = getcwd() + db_dir_;
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
    copy_shadows_to_mains();
    remove(shadow_name.c_str());
  }    
  remove_shadows();
  remove((shadow_name + ".lock").c_str());
  set_current_footprints();
}


Dispatcher::~Dispatcher()
{
  munmap((void*)dispatcher_shm_ptr, SHM_SIZE + db_dir.size() + shadow_name.size());
  shm_unlink(dispatcher_share_name.c_str());
}


void Dispatcher::write_start(pid_t pid)
{
  // Lock the writing lock file for the client.
  try
  {
    Raw_File shadow_file(shadow_name + ".lock", O_RDWR|O_CREAT|O_EXCL, S_666, "write_start:1");
 
    copy_mains_to_shadows();
    std::vector< pid_t > registered = write_index_of_empty_blocks();
    if (logger)
      logger->write_start(pid, registered);
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
    }
    std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    return;
  }

  try
  {
    std::ofstream lock((shadow_name + ".lock").c_str());
    lock<<pid;
  }
  catch (...) {}
}


void Dispatcher::write_rollback(pid_t pid)
{
  if (logger)
    logger->write_rollback(pid);
  remove_shadows();
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

  if (logger)
    logger->write_commit(pid);
  try
  {
    Raw_File shadow_file(shadow_name, O_RDWR|O_CREAT|O_EXCL, S_666, "write_commit:1");
    
    copy_shadows_to_mains();
  }
  catch (File_Error e)
  {
    std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    return;
  }
  
  remove(shadow_name.c_str());
  remove_shadows();
  remove((shadow_name + ".lock").c_str());
  set_current_footprints();
}


void Dispatcher::request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space,
				      uint32 client_token)
{ 
  if (logger)
    logger->request_read_and_idx(pid, max_allowed_time, max_allowed_space);
  ++requests_started_counter;
  
  for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->register_pid(pid);
  for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->register_pid(pid);
  
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
  
  for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->unregister_pid(pid);
  for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->unregister_pid(pid);
  processes_reading_idx.erase(pid);
  disconnected.erase(pid);
  global_resource_planner.remove(pid);
}


void Dispatcher::read_aborted(pid_t pid)
{
  if (logger)
    logger->read_aborted(pid);
  for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->unregister_pid(pid);
  for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->unregister_pid(pid);
  processes_reading_idx.erase(pid);
  disconnected.erase(pid);
  global_resource_planner.remove(pid);
}


void Dispatcher::copy_shadows_to_mains()
{
  for (std::vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
      copy_file(db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
                + (*it)->get_index_suffix() + (*it)->get_shadow_suffix(),
		db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
		+ (*it)->get_index_suffix());
      copy_file(db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
                + (*it)->get_index_suffix() + (*it)->get_shadow_suffix(),
		db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
		+ (*it)->get_index_suffix());
  }
}


void Dispatcher::copy_mains_to_shadows()
{
  for (std::vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
      copy_file(db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
                + (*it)->get_index_suffix(),
		db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
		+ (*it)->get_index_suffix() + (*it)->get_shadow_suffix());
      copy_file(db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
                + (*it)->get_index_suffix(),
		db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
		+ (*it)->get_index_suffix() + (*it)->get_shadow_suffix());
  }
}


void Dispatcher::remove_shadows()
{
  for (std::vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
    remove((db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
            + (*it)->get_index_suffix() + (*it)->get_shadow_suffix()).c_str());
    remove((db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
            + (*it)->get_index_suffix() + (*it)->get_shadow_suffix()).c_str());
    remove((db_dir + (*it)->get_file_name_trunk() + (*it)->get_data_suffix()
            + (*it)->get_shadow_suffix()).c_str());
    remove((db_dir + (*it)->get_file_name_trunk() + (*it)->get_id_suffix()
            + (*it)->get_shadow_suffix()).c_str());
  }
}


void Dispatcher::set_current_footprints()
{
  for (std::vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    try
    {
      data_footprints[i].set_current_footprint
          (controlled_files[i]->get_data_footprint(db_dir));
    }
    catch (File_Error e)
    {
      std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    catch (...) {}
    
    try
    {
      map_footprints[i].set_current_footprint
          (controlled_files[i]->get_map_footprint(db_dir));
    }
    catch (File_Error e)
    {
      std::cerr<<"File_Error "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    catch (...) {}
  }
}


void write_to_index_empty_file_data(const std::vector< bool >& footprint, const std::string& filename)
{
  Void_Pointer< std::pair< uint32, uint32 > > buffer(footprint.size() * 8);  
  std::pair< uint32, uint32 >* pos = buffer.ptr;
  uint32 last_start = 0;
  for (uint32 i = 0; i < footprint.size(); ++i)
  {
    if (footprint[i])
    {
      if (last_start < i)
      {
	*pos = std::make_pair(i - last_start, last_start);
	++pos;
      }
      last_start = i+1;
    }
  }
  if (last_start < footprint.size())
    *pos = std::make_pair(footprint.size() - last_start, last_start);
  
  Raw_File file(filename, O_RDWR|O_CREAT|O_TRUNC,
		S_666, "write_to_index_empty_file_data:1");
  file.write((uint8*)buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr), "Dispatcher:26");
}


void write_to_index_empty_file_ids(const std::vector< bool >& footprint, const std::string& filename)
{
  Void_Pointer< std::pair< uint32, uint32 > > buffer(footprint.size() * 8);
  std::pair< uint32, uint32 >* pos = buffer.ptr;
  uint32 last_start = 0;
  for (uint32 i = 0; i < footprint.size(); ++i)
  {
    if (footprint[i])
    {
      if (last_start < i)
      {
	*pos = std::make_pair(i - last_start, last_start);
	++pos;
      }
      last_start = i+1;
    }
  }
  if (last_start < footprint.size())
    *pos = std::make_pair(footprint.size() - last_start, last_start);
  
  Raw_File file(filename, O_RDWR|O_CREAT|O_TRUNC,
		S_666, "write_to_index_empty_file_ids:1");
  file.write((uint8*)buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr), "Dispatcher:36");
}


std::vector< Dispatcher::pid_t > Dispatcher::write_index_of_empty_blocks()
{
  std::set< pid_t > registered;
  for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
  {
    std::vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (std::vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      registered.insert(*it);
  }
  for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
  {
    std::vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (std::vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      registered.insert(*it);
  }
  
  for (std::vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    if (file_exists(db_dir + controlled_files[i]->get_file_name_trunk()
        + controlled_files[i]->get_data_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file_data
          (data_footprints[i].total_footprint(),
	   db_dir + controlled_files[i]->get_file_name_trunk()
	   + controlled_files[i]->get_data_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
    if (file_exists(db_dir + controlled_files[i]->get_file_name_trunk()
        + controlled_files[i]->get_id_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file_ids
          (map_footprints[i].total_footprint(),
	   db_dir + controlled_files[i]->get_file_name_trunk()
	   + controlled_files[i]->get_id_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
  }
  
  std::vector< pid_t > registered_v;
  registered_v.assign(registered.begin(), registered.end());
  return registered_v;
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
	  check_and_purge();
	  write_start(client_pid);
	}
	else if (command == WRITE_COMMIT)
	{
	  check_and_purge();
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
	std::vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(4);
	if (arguments.size() < 4)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}	
	uint32 max_allowed_time = arguments[0];
	uint64 max_allowed_space = (((uint64)arguments[2])<<32 | arguments[1]);
	uint32 client_token = arguments[3];
	
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
	std::vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(1);
	if (arguments.size() < 1)
	  continue;
	uint32 target_token = arguments[0];

	pid_t target_pid = 0;
        for (std::vector< Reader_Entry >::const_iterator it = global_resource_planner.get_active().begin();
	    it != global_resource_planner.get_active().end(); ++it)
	{
	  if (it->client_token == target_token)
	    target_pid = it->client_pid;
	}
	
	connection_per_pid.get(client_pid)->send_result(target_pid);
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
    std::ofstream status((shadow_name + ".status").c_str());
    
    status<<"Number of not yet opened connections: "<<socket.num_started_connections()<<'\n'
        <<"Number of connected clients: "<<connection_per_pid.base_map().size()<<'\n'
        <<"Rate limit: "<<global_resource_planner.get_rate_limit()<<'\n'
        <<"Total available space: "<<global_resource_planner.get_total_available_space()<<'\n'
        <<"Total claimed space: "<<global_resource_planner.get_total_claimed_space()<<'\n'
        <<"Average claimed space: "<<global_resource_planner.get_average_claimed_space()<<'\n'
        <<"Total available time units: "<<global_resource_planner.get_total_available_time()<<'\n'
        <<"Total claimed time units: "<<global_resource_planner.get_total_claimed_time()<<'\n'
        <<"Average claimed time units: "<<global_resource_planner.get_average_claimed_time()<<'\n'
        <<"Counter of started requests: "<<requests_started_counter<<'\n'
        <<"Counter of finished requests: "<<requests_finished_counter<<'\n';

    std::set< pid_t > collected_pids;
    
    for (std::vector< Reader_Entry >::const_iterator it = global_resource_planner.get_active().begin();
	 it != global_resource_planner.get_active().end(); ++it)
    {
      if (processes_reading_idx.find(it->client_pid) != processes_reading_idx.end())
	status<<REQUEST_READ_AND_IDX;
      else
	status<<READ_IDX_FINISHED;
      status<<' '<<it->client_pid<<' '<<it->client_token<<' '
          <<it->max_space<<' '<<it->max_time<<' '<<it->start_time<<'\n';
        
      collected_pids.insert(it->client_pid);
    }
        
    for (std::vector< Idx_Footprints >::iterator it(data_footprints.begin());
        it != data_footprints.end(); ++it)
    {
      std::vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
      for (std::vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
          it != registered_processes.end(); ++it)
	collected_pids.insert(*it);
    }
    for (std::vector< Idx_Footprints >::iterator it(map_footprints.begin());
        it != map_footprints.end(); ++it)
    {
      std::vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
      for (std::vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
          it != registered_processes.end(); ++it)
	collected_pids.insert(*it);
    }

    for (std::map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.base_map().begin();
	 it != connection_per_pid.base_map().end(); ++it)
    {
      if (processes_reading_idx.find(it->first) == processes_reading_idx.end()
	  && collected_pids.find(it->first) == collected_pids.end())
	status<<"pending\t"<<it->first<<'\n';
    }
    
    for (std::vector< Quota_Entry >::const_iterator it = global_resource_planner.get_afterwards().begin();
	 it != global_resource_planner.get_afterwards().end(); ++it)
    {
      status<<"quota\t"<<it->client_token<<' '<<it->expiration_time<<'\n';
    }
  }
  catch (...) {}
}


void Idx_Footprints::set_current_footprint(const std::vector< bool >& footprint)
{
  current_footprint = footprint;
}


void Idx_Footprints::register_pid(pid_t pid)
{
  footprint_per_pid[pid] = current_footprint;
}


void Idx_Footprints::unregister_pid(pid_t pid)
{
  footprint_per_pid.erase(pid);
}


std::vector< Idx_Footprints::pid_t > Idx_Footprints::registered_processes() const
{
  std::vector< pid_t > result;
  for (std::map< pid_t, std::vector< bool > >::const_iterator
      it(footprint_per_pid.begin()); it != footprint_per_pid.end(); ++it)
    result.push_back(it->first);
  return result;
}


std::vector< bool > Idx_Footprints::total_footprint() const
{
  std::vector< bool > result = current_footprint;
  for (std::map< pid_t, std::vector< bool > >::const_iterator
      it(footprint_per_pid.begin()); it != footprint_per_pid.end(); ++it)
  {
    // By construction, it->second.size() <= result.size()
    for (std::vector< bool >::size_type i = 0; i < it->second.size(); ++i)
      result[i] = result[i] | (it->second)[i];
  }
  return result;
}


void Dispatcher::check_and_purge()
{
  global_resource_planner.purge(connection_per_pid);
}
