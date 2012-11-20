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
#include <unistd.h>

#include <deque>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

void copy_file(const string& source, const string& dest)
{
  if (!file_exists(source))
    return;
  
  Raw_File source_file(source, O_RDONLY, S_666, "Dispatcher:1");
  uint64 size = source_file.size("Dispatcher:2");
  Raw_File dest_file(dest, O_RDWR|O_CREAT, S_666, "Dispatcher:3");
  dest_file.resize(size, "Dispatcher:4");
  
  Void_Pointer< uint8 > buf(64*1024);
  while (size > 0)
  {
    size = read(source_file.fd(), buf.ptr, 64*1024);
    dest_file.write(buf.ptr, size, "Dispatcher:5");
  }
}

string getcwd()
{
  int size = 256;
  char* buf;
  while (true)
  {
    buf = (char*)malloc(size);
    errno = 0;
    buf = getcwd(buf, size);
    if (errno != ERANGE)
      break;
    
    free(buf);
    size *= 2;
  }
  if (errno != 0)
  {
    free(buf);
    throw File_Error(errno, "wd", "Dispatcher::getcwd");
  }
  string result(buf);
  free(buf);
  if ((result != "") && (result[result.size()-1] != '/'))
    result += '/';
  return result;
}

void millisleep(uint32 milliseconds)
{
  struct timeval timeout_;
  timeout_.tv_sec = milliseconds/1000;
  timeout_.tv_usec = milliseconds*1000;
  select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
}

struct sockaddr_un
{
  unsigned short sun_family;
  char sun_path[255];
};


Blocking_Client_Socket::Blocking_Client_Socket
  (int socket_descriptor_) : socket_descriptor(socket_descriptor_), state(waiting) {}

uint32 Blocking_Client_Socket::get_command()
{
  if (state == disconnected)
    return Dispatcher::HANGUP;
  else if (state == processing_command)
    return last_command;
  
  int bytes_read = recv(socket_descriptor, &last_command, sizeof(uint32), 0);
  if (bytes_read == -1)
    return 0;
  else if (bytes_read == 0)
  {
    state = disconnected;
    return Dispatcher::HANGUP;
  }
  else
  {
    state = processing_command;
    return last_command;
  }
}

vector< uint32 > Blocking_Client_Socket::get_arguments(int num_arguments)
{
  vector< uint32 > result;
  if (state == disconnected || state == waiting)
    return result;
  
  for (int i = 0; i < num_arguments; ++i)
  {
    // Wait for each argument up to 0.1 seconds
    result.push_back(0);
    int bytes_read = recv(socket_descriptor, &result.back(), sizeof(uint32), 0);
    uint counter = 0;
    while (bytes_read == -1 && counter <= 100)
    {
      bytes_read = recv(socket_descriptor, &result.back(), sizeof(uint32), 0);
      millisleep(1);
      ++counter;
    }
    if (bytes_read == 0)
    {
      state = disconnected;
      result.clear();
      break;
    }
    if (bytes_read == -1)
      break;
  }
  return result;
}

void Blocking_Client_Socket::clear_state()
{
  if (state == disconnected || state == waiting)
    return;
  
  // remove any pending data. The connection should be clear at the end of the command.
  uint32 dummy;
  int bytes_read = recv(socket_descriptor, &dummy, sizeof(uint32), 0);
  while (bytes_read > 0)
    bytes_read = recv(socket_descriptor, &dummy, sizeof(uint32), 0);
  
  if (bytes_read == 0)
  {
    state = disconnected;
    return;
  }
  state = waiting;
}

void Blocking_Client_Socket::send_result(uint32 result)
{
  if (state == disconnected || state == waiting)
    return;
  
  clear_state();

  if (state == disconnected)
    return;
  
  send(socket_descriptor, &result, sizeof(uint32), 0);
  state = waiting;
}

Blocking_Client_Socket::~Blocking_Client_Socket()
{
  close(socket_descriptor);
}


