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

#include <stdio.h>

#include "file_blocks.h"
#include "transaction.h"


/**
 * Tests the library file_blocks
 */

//-----------------------------------------------------------------------------

/* Sample class for TIndex */
struct IntIndex
{
  IntIndex(uint32 i) : value(i) {}
  IntIndex(void* data) : value(*(uint32*)data) {}

  static bool equal(void* lhs, void* rhs) { return *(uint32*)lhs == *(uint32*)rhs; }
  bool less(void* rhs) const { return value < *(uint32*)rhs; }
  bool leq(void* rhs) const { return value <= *(uint32*)rhs; }
  
  uint32 size_of() const { return (value < 24 ? 12 : value-12); }
  static uint32 size_of(void* data) { return ((*(uint32*)data) < 24 ? 12 : (*(uint32*)data)-12); }

  void to_data(void* data) const
  {
    uint32 size = size_of();
    *(uint32*)(((uint8*)data) + size - 4) = 0x5a5a5a5a;
    *(uint32*)data = value;
    uint32 i = 1;
    while (i < size/4)
    {
      *(uint32*)(((uint8*)data) + 4*i) = 4*i/3;
      ++i;
    }
  }

  bool operator<(const IntIndex& index) const { return this->value < index.value; }
  bool operator==(const IntIndex& index) const { return this->value == index.value; }

  uint32 val() const { return value; }

  private:
    uint32 value;
};


typedef std::list< IntIndex >::const_iterator IntIterator;


struct IntRangeIterator : std::list< std::pair< IntIndex, IntIndex > >::const_iterator
{
  IntRangeIterator() {}

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

  uint32 get_map_compression_factor() const
  {
    return 1;
  }

  uint32 get_compression_method() const
  {
    return File_Blocks_Index_Base::NO_COMPRESSION;
  }

  uint32 get_map_compression_method() const
  {
    return File_Blocks_Index_Base::NO_COMPRESSION;
  }

  uint32 get_map_block_size() const
  {
    return 16;
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

  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    if (writeable)
      return new Writeable_File_Blocks_Index< IntIndex >
          (*this, use_shadow, db_dir, file_name_extension);
    return new Readonly_File_Blocks_Index< IntIndex >
        (*this, use_shadow, db_dir, file_name_extension);
  }
};


struct Variable_Block_Test_File : File_Properties
{
  const std::string& get_basedir() const
  {
    return BASE_DIRECTORY;
  }

  const std::string& get_file_name_trunk() const
  {
    static std::string result("variable");
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
    return 64;
  }

  uint32 get_compression_factor() const
  {
    return 8;
  }

  uint32 get_map_compression_factor() const
  {
    return 1;
  }

  uint32 get_compression_method() const
  {
    return File_Blocks_Index_Base::NO_COMPRESSION;
  }

  uint32 get_map_compression_method() const
  {
    return File_Blocks_Index_Base::NO_COMPRESSION;
  }

  uint32 get_map_block_size() const
  {
    return 16;
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

  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    if (writeable)
      return new Writeable_File_Blocks_Index< IntIndex >
          (*this, use_shadow, db_dir, file_name_extension);
    return new Readonly_File_Blocks_Index< IntIndex >
        (*this, use_shadow, db_dir, file_name_extension);
  }
};


struct Compressed_Test_File : File_Properties
{
  const std::string& get_basedir() const
  {
    return BASE_DIRECTORY;
  }

  const std::string& get_file_name_trunk() const
  {
    static std::string result("compressed");
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
    return 8*1024;
  }

  uint32 get_compression_factor() const
  {
    return 8;
  }

  uint32 get_map_compression_factor() const
  {
    return 8;
  }

  uint32 get_compression_method() const
  {
#ifdef HAVE_LZ4
    return File_Blocks_Index_Base::LZ4_COMPRESSION;
#else
    return File_Blocks_Index_Base::ZLIB_COMPRESSION;
#endif
  }

  uint32 get_map_compression_method() const
  {
    return File_Blocks_Index_Base::NO_COMPRESSION;
  }

  uint32 get_map_block_size() const
  {
    return 4*1024;
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

  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    if (writeable)
      return new Writeable_File_Blocks_Index< IntIndex >
          (*this, use_shadow, db_dir, file_name_extension);
    return new Readonly_File_Blocks_Index< IntIndex >
        (*this, use_shadow, db_dir, file_name_extension);
  }
};

