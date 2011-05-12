#include <fstream>
#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../core/datatypes.h"
#include "dispatcher.h"

using namespace std;

bool file_exists(const string& filename)
{
  struct stat statbuf;
  return (stat(filename.c_str(), &statbuf) == 0);
}

void copy_file(const string& source, const string& dest)
{
  ifstream in(source.c_str());
  if (!in.good())
    return;
  ofstream out(dest.c_str());
  out<<in.rdbuf();
}

void touch_file(const string& filename)
{
  ofstream out(filename.c_str());
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

Dispatcher::Dispatcher
    (string dispatcher_share_name_,
     string index_share_name,
     string shadow_name_,
     string db_dir_,
     const vector< File_Properties* >& controlled_files_)
    : controlled_files(controlled_files_),
      data_footprints(controlled_files_.size()),
      map_footprints(controlled_files_.size()),
      shadow_name(shadow_name_), db_dir(db_dir_),
      dispatcher_share_name(dispatcher_share_name_)
{
  // get the absolute pathname of the current directory
  if (db_dir.substr(0, 1) != "/")
    db_dir = getcwd() + db_dir_;
  if (shadow_name.substr(0, 1) != "/")
    shadow_name = getcwd() + shadow_name_;
  
  // open dispatcher_share
  dispatcher_shm_fd = shm_open
      (dispatcher_share_name.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
  if (dispatcher_shm_fd < 0)
    throw File_Error
        (errno, dispatcher_share_name, "Dispatcher_Server::1");
  int foo = ftruncate(dispatcher_shm_fd, SHM_SIZE);
  dispatcher_shm_ptr = (uint8*)mmap
      (0, SHM_SIZE + db_dir.size(),
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
  set_current_footprints();
}

Dispatcher::~Dispatcher()
{
  shm_unlink(dispatcher_share_name.c_str());
}

void Dispatcher::write_start(pid_t pid)
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

  try
  {
    ofstream lock((shadow_name + ".lock").c_str());
    lock<<pid;
  }
  catch (...) {}

  bool lock_obtained = false;
  try
  {
    pid_t locked_pid;
    ifstream lock((shadow_name + ".lock").c_str());
    lock>>locked_pid;
    if (locked_pid == pid)
      lock_obtained = true;
  }
  catch (...) {}
  
  if (!lock_obtained)
    return;
  // Now we have successfully placed the lock.

  copy_mains_to_shadows();
  write_index_of_empty_blocks();
}

void Dispatcher::write_rollback()
{
  remove_shadows();
  remove((shadow_name + ".lock").c_str());
}

void Dispatcher::write_commit()
{
  if (!processes_reading_idx.empty())
    return;
  
  touch_file(shadow_name);
  copy_shadows_to_mains();
  remove(shadow_name.c_str());
  remove_shadows();
  remove((shadow_name + ".lock").c_str());
  set_current_footprints();
}

void Dispatcher::request_read_and_idx(pid_t pid)
{
  for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->register_pid(pid);
  for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->register_pid(pid);
  processes_reading_idx.insert(pid);
}

void Dispatcher::read_idx_finished(pid_t pid)
{
  processes_reading_idx.erase(pid);
}

void Dispatcher::read_finished(pid_t pid)
{
  for (vector< Idx_Footprints >::iterator it(data_footprints.begin());
      it != data_footprints.end(); ++it)
    it->unregister_pid(pid);
  for (vector< Idx_Footprints >::iterator it(map_footprints.begin());
      it != map_footprints.end(); ++it)
    it->unregister_pid(pid);
}

void Dispatcher::copy_shadows_to_mains()
{
  for (vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
    try
    {
      copy_file((*it)->get_file_base_name() + (*it)->get_data_suffix()
                + (*it)->get_index_suffix() + (*it)->get_shadow_suffix(),
		(*it)->get_file_base_name() + (*it)->get_data_suffix()
		+ (*it)->get_index_suffix());
    }
    catch (...) {}
    try
    {
      copy_file((*it)->get_file_base_name() + (*it)->get_id_suffix()
                + (*it)->get_index_suffix() + (*it)->get_shadow_suffix(),
		(*it)->get_file_base_name() + (*it)->get_id_suffix()
		+ (*it)->get_index_suffix());
    }
    catch (...) {}
  }
}

void Dispatcher::copy_mains_to_shadows()
{
  for (vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
    try
    {
      copy_file((*it)->get_file_base_name() + (*it)->get_data_suffix()
                + (*it)->get_index_suffix(),
		(*it)->get_file_base_name() + (*it)->get_data_suffix()
		+ (*it)->get_index_suffix() + (*it)->get_shadow_suffix());
    }
    catch (...) {}
    try
    {
      copy_file((*it)->get_file_base_name() + (*it)->get_id_suffix()
                + (*it)->get_index_suffix(),
		(*it)->get_file_base_name() + (*it)->get_id_suffix()
		+ (*it)->get_index_suffix() + (*it)->get_shadow_suffix());
    }
    catch (...) {}
  }
}

void Dispatcher::remove_shadows()
{
  for (vector< File_Properties* >::const_iterator it(controlled_files.begin());
      it != controlled_files.end(); ++it)
  {
    remove(((*it)->get_file_base_name() + (*it)->get_data_suffix()
            + (*it)->get_index_suffix() + (*it)->get_shadow_suffix()).c_str());
    remove(((*it)->get_file_base_name() + (*it)->get_id_suffix()
            + (*it)->get_index_suffix() + (*it)->get_shadow_suffix()).c_str());
    remove(((*it)->get_file_base_name() + (*it)->get_data_suffix()
            + (*it)->get_shadow_suffix()).c_str());
    remove(((*it)->get_file_base_name() + (*it)->get_id_suffix()
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
          (controlled_files[i]->get_data_footprint());
    }
    catch (...) {}
    try
    {
      map_footprints[i].set_current_footprint
          (controlled_files[i]->get_map_footprint());
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
		S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, "write_to_index_empty_file:1");
  int foo(write(file.fd, buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr))); foo = 0;
}

void Dispatcher::write_index_of_empty_blocks()
{
  for (vector< File_Properties* >::size_type i = 0;
      i < controlled_files.size(); ++i)
  {
    if (file_exists(controlled_files[i]->get_file_base_name()
        + controlled_files[i]->get_data_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file
          (data_footprints[i].total_footprint(),
	   controlled_files[i]->get_file_base_name()
	   + controlled_files[i]->get_data_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
    if (file_exists(controlled_files[i]->get_file_base_name()
        + controlled_files[i]->get_id_suffix()
	+ controlled_files[i]->get_index_suffix()
	+ controlled_files[i]->get_shadow_suffix()))
    {
      write_to_index_empty_file
          (map_footprints[i].total_footprint(),
	   controlled_files[i]->get_file_base_name()
	   + controlled_files[i]->get_id_suffix()
	   + controlled_files[i]->get_shadow_suffix());
    }
  }
}

void Dispatcher::standby_loop(uint64 milliseconds)
{
  uint32 counter = 0;
  while ((milliseconds == 0) || (counter < milliseconds/100))
  {
    if (*(uint32*)dispatcher_shm_ptr == 0)
    {
      ++counter;
      //sleep for a tenth of a second
      struct timeval timeout_;
      timeout_.tv_sec = 0;
      timeout_.tv_usec = 100*1000;
      select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);

      continue;
    }
    
    uint32 command = *(uint32*)dispatcher_shm_ptr;
    uint32 client_pid = *(uint32*)(dispatcher_shm_ptr + sizeof(uint32));
    if (command == WRITE_START)
      write_start(client_pid);
    else if (command == WRITE_ROLLBACK)
      write_rollback();
    else if (command == WRITE_COMMIT)
      write_commit();
    else if (command == REQUEST_READ_AND_IDX)
    {
      request_read_and_idx(client_pid);
      *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
    }
    else if (command == READ_IDX_FINISHED)
    {
      read_idx_finished(client_pid);
      *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
    }
    else if (command == READ_FINISHED)
    {
      read_finished(client_pid);
      *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = client_pid;
    }
    
    // Set command state to zero.
    *(uint32*)dispatcher_shm_ptr = 0;
  }
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

Dispatcher_Client::Dispatcher_Client
    (string dispatcher_share_name_)
    : dispatcher_share_name(dispatcher_share_name_)
{
  // open dispatcher_share
  dispatcher_shm_fd = shm_open
      (dispatcher_share_name.c_str(), O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
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
}

Dispatcher_Client::~Dispatcher_Client()
{
  shm_unlink(dispatcher_share_name.c_str());
}

void Dispatcher_Client::write_start()
{
  uint32 pid = getpid();
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::WRITE_START;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    //sleep for a tenth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (file_exists(shadow_name + ".lock"))
    {
      try
      {
	pid_t locked_pid;
	ifstream lock((shadow_name + ".lock").c_str());
	lock>>locked_pid;
	if (locked_pid == pid)
	  return;
      }
      catch (...) {}
    }

    timeout_.tv_usec = 1000*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  }
}

void Dispatcher_Client::write_rollback()
{
  uint32 pid = getpid();
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::WRITE_ROLLBACK;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    //sleep for a tenth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
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

    timeout_.tv_usec = 1000*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  }
}

void Dispatcher_Client::write_commit()
{
  uint32 pid = getpid();
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::WRITE_COMMIT;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    //sleep for a tenth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
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
    
    timeout_.tv_usec = 1000*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  }
}

void Dispatcher_Client::request_read_and_idx()
{
  uint32 pid = getpid();
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::REQUEST_READ_AND_IDX;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
    
    //sleep for a tenth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  }
}

void Dispatcher_Client::read_idx_finished()
{
  uint32 pid = getpid();
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::READ_IDX_FINISHED;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
    
    //sleep for a tenth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  }
}

void Dispatcher_Client::read_finished()
{
  uint32 pid = getpid();
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::READ_FINISHED;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
    
    //sleep for a tenth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  }
}

/**
  * Dispatcher - manages that a query gets a usable database and that
  * concurrent queries don't overflow the server's RAM
  *
  * Dispatcher accepts the following messages from a query in the first
  * 16 byte from the shared memory "/osm3s_v#.#.#" (see dispatcher.h::shared_name).
  * It answers in the following 8 bytes:
  * - (REGISTER_PID, pid, msg_id, void, void): registers a new query by its pid
  *   returns (pid, database_id) where database_id is the id of the usable
  *   database
  * - (SET_LIMITS, pid, msg_id, max_ram, timeout): sets the limit for a registered query
  *   after it has completed its forecast. returns (pid, ACCEPTED) or
  *   (pid, REJECTED) depeding on wether there a enough ressources on the
  *   server available
  * - (UNREGISTER_PID, pid, msg_id, void, void): unregisters a pid
  * - (SERVER_STATE, pid, msg_id, void, void): writes a full server state report into
  *   the file ${db_dir}/state.txt
  *
  * The dispatcher will issue a kill command if a process has not called
  * (UNREGISTER_PID, msg_id, pid, void, void) at latest at its timeout.
  *
  * Dispatcher monitors the database states as follows:
  * for each database in ${db_dir}/1/ or ${db_dir}/2/
  * - the database is considered updating when a file "dirty" exists
  * - if no such file exsists, the database's replicate version is read from
  *   the file "replicate_id" as plain integer
  * - if no "dirty" exists, the database's timestamp is read from
  *   the file "state" as string after the first equal sign, removing all
  *   backslashes
  */

/*struct Query_Skeleton
{
  uint32 used_db;
  uint32 max_ram;
  uint32 timeout;
};

void log_timestamp(ostream& logfile)
{
  time_t tt(time(NULL));
  struct tm* t(localtime(&tt));
  
  logfile<<(t->tm_year + 1900)<<'-'<<t->tm_mon<<'-'<<t->tm_mday<<' '
      <<t->tm_hour<<':'<<t->tm_min<<':'<<t->tm_sec<<": ";
}

void register_pid
  (uint8* shm_ptr, map< uint32, Query_Skeleton >& queries, ostream& logfile)
{
  uint32 pid(*(uint32*)(shm_ptr + 4));
  uint32 msg_id(*(uint32*)(shm_ptr + 8));
  Query_Skeleton& skel(queries[pid]);
  if (*(uint32*)(shm_ptr+OFFSET_DB_1) < *(uint32*)(shm_ptr+OFFSET_DB_2))
    skel.used_db = 2;
  else if (*(uint32*)(shm_ptr+OFFSET_DB_1) > 0)
    skel.used_db = 1;
  else
    skel.used_db = 0;
  skel.max_ram = 0;
  skel.timeout = time(NULL) + 60;
  
  *(uint32*)(shm_ptr+OFFSET_BACK) = pid;
  *(uint32*)(shm_ptr+OFFSET_BACK+4) = msg_id;
  *(uint32*)(shm_ptr+OFFSET_BACK+8) = skel.used_db;
  *(uint32*)shm_ptr = 0;
  
  log_timestamp(logfile);
  logfile<<"register "<<pid<<" @db "<<skel.used_db<<'\n';
  
  if (skel.used_db == 0)
    queries.erase(pid);
}

void set_limits
  (uint8* shm_ptr, map< uint32, Query_Skeleton >& queries,
   uint32& available_ram, ostream& logfile)
{
  uint32 pid(*(uint32*)(shm_ptr + 4));
  uint32 msg_id(*(uint32*)(shm_ptr + 8));
  Query_Skeleton& skel(queries[pid]);
  available_ram += skel.max_ram;
  skel.max_ram = *(uint32*)(shm_ptr + 12);
  skel.timeout = *(uint32*)(shm_ptr + 16);
  skel.timeout += time(NULL);
  
  if (skel.max_ram > available_ram)
  {
    *(uint32*)(shm_ptr+OFFSET_BACK+8) = QUERY_REJECTED;
    queries.erase(pid);
    
    log_timestamp(logfile);
    logfile<<"set_limits "<<pid<<" failed (too few ram available)\n";  
  }
  else
  {
    *(uint32*)(shm_ptr+OFFSET_BACK+8) = SET_LIMITS;
    available_ram -= skel.max_ram;
    
    log_timestamp(logfile);
    logfile<<"set_limits "<<pid<<" ram "<<skel.max_ram<<" timeout "<<skel.timeout
        <<'\n';  
  }
  *(uint32*)(shm_ptr+OFFSET_BACK) = pid;
  *(uint32*)(shm_ptr+OFFSET_BACK+4) = msg_id;
  *(uint32*)shm_ptr = 0;
}

void unregister_pid
  (uint8* shm_ptr, map< uint32, Query_Skeleton >& queries,
   uint32& available_ram, ostream& logfile)
{
  uint32 pid(*(uint32*)(shm_ptr + 4));
  uint32 msg_id(*(uint32*)(shm_ptr + 8));
  Query_Skeleton& skel(queries[pid]);
  available_ram += skel.max_ram;
  
  *(uint32*)(shm_ptr+OFFSET_BACK) = pid;
  *(uint32*)(shm_ptr+OFFSET_BACK+4) = msg_id;
  *(uint32*)shm_ptr = 0;
  
  log_timestamp(logfile);
  logfile<<"unregister "<<pid<<'\n';
  
  queries.erase(pid);
}

void report_server_state(uint8* shm_ptr, const string& db_dir,
			 const map< uint32, Query_Skeleton >& queries,
			 uint32 available_ram)
{
  uint32 pid(*(uint32*)(shm_ptr + 4));
  uint32 msg_id(*(uint32*)(shm_ptr + 8));
  ofstream state((db_dir + "state.txt").c_str());
  state<<"avail_ram\t"<<available_ram<<'\n';
  
  if (*(uint32*)(shm_ptr+OFFSET_DB_1))
    state<<"db 1\t"<<*(uint32*)(shm_ptr+OFFSET_DB_1)
        <<'\t'<<(shm_ptr+OFFSET_DB_1+4)<<'\n';
  else
    state<<"db 1\tdirty\n";
  
  if (*(uint32*)(shm_ptr+OFFSET_DB_2))
    state<<"db 2\t"<<*(uint32*)(shm_ptr+OFFSET_DB_2)
    <<'\t'<<(shm_ptr+OFFSET_DB_2+4)<<'\n';
  else
    state<<"db 2\tdirty\n";
  
  for (map< uint32, Query_Skeleton >::const_iterator it(queries.begin());
      it != queries.end(); ++it)
    state<<it->first<<'\t'<<it->second.used_db<<'\t'<<it->second.max_ram
        <<'\t'<<it->second.timeout<<'\n';
  
  *(uint32*)(shm_ptr+OFFSET_BACK) = pid;
  *(uint32*)(shm_ptr+OFFSET_BACK+4) = msg_id;
  *(uint32*)shm_ptr = 0;
}

void poll_db_state(uint8* shm_ptr, const string& db_dir)
{
  struct stat stat_buf;
  
  // query state of database 1
  if (!stat((db_dir + "1/dirty").c_str(), &stat_buf))
    *(uint32*)(shm_ptr+OFFSET_DB_1) = 0;
  else
  {
    ifstream replicate_id((db_dir + "1/replicate_id").c_str());
    if (!replicate_id)
    {
      cerr<<"Database 1 is clean but doesn't have a replicate id\n";
      exit(1);
    }
    replicate_id>>(*(uint32*)(shm_ptr+OFFSET_DB_1));

    char buf[256];
    ifstream timestamp((db_dir + "1/state").c_str());
    if (!timestamp)
    {
      cerr<<"Database 1 is clean but doesn't have a timestamp\n";
      exit(1);
    }
    timestamp.getline(buf, 255);
    string timestamp_s(buf);
    if (timestamp_s.find('=') != string::npos)
      timestamp_s = timestamp_s.substr(timestamp_s.find('=') + 1);
    while (timestamp_s.find('\\') != string::npos)
      timestamp_s = timestamp_s.substr(0, timestamp_s.find('\\'))
      + timestamp_s.substr(timestamp_s.find('\\') + 1);
    memcpy(shm_ptr+OFFSET_DB_1+4+1, timestamp_s.c_str(), timestamp_s.size()+1);
  }
  
  // query state of database 2
  if (!stat((db_dir + "2/dirty").c_str(), &stat_buf))
    *(uint32*)(shm_ptr+OFFSET_DB_2) = 0;
  else
  {
    ifstream replicate_id((db_dir + "2/replicate_id").c_str());
    if (!replicate_id)
    {
      cerr<<"Database 2 is clean but doesn't have a replicate id\n";
      exit(1);
    }
    replicate_id>>(*(uint32*)(shm_ptr+OFFSET_DB_2));
    
    char buf[256];
    ifstream timestamp((db_dir + "2/state").c_str());
    if (!timestamp)
    {
      cerr<<"Database 2 is clean but doesn't have a timestamp\n";
      exit(1);
    }
    timestamp.getline(buf, 255);
    string timestamp_s(buf);
    if (timestamp_s.find('=') != string::npos)
      timestamp_s = timestamp_s.substr(timestamp_s.find('=') + 1);
    while (timestamp_s.find('\\') != string::npos)
      timestamp_s = timestamp_s.substr(0, timestamp_s.find('\\'))
      + timestamp_s.substr(timestamp_s.find('\\') + 1);
    memcpy(shm_ptr+OFFSET_DB_2+4+1, timestamp_s.data(), timestamp_s.size()+1);
  }
}

void try_update_db(uint8* shm_ptr, const string& db_dir,
		   const map< uint32, Query_Skeleton >& queries, ostream& logfile)
{
  // ensure that both databases are cleared
  if (*(uint32*)(shm_ptr+OFFSET_DB_1) == 0)
    return;
  if (*(uint32*)(shm_ptr+OFFSET_DB_2) == 0)
    return;
  
  // mark the older of the two databases as dirty
  if (*(uint32*)(shm_ptr+OFFSET_DB_1) < *(uint32*)(shm_ptr+OFFSET_DB_2))
  {
    for (map< uint32, Query_Skeleton >::const_iterator it(queries.begin());
        it != queries.end(); ++it)
    {
      if (it->second.used_db == 1)
	return;
    }
    
    log_timestamp(logfile);
    logfile<<"mark db 1 as dirty\n";
    
    *(uint32*)(shm_ptr+OFFSET_DB_1) = 0;
    ofstream dummy((db_dir + "1/dirty").c_str());
    dummy.close();
  }
  else
  {
    for (map< uint32, Query_Skeleton >::const_iterator it(queries.begin());
    it != queries.end(); ++it)
    {
      if (it->second.used_db == 2)
	return;
    }
    
    log_timestamp(logfile);
    logfile<<"mark db 2 as dirty\n";
    
    *(uint32*)(shm_ptr+OFFSET_DB_2) = 0;
    ofstream dummy((db_dir + "2/dirty").c_str());
    dummy.close();
  }
}

void kill_runaway_processes
    (map< uint32, Query_Skeleton >& queries, ostream& logfile)
{
  uint32 time_(time(NULL));
  for (map< uint32, Query_Skeleton >::iterator it(queries.begin());
    it != queries.end(); )
  {
    if (it->second.timeout < time_)
    {
      uint32 pid(it->first);
      kill(pid, SIGTERM);
      queries.erase(pid);
      it = queries.upper_bound(pid);
      
      log_timestamp(logfile);
      logfile<<"killed runaway process "<<pid<<'\n';
    }
    else
      ++it;
  }
}

int main(int argc, char* argv[])
{
  // read command line arguments
  string db_dir("./");
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    ++argpos;
  }
  
  int shm_fd(shm_open(shared_name.c_str(), O_RDWR|O_CREAT|O_TRUNC,
		      S_IRWXU|S_IRWXG|S_IRWXO));
  if (shm_fd < 0)
  {
    cerr<<"Can't create shared memory "<<shared_name<<'\n';
    exit(1);
  }
  int foo(ftruncate(shm_fd, SHM_SIZE));
  uint8* shm_ptr((uint8*)
      mmap(0, SHM_SIZE + db_dir.size() + 1,
	   PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0));
  memcpy(shm_ptr+SHM_SIZE, db_dir.c_str(), db_dir.size()+1);
  
  ofstream logfile((db_dir + "dispatcher.log").c_str(), ios_base::app);
  logfile<<'\n';
      
  unsigned int counter(0);
  uint32 available_ram(2048);
  map< uint32, Query_Skeleton > queries;
  
  poll_db_state(shm_ptr, db_dir);
  
  while (true)
  {
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
    
    if (*(uint32*)shm_ptr == REGISTER_PID)
      register_pid(shm_ptr, queries, logfile);
    else if (*(uint32*)shm_ptr == SET_LIMITS)
      set_limits(shm_ptr, queries, available_ram, logfile);
    else if (*(uint32*)shm_ptr == UNREGISTER_PID)
      unregister_pid(shm_ptr, queries, available_ram, logfile);
    else if (*(uint32*)shm_ptr == SERVER_STATE)
      report_server_state(shm_ptr, db_dir, queries, available_ram);
    *(uint32*)shm_ptr = 0;
    logfile.flush();
    
    if (++counter >= 100)
    {
      kill_runaway_processes(queries, logfile);
      poll_db_state(shm_ptr, db_dir);
      try_update_db(shm_ptr, db_dir, queries, logfile);
      counter = 0;
    }
  }
  
  return 0;
}*/
