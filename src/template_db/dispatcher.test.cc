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
string BASE_DIRECTORY("./");
string ID_SUFFIX(".map");


struct Test_File : File_Properties
{
  Test_File(string basename_) : basename(basename_), basedir(BASE_DIRECTORY) {}
  
  const std::string& get_basedir() const
  {
    return basedir;
  }
  
  void set_basedir(const string& basedir_)
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
  
  uint32 get_max_size() const
  {
    return 1;
  }
  
  uint32 get_compression_method() const
  {
    return 0;
  }
  
  uint32 get_map_block_size() const
  {
    return 16*IntIndex::max_size_of();
  }
  
  uint32 get_map_max_size() const
  {
    return 1;
  }
  
  vector< bool > get_data_footprint(const std::string& db_dir) const
  {
    return get_data_index_footprint< IntIndex >(*this, db_dir);
  }
  
  vector< bool > get_map_footprint(const std::string& db_dir) const
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
  
  string basename, basedir;
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
      ofstream test_bin_out((BASE_DIRECTORY + test_file_1.get_file_name_trunk()
          + test_file_1.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 1\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_1.get_file_name_trunk()
          + test_file_1.get_data_suffix() + test_file_1.get_index_suffix()).c_str());
      test_idx_out<<"This is test file bin idx 1\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_2.get_file_name_trunk()
          + test_file_2.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 2\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_2.get_file_name_trunk()
          + test_file_2.get_id_suffix() + test_file_2.get_index_suffix()).c_str());
      test_idx_out<<"This is test file map idx 2\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 3\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_data_suffix() + test_file_3.get_index_suffix()).c_str());
      test_idx_out<<"This is test file bin idx 3\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 3\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_id_suffix()+ test_file_3.get_index_suffix()).c_str());
      test_idx_out<<"This is test file map idx 3\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 4\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_data_suffix() + test_file_4.get_index_suffix()).c_str());
      test_idx_out<<"This is test file bin idx 4\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 4\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_id_suffix()+ test_file_4.get_index_suffix()).c_str());
      test_idx_out<<"This is test file map idx 4\n";
    }
  }
  else
  {
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_1.get_file_name_trunk()
          + test_file_1.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 1\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_1.get_file_name_trunk()
          + test_file_1.get_data_suffix() + test_file_1.get_index_suffix()
	  + test_file_1.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file bin idx shadow 1\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_2.get_file_name_trunk()
          + test_file_2.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 2\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_2.get_file_name_trunk()
          + test_file_2.get_id_suffix() + test_file_2.get_index_suffix()
	  + test_file_2.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file map idx shadow 2\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 3\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_data_suffix() + test_file_3.get_index_suffix()
	  + test_file_3.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file bin idx shadow 3\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 3\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_3.get_file_name_trunk()
          + test_file_3.get_id_suffix() + test_file_3.get_index_suffix()
	  + test_file_3.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file map idx shadow 3\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_data_suffix()).c_str());
      test_bin_out<<"This is test file bin 4\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_data_suffix() + test_file_4.get_index_suffix()
	  + test_file_4.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file bin idx shadow 4\n";
    }
    {
      ofstream test_bin_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_id_suffix()).c_str());
      test_bin_out<<"This is test file map 4\n";
      ofstream test_idx_out((BASE_DIRECTORY + test_file_4.get_file_name_trunk()
          + test_file_4.get_id_suffix()+ test_file_4.get_index_suffix()
	  + test_file_4.get_shadow_suffix()).c_str());
      test_idx_out<<"This is test file map idx shadow 4\n";
    }
  }
}

void map_read_test(bool use_shadow = false)
{
  try
  {
    cout<<"Read test\n";
    vector< bool > footprint =
        get_map_index_footprint(Test_File("Test_File"), BASE_DIRECTORY, use_shadow);
    cout<<"Index footprint: ";
    for (vector< bool >::const_iterator it(footprint.begin());
        it != footprint.end(); ++it)
    cout<<*it;
    cout<<'\n';

    Nonsynced_Transaction transaction(false, use_shadow, BASE_DIRECTORY, "");
    Test_File tf("Test_File");
    Random_File< IntIndex, IntIndex > id_file(transaction.random_index(&tf));
    
    cout<<id_file.get(0u).val()<<'\n';
    
    cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
}

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Flat_Iterator& it)
{
  if (it == blocks.flat_end())
  {
    cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.flat_end()))
  {
    if (!(current_idx == it.index()))
    {
      current_idx = it.index();
      cout<<"\nIndex "<<current_idx.val()<<": ";
    }
    cout<<it.object().val()<<' ';
    ++it;
  }
  cout<<'\n';
}