//-----------------------------------------------------------------------------

void read_loop(
    File_Blocks< IntIndex, IntIterator >& blocks,
    File_Blocks< IntIndex, IntIterator >::Flat_Iterator& it,
    uint32 block_size)
{
  while (!it.is_end())
  {
    std::cout<<"Predicted size "<<blocks.answer_size(it);
    uint8* data = (uint8*)(blocks.read_block(it));
    uint32 max_keysize = *(uint32*)(data+sizeof(uint32));
    std::cout<<", real size "<<(*(uint32*)data)<<" bytes, "
    <<"first block size "<<max_keysize<<" bytes, "
    <<"first index "<<*(uint32*)(data+2*sizeof(uint32));
    if (max_keysize < (*(uint32*)data)-sizeof(uint32))
    {
      uint8* pos(data+sizeof(uint32));
      pos += *(uint32*)pos;
      std::cout<<", second block size "<<(*(uint32*)pos)<<" bytes, "
      <<"second index "<<*(uint32*)(pos+sizeof(uint32))<<'\n';
    }
    else if (max_keysize > (*(uint32*)data)-sizeof(uint32))
    {
      uint32 large_block_size = ((max_keysize+3)/block_size+1)*block_size;
      std::cout<<"\nChecking "<<large_block_size<<" bytes for oversized object.\n";

      uint first_deviating = block_size;
      for (uint i = 5; i < block_size/4; ++i)
      {
        if (*(uint32*)(data+4*i) != 3*i + 91000)
        {
          first_deviating = i;
          break;
        }
      }
      if (first_deviating < block_size)
        std::cout<<"Wrong block content at block 0, offset "<<4*first_deviating<<": "
            "expected "<<(3*first_deviating + 91000)<<", found "
            <<(*(uint32*)(data+4*first_deviating))<<'\n';
      else
        std::cout<<"Content ok in block 0.\n";

      for (uint i = block_size; i < large_block_size; i += block_size)
      {
        ++it;
        if (it.is_end())
        {
          std::cout<<"Unexpected end of index inside oversized object.\n";
          break;
        }
        data = (uint8*)(blocks.read_block(it, false));
        uint32 offset = i/4;
        for (uint j = 0; 4*j < block_size && 4*j + i <= max_keysize; ++j)
        {
          if (*(uint32*)(data+4*j) != 3*(j + offset) + 91000)
          {
            first_deviating = j;
            break;
          }
        }
        if (first_deviating < block_size)
          std::cout<<"Wrong block content at block "<<i/block_size<<", offset "<<4*first_deviating<<": "
              "expected "<<(3*(first_deviating + offset) + 91000)<<", found "
              <<(*(uint32*)(data+4*first_deviating))<<'\n';
        else
          std::cout<<"Content ok in block "<<i/block_size<<".\n";
      }
    }
    else
      std::cout<<'\n';
    if (!it.is_end())
      ++it;
  }
}

void read_loop(
    File_Blocks< IntIndex, IntIterator >& blocks,
    File_Blocks< IntIndex, IntIterator >::Discrete_Iterator& it,
    uint32 block_size)
{
  while (!it.is_end())
  {
    uint32 answer_size(blocks.answer_size(it));
    std::cout<<"Predicted size "<<answer_size;
    if (answer_size > 0)
    {
      uint8* data((uint8*)(blocks.read_block(it)));
      std::cout<<", real size "<<(*(uint32*)data)<<" bytes, "
	  <<"first block size "<<*(uint32*)(data+sizeof(uint32))<<" bytes, "
	  <<"first index "<<*(uint32*)(data+2*sizeof(uint32));
      if (*(uint32*)(data+sizeof(uint32)) < (*(uint32*)data)-sizeof(uint32))
      {
	uint8* pos(data+sizeof(uint32));
	pos += *(uint32*)pos;
	std::cout<<", second block size "<<(*(uint32*)pos)<<" bytes, "
	    <<"second index "<<*(uint32*)(pos+sizeof(uint32));
      }
      else if (*(uint32*)(data+sizeof(uint32)) > (*(uint32*)data)-sizeof(uint32))
      {
        uint32 large_block_size = (((*(uint32*)(data+4))+3)/block_size+1)*block_size;
        std::cout<<"\nSkipping "<<large_block_size<<" bytes for oversized object.";
        for (uint i = block_size; i < large_block_size; i += block_size)
        {
          ++it;
          if (it.is_end())
          {
            std::cout<<"\nUnexpected end of index inside oversized object.";
            break;
          }
        }
      }
    }
    std::cout<<'\n';
    if (!it.is_end())
      ++it;
  }
}

