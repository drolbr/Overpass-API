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

#ifndef DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_H
#define DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_H

#include "random_file_index.h"
#include "types.h"
#include "lz4_wrapper.h"
#include "zlib_wrapper.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <vector>


template< typename Key, typename Value >
struct Random_File
{
private:
  Random_File(const Random_File& f) {}

public:
  Random_File(Random_File_Index*);
  ~Random_File();

  Value get(Key pos);
  void put(Key pos, const Value& index);

private:
  bool changed;
  uint32 index_size;
  uint32 compression_factor;

  Raw_File val_file;
  Random_File_Index* index;
  Void_Pointer< uint8 > cache;
  uint32 cache_pos;
  uint32 block_size;

  Void_Pointer< uint8 > buffer;

  void move_cache_window(uint32 pos);
  uint32 allocate_block(uint32 data_size);
};


/** Implementation Random_File: ---------------------------------------------*/


template< typename Key, typename Value >
Random_File< Key, Value >::Random_File(Random_File_Index* index_)
  : changed(false), index_size(Value::max_size_of()),
  compression_factor(index_->get_compression_factor()),
  val_file(index_->get_map_file_name(),
	   index_->writeable() ? O_RDWR|O_CREAT : O_RDONLY,
	   S_666, "Random_File:3"),
  index(index_),
  cache(index_->get_block_size() * index_->get_compression_factor()), cache_pos(index->npos),
  block_size(index_->get_block_size()),
  buffer(index_->get_block_size() * index_->get_compression_factor() * 2)  // increased buffer size for lz4
{}


template< typename Key, typename Value >
Random_File< Key, Value >::~Random_File()
{
  move_cache_window(index->npos);
  //delete index;
}


template< typename Key, typename Value >
Value Random_File< Key, Value >::get(Key pos)
{
  move_cache_window(pos.val() / (block_size*compression_factor /index_size));
  return Value(cache.ptr + (pos.val() % (block_size*compression_factor/index_size))*index_size);
}


template< typename Key, typename Value >
void Random_File< Key, Value >::put(Key pos, const Value& val)
{
  if (!index->writeable())
    throw File_Error(0, index->get_map_file_name(), "Random_File:2");

  move_cache_window(pos.val() / (block_size*compression_factor/index_size));
  val.to_data(cache.ptr + (pos.val() % (block_size*compression_factor/index_size))*index_size);
  changed = true;
}


template< typename Key, typename Value >
void Random_File< Key, Value >::move_cache_window(uint32 pos)
{
  // The cache already contains the needed position.
  if ((pos == cache_pos) && (cache_pos != index->npos))
    return;

  if (sigterm_status() && pos != index->npos)
    throw File_Error(0, "-", "SIGTERM received");

  if (pos != index->npos && pos >= 256*1024*1024/Value::max_size_of())
    throw File_Error(0, index->get_map_file_name(), "Random_File: id too large for map file");

  if (changed)
  {
    uint32 data_size = compression_factor;
    void* target = cache.ptr;

    if (index->get_compression_method() == File_Blocks_Index_Base::ZLIB_COMPRESSION)
    {
      target = buffer.ptr;
      uint32 compressed_size = Zlib_Deflate(1)
          .compress(cache.ptr, block_size * compression_factor, target, block_size * index->get_compression_factor());
      data_size = (compressed_size - 1) / block_size + 1;
      zero_padding((uint8*)target + compressed_size, block_size * data_size - compressed_size);
    }
    else if (index->get_compression_method() == File_Blocks_Index_Base::LZ4_COMPRESSION)
    {
      target = buffer.ptr;
      uint32 compressed_size = LZ4_Deflate()
          .compress(cache.ptr, block_size * compression_factor, target, block_size * index->get_compression_factor() * 2);
      data_size = (compressed_size - 1) / block_size + 1;
      zero_padding((uint8*)target + compressed_size, block_size * data_size - compressed_size);
    }

    uint32 disk_pos = allocate_block(data_size);

    // Save the found position to the index.
    if (index->get_blocks().size() <= cache_pos)
      index->get_blocks().resize(cache_pos+1, Random_File_Index_Entry(index->npos, 1));
    index->get_void_blocks().release_block(
        index->get_blocks()[cache_pos].pos, index->get_blocks()[cache_pos].size);
    index->get_blocks()[cache_pos] = Random_File_Index_Entry(disk_pos, data_size);

    // Write the data at the found position.
    val_file.seek((int64)disk_pos*block_size, "Random_File:21");
    val_file.write((uint8*)target, block_size * data_size, "Random_File:22");
  }
  changed = false;

  if (pos == index->npos)
    return;

  if ((index->get_blocks().size() <= pos) || (index->get_blocks()[pos].pos == index->npos))
  {
    // Reset the whole cache to zero.
    for (uint32 i = 0; i < block_size * compression_factor; ++i)
      *(cache.ptr + i) = 0;
  }
  else
  {
    val_file.seek((int64)(index->get_blocks()[pos].pos)*block_size, "Random_File:23");
    if (index->get_compression_method() == File_Blocks_Index_Base::NO_COMPRESSION)
      val_file.read(cache.ptr, block_size * index->get_blocks()[pos].size, "Random_File:24");
    else if (index->get_compression_method() == File_Blocks_Index_Base::ZLIB_COMPRESSION)
    {
      val_file.read(buffer.ptr, block_size * index->get_blocks()[pos].size, "Random_File:25");
      Zlib_Inflate().decompress
          (buffer.ptr, block_size * index->get_blocks()[pos].size, cache.ptr, block_size * index->get_compression_factor());
    }
    else if (index->get_compression_method() == File_Blocks_Index_Base::LZ4_COMPRESSION)
    {
      val_file.read(buffer.ptr, block_size * index->get_blocks()[pos].size, "Random_File:26");
      LZ4_Inflate().decompress
          (buffer.ptr, block_size * index->get_blocks()[pos].size, cache.ptr, block_size * index->get_compression_factor());
    }
  }
  cache_pos = pos;
}


// Finds an appropriate block, removes it from the list of available blocks, and returns it
template< typename Key, typename Value >
uint32 Random_File< Key, Value >::allocate_block(uint32 data_size)
{
  return index->get_void_blocks().allocate_block(data_size);
}


#endif
