#include "dispatcher.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace std;

bool file_exists(const string& filename)
{
  return (access(filename.c_str(), F_OK) == 0);
}

void copy_file(const string& source, const string& dest)
{
  if (!file_exists(source))
    return;
  
  Raw_File source_file(source, O_RDONLY, S_666, "Dispatcher:1");
  uint64 size = lseek64(source_file.fd(), 0, SEEK_END);
  lseek64(source_file.fd(), 0, SEEK_SET);
  Raw_File dest_file(dest, O_RDWR|O_CREAT, S_666, "Dispatcher:2");
  int foo = ftruncate64(dest_file.fd(), size); foo = 0;
  
  Void_Pointer< uint8 > buf(64*1024);
  while (size > 0)
  {
    size = read(source_file.fd(), buf.ptr, 64*1024);
    size = write(dest_file.fd(), buf.ptr, size);
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
      (dispatcher_share_name.c_str(), O_RDWR|O_CREAT|O_TRUNC|O_EXCL, S_666);
  if (dispatcher_shm_fd < 0)
    throw File_Error
        (errno, dispatcher_share_name, "Dispatcher_Server::1");
  fchmod(dispatcher_shm_fd, S_666);
  int foo = ftruncate(dispatcher_shm_fd,
		      SHM_SIZE + db_dir.size() + shadow_name.size()); foo = 0;
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
    write_index_of_empty_blocks();
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

void Dispatcher::write_rollback()
{
  remove_shadows();
  remove((shadow_name + ".lock").c_str());
}

void Dispatcher::write_commit()
{
  if (!processes_reading_idx.empty())
    return;

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
  processes_reading_idx.erase(pid);
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
    catch (...) {}
    try
    {
      map_footprints[i].set_current_footprint
          (controlled_files[i]->get_map_footprint(db_dir));
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
  int foo(write(file.fd(), buffer.ptr, ((uint8*)pos) - ((uint8*)buffer.ptr))); foo = 0;
}

void Dispatcher::write_index_of_empty_blocks()
{
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
    
    try
    {
      uint32 command = *(uint32*)dispatcher_shm_ptr;
      uint32 client_pid = *(uint32*)(dispatcher_shm_ptr + sizeof(uint32));
      // Set command state to zero.
      *(uint32*)dispatcher_shm_ptr = 0;
      if (command == TERMINATE)
      {
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) =
	    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32));
      
        break;
      }
      else if (command == OUTPUT_STATUS)
      {
	output_status();
	*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) =
	    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32));
      }
      else if (command == WRITE_START)
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
    }
    catch (File_Error e)
    {
      cerr<<"File_Error "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
      
      counter += 30;
      //sleep for three seconds
      struct timeval timeout_;
      timeout_.tv_sec = 3;
      timeout_.tv_usec = 0;
      select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
  
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
    for (set< pid_t >::const_iterator it = processes_reading_idx.begin();
        it != processes_reading_idx.end(); ++it)
      status<<REQUEST_READ_AND_IDX<<' '<<*it<<'\n';
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
    for (set< pid_t >::const_iterator it = collected_pids.begin();
    it != collected_pids.end(); ++it)
    {
      if (processes_reading_idx.find(*it) == processes_reading_idx.end())
	status<<READ_IDX_FINISHED<<' '<<*it<<'\n';
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

Dispatcher_Client::Dispatcher_Client
    (string dispatcher_share_name_)
    : dispatcher_share_name(dispatcher_share_name_)
{
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
}

Dispatcher_Client::~Dispatcher_Client()
{
  munmap((void*)dispatcher_shm_ptr,
	 Dispatcher::SHM_SIZE + db_dir.size() + shadow_name.size());
  close(dispatcher_shm_fd);
}

void Dispatcher_Client::write_start()
{
  pid_t pid = getpid();
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
	pid_t locked_pid = 0;
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
  pid_t pid = getpid();
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
  pid_t pid = getpid();
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
    timeout_.tv_usec = 10*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
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
    timeout_.tv_usec = 10*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
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
    timeout_.tv_usec = 10*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
  }
}

void Dispatcher_Client::purge(uint32 pid)
{
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
    timeout_.tv_usec = 10*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
  }
}

void Dispatcher_Client::terminate()
{
  uint32 pid = getpid();
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::TERMINATE;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
    
    //sleep for a hundreth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 10*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
  }
}

void Dispatcher_Client::output_status()
{
  uint32 pid = getpid();
  *(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) = 0;
  while (true)
  {
    *(uint32*)dispatcher_shm_ptr = Dispatcher::OUTPUT_STATUS;
    *(uint32*)(dispatcher_shm_ptr + sizeof(uint32)) = pid;
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
    
    //sleep for a hundreth of a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 10*1000;
    select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    if (*(uint32*)(dispatcher_shm_ptr + 2*sizeof(uint32)) == pid)
      return;
  }
}
