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

#include <iostream>
#include <list>

#include <stdio.h>

#include "random_file.h"
#include "transaction.h"

using namespace std;

/**
 * Tests the library random_file
 */

//-----------------------------------------------------------------------------

/* Sample class for TIndex */
struct IntIndex
{
  IntIndex(uint32 i) : value(i) {}
  IntIndex(void* data) : value(*(uint32*)data) {}
  
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

//-----------------------------------------------------------------------------

/* We use our own test settings */
string BASE_DIRECTORY("./");
string ID_SUFFIX(".map");


struct Test_File : File_Properties
{
  const std::string& get_basedir() const
  {
    return BASE_DIRECTORY;
  }
  
  const std::string& get_file_name_trunk() const
  {
    static std::string result("testfile");
    return result;
  }
  
  const std::string& get_index_suffix() const
  {
    static std::string result(".idx");
    return result;
  }

  const std::string& get_data_suffix() const
  {
    static std::string result("");
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
    return vector< bool >();
  }
  
  vector< bool > get_map_footprint(const std::string& db_dir) const
  {
    return vector< bool >();
  }  

  uint32 id_max_size_of() const
  {
    return IntIndex::max_size_of();
  }
  
  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    throw string();
    return 0;
  }
};

//-----------------------------------------------------------------------------

void read_test()
{
  try
  {
    cout<<"Read test\n";
    vector< bool > footprint = get_map_index_footprint
        (Test_File(), BASE_DIRECTORY, false);
    cout<<"Index footprint: ";
    for (vector< bool >::const_iterator it(footprint.begin()); it != footprint.end();
        ++it)
      cout<<*it;
    cout<<'\n';

    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > id_file(transaction.random_index(&tf));

    cout<<id_file.get(0u).val()<<'\n';
    cout<<id_file.get(1u).val()<<'\n';
    cout<<id_file.get(2u).val()<<'\n';
    cout<<id_file.get(3u).val()<<'\n';
    cout<<id_file.get(5u).val()<<'\n';
    cout<<id_file.get(6u).val()<<'\n';
    cout<<id_file.get(8u).val()<<'\n';
    cout<<id_file.get(16u).val()<<'\n';
    cout<<id_file.get(32u).val()<<'\n';
    cout<<id_file.get(48u).val()<<'\n';
    cout<<id_file.get(64u).val()<<'\n';
    cout<<id_file.get(80u).val()<<'\n';
    cout<<id_file.get(96u).val()<<'\n';
    cout<<id_file.get(112u).val()<<'\n';
    
    cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
}

int main(int argc, char* args[])
{
  string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
    cout<<"** Test the behaviour for an empty file\n";
  int data_fd = open64
      ((BASE_DIRECTORY + Test_File().get_file_name_trunk() + Test_File().get_id_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  int index_fd = open64
      ((BASE_DIRECTORY + Test_File().get_file_name_trunk() + Test_File().get_id_suffix()
          + Test_File().get_index_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(index_fd);
  if ((test_to_execute == "") || (test_to_execute == "1"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "2"))
    cout<<"** Test the behaviour for a file with two entries - part 1\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    {
      Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
      blocks.put(2u, 12);
      blocks.put(5u, 15);
    }
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "3"))
    cout<<"** Add at the end\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
    blocks.put(6u, 16);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
    read_test();
    
  if ((test_to_execute == "") || (test_to_execute == "4"))
    cout<<"** Overwrite an existing block\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
    blocks.put(2u, 32);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "5"))
    cout<<"** Write a second block\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
    blocks.put(16u, 1);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "6"))
    cout<<"** Write several blocks at once.\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
    blocks.put(0u, 2);
    blocks.put(32u, 3);
    blocks.put(48u, 4);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "7"))
    cout<<"** Leave a gap.\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
    blocks.put(80u, 5);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "7"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "8"))
    cout<<"** Fill the gap.\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Random_File< IntIndex, IntIndex > blocks(transaction.random_index(&tf));
    
    blocks.put(64u, 6);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
    read_test();
  
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_id_suffix()).c_str());
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_id_suffix()
      + Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_id_suffix()
      + Test_File().get_shadow_suffix()).c_str());
  
  return 0;
}
