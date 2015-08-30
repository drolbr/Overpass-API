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
#include <set>

#include <cstdio>

#include "block_backend.h"
#include "block_backend_cache.h"
#include "transaction.h"


/**
 * Tests the library block_backend
 */

//-----------------------------------------------------------------------------

/* Sample class for TIndex */
struct IntIndex
{
  IntIndex(void* data) : value(*(uint32*)data) {}
  IntIndex(int i) : value(i) {}
  IntIndex() : value(std::numeric_limits< uint32 >::max()) {}
  
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
string BASE_DIRECTORY("./");
string DATA_SUFFIX(".bin");
string INDEX_SUFFIX(".idx");


struct Test_File : File_Properties
{
  const std::string& get_basedir() const { return BASE_DIRECTORY; }
  const std::string& get_index_suffix() const { return INDEX_SUFFIX; }
  const std::string& get_data_suffix() const { return DATA_SUFFIX; }

  uint32 get_block_size() const { return 512; }
  uint32 get_max_size() const { return 1; }
  uint32 get_compression_method() const { return File_Blocks_Index< IntIndex >::NO_COMPRESSION; }
  uint32 get_map_block_size() const { return 16; }
  
  vector< bool > get_data_footprint(const string& db_dir) const { return vector< bool >(); }
  vector< bool > get_map_footprint(const string& db_dir) const { return vector< bool >(); }  

  const std::string& get_id_suffix() const
  {
    static std::string result("");
    return result;
  }

  const std::string& get_file_name_trunk() const
  {
    static std::string result("testfile");
    return result;
  }
  
  const std::string& get_shadow_suffix() const
  {
    static std::string result(".shadow");
    return result;
  }

  uint32 id_max_size_of() const
  {
    throw std::string();
    return 0;
  }
  
  virtual File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const string& db_dir, const string& file_name_extension)
      const
  {
    return new File_Blocks_Index< IntIndex >
        (*this, writeable, use_shadow, db_dir, file_name_extension);
  }
  
