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

#include "block_backend.h"
#include "dispatcher.h"
#include "dispatcher_client.h"
#include "file_blocks.h"
#include "random_file.h"
#include "transaction.h"

#include <fstream>
#include <iostream>
#include <sstream>


//-----------------------------------------------------------------------------


/* Sample class for TIndex */
struct IntIndex
{
  typedef uint32 Id_Type;

  IntIndex(uint32 i) : value(i) {}
  IntIndex(void* data) : value(*(uint32*)data) {}

  uint32 size_of() const
  {
    return 4;
  }

  static uint32 size_of(void* data)
  {
    return 4;
  }

  static uint32 max_size_of()
  {
    return 4;
  }

  void to_data(void* data) const
  {
    *(uint32*)data = value;
  }

  bool operator<(const IntIndex& index) const
  {
    return this->value < index.value;
  }

  bool operator==(const IntIndex& index) const
  {
    return this->value == index.value;
  }

  uint32 val() const
  {
    return value;
  }

  private:
    uint32 value;
};


struct IntObject
{
  typedef uint32 Id_Type;

  IntObject(void* data) : value(*(uint32*)data) {}
  IntObject(int i) : value(i) {}

  uint32 size_of() const
  {
    return 4;
  }

  static uint32 size_of(void* data)
  {
    return 4;
  }

  void to_data(void* data) const
  {
    *(uint32*)data = value;
  }

  bool operator<(const IntObject& index) const
  {
    return this->value < index.value;
  }

  int val() const
  {
    return value;
  }

  private:
    uint32 value;
};


//-----------------------------------------------------------------------------

/* We use our own test settings */
std::string BASE_DIRECTORY("./");
std::string ID_SUFFIX(".map");


struct Test_File : File_Properties
{
  Test_File(std::string basename_) : basename(basename_), basedir(BASE_DIRECTORY) {}

  const std::string& get_basedir() const
  {
    return basedir;
  }

  void set_basedir(const std::string& basedir_)
  {
    basedir = basedir_;
  }

  const std::string& get_file_name_trunk() const
  {
    return basename;
  }

  const std::string& get_index_suffix() const
  {
    static std::string result(".idx");
    return result;
  }

  const std::string& get_data_suffix() const
  {
    static std::string result(".bin");
    return result;
  }

  const std::string& get_id_suffix() const
  {
    return ID_SUFFIX;
  }

  const std::string& get_shadow_suffix() const
  {
    static std::string result(".shadow");
    return result;
  }

  uint32 get_block_size() const
  {
    return 512;
  }

  uint32 get_compression_factor() const
  {
    return 1;
  }

  uint32 get_compression_method() const
  {
    return 0;
  }

  uint32 get_map_compression_method() const
  {
    return 0;
  }

  uint32 get_map_block_size() const
  {
    return 16*IntIndex::max_size_of();
  }

  uint32 get_map_compression_factor() const
  {
    return 1;
  }

  std::vector< bool > get_data_footprint(const std::string& db_dir) const
  {
    return get_data_index_footprint< IntIndex >(*this, db_dir);
  }

  std::vector< bool > get_map_footprint(const std::string& db_dir) const
  {
    return get_map_index_footprint(*this, db_dir);
  }

  uint32 id_max_size_of() const
  {
    return IntIndex::max_size_of();
  }

  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    return new File_Blocks_Index< IntIndex >
        (*this, writeable, use_shadow, db_dir, file_name_extension);
  }

  std::string basename, basedir;
};


//-----------------------------------------------------------------------------


struct Simple_Dispatcher_Logger : Dispatcher_Logger
{
  typedef uint pid_t;

  virtual void write_start(pid_t pid, const std::vector< pid_t >& registered) {}
  virtual void write_rollback(pid_t pid) {}
  virtual void write_commit(pid_t pid) {}
  virtual void request_read_and_idx(
      pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space, const std::string& client_token);
  virtual void read_idx_finished(pid_t pid) {}
  virtual void prolongate(pid_t pid) {}
  virtual void idle_counter(uint32 idle_count) {}
  virtual void read_finished(pid_t pid);
  virtual void read_aborted(pid_t pid) {}
  virtual void purge(pid_t pid) {}
};


std::string to_hex(const std::string& input)
{
  std::string result(input.size()*2, ' ');
  for (uint i = 0; i < input.size(); ++i)
  {
    char upper = (input[i]>>4) & 0xf;
    char lower = input[i] & 0xf;
    result[i*2] = (upper > 9 ? upper + ('a' - 10) : upper + '0');
    result[i*2+1] = (lower > 9 ? lower + ('a' - 10) : lower + '0');
  }
  return result;
}


void Simple_Dispatcher_Logger::request_read_and_idx
    (pid_t pid, uint32 max_allowed_time, uint64 max_allowed_space, const std::string& client_token)
{
  std::cout<<"request_read_and_idx of process "<<pid<<" timeout "<<max_allowed_time
      <<" space "<<max_allowed_space<<" token "<<to_hex(client_token)<<'\n';
}


void Simple_Dispatcher_Logger::read_finished(pid_t pid)
{
  std::cout<<"read_finished of process "<<pid<<'\n';
}


//-----------------------------------------------------------------------------


void put_elem(uint32 idx, uint32 val, const Test_File& tf,
	      const std::string& db_dir = BASE_DIRECTORY)
{
  std::map< IntIndex, std::set< IntObject > > to_delete;
  std::map< IntIndex, std::set< IntObject > > to_insert;
  to_insert[IntIndex(idx)].insert(IntObject(val));
  try
  {
    Nonsynced_Transaction transaction(true, true, db_dir, "");
    Block_Backend< IntIndex, IntObject > db_backend
        (transaction.data_index(&tf));
    db_backend.update(to_delete, to_insert);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}


int main(int argc, char* args[])
{
  std::string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];

  if (test_to_execute == "server")
  {
    Test_File test_file("Test_File");

    std::vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Simple_Dispatcher_Logger logger;
      Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			    BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			    5, 180, 4ull*1024*1024*1024,  4*1024*1024, file_properties, &logger);
      dispatcher.write_start(480);
      put_elem(0, 1, test_file);
      dispatcher.write_commit(0);

      std::cerr<<"[server] Starting ...\n";
      dispatcher.standby_loop(0);
      std::cerr<<"[server] done.\n";
      return 0;
    }
    catch (File_Error e)
    {
      std::cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  if (test_to_execute == "client" && argc > 3)
  {
    uint32 execution_time = atoi(args[2]);
    
    Test_File test_file("Test_File");

    std::vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());

      dispatcher_client.request_read_and_idx(24*60, 1024*1024, args[3]);
      dispatcher_client.read_idx_finished();

      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = execution_time*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }

      dispatcher_client.read_finished();
    }
    catch (File_Error e)
    {
      std::cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  if (test_to_execute == "status")
  {
    Test_File test_file("Test_File");

    std::vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());

      dispatcher_client.output_status();
    }
    catch (File_Error e)
    {
      std::cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  if (test_to_execute == "terminate")
  {
    Test_File test_file("Test_File");

    std::vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());

      dispatcher_client.terminate();
    }
    catch (File_Error e)
    {
      std::cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
}