Dispatcher::Dispatcher
    (string dispatcher_share_name_,
     string index_share_name,
     string shadow_name_,
     string db_dir_,
     uint max_num_reading_processes_, uint purge_timeout_,
     uint64 total_available_space_,
     uint64 total_available_time_units_,
     const vector< File_Properties* >& controlled_files_,
     Dispatcher_Logger* logger_)
    : controlled_files(controlled_files_),
      data_footprints(controlled_files_.size()),
      map_footprints(controlled_files_.size()),
      shadow_name(shadow_name_), db_dir(db_dir_),
      dispatcher_share_name(dispatcher_share_name_),
      max_num_reading_processes(max_num_reading_processes_),
      purge_timeout(purge_timeout_),
      total_available_space(total_available_space_),
      total_available_time_units(total_available_time_units_),
      logger(logger_),
      pending_commit(false)
{
  signal(SIGPIPE, SIG_IGN);
  
  // get the absolute pathname of the current directory
  if (db_dir.substr(0, 1) != "/")
    db_dir = getcwd() + db_dir_;
  if (shadow_name.substr(0, 1) != "/")
    shadow_name = getcwd() + shadow_name_;
  
  // initialize the socket for the server
  string socket_name = db_dir + dispatcher_share_name;
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
  
  // open dispatcher_share
  dispatcher_shm_fd = shm_open
      (dispatcher_share_name.c_str(), O_RDWR|O_CREAT|O_TRUNC|O_EXCL, S_666);
  if (dispatcher_shm_fd < 0)
  {
    remove(socket_name.c_str());
    throw File_Error
        (errno, dispatcher_share_name, "Dispatcher_Server::1");
  }
  fchmod(dispatcher_shm_fd, S_666);
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
  close(socket_descriptor);
  munmap((void*)dispatcher_shm_ptr, SHM_SIZE + db_dir.size() + shadow_name.size());
  shm_unlink(dispatcher_share_name.c_str());
  remove((db_dir + dispatcher_share_name).c_str());
}

void Dispatcher::write_start(pid_t pid)
{
  // Lock the writing lock file for the client.
  try
  {
    Raw_File shadow_file(shadow_name + ".lock", O_RDWR|O_CREAT|O_EXCL, S_666, "write_start:1");
 
    copy_mains_to_shadows();
    vector< pid_t > registered = write_index_of_empty_blocks();
    if (logger)
      logger->write_start(pid, registered);
  }
  catch (File_Error e)
  {
    if ((e.error_number == EEXIST) && (e.filename == (shadow_name + ".lock")))
    {
      pid_t locked_pid;
      ifstream lock((shadow_name + ".lock").c_str());
      lock>>locked_pid;
      if (locked_pid == pid)
	return;
    }
    cerr<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    return;
  }

  try
  {
    ofstream lock((shadow_name + ".lock").c_str());
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
    cerr<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    return;
  }
  
  remove(shadow_name.c_str());
  remove_shadows();
  remove((shadow_name + ".lock").c_str());
  set_current_footprints();
}

void Dispatcher::request_read_and_idx(pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space)
{
  if (logger)
    logger->request_read_and_idx(pid, max_allowed_time, max_allowed_space);
  for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->register_pid(pid);
  for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->register_pid(pid);
  processes_reading_idx.insert(pid);
  processes_reading[pid] = Reader_Entry(time(NULL), max_allowed_space, max_allowed_time);
}

void Dispatcher::read_idx_finished(pid_t pid)
{
  if (logger)
    logger->read_idx_finished(pid);
  processes_reading_idx.erase(pid);
}

void Dispatcher::prolongate(pid_t pid)
{
  if (logger)
    logger->prolongate(pid);
  processes_reading[pid].ping_time = time(NULL);
}

void Dispatcher::read_finished(pid_t pid)
{
  if (logger)
    logger->read_finished(pid);
  for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->unregister_pid(pid);
  for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->unregister_pid(pid);
  processes_reading_idx.erase(pid);
  processes_reading.erase(pid);
  disconnected.erase(pid);
}

