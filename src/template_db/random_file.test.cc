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
  string get_basedir() const
  {
    return BASE_DIRECTORY;
  }
  
  string get_file_base_name() const
  {
    return BASE_DIRECTORY + "testfile";
  }
  
  string get_index_suffix() const
  {
    return ".idx";
  }
  
  string get_data_suffix() const
  {
    return "";
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
  
  uint32 id_max_size_of() const
  {
    return IntIndex::max_size_of();
  }
  
  File_Blocks_Index_Base* new_data_index
      (string index_file_name, string empty_index_file_name,
       string file_name_extension, uint32 block_count) const
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
    vector< bool > footprint = get_map_index_footprint< IntIndex >(Test_File());
    cout<<"Index footprint: ";
    for (vector< bool >::const_iterator it(footprint.begin()); it != footprint.end();
        ++it)
      cout<<*it;
    cout<<'\n';

    Nonsynced_Transaction transaction(false, false);
    Test_File tf;
    Random_File< IntIndex > id_file(tf, transaction.random_index(&tf));

    cout<<id_file.get(0).val()<<'\n';
    cout<<id_file.get(1).val()<<'\n';
    cout<<id_file.get(2).val()<<'\n';
    cout<<id_file.get(3).val()<<'\n';
    cout<<id_file.get(5).val()<<'\n';
    cout<<id_file.get(6).val()<<'\n';
    cout<<id_file.get(8).val()<<'\n';
    cout<<id_file.get(16).val()<<'\n';
    cout<<id_file.get(32).val()<<'\n';
    cout<<id_file.get(48).val()<<'\n';
    cout<<id_file.get(64).val()<<'\n';
    cout<<id_file.get(80).val()<<'\n';
    cout<<id_file.get(96).val()<<'\n';
    cout<<id_file.get(112).val()<<'\n';
    
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
      ((Test_File().get_file_base_name() + Test_File().get_id_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  int index_fd = open64
      ((Test_File().get_file_base_name() + Test_File().get_id_suffix()
          + Test_File().get_index_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(index_fd);
  if ((test_to_execute == "") || (test_to_execute == "1"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "2"))
    cout<<"** Test the behaviour for a file with two entries - part 1\n";
  try
  {
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    {
      Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
      blocks.put(2, 12);
      blocks.put(5, 15);
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
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
    blocks.put(6, 16);
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
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
    blocks.put(2, 32);
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
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
    blocks.put(16, 1);
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
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
    blocks.put(0, 2);
    blocks.put(32, 3);
    blocks.put(48, 4);
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
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
    blocks.put(80, 5);
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
    Nonsynced_Transaction transaction(true, false);
    Test_File tf;
    Random_File< IntIndex > blocks(tf, transaction.random_index(&tf));
    
    blocks.put(64, 6);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
    read_test();
  
  remove((Test_File().get_file_base_name() + Test_File().get_id_suffix()).c_str());
  remove((Test_File().get_file_base_name() + Test_File().get_id_suffix()
      + Test_File().get_index_suffix()).c_str());
  remove((Test_File().get_file_base_name() + Test_File().get_id_suffix()
      + Test_File().get_shadow_suffix()).c_str());
  
  return 0;
}