void read_loop(
    File_Blocks< IntIndex, IntIterator >& blocks,
    File_Blocks_Range_Iterator< IntIndex, IntRangeIterator >& it,
    uint32 block_size)
{
  while (!it.is_end())
  {
    std::cout<<"Predicted size "<<blocks.answer_size(it);
    uint8* data((uint8*)(blocks.read_block(it)));
    std::cout<<", real size "<<(*(uint32*)data)<<" bytes, "
    <<"first block size "<<*(uint32*)(data+sizeof(uint32))<<" bytes, "
    <<"first index "<<*(uint32*)(data+2*sizeof(uint32));
    if (*(uint32*)(data+sizeof(uint32)) < (*(uint32*)data)-sizeof(uint32))
    {
      uint8* pos(data+sizeof(uint32));
      pos += *(uint32*)pos;
      std::cout<<", second block size "<<(*(uint32*)pos)<<" bytes, "
      <<"second index "<<*(uint32*)(pos+sizeof(uint32));
    }
    else if (*(uint32*)(data+sizeof(uint32)) > (*(uint32*)data)-sizeof(uint32))
    {
      uint32 large_block_size = (((*(uint32*)(data+4))+3)/block_size+1)*block_size;
      std::cout<<"\nSkipping "<<large_block_size<<" bytes for oversized object.";
      for (uint i = block_size; i < large_block_size; i += block_size)
      {
        ++it;
        if (it.is_end())
        {
          std::cout<<"\nUnexpected end of index inside oversized object.";
          break;
        }
      }
    }
    std::cout<<'\n';
    if (!it.is_end())
      ++it;
  }
}


