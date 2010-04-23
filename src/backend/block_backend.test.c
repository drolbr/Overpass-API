#include <iostream>
#include <list>
#include <set>

#include <stdio.h>

#include "../dispatch/settings.h"
#include "block_backend.h"

using namespace std;

/**
 * Tests the library block_backend
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
  IntIndex(void* data) : value(*(uint32*)data) {}
  IntIndex(int i) : value(i) {}
  
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
  
  bool operator<(const IntIndex& index) const
  {
    return this->value < index.value;
  }
  
  bool operator==(const IntIndex& index) const
  {
    return this->value == index.value;
  }
  
  int val() const
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

void fill_db
  (const map< IntIndex, set< IntObject > >& to_delete,
   const map< IntIndex, set< IntObject > >& to_insert,
   unsigned int step)
{
  try
  {
    Block_Backend< IntIndex, IntObject > db_backend(0, true);
    db_backend.update(to_delete, to_insert);
  }
  catch (File_Error e)
  {
    cout<<"File error catched in part "<<step<<": "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is unexpected)\n";
  }
}

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Read_Iterator& it)
{
  if (it == blocks.end())
  {
    cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.end()))
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

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Range_Iterator& it)
{
  if (it == blocks.range_end())
  {
    cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.range_end()))
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

void read_test(unsigned int step)
{
  try
  {
    Block_Backend< IntIndex, IntObject >
	db_backend(0, false);
    
    cout<<"Read test\n";
  
    cout<<"Reading all blocks ...\n";
    Block_Backend< IntIndex, IntObject >::Read_Iterator it(db_backend.begin());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";

    set< IntIndex > index_list;
    for (unsigned int i(0); i < 100; i += 9)
      index_list.insert(&i);
    cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
    it = db_backend.select_blocks(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(0); i < 10; ++i)
      index_list.insert(&i);
    cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
    it = db_backend.select_blocks(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";

    set< pair< IntIndex, IntIndex > > range_list;
    uint32 fool(0), foou(10);
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [0, 10[ ...\n";
    Block_Backend< IntIndex, IntObject >::Range_Iterator
	rit(db_backend.select_blocks
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end())));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(90); i < 100; ++i)
      index_list.insert(&i);
    cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
    it = db_backend.select_blocks(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 90;
    foou = 100;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [90, 100[ ...\n";
    rit = db_backend.select_blocks
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    uint32 foo(50);
    index_list.insert(&foo);
    cout<<"Reading blocks with index 50 ...\n";
    it = db_backend.select_blocks(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 50;
    foou = 51;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [50, 51[ ...\n";
    rit = db_backend.select_blocks
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 0;
    foou = 10;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 50;
    foou = 51;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 90;
    foou = 100;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
    rit = db_backend.select_blocks
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    cout<<"Reading blocks with indices \\emptyset ...\n";
    it = db_backend.select_blocks(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    cout<<"File error catched in part "<<step<<": "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is the unexpected)\n";
  }
}

int main(int argc, char* args[])
{
  cout<<"** Test the behaviour for non-exsiting files\n";
  remove((get_file_base_name(0) + get_index_suffix(0)).c_str());
  remove((get_file_base_name(0) + get_data_suffix(0)).c_str());
  try
  {
    Block_Backend< IntIndex, IntIndex > db_backend(0, false);
  }
  catch (File_Error e)
  {
    cout<<"File error catched in part 1: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    cout<<"(This is the expected correct behaviour)\n";
  }

  map< IntIndex, set< IntObject > > to_delete;
  map< IntIndex, set< IntObject > > to_insert;
  set< IntObject > objects;
  
  cout<<"** Test the behaviour for an empty db\n";
  fill_db(to_delete, to_insert, 2);
  read_test(3);
  
  cout<<"** Test the behaviour for a db with one entry\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  objects.insert(IntObject(642));
  to_insert[42] = objects;
  
  fill_db(to_delete, to_insert, 4);
  read_test(5);
  
  cout<<"** Test the behaviour for a db with multiple short indizes\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; i += 9)
  {
    set< IntObject > objects;
    objects.insert(IntObject((i%10 + 10)*100 + i));
    to_insert[i] = objects;
  }
  
  fill_db(to_delete, to_insert, 6);
  read_test(7);
  
  cout<<"** Delete an item\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  objects.insert(IntObject(642));
  to_delete[42] = objects;
  
  fill_db(to_delete, to_insert, 8);
  read_test(9);
  
  cout<<"** Add some empty indizes (should not appear)\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 99; i += 9)
  {
    set< IntObject > objects;
    to_insert[i+1] = objects;
  }
  
  fill_db(to_delete, to_insert, 10);
  read_test(11);
  
  cout<<"** Add much more items\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; ++i)
  {
    set< IntObject > objects;
    objects.insert(IntObject(900 + i));
    objects.insert(IntObject(2000 + i));
    to_insert[i] = objects;
  }
  
  fill_db(to_delete, to_insert, 12);
  read_test(13);
  
  cout<<"** Blow up a single index\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  for (unsigned int i(0); i < 100; ++i)
    objects.insert(IntObject(i*100 + 124));
  to_insert[24] = objects;
  
  fill_db(to_delete, to_insert, 14);
  read_test(15);
  
  cout<<"** Delete much items\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; ++i)
  {
    set< IntObject > objects;
    objects.insert(IntObject(900 + i));
    objects.insert(IntObject(2000 + i));
    to_delete[i] = objects;
  }
  
  fill_db(to_delete, to_insert, 16);
  read_test(17);
  
  return 0;
}