  virtual Block_Backend_Cache_Base* new_cache
      (File_Blocks_Index_Base& db_index_, Transaction& transaction_) const
  {
    return new Block_Backend_Cache< IntIndex, IntObject >(db_index_, transaction_);
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
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntObject > db_backend
        (transaction.data_index(tf));
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


bool is_end(Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Flat_Iterator& it)
{
  return it == blocks.flat_end();
}


bool is_end(Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Discrete_Iterator& it)
{
  return it == blocks.discrete_end();
}


bool is_end(Block_Backend< IntIndex, IntObject >& blocks,
     Block_Backend< IntIndex, IntObject >::Range_Iterator& it)
{
  return it == blocks.range_end();
}


template< typename Iterator >
void read_loop(Block_Backend< IntIndex, IntObject >& blocks, Iterator& it, int max_count = 0)
{
  if (is_end(blocks, it))
  {
    std::cout<<"[empty]\n";
    return;
  }
  IntIndex current_idx(it.index());
  std::cout<<"Index "<<current_idx.val()<<": ";
  while (--max_count != 0 && !is_end(blocks, it))
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


template< typename Iterator >
void key_vector_read_loop(Block_Backend< IntIndex, IntObject >& blocks, Iterator& it)
{
  if (is_end(blocks, it))
  {
    std::cout<<"[empty]\n";
    return;
  }
  while (!is_end(blocks, it))
  {
    std::cout<<"Index "<<it.index().val()<<": ";
    
    std::vector< IntObject > values;
    std::pair< int, IntIndex > meta = it.read_whole_key(values);
    
    for (std::vector< IntObject >::const_iterator it2 = values.begin(); it2 != values.end(); ++it2)
      std::cout<<it2->val()<<' ';
    std::cout<<"\nTotal size: "<<meta.first<<", next index: "<<meta.second.val()<<'\n';
  }
}


void print_statistics(const Transaction& transaction)
{
  std::cout<<"Total read: "<<transaction.size_total_requested()<<" bytes in "
      <<transaction.num_total_requested()<<" blocks.\n";
  std::cout<<"Read from disk: "<<transaction.size_read_from_disk()<<" bytes in "
      <<transaction.num_read_from_disk()<<" blocks.\n";
  std::cout<<"Cached: "<<transaction.size_cached()<<" bytes in "<<transaction.num_cached()<<" blocks.\n";
}


void read_test(unsigned int step, long long max_cache_size = 0)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "", max_cache_size);
    Test_File tf;
    Block_Backend< IntIndex, IntObject > db_backend(transaction.data_index(tf));
    
    std::cout<<"Read test\n";
  
    std::cout<<"Reading all blocks ...\n";
    Block_Backend< IntIndex, IntObject >::Flat_Iterator fit(db_backend.flat_begin());
    read_loop(db_backend, fit);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading all blocks ...\n";
    fit = db_backend.flat_begin();
    key_vector_read_loop(db_backend, fit);
    std::cout<<"... all blocks read.\n";
    
    print_statistics(transaction);
    std::cout<<"Reading all blocks with cache ...\n";
    Block_Backend_Flat_Cached_Request< IntIndex, IntObject > flat_request(transaction.get_cache(tf), time(0));
    std::pair< IntIndex, const std::vector< IntObject >* > payload = flat_request.read_whole_key();
    if (!payload.second)
      std::cout<<"[empty]\n";
    while (payload.second)
    {
      std::cout<<"Index "<<payload.first.val()<<": ";
      for (std::vector< IntObject >::const_iterator it = payload.second->begin(); it != payload.second->end(); ++it)
	std::cout<<it->val()<<' ';
      std::cout<<'\n';
      payload = flat_request.read_whole_key();
    }
    std::cout<<"... all blocks read.\n";
    print_statistics(transaction);

    std::set< IntIndex > index_list;
    for (unsigned int i(0); i < 100; i += 9)
      index_list.insert(&i);
    
    std::cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
    Block_Backend< IntIndex, IntObject >::Discrete_Iterator
	it(db_backend.discrete_begin(index_list.begin(), index_list.end()));
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices {0, 9, ..., 99} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    key_vector_read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    
    print_statistics(transaction);
    std::cout<<"Reading blocks with indices {0, 9, ..., 99} with cache ...\n";
    {
      Block_Backend_Discrete_Cached_Request< IntIndex, IntObject > discrete_request
          (transaction.get_cache(tf), time(0), index_list.begin(), index_list.end());
      payload = discrete_request.read_whole_key();
      if (!payload.second)
        std::cout<<"[empty]\n";
      while (payload.second)
      {
        std::cout<<"Index "<<payload.first.val()<<": ";
        for (std::vector< IntObject >::const_iterator it = payload.second->begin(); it != payload.second->end(); ++it)
	  std::cout<<it->val()<<' ';
        std::cout<<'\n';
        payload = discrete_request.read_whole_key();
      }
    }
    std::cout<<"... all blocks read.\n";
    print_statistics(transaction);
    
    if (max_cache_size > 0)
    {
      std::cout<<"Check whether exposed cache value block is sticky ...\n";
      Block_Backend_Discrete_Cached_Request< IntIndex, IntObject > discrete_request
          (transaction.get_cache(tf), time(0), index_list.begin(), index_list.end());
      std::pair< IntIndex, const std::vector< IntObject >* > discrete_payload = discrete_request.read_whole_key();
      if (!discrete_payload.second)
        std::cout<<"[empty]\n";
      
      print_statistics(transaction);
      std::cout<<"Execute flat request to flush cache ...\n";
      Block_Backend_Flat_Cached_Request< IntIndex, IntObject > flat_request(transaction.get_cache(tf), time(0));
      std::pair< IntIndex, const std::vector< IntObject >* > flat_payload = flat_request.read_whole_key();
      if (!flat_payload.second)
        std::cout<<"[empty]\n";
      while (flat_payload.second)
      {
        std::cout<<"Index "<<flat_payload.first.val()<<": ";
        for (std::vector< IntObject >::const_iterator it = flat_payload.second->begin();
	     it != flat_payload.second->end(); ++it)
	  std::cout<<it->val()<<' ';
        std::cout<<'\n';
        flat_payload = flat_request.read_whole_key();
      }
      std::cout<<"... flat request completed.\n";
      print_statistics(transaction);
      
      Block_Backend_Discrete_Cached_Request< IntIndex, IntObject > discrete_request_2
          (transaction.get_cache(tf), time(0), index_list.begin(), index_list.end());
      std::pair< IntIndex, const std::vector< IntObject >* > discrete_payload_2 = discrete_request_2.read_whole_key();
      if (!discrete_payload_2.second)
        std::cout<<"[empty]\n";
      
      std::cout<<"Identical array for both discrete requests: "
          <<(discrete_payload.second == discrete_payload_2.second)<<'\n';
      
      while (discrete_payload.second)
      {
        std::cout<<"Index "<<discrete_payload.first.val()<<": ";
        for (std::vector< IntObject >::const_iterator it = discrete_payload.second->begin();
	     it != discrete_payload.second->end(); ++it)
	  std::cout<<it->val()<<' ';
        std::cout<<'\n';
        discrete_payload = discrete_request.read_whole_key();
      }
      std::cout<<"... done.\n";
    }
  
    index_list.clear();
    for (unsigned int i(0); i < 10; ++i)
      index_list.insert(&i);
    std::cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices {0, 1, ..., 9} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    key_vector_read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    
    print_statistics(transaction);
    std::cout<<"Reading blocks with indices {0, 1, ..., 9} with cache ...\n";
    {
      Block_Backend_Discrete_Cached_Request< IntIndex, IntObject > discrete_request
          (transaction.get_cache(tf), time(0), index_list.begin(), index_list.end());
      payload = discrete_request.read_whole_key();
      if (!payload.second)
        std::cout<<"[empty]\n";
      while (payload.second)
      {
        std::cout<<"Index "<<payload.first.val()<<": ";
        for (std::vector< IntObject >::const_iterator it = payload.second->begin(); it != payload.second->end(); ++it)
	  std::cout<<it->val()<<' ';
        std::cout<<'\n';
        payload = discrete_request.read_whole_key();
      }
    }
    std::cout<<"... all blocks read.\n";
    print_statistics(transaction);

    std::set< std::pair< IntIndex, IntIndex > > range_list;
    uint32 fool(0), foou(10);
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0, 10[ ...\n";
    Block_Backend< IntIndex, IntObject >::Range_Iterator
	rit(db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end())));
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices [0, 10[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    key_vector_read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(90); i < 100; ++i)
      index_list.insert(&i);
    std::cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices {90, 91, ..., 99} ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    key_vector_read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    
    print_statistics(transaction);
    std::cout<<"Reading blocks with indices {90, 91, ..., 99} with cache ...\n";
    {
      Block_Backend_Discrete_Cached_Request< IntIndex, IntObject > discrete_request
          (transaction.get_cache(tf), time(0), index_list.begin(), index_list.end());
      payload = discrete_request.read_whole_key();
      if (!payload.second)
        std::cout<<"[empty]\n";
      while (payload.second)
      {
        std::cout<<"Index "<<payload.first.val()<<": ";
        for (std::vector< IntObject >::const_iterator it = payload.second->begin(); it != payload.second->end(); ++it)
	  std::cout<<it->val()<<' ';
        std::cout<<'\n';
        payload = discrete_request.read_whole_key();
      }
    }
    std::cout<<"... all blocks read.\n";
    print_statistics(transaction);
  
    range_list.clear();
    fool = 90;
    foou = 100;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    key_vector_read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
  
    index_list.clear();
    uint32 foo(50);
    index_list.insert(&foo);
    std::cout<<"Reading blocks with index 50 ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with index 50 ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    key_vector_read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 50;
    foou = 51;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [50, 51[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices [50, 51[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    key_vector_read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 0;
    foou = 10;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 50;
    foou = 51;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 90;
    foou = 100;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    key_vector_read_loop(db_backend, rit);
    std::cout<<"... all blocks read.\n";
  
    index_list.clear();
    std::cout<<"Reading blocks with indices \\emptyset ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, it);
    std::cout<<"... all blocks read.\n";
    std::cout<<"Keywise reading blocks with indices \\emptyset ...\n";
    it = db_backend.discrete_begin(index_list.begin(), index_list.end());
    key_vector_read_loop(db_backend, it);
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


void read_test_skip_part_1(unsigned int step)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntObject > db_backend(transaction.data_index(tf));
    
    std::cout<<"Read and skip test\n";
  
    std::cout<<"Reading all blocks ...\n";
    Block_Backend< IntIndex, IntObject >::Flat_Iterator fit(db_backend.flat_begin());
    read_loop(db_backend, fit, 2);
    std::cout<<"... skip to index 9 ...\n";
    fit.skip_to_index(IntIndex(9));
    read_loop(db_backend, fit, 2);
    std::cout<<"... skip to index 21 ...\n";
    fit.skip_to_index(IntIndex(21));
    read_loop(db_backend, fit, 2);
    std::cout<<"... skip to index 99 ...\n";
    fit.skip_to_index(IntIndex(99));
    read_loop(db_backend, fit, 0);
    std::cout<<"... all blocks read.\n";

    std::set< IntIndex > index_list;
    for (unsigned int i(0); i < 100; i += 9)
      index_list.insert(&i);
    
    std::cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
    Block_Backend< IntIndex, IntObject >::Discrete_Iterator
	dit(db_backend.discrete_begin(index_list.begin(), index_list.end()));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 9 ...\n";
    dit.skip_to_index(IntIndex(9));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 21 ...\n";
    dit.skip_to_index(IntIndex(21));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 99 ...\n";
    dit.skip_to_index(IntIndex(99));
    read_loop(db_backend, dit, 0);
    std::cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(0); i < 10; ++i)
      index_list.insert(&i);
    std::cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
    dit = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 9 ...\n";
    dit.skip_to_index(IntIndex(9));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 21 ...\n";
    dit.skip_to_index(IntIndex(21));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 99 ...\n";
    dit.skip_to_index(IntIndex(99));
    read_loop(db_backend, dit, 0);
    std::cout<<"... all blocks read.\n";

    std::set< std::pair< IntIndex, IntIndex > > range_list;
    uint32 fool(0), foou(10);
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0, 10[ ...\n";
    Block_Backend< IntIndex, IntObject >::Range_Iterator
	rit(db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end())));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 9 ...\n";
    rit.skip_to_index(IntIndex(9));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 21 ...\n";
    rit.skip_to_index(IntIndex(21));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 99 ...\n";
    rit.skip_to_index(IntIndex(99));
    read_loop(db_backend, rit, 0);
    std::cout<<"... all blocks read.\n";
  
    index_list.clear();
    for (unsigned int i(90); i < 100; ++i)
      index_list.insert(&i);
    std::cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
    dit = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 9 ...\n";
    dit.skip_to_index(IntIndex(9));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 21 ...\n";
    dit.skip_to_index(IntIndex(21));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 99 ...\n";
    dit.skip_to_index(IntIndex(99));
    read_loop(db_backend, dit, 0);
    std::cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 90;
    foou = 100;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 9 ...\n";
    rit.skip_to_index(IntIndex(9));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 21 ...\n";
    rit.skip_to_index(IntIndex(21));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 99 ...\n";
    rit.skip_to_index(IntIndex(99));
    read_loop(db_backend, rit, 0);
    std::cout<<"... all blocks read.\n";
  
    index_list.clear();
    uint32 foo(50);
    index_list.insert(&foo);
    std::cout<<"Reading blocks with index 50 ...\n";
    dit = db_backend.discrete_begin(index_list.begin(), index_list.end());
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 9 ...\n";
    dit.skip_to_index(IntIndex(9));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 21 ...\n";
    dit.skip_to_index(IntIndex(21));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 99 ...\n";
    dit.skip_to_index(IntIndex(99));
    read_loop(db_backend, dit, 0);
    std::cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 50;
    foou = 51;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [50, 51[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 9 ...\n";
    rit.skip_to_index(IntIndex(9));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 21 ...\n";
    rit.skip_to_index(IntIndex(21));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 99 ...\n";
    rit.skip_to_index(IntIndex(99));
    read_loop(db_backend, rit, 0);
    std::cout<<"... all blocks read.\n";
  
    range_list.clear();
    fool = 0;
    foou = 10;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 50;
    foou = 51;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 90;
    foou = 100;
    range_list.insert(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
    rit = db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end()));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 9 ...\n";
    rit.skip_to_index(IntIndex(9));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 21 ...\n";
    rit.skip_to_index(IntIndex(21));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 99 ...\n";
    rit.skip_to_index(IntIndex(99));
    read_loop(db_backend, rit, 0);
    std::cout<<"... all blocks read.\n";
  
    std::cout<<"This block of read and skip tests is complete.\n";
  }
  catch (File_Error& e)
  {
    std::cout<<"File error catched in part "<<step<<": "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
    
    throw;
  }
}


void read_test_skip_part_2(unsigned int step)
{
  try
  {
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntObject > db_backend(transaction.data_index(tf));

// sktp_to_index:
// - in 4: skip to next index 9, skip to nonexist 21, skip close to end 99 (with all variants)
// - in 9: skip between blocks to 49, skip beyond end 100, (0, 48, 50, 99), [47, 52[
    
    std::cout<<"Read and skip test\n";
  
    std::cout<<"Reading all blocks ...\n";
    Block_Backend< IntIndex, IntObject >::Flat_Iterator fit(db_backend.flat_begin());
    read_loop(db_backend, fit, 2);
    std::cout<<"... skip to index 49 ...\n";
    fit.skip_to_index(IntIndex(49));
    read_loop(db_backend, fit, 2);
    std::cout<<"... skip to index 100 ...\n";
    fit.skip_to_index(IntIndex(100));
    read_loop(db_backend, fit, 0);
    std::cout<<"... all blocks read.\n";

    std::set< IntIndex > index_list;
    index_list.insert(0);
    index_list.insert(48);
    index_list.insert(50);
    index_list.insert(99);
    
    std::cout<<"Reading blocks with indices {0, 48, 50, 99} ...\n";
    Block_Backend< IntIndex, IntObject >::Discrete_Iterator
	dit(db_backend.discrete_begin(index_list.begin(), index_list.end()));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 49 ...\n";
    dit.skip_to_index(IntIndex(49));
    read_loop(db_backend, dit, 2);
    std::cout<<"... skip to index 100 ...\n";
    dit.skip_to_index(IntIndex(100));
    read_loop(db_backend, dit, 0);
    std::cout<<"... all blocks read.\n";
  
    std::set< std::pair< IntIndex, IntIndex > > range_list;
    range_list.insert(std::make_pair(IntIndex(47), IntIndex(52)));
    std::cout<<"Reading blocks with indices [47, 52[ ...\n";
    Block_Backend< IntIndex, IntObject >::Range_Iterator
	rit(db_backend.range_begin
	(Default_Range_Iterator< IntIndex >(range_list.begin()),
	 Default_Range_Iterator< IntIndex >(range_list.end())));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 49 ...\n";
    rit.skip_to_index(IntIndex(49));
    read_loop(db_backend, rit, 2);
    std::cout<<"... skip to index 100 ...\n";
    rit.skip_to_index(IntIndex(100));
    read_loop(db_backend, rit, 0);
    std::cout<<"... all blocks read.\n";
  
    std::cout<<"This block of read and skip tests is complete.\n";
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
  string test_to_execute;
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
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Test_File tf;
    Block_Backend< IntIndex, IntIndex > db_backend(transaction.data_index(tf));
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
  {
    read_test(4);
    read_test_skip_part_1(4);
  }
  
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
  {
    read_test(7);
    read_test(7, 100);
  }
  
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
  {
    read_test(9);
    read_test_skip_part_2(9);
  }
  
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
