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

#include <iostream>
#include <list>
#include <set>

#include <cstdio>

#include "block_backend.h"
#include "block_backend_write.h"
#include "transaction.h"


/**
 * Tests the library block_backend
 */

//-----------------------------------------------------------------------------

/* Sample class for TIndex */
struct IntIndex
{
  typedef uint32 Id_Type;

  IntIndex(void* data) : value(*(uint32*)data) {}
  IntIndex(int i) : value(i) {}

  static bool equal(void* lhs, void* rhs) { return *(uint32*)lhs == *(uint32*)rhs; }
  bool less(void* rhs) const { return value < *(uint32*)rhs; }
  bool leq(void* rhs) const { return value <= *(uint32*)rhs; }
  bool equal(void* rhs) const { return value == *(uint32*)rhs; }

  uint32 size_of() const { return 4; }
  static constexpr uint32 const_size() { return 4; }
  static uint32 size_of(void* data) { return 4; }

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
  typedef uint32 Id_Type;

  IntObject(void* data) : value(*(uint32*)data)
  {
    if (value >= 1000000000 && value/100000000%2 == 0)
    {
      uint size = (value - 1000000000)%1000000 + 4;
      for (uint i = 1; 4*i < size - 3; ++i)
      {
        if (*(((uint32*)data)+i) != (0x55aa0000 | (i & 0xffff)))
        {
          value = -i;
          break;
        }
      }
      if (value > 0 && size%4 >= 3 && *(((uint8*)data)+size-3) != 0xa5)
        value = -1000000003;
      if (value > 0 && size%4 >= 2 && *(((uint8*)data)+size-2) != 0x5a)
        value = -1000000002;
      if (value > 0 && size%4 >= 1 && *(((uint8*)data)+size-1) != 0xa5)
        value = -1000000001;
    }
  }

  IntObject(int i) : value(i) {}

  uint32 size_of() const
  {
    return value < 1000000000 || value/100000000%2 == 1 ? 4 : (value - 1000000000)%1000000 + 4;
  }

  static uint32 size_of(void* data)
  {
    return *(uint32*)data < 1000000000 || *(uint32*)data/100000000%2 == 1
        ? 4 : (*(uint32*)data - 1000000000)%1000000 + 4;
  }

  void to_data(void* data) const
  {
    if (value >= 1000000000 && value/100000000%2 == 0)
    {
      uint size = (value - 1000000000)%1000000 + 4;
      *(((uint8*)data)+size-3) = 0xa5;
      *(((uint8*)data)+size-2) = 0x5a;
      *(((uint8*)data)+size-1) = 0xa5;
      for (uint i = 1; 4*i < size - 3; ++i)
        *(((uint32*)data)+i) = (0x55aa0000 | (i & 0xffff));
    }
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
    int32 value;
};


typedef std::list< IntIndex >::const_iterator IntIterator;

struct IntRangeIterator : std::list< std::pair< IntIndex, IntIndex > >::const_iterator
{
  IntRangeIterator
      (const std::list< std::pair< IntIndex, IntIndex > >::const_iterator it)
  : std::list< std::pair< IntIndex, IntIndex > >::const_iterator(it) {}

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
std::string BASE_DIRECTORY("./");
std::string DATA_SUFFIX(".bin");
std::string INDEX_SUFFIX(".idx");


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
    return INDEX_SUFFIX;
  }

  const std::string& get_data_suffix() const
  {
    return DATA_SUFFIX;
  }

  const std::string& get_id_suffix() const
  {
    static std::string result("");
    return result;
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
    return File_Blocks_Index_Base::NO_COMPRESSION;
  }

  uint32 get_map_block_size() const
  {
    return 16;
  }

  uint32 get_map_compression_factor() const
  {
    return 1;
  }

  std::vector< bool > get_data_footprint(const std::string& db_dir) const
  {
    return std::vector< bool >();
  }

  std::vector< bool > get_map_footprint(const std::string& db_dir) const
  {
    return std::vector< bool >();
  }

  uint32 id_max_size_of() const
  {
    throw std::string();
    return 0;
  }

  File_Blocks_Index_Base* new_data_index(
      Access_Mode access_mode, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    if (access_mode == Access_Mode::writeable || access_mode == Access_Mode::truncate)
      return new Writeable_File_Blocks_Index< IntIndex >
          (*this, access_mode, use_shadow, db_dir, file_name_extension);
    return new Readonly_File_Blocks_Index< IntIndex >
        (*this, use_shadow, db_dir, file_name_extension);
  }
};

//-----------------------------------------------------------------------------

