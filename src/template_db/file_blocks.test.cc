/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Template_DB.
*
* Template_DB is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Template_DB is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Template_DB.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <list>

#include <stdio.h>

#include "file_blocks.h"
#include "transaction.h"

using namespace std;

/**
 * Tests the library file_blocks
 */

//-----------------------------------------------------------------------------

/* Sample class for TIndex */
struct IntIndex
{
  IntIndex(uint32 i) : value(i) {}
  IntIndex(void* data) : value(*(uint32*)data) {}
  
  uint32 size_of() const
  {
    return (value < 24 ? 12 : value-12);
  }
  
  static uint32 size_of(void* data)
  {
    return ((*(uint32*)data) < 24 ? 12 : (*(uint32*)data)-12);
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
  IntRangeIterator() {}
  
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

/* We use our own test settings */
string BASE_DIRECTORY("./");
string DATA_SUFFIX(".bin");
string INDEX_SUFFIX(".idx");

struct Test_File : File_Properties
{
  string get_basedir() const
  {
    return BASE_DIRECTORY;
  }
  
  string get_file_name_trunk() const
  {
    return "testfile";
  }
  
  string get_index_suffix() const
  {
    return INDEX_SUFFIX;
  }

  string get_data_suffix() const
  {
    return DATA_SUFFIX;
  }

  string get_id_suffix() const
  {
    return "";
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
  
  vector< bool > get_data_footprint(const string& db_dir) const
  {
    return vector< bool >();
  }
  
  vector< bool > get_map_footprint(const string& db_dir) const
  {
    return vector< bool >();
  }  

  uint32 id_max_size_of() const
  {
    throw string();
    return 0;
  }
  
  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, string db_dir, string file_name_extension)
      const
  {
    return new File_Blocks_Index< IntIndex >
        (*this, writeable, use_shadow, db_dir, file_name_extension);
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
    uint32 answer_size(blocks.answer_size(it));
    cout<<"Predicted size "<<answer_size;
    if (answer_size > 0)
    {
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

void read_test()
{
  try
  {
    cout<<"Read test\n";
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));

    vector< bool > footprint = get_data_index_footprint< IntIndex >
        (Test_File(), BASE_DIRECTORY);
    cout<<"Index footprint: ";
    for (vector< bool >::const_iterator it(footprint.begin()); it != footprint.end();
        ++it)
      cout<<*it;
    cout<<'\n';

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
	rit(blocks.range_begin
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
    rit = blocks.range_begin
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
    rit = blocks.range_begin
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
    rit = blocks.range_begin
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
  catch (File_Error e)
  {
    cout<<"File error catched: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is the expected correct behaviour)\n";
  }
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
    if ((*it).val() + 12 > max_keysize)
      max_keysize = (*it).val() + 12;
    
    *(uint32*)(((uint8*)block)+pos) = (*it).val() + 12;
    *(uint32*)(((uint8*)block)+pos+sizeof(uint32)) = (*it).val();
    pos += (*it).val() + 12;
  }
  
  *(uint32*)block = pos;
  
  return max_keysize;
}

int main(int argc, char* args[])
{
  string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];
  
  int data_fd = open64
      ((BASE_DIRECTORY
        + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  int index_fd = open64
      ((BASE_DIRECTORY
        + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()
        + Test_File().get_index_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(index_fd);
  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    cout<<"** Test the behaviour for an empty file\n";
    read_test();
  }
  
  if ((test_to_execute == "") || (test_to_execute == "2"))
    cout<<"** Test the behaviour for a file with one entry - part 1\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(49));
    indices.push_back(IntIndex(50));
    void* buf = malloc(Test_File().get_block_size());
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
  if ((test_to_execute == "") || (test_to_execute == "2"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "3"))
    cout<<"** Test the behaviour for a file with one entry - part 2\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(51));
    void* buf = malloc(Test_File().get_block_size());
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
  if ((test_to_execute == "") || (test_to_execute == "3"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "4"))
    cout<<"** Test the behaviour for a file with three entries\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(9));
    void* buf = malloc(Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.discrete_begin(indices.begin(), indices.end()), buf, max_keysize);
    
    indices.clear();
    indices.push_back(IntIndex(89));
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
  if ((test_to_execute == "") || (test_to_execute == "4"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "5"))
    cout<<"** Test insertion everywhere\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(10));
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(63));
    indices.push_back(IntIndex(64));
    indices.push_back(IntIndex(65));
    indices.push_back(IntIndex(89));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    ++it;
    indices.clear();
    indices.push_back(IntIndex(10));
    void* buf = malloc(Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    ++it;
    
    indices.clear();
    indices.push_back(IntIndex(63));
    max_keysize = prepare_block(buf, indices);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    
    indices.clear();
    indices.push_back(IntIndex(64));
    max_keysize = prepare_block(buf, indices);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    
    indices.clear();
    indices.push_back(IntIndex(65));
    max_keysize = prepare_block(buf, indices);
    it = blocks.insert_block(it, buf, max_keysize);
    free(buf);
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
    cout<<"** Test to replace blocks\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices, work;
    
    indices.clear();
    indices.push_back(IntIndex(7));
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(52));
    indices.push_back(IntIndex(89));
    indices.push_back(IntIndex(90));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    work.clear();
    work.push_back(IntIndex(7));
    void* buf = malloc(Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, work));
    it = blocks.replace_block(it, buf, max_keysize);
    ++it;
    
    work.clear();
    work.push_back(IntIndex(51));
    work.push_back(IntIndex(52));
    max_keysize = prepare_block(buf, work);
    it = blocks.replace_block(it, buf, max_keysize);
    ++it;
    
    work.clear();
    work.push_back(IntIndex(89));
    work.push_back(IntIndex(90));
    max_keysize = prepare_block(buf, work);
    it = blocks.replace_block(it, buf, max_keysize);
    free(buf);
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
    cout<<"** Delete blocks in between\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(64));
    indices.push_back(IntIndex(65));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
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
    cout<<"** Delete blocks at the begin and the end\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(7));
    indices.push_back(IntIndex(90));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "9"))
    cout<<"** Test insertion again\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices, work;
    
    indices.clear();
    for (unsigned int i(20); i < 30; ++i)
      indices.push_back(IntIndex(i));
    indices.push_back(IntIndex(63));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    ++it;
    for (unsigned int i(20); i < 30; ++i)
    {
      work.clear();
      work.push_back(IntIndex(i));
      void* buf = malloc(Test_File().get_block_size());
      uint32 max_keysize(prepare_block(buf, work));
      it = blocks.insert_block(it, buf, max_keysize);
      free(buf);
      ++it;
    }
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "9"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "10"))
    cout<<"** Delete everything\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    for (unsigned int i(0); i < 100; ++i)
      indices.push_back(IntIndex(i));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
    it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    while (!(it == blocks.discrete_end()))
    {
      it = blocks.replace_block(it, 0, 0);
    }
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "10"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "11"))
    cout<<"** Insert two series of segments\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices;
    
    indices.clear();
    indices.push_back(IntIndex(40));
    void* buf = malloc(Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    
    indices.clear();
    indices.push_back(IntIndex(60));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    blocks.insert_block(blocks.discrete_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "11"))
    read_test();
  
  if ((test_to_execute == "") || (test_to_execute == "12"))
    cout<<"** Replace by other series of segments\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator, IntRangeIterator > blocks
        (transaction.data_index(&tf));
    list< IntIndex > indices, work;
    
    indices.clear();
    indices.push_back(IntIndex(8));
    indices.push_back(IntIndex(40));
    indices.push_back(IntIndex(50));
    indices.push_back(IntIndex(60));
    indices.push_back(IntIndex(90));
    indices.push_back(IntIndex(99));
    
    File_Blocks< IntIndex, IntIterator, IntRangeIterator >::Discrete_Iterator
        it(blocks.discrete_begin(indices.begin(), indices.end()));
    
    work.clear();
    work.push_back(IntIndex(8));
    void* buf = malloc(Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, work));
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    ++it;
    
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    
    work.clear();
    work.push_back(IntIndex(50));
    max_keysize = prepare_block(buf, work);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    ++it;
    
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    it = blocks.replace_block(it, 0, 0);
    
    work.clear();
    work.push_back(IntIndex(90));
    max_keysize = prepare_block(buf, work);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    
    work.clear();
    work.push_back(IntIndex(99));
    max_keysize = prepare_block(buf, work);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;
    free(buf);
  }
  catch (File_Error e)
  {
    cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "12"))
    read_test();
  
  remove((BASE_DIRECTORY
      + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()
      + Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()
      + Test_File().get_shadow_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()).c_str());
  
  return 0;
}