void Dispatcher::copy_shadows_to_mains()
{
  for (vector< File_Properties* >::const_iterator it(controlled_files.begin());
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
  for (vector< File_Properties* >::const_iterator it(controlled_files.begin());
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
  for (vector< File_Properties* >::const_iterator it(controlled_files.begin());
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
  for (vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    try
    {
      data_footprints[i].set_current_footprint
          (controlled_files[i]->get_data_footprint(db_dir));
    }
    catch (File_Error e)
    {
      cerr<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    catch (...) {}
    
    try
    {
      map_footprints[i].set_current_footprint
          (controlled_files[i]->get_map_footprint(db_dir));
    }
    catch (File_Error e)
    {
      cerr<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
    catch (...) {}
  }
}

void write_to_index_empty_file(const vector< bool >& footprint, string filename)
{
  Void_Pointer< uint32 > buffer(footprint.size()*sizeof(uint32));  
  uint32* pos = buffer.ptr;
  for (uint32 i = 0; i < footprint.size(); ++i)
  {
    if (!footprint[i])
    {
      *pos = i;
      ++pos;
    }
  }
  
  Raw_File file(filename, O_RDWR|O_CREAT|O_TRUNC,
		S_666, "write_to_index_empty_file:1");
  file.write((uint8*)buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr), "Dispatcher:6");
}

vector< Dispatcher::pid_t > Dispatcher::write_index_of_empty_blocks()
{
  set< pid_t > registered;
  for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
  {
    vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      registered.insert(*it);
  }
  for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
  {
    vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      registered.insert(*it);
  }
  
  for (vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    if (file_exists(db_dir + controlled_files[i]->get_file_name_trunk()
        + controlled_files[i]->get_data_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file
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
      write_to_index_empty_file
          (map_footprints[i].total_footprint(),
	   db_dir + controlled_files[i]->get_file_name_trunk()
	   + controlled_files[i]->get_id_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
  }
  
  vector< pid_t > registered_v;
  registered_v.assign(registered.begin(), registered.end());
  return registered_v;
}

void Dispatcher::standby_loop(uint64 milliseconds)
{
  uint32 counter = 0;
  uint32 idle_counter = 0;
  uint32 last_pid = 0;
  while ((milliseconds == 0) || (counter < milliseconds/100))
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
    for (vector< int >::iterator it = started_connections.begin();
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
    
    uint32 command = 0;
    uint32 client_pid = 0;
    
    // poll all open connections round robin
    for (map< pid_t, Blocking_Client_Socket* >::const_iterator
        it = connection_per_pid.base_map().upper_bound(last_pid);
        it != connection_per_pid.base_map().end(); ++it)
    {
      command = it->second->get_command();
      if (command != 0)
      {
	client_pid = it->first;
	break;
      }
    }
    if (command == 0)
    {
      for (map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.base_map().begin();
          it != connection_per_pid.base_map().upper_bound(last_pid); ++it)
      {
        command = it->second->get_command();
        if (command != 0)
        {
	  client_pid = it->first;
	  break;
        }
      }
    }
    if (command != 0)
      last_pid = client_pid;
    
    if (command == Dispatcher::HANGUP
        && processes_reading.find(client_pid) != processes_reading.end())
      command = Dispatcher::READ_ABORTED;

    
    if (*(uint32*)dispatcher_shm_ptr == 0 && command == 0)
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
      if (command == 0)
      {
        command = *(uint32*)dispatcher_shm_ptr;
        *(uint32*)dispatcher_shm_ptr = 0;
        client_pid = *(uint32*)(dispatcher_shm_ptr + sizeof(uint32));
      }
      // Set command state to zero.
      *(uint32*)dispatcher_shm_ptr = 0;
      if (command == TERMINATE)
      {
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
      
	if (connection_per_pid.get(client_pid) != 0)
	{
	  connection_per_pid.get(client_pid)->send_result(command);
	  connection_per_pid.set(client_pid, 0);
	}
	
	break;
      }
      else if (command == OUTPUT_STATUS)
      {
	output_status();
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
	
	if (connection_per_pid.get(client_pid) != 0)
	{
	  connection_per_pid.get(client_pid)->send_result(command);
	  connection_per_pid.set(client_pid, 0);
	}
      }
      else if (command == WRITE_START)
      {
	check_and_purge();
	write_start(client_pid);
	connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == WRITE_ROLLBACK)
      {
	write_rollback(client_pid);
	connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == WRITE_COMMIT)
      {
	check_and_purge();
	write_commit(client_pid);
	connection_per_pid.get(client_pid)->send_result(command);
      }
      else if (command == HANGUP)
      {
	if (connection_per_pid.get(client_pid) != 0)
	  connection_per_pid.set(client_pid, 0);
      }
      else if (command == READ_ABORTED)
      {
	read_finished(client_pid);
	if (connection_per_pid.get(client_pid) != 0)
	  connection_per_pid.set(client_pid, 0);
      }
      else if (command == REQUEST_READ_AND_IDX)
      {
	if (processes_reading.size() >= max_num_reading_processes)
	{
	  check_and_purge();
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}
	
	if (pending_commit)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}
	
	vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(3);
	if (arguments.size() < 3)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}
	
	uint32 max_allowed_time = arguments[0];
	uint64 max_allowed_space = (((uint64)arguments[2])<<32 | arguments[1]);
	
	if (max_allowed_space > (total_available_space - total_claimed_space())/2)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}
	
	if (max_allowed_time > (total_available_time_units - total_claimed_time_units())/2)
	{
	  connection_per_pid.get(client_pid)->send_result(0);
	  continue;
	}
	
	request_read_and_idx(client_pid, max_allowed_time, max_allowed_space);
	connection_per_pid.get(client_pid)->send_result(command);
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
      }
      else if (command == READ_IDX_FINISHED)
      {
	read_idx_finished(client_pid);
        if (connection_per_pid.get(client_pid) != 0)
	  connection_per_pid.get(client_pid)->send_result(command);
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
      }
      else if (command == READ_FINISHED)
      {
	read_finished(client_pid);	
	if (connection_per_pid.get(client_pid) != 0)
	{
	  connection_per_pid.get(client_pid)->send_result(command);
	  connection_per_pid.set(client_pid, 0);
	}
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
      }
      else if (command == PURGE)
      {
	vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(1);
	if (arguments.size() < 1)
	  continue;
	uint32 target_pid = arguments[0];

	read_finished(target_pid);
        if (connection_per_pid.get(target_pid) != 0)
        {
	  connection_per_pid.get(target_pid)->send_result(READ_FINISHED);
	  connection_per_pid.set(target_pid, 0);
        }
        
	connection_per_pid.get(client_pid)->send_result(command);
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
      }
      else if (command == SET_GLOBAL_LIMITS)
      {
	vector< uint32 > arguments = connection_per_pid.get(client_pid)->get_arguments(4);
	if (arguments.size() < 4)
	  continue;
	
	uint64 new_total_available_space = (((uint64)arguments[1])<<32 | arguments[0]);
	uint64 new_total_available_time_units = (((uint64)arguments[3])<<32 | arguments[2]);
	
	if (new_total_available_space > 0)
	  total_available_space = new_total_available_space;
	if (new_total_available_time_units > 0)
	  total_available_time_units = new_total_available_time_units;
	
	connection_per_pid.get(client_pid)->send_result(command);
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
      }
// Ping-Feature removed. The concept of unassured messages doesn't fit in the context of strict
// two-directional communication.
//       else if (command == PING)
//       {
// 	prolongate(client_pid);
// 	connection_per_pid.get(client_pid)->send_result(0);
//       }
    }
    catch (File_Error e)
    {
      cerr<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
      
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
    ofstream status((shadow_name + ".status").c_str());
    
    status<<started_connections.size()<<' '<<connection_per_pid.base_map().size()
        <<' '<<total_available_space<<' '<<total_claimed_space()
	<<' '<<total_available_time_units<<' '<<total_claimed_time_units()<<'\n';
    
    for (set< pid_t >::const_iterator it = processes_reading_idx.begin();
        it != processes_reading_idx.end(); ++it)
    {
      status<<REQUEST_READ_AND_IDX<<' '<<*it
	  <<' '<<processes_reading[*it].max_space
	  <<' '<<processes_reading[*it].max_time<<'\n';
    }
    set< pid_t > collected_pids;
    for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
        it != data_footprints.end(); ++it)
    {
      vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
      for (vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
          it != registered_processes.end(); ++it)
	collected_pids.insert(*it);
    }
    for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
        it != map_footprints.end(); ++it)
    {
      vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
      for (vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
          it != registered_processes.end(); ++it)
	collected_pids.insert(*it);
    }
    for (map< pid_t, Reader_Entry >::const_iterator it = processes_reading.begin();
        it != processes_reading.end(); ++it)
      collected_pids.insert(it->first);
    
    for (set< pid_t >::const_iterator it = collected_pids.begin();
        it != collected_pids.end(); ++it)
    {
      if (processes_reading_idx.find(*it) == processes_reading_idx.end())
	status<<READ_IDX_FINISHED<<' '<<*it
	  <<' '<<processes_reading[*it].max_space
	  <<' '<<processes_reading[*it].max_time<<'\n';
    }
    
    for (map< pid_t, Blocking_Client_Socket* >::const_iterator it = connection_per_pid.base_map().begin();
	 it != connection_per_pid.base_map().end(); ++it)
    {
      if (processes_reading_idx.find(it->first) == processes_reading_idx.end()
	  && collected_pids.find(it->first) == collected_pids.end())
	status<<"pending\t"<<it->first<<'\n';
    }
  }
  catch (...) {}
}

void Idx_Footprints::set_current_footprint(const vector< bool >& footprint)
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

vector< Idx_Footprints::pid_t > Idx_Footprints::registered_processes() const
{
  vector< pid_t > result;
  for (map< pid_t, vector< bool > >::const_iterator
      it(footprint_per_pid.begin()); it != footprint_per_pid.end(); ++it)
    result.push_back(it->first);
  return result;
}

vector< bool > Idx_Footprints::total_footprint() const
{
  vector< bool > result = current_footprint;
  for (map< pid_t, vector< bool > >::const_iterator
      it(footprint_per_pid.begin()); it != footprint_per_pid.end(); ++it)
  {
    // By construction, it->second.size() <= result.size()
    for (vector< bool >::size_type i = 0; i < it->second.size(); ++i)
      result[i] = result[i] | (it->second)[i];
  }
  return result;
}

uint64 Dispatcher::total_claimed_space() const
{
  uint64 result = 0;
  for (map< pid_t, Reader_Entry >::const_iterator it = processes_reading.begin();
      it != processes_reading.end(); ++it)
    result += it->second.max_space;
  
  return result;
}

uint64 Dispatcher::total_claimed_time_units() const
{
  uint64 result = 0;
  for (map< pid_t, Reader_Entry >::const_iterator it = processes_reading.begin();
      it != processes_reading.end(); ++it)
    result += it->second.max_time;
  
  return result;
}

void Dispatcher::check_and_purge()
{
  set< pid_t > collected_pids;
  for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
  {
    vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      collected_pids.insert(*it);
  }
  for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
  {
    vector< Idx_Footprints::pid_t > registered_processes = it->registered_processes();
    for (vector< Idx_Footprints::pid_t >::const_iterator it = registered_processes.begin();
        it != registered_processes.end(); ++it)
      collected_pids.insert(*it);
  }
  for (map< pid_t, Reader_Entry >::const_iterator it = processes_reading.begin();
      it != processes_reading.end(); ++it)
    collected_pids.insert(it->first);
  
  uint32 current_time = time(NULL);
  for (set< pid_t >::const_iterator it = collected_pids.begin();
      it != collected_pids.end(); ++it)
  {
    map< pid_t, Reader_Entry >::const_iterator alive_it = processes_reading.find(*it);
    if ((alive_it != processes_reading.end()) &&
        (alive_it->second.ping_time + purge_timeout > current_time))
      continue;
    if (disconnected.find(*it) != disconnected.end()
        || alive_it == processes_reading.end()
	|| alive_it->second.ping_time + purge_timeout > current_time)
    {
      if (logger)
	logger->purge(*it);
      read_finished(*it);
    }
  }
}

Dispatcher_Client::Dispatcher_Client
    (string dispatcher_share_name_)
    : dispatcher_share_name(dispatcher_share_name_)
{
  signal(SIGPIPE, SIG_IGN);
  
  // open dispatcher_share
  dispatcher_shm_fd = shm_open
      (dispatcher_share_name.c_str(), O_RDWR, S_666);
  if (dispatcher_shm_fd < 0)
    throw File_Error
        (errno, dispatcher_share_name, "Dispatcher_Client::1");
  struct stat stat_buf;
  fstat(dispatcher_shm_fd, &stat_buf);
  dispatcher_shm_ptr = (uint8*)mmap
      (0, stat_buf.st_size,
       PROT_READ|PROT_WRITE, MAP_SHARED, dispatcher_shm_fd, 0);

  // get db_dir and shadow_name
  db_dir = string((const char *)(dispatcher_shm_ptr + 4*sizeof(uint32)),
		  *(uint32*)(dispatcher_shm_ptr + 3*sizeof(uint32)));
  shadow_name = string((const char *)(dispatcher_shm_ptr + 5*sizeof(uint32)
      + db_dir.size()), *(uint32*)(dispatcher_shm_ptr + db_dir.size() +
		       4*sizeof(uint32)));

  // initialize the socket for the client
  string socket_name = db_dir + dispatcher_share_name_;
  socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_descriptor == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Client::2");  
  struct sockaddr_un local;
  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, socket_name.c_str());
  if (connect(socket_descriptor, (struct sockaddr*)&local,
      sizeof(local.sun_family) + strlen(local.sun_path)) == -1)
    throw File_Error
        (errno, socket_name, "Dispatcher_Client::3");
  
  pid_t pid = getpid();
  if (send(socket_descriptor, &pid, sizeof(pid_t), 0) == -1)
    throw File_Error(errno, dispatcher_share_name, "Dispatcher_Client::4");
}

Dispatcher_Client::~Dispatcher_Client()
{
  close(socket_descriptor);
  munmap((void*)dispatcher_shm_ptr,
	 Dispatcher::SHM_SIZE + db_dir.size() + shadow_name.size());
  close(dispatcher_shm_fd);
}


template< class TObject >
void Dispatcher_Client::send_message(TObject message, string source_pos)
{
  if (send(socket_descriptor, &message, sizeof(TObject), 0) == -1)
    throw File_Error(errno, dispatcher_share_name, source_pos);
}


bool Dispatcher_Client::ack_arrived()
{
  uint32 answer = 0;
  int bytes_read = recv(socket_descriptor, &answer, sizeof(uint32), 0);
  while (bytes_read == -1)
  {
    millisleep(50);
    bytes_read = recv(socket_descriptor, &answer, sizeof(uint32), 0);
  }
  if (bytes_read == 4)
    return (answer != 0);
  
  uint32 pid = getpid();
  if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
    return true;
  millisleep(50);  
  return (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid);
}

void Dispatcher_Client::write_start()
{
  pid_t pid = getpid();
  
  send_message(Dispatcher::WRITE_START, "Dispatcher_Client::write_start::socket");

  while (true)
  {
    if (ack_arrived() && file_exists(shadow_name + ".lock"))
    {
      try
      {
	pid_t locked_pid = 0;
	ifstream lock((shadow_name + ".lock").c_str());
	lock>>locked_pid;
	if (locked_pid == pid)
	  return;
      }
      catch (...) {}
    }
    millisleep(500);
  }
}

void Dispatcher_Client::write_rollback()
{
  pid_t pid = getpid();
  
  send_message(Dispatcher::WRITE_ROLLBACK, "Dispatcher_Client::write_rollback::socket");

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
	  pid_t locked_pid;
	  ifstream lock((shadow_name + ".lock").c_str());
	  lock>>locked_pid;
	  if (locked_pid != pid)
	    return;
        }
        catch (...) {}
      }
      else
        return;
    }
    
    millisleep(500);
  }
}

