#include <iostream>
#include <list>

#include <stdio.h>

#include "../dispatch/settings.h"
#include "file_blocks.h"

using namespace std;

/**
 * Tests the library file_blocks
 */

//-----------------------------------------------------------------------------

/* We use our own test settings */
string BASE_DIRECTORY("./");
string DATA_SUFFIX(".bin");
string INDEX_SUFFIX(".idx");

string get_file_base_name(int32 FILE_PROPERTIES)
{
  return BASE_DIRECTORY + "testfile";
}

string get_index_suffix(int32 FILE_PROPERTIES)
{
  return INDEX_SUFFIX;
}

string get_data_suffix(int32 FILE_PROPERTIES)
{
  return DATA_SUFFIX;
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
  
  uint32 size_of() const
  {
    return (value-4 < sizeof(uint32) ? sizeof(uint32) : value-sizeof(uint32));
  }
  
  static uint32 size_of(void* data)
  {
    return ((*(uint32*)data)-sizeof(uint32) < sizeof(uint32)
      ? sizeof(uint32) : (*(uint32*)data)-sizeof(uint32));
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

typedef list< IntIndex >::const_iterator IntIterator;

struct IntRangeIterator : list< pair< IntIndex, IntIndex > >::const_iterator
{
  IntRangeIterator
    (const list< pair< IntIndex, IntIndex > >::const_iterator it)
    : list< pair< IntIndex, IntIndex > >::const_iterator(it) {}
  
  const IntIndex& lower_bound() const
  {
    return (*this)->first;
  }
  
  const IntIndex& upper_bound() const
  {
    return (*this)->second;
  }
};

//-----------------------------------------------------------------------------

void read_loop
(File_Blocks< IntIndex, IntIterator, IntRangeIterator >& blocks,
 File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Flat_Iterator& it)
{
  while (!(it == blocks.flat_end()))
  {
    cout<<"Predicted size "<<blocks.answer_size(it);
    uint8* data((uint8*)(blocks.read_block(it)));
    cout<<", real size "<<(*(uint32*)data)<<" bytes, "
    <<"first block size "<<*(uint32*)(data+sizeof(uint32))<<" bytes, "
    <<"first index "<<*(uint32*)(data+2*sizeof(uint32));
    if (*(uint32*)(data+sizeof(uint32)) < (*(uint32*)data)-sizeof(uint32))
    {
      uint8* pos(data+sizeof(uint32));
      pos += *(uint32*)pos;
      cout<<", second block size "<<(*(uint32*)pos)<<" bytes, "
      <<"second index "<<*(uint32*)(pos+sizeof(uint32));
    }
    cout<<'\n';
    ++it;
  }
}

void read_loop
  (File_Blocks< IntIndex, IntIterator, IntRangeIterator >& blocks,
   File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator& it)
{
  while (!(it == blocks.discrete_end()))
  {
    cout<<"Predicted size "<<blocks.answer_size(it);
    uint8* data((uint8*)(blocks.read_block(it)));
    cout<<", real size "<<(*(uint32*)data)<<" bytes, "
    <<"first block size "<<*(uint32*)(data+sizeof(uint32))<<" bytes, "
    <<"first index "<<*(uint32*)(data+2*sizeof(uint32));
    if (*(uint32*)(data+sizeof(uint32)) < (*(uint32*)data)-sizeof(uint32))
    {
      uint8* pos(data+sizeof(uint32));
      pos += *(uint32*)pos;
      cout<<", second block size "<<(*(uint32*)pos)<<" bytes, "
          <<"second index "<<*(uint32*)(pos+sizeof(uint32));
    }
    cout<<'\n';
    ++it;
  }
}

void read_loop
  (File_Blocks< IntIndex, IntIterator, IntRangeIterator >& blocks,
   File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Range_Iterator& it)
{
  while (!(it == blocks.range_end()))
  {
    cout<<"Predicted size "<<blocks.answer_size(it);
    uint8* data((uint8*)(blocks.read_block(it)));
    cout<<", real size "<<(*(uint32*)data)<<" bytes, "
    <<"first block size "<<*(uint32*)(data+sizeof(uint32))<<" bytes, "
    <<"first index "<<*(uint32*)(data+2*sizeof(uint32));
    if (*(uint32*)(data+sizeof(uint32)) < (*(uint32*)data)-sizeof(uint32))
    {
      uint8* pos(data+sizeof(uint32));
      pos += *(uint32*)pos;
      cout<<", second block size "<<(*(uint32*)pos)<<" bytes, "
      <<"second index "<<*(uint32*)(pos+sizeof(uint32));
    }
    cout<<'\n';
    ++it;
  }
}

void read_test(File_Blocks< IntIndex, IntIterator, IntRangeIterator >& blocks)
{
  cout<<"Read test\n";
  
  cout<<"Reading all blocks ...\n";
  File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Flat_Iterator
    fit(blocks.flat_begin());
  read_loop(blocks, fit);
  cout<<"... all blocks read.\n";

  list< IntIndex > index_list;
  for (unsigned int i(0); i < 100; i += 9)
    index_list.push_back(&i);
  cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
  File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(index_list.begin(), index_list.end()));
  read_loop(blocks, it);
  cout<<"... all blocks read.\n";
  
  index_list.clear();
  for (unsigned int i(0); i < 10; ++i)
    index_list.push_back(&i);
  cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
  it = blocks.discrete_begin(index_list.begin(), index_list.end());
  read_loop(blocks, it);
  cout<<"... all blocks read.\n";

  list< pair< IntIndex, IntIndex > > range_list;
  uint32 fool(0), foou(10);
  range_list.push_back(make_pair(IntIndex(&fool), IntIndex(&foou)));
  cout<<"Reading blocks with indices [0, 10[ ...\n";
  File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Range_Iterator
    rit(blocks.select_blocks
      (IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end())));
  read_loop(blocks, rit);
  cout<<"... all blocks read.\n";
  
  index_list.clear();
  for (unsigned int i(90); i < 100; ++i)
    index_list.push_back(&i);
  cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
  it = blocks.discrete_begin(index_list.begin(), index_list.end());
  read_loop(blocks, it);
  cout<<"... all blocks read.\n";
  
  range_list.clear();
  fool = 90;
  foou = 100;
  range_list.push_back(make_pair(IntIndex(&fool), IntIndex(&foou)));
  cout<<"Reading blocks with indices [90, 100[ ...\n";
  rit = blocks.select_blocks
    (IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end()));
  read_loop(blocks, rit);
  cout<<"... all blocks read.\n";
  
  index_list.clear();
  uint32 foo(50);
  index_list.push_back(&foo);
  cout<<"Reading blocks with index 50 ...\n";
  it = blocks.discrete_begin(index_list.begin(), index_list.end());
  read_loop(blocks, it);
  cout<<"... all blocks read.\n";
  
  range_list.clear();
  fool = 50;
  foou = 51;
  range_list.push_back(make_pair(IntIndex(&fool), IntIndex(&foou)));
  cout<<"Reading blocks with indices [50, 51[ ...\n";
  rit = blocks.select_blocks
    (IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end()));
  read_loop(blocks, rit);
  cout<<"... all blocks read.\n";
  
  range_list.clear();
  fool = 0;
  foou = 10;
  range_list.push_back(make_pair(IntIndex(&fool), IntIndex(&foou)));
  fool = 50;
  foou = 51;
  range_list.push_back(make_pair(IntIndex(&fool), IntIndex(&foou)));
  fool = 90;
  foou = 100;
  range_list.push_back(make_pair(IntIndex(&fool), IntIndex(&foou)));
  cout<<"Reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
  rit = blocks.select_blocks
    (IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end()));
  read_loop(blocks, rit);
  cout<<"... all blocks read.\n";
  
  index_list.clear();
  cout<<"Reading blocks with indices \\emptyset ...\n";
  it = blocks.discrete_begin(index_list.begin(), index_list.end());
  read_loop(blocks, it);
  cout<<"... all blocks read.\n";
  
  cout<<"This block of read tests is complete.\n";
}

