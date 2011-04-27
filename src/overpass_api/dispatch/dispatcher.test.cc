#include <fstream>
#include <iostream>

/*#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../core/datatypes.h"*/
#include "dispatcher.h"

using namespace std;

//-----------------------------------------------------------------------------

/* We use our own test settings */
string BASE_DIRECTORY("./");
string ID_SUFFIX(".map");

struct Test_File : File_Properties
{
  Test_File(string basename_) : basename(basename_) {}
  
  string get_basedir() const
  {
    return BASE_DIRECTORY;
  }
  
  string get_file_base_name() const
  {
    return BASE_DIRECTORY + basename;
  }
  
  string get_index_suffix() const
  {
    return ".idx";
  }
  
  string get_data_suffix() const
  {
    return ".bin";
  }
  
  string get_id_suffix() const
  {
    return ID_SUFFIX;
  }
  
  string get_shadow_suffix() const
  {
    return ".shadow";
  }
  
  uint32 get_block_size() const
  {
    return 512;
  }
  
  uint32 get_map_block_size() const
  {
    return 16;
  }
  
  vector< bool > get_data_footprint() const
  {
    return vector< bool >();
  }
  
  vector< bool > get_map_footprint() const
  {
    return vector< bool >();
  }  
  
  string basename;
};

//-----------------------------------------------------------------------------

void create_dummy_files
    (const File_Properties& test_file_1,
     const File_Properties& test_file_2,
     const File_Properties& test_file_3,
     const File_Properties& test_file_4,
     bool shadow)
{
  if (!shadow)
  {
    {
      ofstream test_bin_out((test_file_1.get_file_base_name()
          + test_file_1.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 1\n";
      ofstream test_idx_out((test_file_1.get_file_base_name()
          + test_file_1.get_data_suffix() + test_file_1.get_index_suffix()).c_str());
      test_idx_out<<"This is test file bin idx 1\n";
    }
    {
      ofstream test_bin_out((test_file_2.get_file_base_name()
          + test_file_2.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 2\n";
      ofstream test_idx_out((test_file_2.get_file_base_name()
          + test_file_2.get_id_suffix() + test_file_2.get_index_suffix()).c_str());
      test_idx_out<<"This is test file map idx 2\n";
    }
    {
      ofstream test_bin_out((test_file_3.get_file_base_name()
          + test_file_3.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 3\n";
      ofstream test_idx_out((test_file_3.get_file_base_name()
          + test_file_3.get_data_suffix() + test_file_3.get_index_suffix()).c_str());
      test_idx_out<<"This is test file bin idx 3\n";
    }
    {
      ofstream test_bin_out((test_file_3.get_file_base_name()
          + test_file_3.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 3\n";
      ofstream test_idx_out((test_file_3.get_file_base_name()
          + test_file_3.get_id_suffix()+ test_file_3.get_index_suffix()).c_str());
      test_idx_out<<"This is test file map idx 3\n";
    }
    {
      ofstream test_bin_out((test_file_4.get_file_base_name()
          + test_file_4.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 4\n";
      ofstream test_idx_out((test_file_4.get_file_base_name()
          + test_file_4.get_data_suffix() + test_file_4.get_index_suffix()).c_str());
      test_idx_out<<"This is test file bin idx 4\n";
    }
    {
      ofstream test_bin_out((test_file_4.get_file_base_name()
          + test_file_4.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 4\n";
      ofstream test_idx_out((test_file_4.get_file_base_name()
          + test_file_4.get_id_suffix()+ test_file_4.get_index_suffix()).c_str());
      test_idx_out<<"This is test file map idx 4\n";
    }
  }
  else
  {
    {
      ofstream test_bin_out((test_file_1.get_file_base_name()
          + test_file_1.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin shadow 1\n";
      ofstream test_idx_out((test_file_1.get_file_base_name()
          + test_file_1.get_data_suffix() + test_file_1.get_index_suffix()
	  + test_file_1.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file bin idx shadow 1\n";
    }
    {
      ofstream test_bin_out((test_file_2.get_file_base_name()
          + test_file_2.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map shadow 2\n";
      ofstream test_idx_out((test_file_2.get_file_base_name()
          + test_file_2.get_id_suffix() + test_file_2.get_index_suffix()
	  + test_file_2.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file map idx shadow 2\n";
    }
    {
      ofstream test_bin_out((test_file_3.get_file_base_name()
          + test_file_3.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin shadow 3\n";
      ofstream test_idx_out((test_file_3.get_file_base_name()
          + test_file_3.get_data_suffix() + test_file_3.get_index_suffix()
	  + test_file_3.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file bin idx shadow 3\n";
    }
    {
      ofstream test_bin_out((test_file_3.get_file_base_name()
          + test_file_3.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map shadow 3\n";
      ofstream test_idx_out((test_file_3.get_file_base_name()
          + test_file_3.get_id_suffix() + test_file_3.get_index_suffix()
	  + test_file_3.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file map idx shadow 3\n";
    }
    {
      ofstream test_bin_out((test_file_4.get_file_base_name()
          + test_file_4.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin shadow 4\n";
      ofstream test_idx_out((test_file_4.get_file_base_name()
          + test_file_4.get_data_suffix() + test_file_4.get_index_suffix()
	  + test_file_4.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file bin idx shadow 4\n";
    }
    {
      ofstream test_bin_out((test_file_4.get_file_base_name()
          + test_file_4.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map shadow 4\n";
      ofstream test_idx_out((test_file_4.get_file_base_name()
          + test_file_4.get_id_suffix()+ test_file_4.get_index_suffix()
	  + test_file_4.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file map idx shadow 4\n";
    }
  }
}

int main(int argc, char* args[])
{
  string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    Test_File test_file_1("Test_File_1");
    Test_File test_file_2("Test_File_2");
    Test_File test_file_3("Test_File_3");
    Test_File test_file_4("Test_File_4");
    
    create_dummy_files(test_file_1, test_file_2, test_file_3, test_file_4, false);
      
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file_1);
    file_properties.push_back(&test_file_2);
    file_properties.push_back(&test_file_3);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", file_properties);
  }
  
  if ((test_to_execute == "") || (test_to_execute == "2"))
  {
    Test_File test_file_1("Test_File_1");
    Test_File test_file_2("Test_File_2");
    Test_File test_file_3("Test_File_3");
    Test_File test_file_4("Test_File_4");
    
    create_dummy_files(test_file_1, test_file_2, test_file_3, test_file_4, true);
    {
      ofstream test_bin_out((BASE_DIRECTORY + "test-shadow").c_str());
    }
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file_1);
    file_properties.push_back(&test_file_2);
    file_properties.push_back(&test_file_3);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  "test-shadow", file_properties);
  }
}

/*uint32 msg_id(0);

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
}*/
