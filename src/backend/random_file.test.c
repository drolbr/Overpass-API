#include <iostream>
#include <list>

#include <stdio.h>

#include "../dispatch/settings.h"
#include "random_file.h"

using namespace std;

/**
 * Tests the library random_file
 */

//-----------------------------------------------------------------------------

/* We use our own test settings */
string BASE_DIRECTORY("./");
string ID_SUFFIX(".map");

string get_file_base_name(int32 FILE_PROPERTIES)
{
  return BASE_DIRECTORY + "testfile";
}

string get_index_suffix(int32 FILE_PROPERTIES)
{
  return "";
}

string get_data_suffix(int32 FILE_PROPERTIES)
{
  return "";
}

string get_id_suffix(int32 FILE_PROPERTIES)
{
  return ID_SUFFIX;
}

uint32 get_block_size(int32 FILE_PROPERTIES)
{
  return 512;
}

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
    Random_File< IntIndex > id_file(0, false);
  
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
  cout<<"** Test the behaviour for an empty file\n";
  int data_fd = open64
      ((get_file_base_name(0) + get_id_suffix(0)).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  read_test();
  
  cout<<"** Test the behaviour for a file with two entries - part 1\n";
  try
  {
    Random_File< IntIndex > blocks(0, true);
    
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
  read_test();
  
  cout<<"** Add at the end\n";
  try
  {
    Random_File< IntIndex > blocks(0, true);
    
    blocks.put(6, 16);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  read_test();
    
  cout<<"** Overwrite an existing block\n";
  try
  {
    Random_File< IntIndex > blocks(0, true);
    
    blocks.put(2, 32);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  read_test();
  
  return 0;
}
