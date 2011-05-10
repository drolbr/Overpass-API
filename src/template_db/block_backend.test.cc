#include <iostream>
#include <list>
#include <set>

#include <stdio.h>

#include "block_backend.h"

using namespace std;

/**
 * Tests the library block_backend
 */

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
  
  string get_file_base_name() const
  {
    return BASE_DIRECTORY + "testfile";
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
    throw string();
    return 0;
  }
  
  File_Blocks_Index_Base* new_data_index
  (string index_file_name, string empty_index_file_name,
   string file_name_extension, uint32 block_count) const
   {
     return new File_Blocks_Index< IntIndex >
     (index_file_name, empty_index_file_name, file_name_extension, block_count);
   }
};

//-----------------------------------------------------------------------------

void fill_db
  (const map< IntIndex, set< IntObject > >& to_delete,
   const map< IntIndex, set< IntObject > >& to_insert,
   unsigned int step)
{
/*  remove((get_file_base_name(0) + get_index_suffix(0)).c_str());
  remove((get_file_base_name(0) + get_data_suffix(0)).c_str());*/
  try
  {
    Block_Backend< IntIndex, IntObject > db_backend(Test_File(), true, false);
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

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Discrete_Iterator& it)
{
  if (it == blocks.discrete_end())
  {
    cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.discrete_end()))
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
	db_backend(Test_File(), false, false);
    
    cout<<"Read test\n";
  
    cout<<"Reading all blocks ...\n";
    Block_Backend< IntIndex, IntObject >::Flat_Iterator fit(db_backend.flat_begin());
    read_loop(db_backend, fit);
    cout<<"... all blocks read.\n";

    set< IntIndex > index_list;
    for (unsigned int i(0); i < 100; i += 9)
      index_list.insert(&i);
    cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
    Block_Backend< IntIndex, IntObject >::Discrete_Iterator
	it(db_backend.discrete_begin(index_list.begin(), index_list.end()));
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(0); i < 10; ++i)
      index_list.insert(&i);
    cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";

    set< pair< IntIndex, IntIndex > > range_list;
    uint32 fool(0), foou(10);
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [0, 10[ ...\n";
    Block_Backend< IntIndex, IntObject >::Range_Iterator
	rit(db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end())));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(90); i < 100; ++i)
      index_list.insert(&i);
    cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 90;
    foou = 100;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    uint32 foo(50);
    index_list.insert(&foo);
    cout<<"Reading blocks with index 50 ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 50;
    foou = 51;
    range_list.insert(make_pair(IntIndex(&fool), IntIndex(&foou)));
    cout<<"Reading blocks with indices [50, 51[ ...\n";
    rit = db_backend.range_begin
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
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    cout<<"... all blocks read.\n";
  
    index_list.clear();
    cout<<"Reading blocks with indices \\emptyset ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
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
  string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];
  
  if ((test_to_execute == "") || (test_to_execute == "1"))
    cout<<"** Test the behaviour for non-exsiting files\n";
  remove((Test_File().get_file_base_name() + Test_File().get_index_suffix()).c_str());
  remove((Test_File().get_file_base_name() + Test_File().get_data_suffix()).c_str());
  try
  {
    Block_Backend< IntIndex, IntIndex > db_backend(Test_File(), false, false);
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
  
  if ((test_to_execute == "") || (test_to_execute == "2"))
    cout<<"** Test the behaviour for an empty db\n";
  fill_db(to_delete, to_insert, 2);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    read_test(2);
  
  if ((test_to_execute == "") || (test_to_execute == "3"))
    cout<<"** Test the behaviour for a db with one entry\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  objects.insert(IntObject(642));
  to_insert[42] = objects;
  
  fill_db(to_delete, to_insert, 4);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    read_test(3);
  
  if ((test_to_execute == "") || (test_to_execute == "4"))
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
  if ((test_to_execute == "") || (test_to_execute == "4"))
    read_test(4);
  
  if ((test_to_execute == "") || (test_to_execute == "5"))
    cout<<"** Delete an item\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  objects.insert(IntObject(642));
  to_delete[42] = objects;
  
  fill_db(to_delete, to_insert, 8);
  if ((test_to_execute == "") || (test_to_execute == "5"))
    read_test(5);
  
  if ((test_to_execute == "") || (test_to_execute == "6"))
    cout<<"** Add some empty indices (should not appear)\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 99; i += 9)
  {
    set< IntObject > objects;
    to_insert[i+1] = objects;
  }
  
  fill_db(to_delete, to_insert, 10);
  if ((test_to_execute == "") || (test_to_execute == "6"))
    read_test(6);
  
  if ((test_to_execute == "") || (test_to_execute == "7"))
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
  if ((test_to_execute == "") || (test_to_execute == "7"))
    read_test(7);
  
  if ((test_to_execute == "") || (test_to_execute == "8"))
    cout<<"** Blow up a single index\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  for (unsigned int i(0); i < 200; ++i)
    objects.insert(IntObject(i*200 + 150));
  to_insert[50] = objects;
  
  fill_db(to_delete, to_insert, 14);
  if ((test_to_execute == "") || (test_to_execute == "8"))
    read_test(8);
  
  if ((test_to_execute == "") || (test_to_execute == "9"))
    cout<<"** Blow up an index and delete some data\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  for (unsigned int i(0); i < 200; ++i)
    objects.insert(IntObject(i*100 + 148));
  to_insert[48] = objects;
  objects.clear();
  objects.insert(IntObject(949));
  objects.insert(IntObject(2049));
  to_delete[49] = objects;
  
  fill_db(to_delete, to_insert, 16);
  if ((test_to_execute == "") || (test_to_execute == "9"))
    read_test(9);
  
  if ((test_to_execute == "") || (test_to_execute == "10"))
    cout<<"** Blow up further the same index\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  for (unsigned int i(0); i < 201; ++i)
    objects.insert(IntObject(i*200 + 50));
  to_insert[50] = objects;
  
  fill_db(to_delete, to_insert, 18);
  if ((test_to_execute == "") || (test_to_execute == "10"))
    read_test(10);
  
  if ((test_to_execute == "") || (test_to_execute == "11"))
    cout<<"** Blow up two indices\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  for (unsigned int i(0); i < 200; ++i)
    objects.insert(IntObject(i*100 + 147));
  to_insert[47] = objects;
  objects.clear();
  for (unsigned int i(0); i < 200; ++i)
    objects.insert(IntObject(i*100 + 199));
  to_insert[99] = objects;
  
  fill_db(to_delete, to_insert, 20);
  if ((test_to_execute == "") || (test_to_execute == "11"))
    read_test(11);
  
  if ((test_to_execute == "") || (test_to_execute == "12"))
    cout<<"** Delete an entire block\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(55); i < 95; ++i)
  {
    set< IntObject > objects;
    objects.insert(IntObject(900 + i));
    objects.insert(IntObject(2000 + i));
    if (i%9 == 0)
      objects.insert(IntObject((i%10 + 10)*100 + i));
    to_delete[i] = objects;
  }
  
  fill_db(to_delete, to_insert, 22);
  if ((test_to_execute == "") || (test_to_execute == "12"))
    read_test(12);
  
  if ((test_to_execute == "") || (test_to_execute == "13"))
    cout<<"** Delete many items\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; ++i)
  {
    set< IntObject > objects;
    objects.insert(IntObject(900 + i));
    objects.insert(IntObject(2000 + i));
    to_delete[i] = objects;
  }
  
  fill_db(to_delete, to_insert, 24);
  if ((test_to_execute == "") || (test_to_execute == "13"))
    read_test(13);
  
  remove((Test_File().get_file_base_name() + Test_File().get_data_suffix()
      + Test_File().get_index_suffix()).c_str());
  remove((Test_File().get_file_base_name() + Test_File().get_data_suffix()
      + Test_File().get_shadow_suffix()).c_str());
  remove((Test_File().get_file_base_name() + Test_File().get_data_suffix()).c_str());
  
  return 0;
}
