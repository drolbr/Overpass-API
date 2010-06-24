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

using namespace std;

/**
  * Dispatcher - manages that a query gets a usable database and that
  * concurrent queries don't overflow the server's RAM
  *
  * Dispatcher accepts the following messages from a query in the first
  * 16 byte from the shared memory "/osm3s"; it answers in the following 8 bytes:
  * - (REGISTER_PID, pid, void, void): registers a new query by its pid
  *   returns (pid, database_id) where database_id is the id of the usable
  *   database
  * - (SET_LIMITS, pid, max_ram, timeout): sets the limit for a registered query
  *   after it has completed its forecast. returns (pid, ACCEPTED) or
  *   (pid, REJECTED) depeding on wether there a enough ressources on the
  *   server available
  * - (UNREGISTER_PID, pid, void, void): unregisters a pid
  * - (SERVER_STATE, void, void, void): writes a full server state report into
  *   the file ${db_dir}/state.txt
  *
  * The dispatcher will issue a kill command if a process has not called
  * (UNREGISTER_PID, pid, void, void) at latest at its timeout.
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

const int SHM_SIZE = 16+8+2*(256+4);
const int OFFSET_DB_1 = 16+8;
const int OFFSET_DB_2 = 16+8+(256+4);

struct Query_Skeleton
{
  int used_db;
  int max_ram;
  int timeout;
};

void report_server_state(uint8* shm_ptr, const string& db_dir,
			 const map< int, Query_Skeleton >& queries)
{
  ofstream state((db_dir + "state.txt").c_str());
  
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
    cerr<<(*(uint32*)(shm_ptr+OFFSET_DB_1))<<' ';

    char buf[256];
    ifstream timestamp((db_dir + "1/state").c_str());
    if (!timestamp)
    {
      cerr<<"Database 1 is clean but doesn't have a timestamp\n";
      exit(1);
    }
    timestamp.getline(buf, 256);
    string timestamp_s(buf);
    if (timestamp_s.find('=') != string::npos)
      timestamp_s = timestamp_s.substr(timestamp_s.find('=') + 1);
    while (timestamp_s.find('\\') != string::npos)
      timestamp_s = timestamp_s.substr(0, timestamp_s.find('\\'))
      + timestamp_s.substr(timestamp_s.find('\\') + 1);
    memcpy(shm_ptr+OFFSET_DB_1+4, timestamp_s.data(), timestamp_s.size());
    cerr<<(shm_ptr+OFFSET_DB_1+4)<<'\n';
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
    cerr<<(*(uint32*)(shm_ptr+OFFSET_DB_2))<<' ';
    
    char buf[256];
    ifstream timestamp((db_dir + "2/state").c_str());
    if (!timestamp)
    {
      cerr<<"Database 2 is clean but doesn't have a timestamp\n";
      exit(1);
    }
    timestamp.getline(buf, 256);
    string timestamp_s(buf);
    if (timestamp_s.find('=') != string::npos)
      timestamp_s = timestamp_s.substr(timestamp_s.find('=') + 1);
    while (timestamp_s.find('\\') != string::npos)
      timestamp_s = timestamp_s.substr(0, timestamp_s.find('\\'))
      + timestamp_s.substr(timestamp_s.find('\\') + 1);
    memcpy(shm_ptr+OFFSET_DB_2+4, timestamp_s.data(), timestamp_s.size());
    cerr<<(shm_ptr+OFFSET_DB_2+4)<<'\n';
  }
}

void try_update_db(uint8* shm_ptr, const string& db_dir)
{
  // ensure that both databases are cleared
  if (*(uint32*)(shm_ptr+OFFSET_DB_1) == 0)
    return;
  if (*(uint32*)(shm_ptr+OFFSET_DB_2) == 0)
    return;
  
  // mark the older of the two databases as dirty
  if (*(uint32*)(shm_ptr+OFFSET_DB_1) < *(uint32*)(shm_ptr+OFFSET_DB_2))
  {
    ofstream dummy((db_dir + "1/dirty").c_str());
    dummy.close();
  }
  else
  {
    ofstream dummy((db_dir + "2/dirty").c_str());
    dummy.close();
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
  
  int shm_fd(shm_open("/osm3s", O_RDWR|O_CREAT|O_TRUNC,
		      S_IRWXU|S_IRWXG|S_IRWXO));
  if (shm_fd < 0)
  {
    cerr<<"Can't create shared memory /osm3s\n";
    exit(1);
  }
  int foo(ftruncate(shm_fd, SHM_SIZE));
  uint8* shm_ptr((uint8*)
      mmap(0, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0));
  
  unsigned int counter(0);
  
  while (true)
  {
    //sleep for a second
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
    
/*    cerr<<'.';*/
    
    if (++counter >= 50)
    {
      poll_db_state(shm_ptr, db_dir);
      try_update_db(shm_ptr, db_dir);
      counter = 0;
    }
  }
  
  return 0;
}