void fill_db
  (const std::map< IntIndex, std::set< IntObject > >& to_delete,
   const std::map< IntIndex, std::set< IntObject > >& to_insert,
   unsigned int step)
{
/*  remove((get_file_name_trunk(0) + get_index_suffix(0)).c_str());
  remove((get_file_name_trunk(0) + get_data_suffix(0)).c_str());*/
  try
  {
    Nonsynced_Transaction transaction(Access_Mode::writeable, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntObject > db_backend(transaction.data_index(&tf));
    db_backend.update(to_delete, to_insert);
  }
  catch (File_Error& e)
  {
    std::cout<<"File error catched in part "<<step<<": "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";

    throw;
  }
}

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Flat_Iterator& it)
{
  if (it == blocks.flat_end())
  {
    std::cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  std::cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.flat_end()))
  {
    if (!(current_idx == it.index()))
    {
      current_idx = it.index();
      std::cout<<"\nIndex "<<current_idx.val()<<": ";
    }
    std::cout<<it.object().val()<<' ';
    ++it;
  }
  std::cout<<'\n';
}

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Discrete_Iterator& it)
{
  if (it == blocks.discrete_end())
  {
    std::cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  std::cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.discrete_end()))
  {
    if (!(current_idx == it.index()))
    {
      current_idx = it.index();
      std::cout<<"\nIndex "<<current_idx.val()<<": ";
    }
    std::cout<<it.object().val()<<' ';
    ++it;
  }
  std::cout<<'\n';
}