void Dispatcher_Client::write_commit()
{
  pid_t pid = getpid();
  
  send_message(Dispatcher::WRITE_COMMIT, "Dispatcher_Client::write_commit::socket");  
  millisleep(200);

  while (true)
  {
    if (ack_arrived())
    {
      if (file_exists(shadow_name + ".lock"))
      {
        try
        {
	  pid_t locked_pid;
	  ifstream lock((shadow_name + ".lock").c_str());
	  lock>>locked_pid;
	  if (locked_pid != pid)
	    return;
        }
        catch (...) {}
      }
      else
        return;
    }
    
    send_message(Dispatcher::WRITE_COMMIT, "Dispatcher_Client::write_commit::socket");
    millisleep(200);
  }
}

void Dispatcher_Client::request_read_and_idx(uint32 max_allowed_time, uint64 max_allowed_space)
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  
  uint counter = 0;
  while (++counter <= 300)
  {
    send_message(Dispatcher::REQUEST_READ_AND_IDX,
		 "Dispatcher_Client::request_read_and_idx::socket::1");
    send_message(max_allowed_time, "Dispatcher_Client::request_read_and_idx::socket::2");
    send_message(max_allowed_space, "Dispatcher_Client::request_read_and_idx::socket::3");
    
    if (ack_arrived())
      return;
    
    millisleep(200);
  }
  throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::request_read_and_idx::timeout");
}

