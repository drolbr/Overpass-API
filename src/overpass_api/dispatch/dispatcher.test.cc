#include <fstream>
#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../core/datatypes.h"
#include "dispatcher.h"

using namespace std;

uint32 msg_id(0);

void show_state(uint8* shm_ptr, const string& db_dir)
{
  uint32 pid(getpid());
  
  cerr<<"Requesting state ";
  *(uint32*)(shm_ptr + 4) = pid;
  *(uint32*)(shm_ptr + 8) = ++msg_id;
  *(uint32*)shm_ptr = SERVER_STATE;
  
  while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
      (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
  {
    cerr<<'.';
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);

    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)(shm_ptr + 8) = msg_id;
    *(uint32*)shm_ptr = SERVER_STATE;
  }
  cerr<<" done.\n";

  ifstream in((db_dir + "state.txt").c_str());
  string s;
  while (in.good())
  {
    getline(in, s);
    cout<<s<<'\n';
  }
}

void register_process(uint8* shm_ptr, uint32 pid)
{
  cerr<<"Registering "<<pid<<' ';
  *(uint32*)(shm_ptr + 4) = pid;
  *(uint32*)(shm_ptr + 8) = ++msg_id;
  *(uint32*)shm_ptr = REGISTER_PID;
  
  while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
    (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
  {
    cerr<<'.';
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);

    *(uint32*)(shm_ptr + 8) = msg_id;
    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)shm_ptr = REGISTER_PID;
  }
  cerr<<" done.\n";
}

bool set_limits(uint8* shm_ptr, uint32 pid, uint32 max_ram, uint32 timeout)
{
  cerr<<"Setting limits "<<pid<<": ram "<<max_ram<<", timeout "<<timeout<<' ';
  *(uint32*)(shm_ptr + 4) = pid;
  *(uint32*)(shm_ptr + 8) = ++msg_id;
  *(uint32*)(shm_ptr + 12) = max_ram;
  *(uint32*)(shm_ptr + 16) = timeout;
  *(uint32*)shm_ptr = SET_LIMITS;
  
  while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
    (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
  {
    cerr<<'.';
    //sleep for a second
    struct timeval timeout_;
    timeout_.tv_sec = 0;
    timeout_.tv_usec = 100000;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout_);
    
    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)(shm_ptr + 8) = msg_id;
    *(uint32*)(shm_ptr + 12) = max_ram;
    *(uint32*)(shm_ptr + 16) = timeout;
    *(uint32*)shm_ptr = SET_LIMITS;
  }
  cerr<<" done.\n";
  return (*(uint32*)(shm_ptr + OFFSET_BACK + 8) == SET_LIMITS);
}

void unregister_process(uint8* shm_ptr, uint32 pid)
{
  cerr<<"Unregistering "<<pid<<' ';
  *(uint32*)(shm_ptr + 8) = ++msg_id;
  *(uint32*)(shm_ptr + 4) = pid;
  *(uint32*)shm_ptr = UNREGISTER_PID;
  
  while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
    (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
  {
    cerr<<'.';
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);

    *(uint32*)(shm_ptr + 8) = msg_id;
    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)shm_ptr = UNREGISTER_PID;
  }
  cerr<<" done.\n";
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
  
  int shm_fd(shm_open(shared_name.c_str(), O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO));
  if (shm_fd < 0)
  {
    cerr<<"Can't open shared memory "<<shared_name<<'\n';
    exit(1);
  }
  uint8* shm_ptr((uint8*)
      mmap(0, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0));
  
  unsigned int counter(0);
  
  show_state(shm_ptr, db_dir);
  register_process(shm_ptr, 496 + getpid());
  show_state(shm_ptr, db_dir);
  set_limits(shm_ptr, 496 + getpid(), 512, 180);
  show_state(shm_ptr, db_dir);
  unregister_process(shm_ptr, 496 + getpid());
  show_state(shm_ptr, db_dir);
  bool is_registered(false);
  while (!is_registered)
  {
    register_process(shm_ptr, getpid());
    is_registered = set_limits(shm_ptr, getpid(), 64, 10);
  }
  show_state(shm_ptr, db_dir);
  
  register_process(shm_ptr, 500 + getpid());
  register_process(shm_ptr, 501 + getpid());
  register_process(shm_ptr, 502 + getpid());
  register_process(shm_ptr, 503 + getpid());
  register_process(shm_ptr, 504 + getpid());
  show_state(shm_ptr, db_dir);
  set_limits(shm_ptr, 500 + getpid(), 512, 180);
  set_limits(shm_ptr, 501 + getpid(), 512, 180);
  set_limits(shm_ptr, 502 + getpid(), 512, 180);
  set_limits(shm_ptr, 503 + getpid(), 512, 180);
  set_limits(shm_ptr, 504 + getpid(), 512, 180);
  show_state(shm_ptr, db_dir);
  unregister_process(shm_ptr, 500 + getpid());
  unregister_process(shm_ptr, 501 + getpid());
  unregister_process(shm_ptr, 502 + getpid());
  unregister_process(shm_ptr, 503 + getpid());
  unregister_process(shm_ptr, 504 + getpid());
  
  cerr<<"Waiting to be killed ";
  while (true)
  {
    cerr<<'.';
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
  }
  cerr<<" done.\n";
  
  return 0;
}