void read_loop
    (Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Range_Iterator& it)
{
  if (it == blocks.range_end())
  {
    std::cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  std::cout<<"Index "<<current_idx.val()<<": ";
  while (!(it == blocks.range_end()))
  {
    if (!(current_idx == it.index()))
    {
      current_idx = it.index();
      std::cout<<"\nIndex "<<current_idx.val()<<": ";
    }
    std::cout<<it.object().val()<<' ';
    ++it;
  }
  std::cout<<'\n';
}

void read_test(unsigned int step)
{
  try
  {
    Nonsynced_Transaction transaction(Access_Mode::readonly, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntObject >
	db_backend(transaction.data_index(&tf));

    std::cout<<"Read test\n";

    std::cout<<"Reading all blocks ...\n";
    Block_Backend< IntIndex, IntObject >::Flat_Iterator fit(db_backend.flat_begin());
    read_loop(db_backend, fit);
    std::cout<<"... all blocks read.\n";

    std::vector< IntIndex > index_list;
    for (unsigned int i(0); i < 100; i += 9)
      index_list.push_back(&i);
    std::cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
    Block_Backend< IntIndex, IntObject >::Discrete_Iterator
	it(db_backend.discrete_begin(index_list.begin(), index_list.end()));
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    for (unsigned int i(0); i < 10; ++i)
      index_list.push_back(&i);
    std::cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";

    uint32 fool(0), foou(10);
    Ranges< IntIndex > ranges((IntIndex(&fool)), (IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0, 10[ ...\n";
    Block_Backend< IntIndex, IntObject >::Range_Iterator
	rit(db_backend.range_begin(ranges));
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    for (unsigned int i(90); i < 100; ++i)
      index_list.push_back(&i);
    std::cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";

    fool = 90;
    foou = 100;
    ranges = Ranges< IntIndex >(IntIndex(&fool), IntIndex(&foou));
    std::cout<<"Reading blocks with indices [90, 100[ ...\n";
    rit = db_backend.range_begin(ranges);
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    uint32 foo(50);
    index_list.push_back(&foo);
    std::cout<<"Reading blocks with index 50 ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";

    fool = 50;
    foou = 51;
    ranges = Ranges< IntIndex >(IntIndex(&fool), IntIndex(&foou));
    std::cout<<"Reading blocks with indices [50, 51[ ...\n";
    rit = db_backend.range_begin(ranges);
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";

    ranges = Ranges< IntIndex >();
    fool = 0;
    foou = 10;
    ranges.push_back(IntIndex(&fool), IntIndex(&foou));
    fool = 50;
    foou = 51;
    ranges.push_back(IntIndex(&fool), IntIndex(&foou));
    fool = 90;
    foou = 100;
    ranges.push_back(IntIndex(&fool), IntIndex(&foou));
    std::cout<<"Reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
    rit = db_backend.range_begin(ranges);
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    std::cout<<"Reading blocks with indices \\emptyset ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";

    std::cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error& e)
  {
    std::cout<<"File error catched in part "<<step<<": "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";

    throw;
  }
}

int main(int argc, char* args[])
{
  std::string test_to_execute;
  if (argc > 1)
    test_to_execute = args[1];

  if ((test_to_execute == "") || (test_to_execute == "1"))
    std::cout<<"** Test the behaviour for non-exsiting files\n";
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_data_suffix()).c_str());
  try
  {
    Nonsynced_Transaction transaction(Access_Mode::readonly, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntIndex > db_backend
        (transaction.data_index(&tf));
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched in part 1: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is the expected correct behaviour)\n";
  }

  std::map< IntIndex, std::set< IntObject > > to_delete;
  std::map< IntIndex, std::set< IntObject > > to_insert;
  std::set< IntObject > objects;

  if ((test_to_execute == "") || (test_to_execute == "2"))
    std::cout<<"** Test the behaviour for an empty db\n";
  fill_db(to_delete, to_insert, 2);
  if ((test_to_execute == "") || (test_to_execute == "2"))
    read_test(2);

  if ((test_to_execute == "") || (test_to_execute == "3"))
    std::cout<<"** Test the behaviour for a db with one entry\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  objects.insert(IntObject(642));
  to_insert[42] = objects;

  fill_db(to_delete, to_insert, 4);
  if ((test_to_execute == "") || (test_to_execute == "3"))
    read_test(3);

  if ((test_to_execute == "") || (test_to_execute == "4"))
    std::cout<<"** Test the behaviour for a db with multiple short indizes\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; i += 9)
  {
    std::set< IntObject > objects;
    objects.insert(IntObject((i%10 + 10)*100 + i));
    to_insert[i] = objects;
  }

  fill_db(to_delete, to_insert, 6);
  if ((test_to_execute == "") || (test_to_execute == "4"))
    read_test(4);

  if ((test_to_execute == "") || (test_to_execute == "5"))
    std::cout<<"** Delete an item\n";
  to_delete.clear();
  to_insert.clear();
  objects.clear();
  objects.insert(IntObject(642));
  to_delete[42] = objects;

  fill_db(to_delete, to_insert, 8);
  if ((test_to_execute == "") || (test_to_execute == "5"))
    read_test(5);

  if ((test_to_execute == "") || (test_to_execute == "6"))
    std::cout<<"** Add some empty indices (should not appear)\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 99; i += 9)
  {
    std::set< IntObject > objects;
    to_insert[i+1] = objects;
  }

  fill_db(to_delete, to_insert, 10);
  if ((test_to_execute == "") || (test_to_execute == "6"))
    read_test(6);

  if ((test_to_execute == "") || (test_to_execute == "7"))
    std::cout<<"** Add much more items\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; ++i)
  {
    std::set< IntObject > objects;
    objects.insert(IntObject(900 + i));
    objects.insert(IntObject(2000 + i));
    to_insert[i] = objects;
  }

  fill_db(to_delete, to_insert, 12);
  if ((test_to_execute == "") || (test_to_execute == "7"))
    read_test(7);

  if ((test_to_execute == "") || (test_to_execute == "8"))
    std::cout<<"** Blow up a single index\n";
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
    std::cout<<"** Blow up an index and delete some data\n";
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
    std::cout<<"** Blow up further the same index\n";
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
    std::cout<<"** Blow up two indices\n";
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
    std::cout<<"** Delete an entire block\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(55); i < 95; ++i)
  {
    std::set< IntObject > objects;
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
    std::cout<<"** Delete many items\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i(0); i < 100; ++i)
  {
    std::set< IntObject > objects;
    objects.insert(IntObject(900 + i));
    objects.insert(IntObject(2000 + i));
    to_delete[i] = objects;
  }

  fill_db(to_delete, to_insert, 24);
  if ((test_to_execute == "") || (test_to_execute == "13"))
    read_test(13);

  if ((test_to_execute == "") || (test_to_execute == "14"))
    std::cout<<"** Delete an item from a non-existing index between two segments\n";

  to_delete.clear();
  to_insert.clear();
  to_delete[49].insert(IntObject(1049));
  fill_db(to_delete, to_insert, 26);

  if ((test_to_execute == "") || (test_to_execute == "14"))
    read_test(14);

  if ((test_to_execute == "") || (test_to_execute == "15"))
    std::cout<<"** Insert an item after the segment at the end\n";

  to_delete.clear();
  to_insert.clear();
  to_insert[100].insert(IntObject(1000));
  fill_db(to_delete, to_insert, 28);

  if ((test_to_execute == "") || (test_to_execute == "15"))
    read_test(15);

  if ((test_to_execute == "") || (test_to_execute == "16"))
    std::cout<<"** Delete an object from a group block and block up an index immediately behind\n";

  to_delete.clear();
  to_delete[0].insert(IntObject(1000));
  to_delete[9].insert(IntObject(1909));
  to_insert.clear();
  for (unsigned int j = 5004; j <= 20004; j += 100)
    to_insert[4].insert(IntObject(j));
  fill_db(to_delete, to_insert, 30);

  if ((test_to_execute == "") || (test_to_execute == "16"))
    read_test(16);

  if ((test_to_execute == "") || (test_to_execute == "17"))
    std::cout<<"** Delete more items\n";
  to_delete.clear();
  to_insert.clear();
  for (unsigned int i = 0; i < 100; ++i)
  {
    std::set< IntObject > objects;
    for (unsigned int j = 1000; j < 2000; ++j)
      objects.insert(IntObject(j));
    to_delete[i] = objects;
  }
  {
    std::set< IntObject > objects;
    for (unsigned int j = 5004; j <= 20004; j += 100)
      objects.insert(IntObject(j));
    to_delete[4] = objects;
  }
  {
    std::set< IntObject > objects;
    for (unsigned int j = 47; j <= 20047; j += 100)
      objects.insert(IntObject(j));
    to_delete[47] = objects;
  }
  {
    std::set< IntObject > objects;
    for (unsigned int j = 48; j <= 20048; j += 100)
      objects.insert(IntObject(j));
    to_delete[48] = objects;
  }
  {
    std::set< IntObject > objects;
    for (unsigned int j = 50; j <= 40050; j += 100)
      objects.insert(IntObject(j));
    to_delete[50] = objects;
  }
  {
    std::set< IntObject > objects;
    for (unsigned int j = 99; j <= 20099; j += 100)
      objects.insert(IntObject(j));
    to_delete[99] = objects;
  }
  to_delete[100].insert(IntObject(1000));

  fill_db(to_delete, to_insert, 32);
  if ((test_to_execute == "") || (test_to_execute == "17"))
    read_test(17);

  if ((test_to_execute == "") || (test_to_execute == "18"))
    std::cout<<"** Insert some oversized objects\n";
  to_delete.clear();
  to_insert.clear();
  to_insert[2].insert(IntObject(1000001221));
  to_insert[3].insert(IntObject(1230));
  to_insert[3].insert(IntObject(1000001231));
  to_insert[4].insert(IntObject(1000001241));
  to_insert[4].insert(IntObject(1100001242));
  to_insert[5].insert(IntObject(1000001251));
  to_insert[5].insert(IntObject(1000001252));
  to_insert[6].insert(IntObject(1000001261));
  to_insert[6].insert(IntObject(1000001262));
  to_insert[7].insert(IntObject(1000001271));
  to_insert[7].insert(IntObject(1100001272));
  to_insert[7].insert(IntObject(1200001273));
  to_insert[8].insert(IntObject(1000001281));

  fill_db(to_delete, to_insert, 34);
  if ((test_to_execute == "") || (test_to_execute == "18"))
    read_test(18);

  if ((test_to_execute == "") || (test_to_execute == "19"))
    std::cout<<"** Keep the oversized objects\n";
  to_delete.clear();
  to_insert.clear();
  to_delete[3].insert(IntObject(1230));
  to_insert[3].insert(IntObject(1100001232));
  to_insert[4].insert(IntObject(1240));
  to_delete[4].insert(IntObject(1100001242));
  to_insert[5].insert(IntObject(1000001253));
  to_insert[7].insert(IntObject(11272));

  fill_db(to_delete, to_insert, 36);
  if ((test_to_execute == "") || (test_to_execute == "19"))
    read_test(19);

  if ((test_to_execute == "") || (test_to_execute == "20"))
    std::cout<<"** Delete multiple oversized objects\n";
  to_delete.clear();
  to_insert.clear();
  to_delete[2].insert(IntObject(1000001221));
  to_delete[3].insert(IntObject(1000001231));
  to_delete[4].insert(IntObject(1000001241));
  to_delete[5].insert(IntObject(1000001251));
  to_delete[6].insert(IntObject(1000001261));
  to_delete[6].insert(IntObject(1000001262));
  to_delete[7].insert(IntObject(1200001273));
  to_delete[8].insert(IntObject(1000001281));
  to_insert[8].insert(IntObject(1282));

  fill_db(to_delete, to_insert, 38);
  if ((test_to_execute == "") || (test_to_execute == "20"))
    read_test(20);

  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_data_suffix()
      + Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_data_suffix()
      + Test_File().get_shadow_suffix()).c_str());
  remove((BASE_DIRECTORY + Test_File().get_file_name_trunk()
      + Test_File().get_data_suffix()).c_str());

  return 0;
}