void data_read_test(const Test_File& tf, Transaction& transaction)
{
  try
  {
    Block_Backend< IntIndex, IntObject >
        db_backend(transaction.data_index(&tf));
    
    cout<<"Read test\n";
    vector< bool > footprint = get_data_index_footprint< IntIndex >
        (tf, tf.get_basedir());
    cout<<"Index footprint: ";
    for (vector< bool >::const_iterator it(footprint.begin());
    it != footprint.end(); ++it)
    cout<<*it;
    cout<<'\n';
    
    Block_Backend< IntIndex, IntObject >::Flat_Iterator fit(db_backend.flat_begin());
    read_loop(db_backend, fit);
    cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void data_read_test(const Test_File& tf)
{
  Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
  data_read_test(tf, transaction);
}

void put_elem(uint32 idx, uint32 val, const Test_File& tf,
	      const string& db_dir = BASE_DIRECTORY)
{
  map< IntIndex, set< IntObject > > to_delete;
  map< IntIndex, set< IntObject > > to_insert;
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
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
}

void sync_log(const string& message)
{
  ofstream out("sync.log", ios_base::app);
  out<<message;
}

int main(int argc, char* args[])
{
  string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    try
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
			    BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			    5, 180, 1024*1024*1024, 1024*1024, file_properties);
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
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
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
  }

  if ((test_to_execute == "") || (test_to_execute == "3"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
  }
  
  if ((test_to_execute == "") || (test_to_execute == "4"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    map_read_test(true);
    remove("Test_File.map");
    remove("Test_File.map.idx.shadow");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "5"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(495);
    dispatcher.write_start(480);
    try
    {
      pid_t locked_pid = 0;
      ifstream lock((BASE_DIRECTORY + "test-shadow" + ".lock").c_str());
      lock>>locked_pid;
      cout<<"The lock file points to pid "<<locked_pid<<'\n';
    }
    catch (...) {}
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    map_read_test(true);
    remove("Test_File.map");
    remove("Test_File.map.idx.shadow");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "6"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.write_commit(0);
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "7"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.write_commit(0);
    dispatcher.write_start(481);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 2);
    }
    dispatcher.write_commit(0);
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "8"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.write_commit(0);
    dispatcher.write_start(481);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 2);
    }
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 3);
    }
    dispatcher.write_commit(0);
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "9"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    put_elem(0, 1, test_file);
    dispatcher.write_commit(0);
    data_read_test(test_file);
    remove("Test_File.bin");
    remove("Test_File.bin.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "10"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    put_elem(0, 1, test_file);
    dispatcher.write_commit(0);
    dispatcher.write_start(481);
    put_elem(0, 2, test_file);
    dispatcher.write_commit(0);
    data_read_test(test_file);
    remove("Test_File.bin");
    remove("Test_File.bin.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "11"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    put_elem(0, 1, test_file);
    dispatcher.write_commit(0);
    dispatcher.write_start(481);
    put_elem(0, 2, test_file);
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    put_elem(0, 3, test_file);
    dispatcher.write_commit(0);
    data_read_test(test_file);
    remove("Test_File.bin");
    remove("Test_File.bin.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "12"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.output_status();
    dispatcher.write_commit(0);
    cout<<"Write lock is "
        <<(file_exists("test-shadow.lock") ? "still set.\n" : "released.\n");
    dispatcher.read_idx_finished(640);
    dispatcher.write_commit(0);
    cout<<"Write lock is "
        <<(file_exists("test-shadow.lock") ? "still set.\n" : "released.\n");
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "13"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.read_idx_finished(640);
    dispatcher.output_status();
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.read_finished(640);
    dispatcher.write_commit(0);
    dispatcher.write_start(481);
    dispatcher.request_read_and_idx(641, 180, 512*1024*1024, 641);
    dispatcher.read_idx_finished(641);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 2);
    }
    dispatcher.read_finished(641);
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    dispatcher.request_read_and_idx(642, 180, 512*1024*1024, 642);
    dispatcher.read_idx_finished(642);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 3);
    }
    dispatcher.read_finished(642);
    dispatcher.write_commit(0);
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "14"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.read_idx_finished(640);
    put_elem(0, 1, test_file);
    dispatcher.read_finished(640);
    dispatcher.output_status();
    dispatcher.write_commit(0);
    dispatcher.write_start(481);
    dispatcher.request_read_and_idx(641, 180, 512*1024*1024, 641);
    dispatcher.read_idx_finished(641);
    put_elem(0, 2, test_file);
    dispatcher.read_finished(641);
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    dispatcher.request_read_and_idx(642, 180, 512*1024*1024, 642);
    dispatcher.read_idx_finished(642);
    put_elem(0, 3, test_file);
    dispatcher.read_finished(642);
    dispatcher.write_commit(0);
    data_read_test(test_file);
    remove("Test_File.bin");
    remove("Test_File.bin.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "15"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.write_commit(0);
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.read_idx_finished(640);
    dispatcher.write_start(481);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 2);
    }
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    dispatcher.read_finished(640);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 3);
    }
    dispatcher.write_commit(0);
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "16"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    put_elem(0, 1, test_file);
    dispatcher.write_commit(0);
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.read_idx_finished(640);
    dispatcher.write_start(481);
    put_elem(0, 2, test_file);
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    dispatcher.read_finished(640);
    put_elem(0, 3, test_file);
    dispatcher.write_commit(0);
    data_read_test(test_file);
    remove("Test_File.bin");
    remove("Test_File.bin.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "17"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 1);
    }
    dispatcher.write_commit(0);
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.read_idx_finished(640);
    dispatcher.write_start(481);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 2);
    }
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 3);
    }
    dispatcher.write_commit(0);
    dispatcher.write_start(483);
    dispatcher.read_finished(640);
    {
      Nonsynced_Transaction transaction(true, true, BASE_DIRECTORY, "");
      Test_File tf("Test_File");
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
      blocks.put(0u, 4);
    }
    dispatcher.write_commit(0);
    map_read_test();
    remove("Test_File.map");
    remove("Test_File.map.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "18"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			  BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			  5, 180, 1024*1024*1024,  1024*1024, file_properties);
    dispatcher.write_start(480);
    put_elem(0, 1, test_file);
    dispatcher.write_commit(0);
    dispatcher.request_read_and_idx(640, 180, 512*1024*1024, 640);
    dispatcher.read_idx_finished(640);
    dispatcher.write_start(481);
    put_elem(0, 2, test_file);
    dispatcher.write_commit(0);
    dispatcher.write_start(482);
    put_elem(0, 3, test_file);
    dispatcher.write_commit(0);
    dispatcher.write_start(483);
    dispatcher.read_finished(640);
    put_elem(0, 4, test_file);
    dispatcher.write_commit(0);
    data_read_test(test_file);
    remove("Test_File.bin");
    remove("Test_File.bin.idx");
  }
  
  if ((test_to_execute == "") || (test_to_execute == "19"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher dispatcher("/", "osm3s_index_share_test",
			    BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			    5, 180, 1024*1024*1024,  1024*1024, file_properties);
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "20"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if (test_to_execute.substr(0, 6) == "server")
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			    BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			    5, 180, 1024*1024*1024,  1024*1024, file_properties);
      dispatcher.write_start(480);
      put_elem(0, 1, test_file);
      dispatcher.write_commit(0);
      
      cerr<<"[server] Starting ...\n";
      uint32 execution_time = 3;
      istringstream sin(test_to_execute.substr(7));
      sin>>execution_time;
      dispatcher.standby_loop(execution_time*1000);
      cerr<<"[server] done.\n";
      return 0;
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }

  if ((test_to_execute == "") || (test_to_execute == "21"))
  {
    vector< File_Properties* > file_properties;
    // Try to start a sceond dispatcher instance. This shall fail with
    // a File_Error.
    try
    {
      Dispatcher dispatcher("osm3s_share_test", "osm3s_index_share_test",
			    BASE_DIRECTORY + "test-shadow", BASE_DIRECTORY,
			    5, 180, 1024*1024*1024,  1024*1024, file_properties);
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "22"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      string shadow_name = dispatcher_client.get_shadow_name() + ".lock";
      if (file_exists(shadow_name))
      {
	cout<<"Failed before write_start(): "
	    <<shadow_name<<" already exists.\n";
	return 0;
      }
      dispatcher_client.write_start();
      if (!file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_start().\n";
	return 0;
      }
      
      ifstream in((dispatcher_client.get_shadow_name() + ".lock").c_str());
      pid_t read_pid;
      in>>read_pid;
      if (read_pid != getpid())
      {
	cout<<"Pid in lock doesn't match.\n";
	return 0;
      }
      
      put_elem(0, 21, test_file, dispatcher_client.get_db_dir());
      dispatcher_client.write_commit();
      if (file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_commit().\n";
	return 0;
      }
      
      cout<<"Writing successfully done.\n";
      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      data_read_test(test_file, transaction);
      remove((dispatcher_client.get_db_dir() + "Test_File.bin").c_str());
      remove((dispatcher_client.get_db_dir() + "Test_File.bin.idx").c_str());
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "23"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      string shadow_name = dispatcher_client.get_shadow_name() + ".lock";
      if (file_exists(shadow_name))
      {
	cout<<"Failed before write_start(): "
	    <<shadow_name<<" already exists.\n";
	return 0;
      }
      dispatcher_client.write_start();      
      if (!file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_start().\n";
	return 0;
      }
      
      ifstream in((dispatcher_client.get_shadow_name() + ".lock").c_str());
      pid_t read_pid;
      in>>read_pid;
      if (read_pid != getpid())
      {
	cout<<"Pid in lock doesn't match.\n";
	return 0;
      }
      
      put_elem(0, 21, test_file, dispatcher_client.get_db_dir());
      dispatcher_client.write_rollback();
      if (file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_rollback().\n";
	return 0;
      }
      
      cout<<"Writing successfully done.\n";
      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      data_read_test(test_file, transaction);
      remove((dispatcher_client.get_db_dir() + "Test_File.bin").c_str());
      remove((dispatcher_client.get_db_dir() + "Test_File.bin.idx").c_str());
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "24"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      string shadow_name = dispatcher_client.get_shadow_name() + ".lock";
      if (file_exists(shadow_name))
      {
	cout<<"Failed before write_start(): "
	    <<shadow_name<<" already exists.\n";
	return 0;
      }
      dispatcher_client.write_start();      
      if (!file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_start().\n";
	return 0;
      }
      
      {
	pid_t read_pid;
	
	ifstream in((dispatcher_client.get_shadow_name() + ".lock").c_str());
        in>>read_pid;
	
	if (read_pid != getpid())
	{
	  cout<<"Pid in lock doesn't match.\n";
	  return 0;
	}
      }
      
      put_elem(0, 21, test_file, dispatcher_client.get_db_dir());
      dispatcher_client.write_commit();
      if (file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_commit().\n";
	return 0;
      }
      
      cout<<"First writing successfully done.\n";

      dispatcher_client.write_start();      
      if (!file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_start().\n";
	return 0;
      }
      
      {
	pid_t read_pid;
	
	ifstream in((dispatcher_client.get_shadow_name() + ".lock").c_str());
	in>>read_pid;
	
	if (read_pid != getpid())
	{
	  cout<<"Pid in lock doesn't match.\n";
	  return 0;
	}
      }
      
      put_elem(0, 23, test_file, dispatcher_client.get_db_dir());
      dispatcher_client.write_commit();
      if (file_exists(dispatcher_client.get_shadow_name() + ".lock"))
      {
	cout<<"Failed after write_commit().\n";
	return 0;
      }
      
      cout<<"Second writing successfully done.\n";
      
      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      data_read_test(test_file, transaction);
      remove((dispatcher_client.get_db_dir() + "Test_File.bin").c_str());
      remove((dispatcher_client.get_db_dir() + "Test_File.bin.idx").c_str());
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
      <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "25r"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      {
	//sleep for a second
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      sync_log("Try request_read().\n");
      dispatcher_client.request_read_and_idx(24*60, 1024*1024, 0);
      sync_log("request_read() returned.\n");
      
      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      sync_log("Announce read_idx_finished().\n");
      dispatcher_client.read_idx_finished();      
      sync_log("read_idx_finished\n");
      dispatcher_client.read_finished();
      //cerr<<"read_finished() done.\n"; //Timing with 24w can't be controlled.
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "25w"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      dispatcher_client.write_start();      
      put_elem(0, 24, test_file, dispatcher_client.get_db_dir());
      sync_log("write_start() done and written an element.\n");
      
      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      sync_log("Try write_commit().\n");
      dispatcher_client.write_commit();
      sync_log("write_commit() done.\n");

      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      data_read_test(test_file, transaction);
      remove((dispatcher_client.get_db_dir() + "Test_File.bin").c_str());
      remove((dispatcher_client.get_db_dir() + "Test_File.bin.idx").c_str());
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "26r"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      {
	//sleep for a second
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      sync_log("Try request_read().\n");
      dispatcher_client.request_read_and_idx(24*60, 1024*1024, 0);
      sync_log("request_read() done.\n");
      
      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      transaction.data_index(&test_file);
      
      dispatcher_client.read_idx_finished();
      sync_log("read_idx_finished() done.\n");
      
      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      data_read_test(test_file, transaction);
      
      sync_log("Announce read_finished().\n");
      dispatcher_client.read_finished();
      sync_log("read_finished() done.\n");
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "26w"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      dispatcher_client.write_start();      
      put_elem(0, 25, test_file, dispatcher_client.get_db_dir());
      sync_log("write_start() done and written an element.\n");
      
      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      sync_log("Try write_commit().\n");
      dispatcher_client.write_commit();
      sync_log("write_commit() done.\n");

      dispatcher_client.write_start();      
      put_elem(0, 26, test_file, dispatcher_client.get_db_dir());
      sync_log("write_start() done and written an element.\n");
      
      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      sync_log("Try write_commit().\n");
      dispatcher_client.write_commit();
      sync_log("write_commit() done.\n");

      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      data_read_test(test_file, transaction);
      remove((dispatcher_client.get_db_dir() + "Test_File.bin").c_str());
      remove((dispatcher_client.get_db_dir() + "Test_File.bin.idx").c_str());
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "27r"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      {
	//sleep for a second
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      sync_log("Try request_read().\n");
      dispatcher_client.request_read_and_idx(24*60, 1024*1024, 0);
      sync_log("request_read() done.\n");
      
      Nonsynced_Transaction transaction
          (false, false, dispatcher_client.get_db_dir(), "");
      transaction.data_index(&test_file);
      
      dispatcher_client.read_idx_finished();
      sync_log("read_idx_finished() done.\n");
      
      {
	//sleep for two seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      data_read_test(test_file, transaction);
      
      sync_log("Announce read_finished().\n");
      dispatcher_client.read_finished();
      sync_log("read_finished() done.\n");
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }
  
  if ((test_to_execute == "") || (test_to_execute == "27w"))
  {
    Test_File test_file("Test_File");
    
    vector< File_Properties* > file_properties;
    file_properties.push_back(&test_file);
    try
    {
      Dispatcher_Client dispatcher_client("osm3s_share_test");
      test_file.set_basedir(dispatcher_client.get_db_dir());
      
      {
	//sleep for a seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      sync_log("Request output status.\n");
      dispatcher_client.output_status();      
      sync_log("Request output status finished.\n");
      
      {
	//sleep for a seconds
	struct timeval timeout_;
	timeout_.tv_sec = 0;
	timeout_.tv_usec = 2*1000*1000;
	select(FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      }
      
      remove((dispatcher_client.get_db_dir() + "Test_File.bin").c_str());
      remove((dispatcher_client.get_db_dir() + "Test_File.bin.idx").c_str());
    }
    catch (File_Error e)
    {
      cout<<"File error catched: "
          <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    }
  }  
}