void read_test()
{
  try
  {
    std::cout<<"Read test\n";
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::vector< bool > footprint = get_data_index_footprint< IntIndex >
        (Test_File(), BASE_DIRECTORY);
    std::cout<<"Index footprint: ";
    for (std::vector< bool >::const_iterator it(footprint.begin()); it != footprint.end();
        ++it)
      std::cout<<*it;
    std::cout<<'\n';

    std::cout<<"Reading all blocks ...\n";
    File_Blocks< IntIndex, IntIterator >::Flat_Iterator
	fit(blocks.flat_begin());
    read_loop(blocks, fit, block_size);
    std::cout<<"... all blocks read.\n";

    std::list< IntIndex > index_list;
    for (unsigned int i(0); i < 100; i += 9)
      index_list.push_back(&i);
    std::cout<<"Reading blocks with indices {0, 9, ..., 99} ...\n";
    File_Blocks< IntIndex, IntIterator >::Discrete_Iterator
	it(blocks.discrete_begin(index_list.begin(), index_list.end()));
    read_loop(blocks, it, block_size);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    for (unsigned int i(0); i < 10; ++i)
      index_list.push_back(&i);
    std::cout<<"Reading blocks with indices {0, 1, ..., 9} ...\n";
    it = blocks.discrete_begin(index_list.begin(), index_list.end());
    read_loop(blocks, it, block_size);
    std::cout<<"... all blocks read.\n";

    std::list< std::pair< IntIndex, IntIndex > > range_list;
    uint32 fool(0), foou(10);
    range_list.push_back(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0, 10[ ...\n";
    File_Blocks_Range_Iterator< IntIndex, IntRangeIterator >
	rit(blocks.range_begin
	(IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end())));
    read_loop(blocks, rit, block_size);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    for (unsigned int i(90); i < 100; ++i)
      index_list.push_back(&i);
    std::cout<<"Reading blocks with indices {90, 91, ..., 99} ...\n";
    it = blocks.discrete_begin(index_list.begin(), index_list.end());
    read_loop(blocks, it, block_size);
    std::cout<<"... all blocks read.\n";

    range_list.clear();
    fool = 90;
    foou = 100;
    range_list.push_back(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [90, 100[ ...\n";
    rit = blocks.range_begin
	(IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end()));
    read_loop(blocks, rit, block_size);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    uint32 foo(50);
    index_list.push_back(&foo);
    std::cout<<"Reading blocks with index 50 ...\n";
    it = blocks.discrete_begin(index_list.begin(), index_list.end());
    read_loop(blocks, it, block_size);
    std::cout<<"... all blocks read.\n";

    range_list.clear();
    fool = 50;
    foou = 51;
    range_list.push_back(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [50, 51[ ...\n";
    rit = blocks.range_begin
	(IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end()));
    read_loop(blocks, rit, block_size);
    std::cout<<"... all blocks read.\n";

    range_list.clear();
    fool = 0;
    foou = 10;
    range_list.push_back(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 50;
    foou = 51;
    range_list.push_back(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    fool = 90;
    foou = 100;
    range_list.push_back(std::make_pair(IntIndex(&fool), IntIndex(&foou)));
    std::cout<<"Reading blocks with indices [0,10[\\cup [50, 51[\\cup [90, 100[ ...\n";
    rit = blocks.range_begin
	(IntRangeIterator(range_list.begin()), IntRangeIterator(range_list.end()));
    read_loop(blocks, rit, block_size);
    std::cout<<"... all blocks read.\n";

    index_list.clear();
    std::cout<<"Reading blocks with indices \\emptyset ...\n";
    it = blocks.discrete_begin(index_list.begin(), index_list.end());
    read_loop(blocks, it, block_size);
    std::cout<<"... all blocks read.\n";

    std::cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is the expected correct behaviour)\n";
  }
}


void variable_block_read_test()
{
  try
  {
    std::cout<<"Compressed Read test\n";
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::vector< bool > footprint = get_data_index_footprint< IntIndex >
        (Variable_Block_Test_File(), BASE_DIRECTORY);
    std::cout<<"Index footprint: ";
    for (std::vector< bool >::const_iterator it(footprint.begin()); it != footprint.end();
        ++it)
      std::cout<<*it;
    std::cout<<'\n';

    std::cout<<"Reading all blocks ...\n";
    File_Blocks< IntIndex, IntIterator >::Flat_Iterator
	fit(blocks.flat_begin());
    read_loop(blocks, fit, block_size);
    std::cout<<"... all blocks read.\n";

    std::cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is the expected correct behaviour)\n";
  }
}


void compressed_read_test()
{
  try
  {
    std::cout<<"Compressed Read test\n";
    Nonsynced_Transaction transaction(false, false, BASE_DIRECTORY, "");
    Compressed_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::vector< bool > footprint = get_data_index_footprint< IntIndex >
        (Variable_Block_Test_File(), BASE_DIRECTORY);
    std::cout<<"Index footprint: ";
    for (std::vector< bool >::const_iterator it(footprint.begin()); it != footprint.end();
        ++it)
      std::cout<<*it;
    std::cout<<'\n';

    std::cout<<"Reading all blocks ...\n";
    File_Blocks< IntIndex, IntIterator >::Flat_Iterator
	fit(blocks.flat_begin());
    read_loop(blocks, fit, block_size);
    std::cout<<"... all blocks read.\n";

    std::cout<<"This block of read tests is complete.\n";
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is the expected correct behaviour)\n";
  }
}


uint32 prepare_block(void* block, const std::list< IntIndex >& indices)
{
  uint32 max_keysize(0);

  if (indices.empty())
  {
    *(uint32*)block = 0;
    return 0;
  }

  uint32 pos(sizeof(uint32));
  for (std::list< IntIndex >::const_iterator it(indices.begin());
      it != indices.end(); ++it)
  {
    if ((*it).val() + 12 > max_keysize)
      max_keysize = (*it).val() + 12;

    *(uint32*)(((uint8*)block)+pos) = (*it).val() + 12;
    (*it).to_data(((uint8*)block)+pos+sizeof(uint32));
    pos += (*it).val() + 12;
  }

  *(uint32*)block = pos;

  return max_keysize;
}


uint32 prepare_large_block(void* block, IntIndex index, uint32 block_size, uint32 total_size, uint offset)
{
  for (uint i = 0; i < block_size/4; ++i)
    *(((uint32*)block)+i) = 3*(i + offset) + 91000;

  if (offset == 0)
  {
    *(uint32*)block = block_size;
    *(((uint32*)block)+1) = total_size;
    index.to_data(((uint32*)block)+2);
  }

  return total_size;
}


int main(int argc, char* args[])
{
  std::string test_to_execute;
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

  if ((test_to_execute == "") || (test_to_execute == "info"))
  {
    Compressed_Test_File tf;

    if (tf.get_compression_method() == File_Blocks_Index_Base::NO_COMPRESSION)
      std::cout<<"Using no compression for bin files.\n";
    else if (tf.get_compression_method() == File_Blocks_Index_Base::ZLIB_COMPRESSION)
      std::cout<<"Using zlib compression for bin files.\n";
    else if (tf.get_compression_method() == File_Blocks_Index_Base::LZ4_COMPRESSION)
      std::cout<<"Using lz4 compression for bin files.\n";

    if (tf.get_map_compression_method() == File_Blocks_Index_Base::NO_COMPRESSION)
      std::cout<<"Using no compression for map files.\n";
    else if (tf.get_map_compression_method() == File_Blocks_Index_Base::ZLIB_COMPRESSION)
      std::cout<<"Using zlib compression for map files.\n";
    else if (tf.get_map_compression_method() == File_Blocks_Index_Base::LZ4_COMPRESSION)
      std::cout<<"Using lz4 compression for map files.\n";
  }

  if ((test_to_execute == "") || (test_to_execute == "1"))
  {
    std::cout<<"** Test the behaviour for an empty file\n";
    read_test();
  }

  if ((test_to_execute == "") || (test_to_execute == "2"))
    std::cout<<"** Test the behaviour for a file with one entry - part 1\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(49));
    indices.push_back(IntIndex(50));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "2"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "3"))
    std::cout<<"** Test the behaviour for a file with one entry - part 2\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(51));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.replace_block(
        blocks.write_begin(indices.begin(), indices.end()), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "3"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "4"))
    std::cout<<"** Test the behaviour for a file with three entries\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(9));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(
        blocks.write_begin(indices.begin(), indices.end()), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(89));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "4"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "5"))
    std::cout<<"** Test insertion everywhere\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(10));
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(63));
    indices.push_back(IntIndex(64));
    indices.push_back(IntIndex(65));
    indices.push_back(IntIndex(89));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
	it = blocks.write_begin(indices.begin(), indices.end());

    ++it;
    indices.clear();
    indices.push_back(IntIndex(10));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;

    indices.clear();
    indices.push_back(IntIndex(63));
    max_keysize = prepare_block(buf, indices);
    it = blocks.insert_block(it, buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(64));
    max_keysize = prepare_block(buf, indices);
    it = blocks.insert_block(it, buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(65));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "5"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "6"))
    std::cout<<"** Test to replace blocks\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices, work;

    indices.clear();
    indices.push_back(IntIndex(7));
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(52));
    indices.push_back(IntIndex(89));
    indices.push_back(IntIndex(90));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it(blocks.write_begin(indices.begin(), indices.end()));

    work.clear();
    work.push_back(IntIndex(7));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
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
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "6"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "7"))
    std::cout<<"** Delete blocks in between\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(64));
    indices.push_back(IntIndex(65));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it(blocks.write_begin(indices.begin(), indices.end()));

    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "7"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "8"))
    std::cout<<"** Delete blocks at the begin and the end\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(7));
    indices.push_back(IntIndex(90));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it(blocks.write_begin(indices.begin(), indices.end()));

    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "8"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "9"))
    std::cout<<"** Test insertion again\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices, work;

    indices.clear();
    for (unsigned int i(20); i < 30; ++i)
      indices.push_back(IntIndex(i));
    indices.push_back(IntIndex(63));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it(blocks.write_begin(indices.begin(), indices.end()));

    ++it;
    for (unsigned int i(20); i < 30; ++i)
    {
      work.clear();
      work.push_back(IntIndex(i));
      uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
      uint32 max_keysize(prepare_block(buf, work));
      it = blocks.insert_block(it, buf, max_keysize);
      free(buf);
    }
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "9"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "10"))
    std::cout<<"** Delete everything\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    for (unsigned int i(0); i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it(blocks.write_begin(indices.begin(), indices.end()));

    while (!it.is_end())
    {
      it = blocks.erase_block(it);
    }
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "10"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "11"))
    std::cout<<"** Insert two series of segments\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(40));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(60));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "11"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "12"))
    std::cout<<"** Replace by other series of segments\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices, work;

    indices.clear();
    indices.push_back(IntIndex(8));
    indices.push_back(IntIndex(40));
    indices.push_back(IntIndex(50));
    indices.push_back(IntIndex(60));
    indices.push_back(IntIndex(90));
    indices.push_back(IntIndex(99));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it(blocks.write_begin(indices.begin(), indices.end()));

    work.clear();
    work.push_back(IntIndex(8));
    uint64* buf = (uint64*)aligned_alloc(8, Test_File().get_block_size());
    uint32 max_keysize(prepare_block(buf, work));
    it = blocks.insert_block(it, buf, max_keysize);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;

    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
    it = blocks.erase_block(it);

    work.clear();
    work.push_back(IntIndex(50));
    max_keysize = prepare_block(buf, work);
    it = blocks.insert_block(it, buf, max_keysize);
    it = blocks.insert_block(it, buf, max_keysize);
    ++it;

    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
    it = blocks.erase_block(it);
    it = blocks.erase_block(it);

    work.clear();
    work.push_back(IntIndex(90));
    max_keysize = prepare_block(buf, work);
    it = blocks.insert_block(it, buf, max_keysize);
    it = blocks.insert_block(it, buf, max_keysize);

    work.clear();
    work.push_back(IntIndex(99));
    max_keysize = prepare_block(buf, work);
    it = blocks.insert_block(it, buf, max_keysize);
    it = blocks.insert_block(it, buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "12"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "13"))
    std::cout<<"** Test deleting file content\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    while (!blocks.flat_begin().is_end())
    {
      std::list< IntIndex > indices;
      indices.push_back(blocks.flat_begin().block().index());
      File_Blocks_Write_Iterator< IntIndex, std::list< IntIndex >::const_iterator > it =
          blocks.write_begin(indices.begin(), indices.end());
      while (!it.is_end())
        it = blocks.erase_block(it);
    }
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "13"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "14"))
    std::cout<<"** Test one isolated oversized object\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::list< IntIndex > indices;
    indices.push_back(IntIndex(6));
    File_Blocks_Write_Iterator< IntIndex, std::list< IntIndex >::const_iterator > it =
        blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, block_size);
    uint32 max_keysize = prepare_large_block(
        buf, *indices.begin(), block_size, 2*block_size + block_size/2, 0);
    it = blocks.insert_block(it, buf, block_size, max_keysize, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, block_size/4);
    it = blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, 2*block_size/4);
    blocks.insert_block(it, buf, block_size/2+4, block_size-4, *indices.begin());
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "14"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "15"))
    std::cout<<"** Test one isolated oversized object plus one ordinary object\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::list< IntIndex > indices;
    indices.push_back(IntIndex(6));
    File_Blocks_Write_Iterator< IntIndex, std::list< IntIndex >::const_iterator > it =
        blocks.write_begin(indices.begin(), indices.end());

    ++it;
    ++it;
    ++it;
    uint64* buf = (uint64*)aligned_alloc(8, block_size);
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "15"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "16"))
    std::cout<<"** Test one ordinary object plus one isolated oversized object\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::list< IntIndex > indices;
    indices.push_back(IntIndex(5));
    File_Blocks_Write_Iterator< IntIndex, std::list< IntIndex >::const_iterator > it =
        blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, block_size);
    uint32 max_keysize = prepare_large_block(buf, *indices.begin(), block_size, block_size-4, 0);
    it = blocks.insert_block(it, buf, block_size, max_keysize, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, block_size-3, 0);
    it = blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, block_size/4);
    blocks.insert_block(it, buf, 1, 1, *indices.begin());
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "16"))
    read_test();

  if ((test_to_execute == "") || (test_to_execute == "17"))
    std::cout<<"** Test deleting file content\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    uint32 block_size = tf.get_block_size();

    std::list< IntIndex > indices;
    indices.push_back(IntIndex(6));
    File_Blocks_Write_Iterator< IntIndex, std::list< IntIndex >::const_iterator > it =
        blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, block_size);
    uint32 max_keysize = prepare_large_block(
        buf, *indices.begin(), block_size, 2*block_size + block_size/2 + 20, 0);
    it = blocks.replace_block(it, buf, block_size, max_keysize, *indices.begin());
    ++it;
    ++it;
    prepare_large_block(buf, *indices.begin(), block_size, 0, 2*block_size/4);
    it = blocks.replace_block(it, buf, block_size/2 + 24, block_size/2 + 24, *indices.begin());

    indices.clear();
    indices.push_back(IntIndex(7));
    it = blocks.write_begin(indices.begin(), indices.end());

    max_keysize = prepare_large_block(buf, *indices.begin(), block_size, 3*block_size-3, 0);
    it = blocks.insert_block(it, buf, block_size, max_keysize, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, block_size/4);
    it = blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, 2*block_size/4);
    it = blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, 3*block_size/4);
    it = blocks.insert_block(it, buf, 1, 1, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 3*block_size-4, 0);
    it = blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, block_size/4);
    it = blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    prepare_large_block(buf, *indices.begin(), block_size, 0, 2*block_size/4);
    blocks.insert_block(it, buf, block_size, block_size, *indices.begin());
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "17"))
    read_test();

  remove((BASE_DIRECTORY
      + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()
      + Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()
      + Test_File().get_shadow_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Test_File().get_file_name_trunk() + Test_File().get_data_suffix()).c_str());


  data_fd = open64
      ((BASE_DIRECTORY
        + Variable_Block_Test_File().get_file_name_trunk()
	+ Variable_Block_Test_File().get_data_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  index_fd = open64
      ((BASE_DIRECTORY
        + Variable_Block_Test_File().get_file_name_trunk()
	+ Variable_Block_Test_File().get_data_suffix()
        + Variable_Block_Test_File().get_index_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(index_fd);
  if ((test_to_execute == "") || (test_to_execute == "18"))
  {
    std::cout<<"** Test the behaviour for an empty file\n";
    variable_block_read_test();
  }

  if ((test_to_execute == "") || (test_to_execute == "19"))
    std::cout<<"** Test the behaviour for a compressed file with one entry - part 1\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    indices.clear();
    indices.push_back(IntIndex(20));
    indices.push_back(IntIndex(21));
    indices.push_back(IntIndex(22));
    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size()
        * Variable_Block_Test_File().get_compression_factor());
    uint32 max_keysize(prepare_block(buf, indices));
    blocks.insert_block(blocks.write_end(), buf, max_keysize);
    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "19"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "20"))
    std::cout<<"** Test the behaviour for a compressed file with multiple small entries\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    indices.clear();
    indices.push_back(IntIndex(30));
    indices.push_back(IntIndex(31));
    indices.push_back(IntIndex(32));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(33));
    indices.push_back(IntIndex(34));
    indices.push_back(IntIndex(35));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(36));
    indices.push_back(IntIndex(37));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(38));
    indices.push_back(IntIndex(39));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(40));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(41));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(50));
    indices.push_back(IntIndex(51));
    indices.push_back(IntIndex(52));
    indices.push_back(IntIndex(53));
    indices.push_back(IntIndex(54));
    indices.push_back(IntIndex(55));
    indices.push_back(IntIndex(56));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(57));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(60));
    indices.push_back(IntIndex(61));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    indices.push_back(IntIndex(62));
    indices.push_back(IntIndex(63));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "20"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "21"))
    std::cout<<"** Test the behaviour for a compressed file with multiple deletions\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    ++it;
    it = blocks.erase_block(it);
    ++it;
    it = blocks.erase_block(it);
    ++it;
    it = blocks.erase_block(it);
    ++it;
    it = blocks.erase_block(it);
    ++it;
    it = blocks.erase_block(it);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "21"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "22"))
    std::cout<<"** Test the behaviour for the gap filling strategy - part 1\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 25)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(25));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "22"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "23"))
    std::cout<<"** Test the behaviour for the gap filling strategy - part 2\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 26)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(26));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "23"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "24"))
    std::cout<<"** Test the behaviour for the gap filling strategy - part 3\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 60)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(60));
    indices.push_back(IntIndex(61));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "24"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "25"))
    std::cout<<"** Test the behaviour for the gap filling strategy - part 4\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 65)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(65));
    indices.push_back(IntIndex(66));
    indices.push_back(IntIndex(67));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "25"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "26"))
    std::cout<<"** Test the behaviour for the gap filling strategy - part 5\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 68)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(68));
    indices.push_back(IntIndex(69));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "26"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "27"))
    std::cout<<"** Test the behaviour for the gap filling strategy - part 6\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 70)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(70));
    indices.push_back(IntIndex(71));
    indices.push_back(IntIndex(72));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "27"))
    variable_block_read_test();

  if ((test_to_execute == "") || (test_to_execute == "28"))
    std::cout<<"** Test the behaviour for the gap filling strategy - with replace block\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Variable_Block_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));

    std::list< IntIndex > indices;
    for (int i = 0; i < 100; ++i)
      indices.push_back(IntIndex(i));

    File_Blocks< IntIndex, IntIterator >::Write_Iterator
        it = blocks.write_begin(indices.begin(), indices.end());

    uint64* buf = (uint64*)aligned_alloc(8, Variable_Block_Test_File().get_block_size() * Variable_Block_Test_File().get_compression_factor());

    while (!it.is_end() && it.block().index < 20)
      ++it;
    indices.clear();
    indices.push_back(IntIndex(20));
    indices.push_back(IntIndex(22));
    uint32 max_keysize = prepare_block(buf, indices);
    it = blocks.replace_block(it, buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "28"))
    variable_block_read_test();

  remove((BASE_DIRECTORY
      + Variable_Block_Test_File().get_file_name_trunk() + Variable_Block_Test_File().get_data_suffix()
      + Variable_Block_Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Variable_Block_Test_File().get_file_name_trunk() + Variable_Block_Test_File().get_data_suffix()
      + Variable_Block_Test_File().get_shadow_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Variable_Block_Test_File().get_file_name_trunk() + Variable_Block_Test_File().get_data_suffix()).c_str());
  
  data_fd = open64
      ((BASE_DIRECTORY
        + Compressed_Test_File().get_file_name_trunk()
	+ Compressed_Test_File().get_data_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(data_fd);
  index_fd = open64
      ((BASE_DIRECTORY
        + Compressed_Test_File().get_file_name_trunk()
	+ Compressed_Test_File().get_data_suffix()
        + Compressed_Test_File().get_index_suffix()).c_str(),
       O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(index_fd);
  if ((test_to_execute == "") || (test_to_execute == "29"))
  {
    std::cout<<"** Test the behaviour for an empty file\n";
    compressed_read_test();
  }

  if ((test_to_execute == "") || (test_to_execute == "30"))
    std::cout<<"** Test the behaviour for a compressed file with some entries\n";
  try
  {
    Nonsynced_Transaction transaction(true, false, BASE_DIRECTORY, "");
    Compressed_Test_File tf;
    File_Blocks< IntIndex, IntIterator > blocks
        (transaction.data_index(&tf));
    std::list< IntIndex > indices;

    uint64* buf = (uint64*)aligned_alloc(8, Compressed_Test_File().get_block_size() * Compressed_Test_File().get_compression_factor());

    indices.clear();
    for (int i = 20; i < 21; ++i)
      indices.push_back(IntIndex(i));
    uint32 max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    for (int i = 100; i < 280; ++i)
      indices.push_back(IntIndex(i));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    indices.clear();
    for (int i = 1000; i < 1060; ++i)
      indices.push_back(IntIndex(i));
    max_keysize = prepare_block(buf, indices);
    blocks.insert_block(blocks.write_end(), buf, max_keysize);

    free(buf);
  }
  catch (File_Error e)
  {
    std::cout<<"File error catched: "
        <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
    std::cout<<"(This is unexpected)\n";
  }
  if ((test_to_execute == "") || (test_to_execute == "30"))
    compressed_read_test();

  remove((BASE_DIRECTORY
      + Compressed_Test_File().get_file_name_trunk() + Compressed_Test_File().get_data_suffix()
      + Compressed_Test_File().get_index_suffix()).c_str());
  remove((BASE_DIRECTORY
      + Compressed_Test_File().get_file_name_trunk() + Compressed_Test_File().get_data_suffix()).c_str());

  return 0;
}