void Dispatcher_Client::read_idx_finished()
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
		     
  uint counter = 0;
  while (++counter <= 300)
  {
    send_message(Dispatcher::READ_IDX_FINISHED, "Dispatcher_Client::read_idx_finished::socket");
    
    if (ack_arrived())
      return;
  }
  throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::read_idx_finished::timeout");
}

void Dispatcher_Client::read_finished()
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  
  uint counter = 0;
  while (++counter <= 300)
  {
    send_message(Dispatcher::READ_FINISHED, "Dispatcher_Client::read_finished::socket");
    
    if (ack_arrived())
      return;
  }
  throw File_Error(0, dispatcher_share_name, "Dispatcher_Client::read_finished::timeout");
}


void Dispatcher_Client::purge(uint32 pid)
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
		     
  while (true)
  {
    send_message(Dispatcher::PURGE, "Dispatcher_Client::purge::socket::1");
    send_message(pid, "Dispatcher_Client::purge::socket::2");
    
    if (ack_arrived())
      return;
  }
}


void Dispatcher_Client::set_global_limits(uint64 max_allowed_space, uint64 max_allowed_time_units)
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
		     
  while (true)
  {
    send_message(Dispatcher::SET_GLOBAL_LIMITS, "Dispatcher_Client::set_global_limits::1");
    send_message(max_allowed_space, "Dispatcher_Client::set_global_limits::2");
    send_message(max_allowed_time_units, "Dispatcher_Client::set_global_limits::3");
    
    if (ack_arrived())
      return;
  }
}


void Dispatcher_Client::ping()
{
// Ping-Feature removed. The concept of unassured messages doesn't fit in the context of strict
// two-directional communication.
//   send_message(Dispatcher::PING, "Dispatcher_Client::ping::socket");
}

void Dispatcher_Client::terminate()
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
		     
  while (true)
  {
    send_message(Dispatcher::TERMINATE, "Dispatcher_Client::terminate::socket");
    
    if (ack_arrived())
      return;
  }
}

void Dispatcher_Client::output_status()
{
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
		     
  while (true)
  {
    send_message(Dispatcher::OUTPUT_STATUS, "Dispatcher_Client::output_status::socket");
    
    if (ack_arrived())
      return;
  }
}
