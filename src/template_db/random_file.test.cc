#include <iostream>
#include <list>

#include <stdio.h>

#include "random_file.h"

using namespace std;

/**
 * Tests the library random_file
 */

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
    return "";
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
};

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

void read_test()
{
  try
  {
    cout<<"Read test\n";
    Random_File< IntIndex > id_file(Test_File(), false);
  
    cout<<id_file.get(0).val()<<'\n';
    cout<<id_file.get(2).val()<<'\n';
    cout<<id_file.get(3).val()<<'\n';
    cout<<id_file.get(5).val()<<'\n';
    cout<<id_file.get(6).val()<<'\n';
    cout<<id_file.get(8).val()<<'\n';
    
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
  if ((test_to_execute == "") || (test_to_execute == "1"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "2"))
    cout<<"** Test the behaviour for a file with two entries - part 1\n";
  try
  {
    Random_File< IntIndex > blocks(Test_File(), true);
    
/*    blocks.put(0, 10);
    blocks.put(1, 11);*/
    blocks.put(2, 12);
/*    blocks.put(3, 13);
    blocks.put(4, 14);*/
    blocks.put(5, 15);
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
    Random_File< IntIndex > blocks(Test_File(), true);
    
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
    Random_File< IntIndex > blocks(Test_File(), true);
    
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
  
  remove((Test_File().get_file_base_name() + Test_File().get_id_suffix()).c_str());
  
  return 0;
}