uint32 prepare_block(void* block, const list< IntIndex >& indices)
{
  uint32 max_keysize(0);
  
  if (indices.empty())
  {
    *(uint32*)block = 0;
    return 0;
  }
  
  uint32 pos(sizeof(uint32));
  for (list< IntIndex >::const_iterator it(indices.begin());
      it != indices.end(); ++it)
  {
    if ((*it).val() + sizeof(uint32) > max_keysize)
      max_keysize = (*it).val() + sizeof(uint32);
    
    *(uint32*)(((uint8*)block)+pos) = (*it).val() + sizeof(uint32);
    *(uint32*)(((uint8*)block)+pos+sizeof(uint32)) = (*it).val();
    pos += (*it).val() + sizeof(uint32);
  }
  
  *(uint32*)block = pos;
  
  return max_keysize;
}

int main(int argc, char* args[])
{
  cout<<"** Test the behaviour for non-exsiting files\n";
  remove((get_file_base_name(0) + get_index_suffix(0)).c_str());
  remove((get_file_base_name(0) + get_data_suffix(0)).c_str());
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is the expected correct behaviour)\n";
  }
  
  cout<<"** Test the behaviour for an empty file\n";
  int data_fd = open64
      ((get_file_base_name(0) + get_data_suffix(0)).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  int index_fd = open64
      ((get_file_base_name(0) + get_index_suffix(0)).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(index_fd);
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  cout<<"** Test the behaviour for a file with one entry - part 1\n";
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, true);
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(49));
    indices.push_back(IntIndex(50));
    void* buf = malloc(get_block_size(0));
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  cout<<"** Test the behaviour for a file with one entry - part 2\n";
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, true);
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(51));
    void* buf = malloc(get_block_size(0));
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.replace_block(blocks.discrete_begin(indices.begin(), indices.end()), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  cout<<"** Test the behaviour if the only block has been deleted\n";
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, true);
    list< IntIndex > indices;
    
    indices.clear();
    void* buf = malloc(get_block_size(0));
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.replace_block(blocks.discrete_begin(indices.begin(), indices.end()), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  cout<<"** Test the behaviour for a file with one entry - part 3\n";
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, true);
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(50);
    void* buf = malloc(get_block_size(0));
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  cout<<"** Test the behaviour for a file with several blocks - part 1\n";
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, true);
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(45);
    void* buf = malloc(get_block_size(0));
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.discrete_begin(indices.begin(), indices.end()), buf, max_keysize);
    free(buf);
    
    indices.clear();
    indices.push_back(54);
    buf = malloc(get_block_size(0));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  cout<<"** Test the behaviour for a file with several blocks - part 2\n";
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, true);
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(45);
    indices.push_back(46);
    void* buf = malloc(get_block_size(0));
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.replace_block(blocks.discrete_begin(indices.begin(), indices.end()), buf, max_keysize);
    free(buf);
    
    indices.clear();
    indices.push_back(55);
    indices.push_back(56);
    buf = malloc(get_block_size(0));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  try
  {
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks(0, false);
    read_test(blocks);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  
  return 0;
}
