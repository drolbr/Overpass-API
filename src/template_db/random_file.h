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

#ifndef DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_H
#define DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_H

#include "random_file_index.h"
#include "types.h"
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
  uint32 max_size;
  
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
  max_size(index_->get_max_size()),
  val_file(index_->get_map_file_name(),
	   index_->writeable() ? O_RDWR|O_CREAT : O_RDONLY,
	   S_666, "Random_File:3"),
  index(index_),
  cache(index_->get_block_size() * index_->get_max_size()), cache_pos(index->npos),
  block_size(index_->get_block_size()),
  buffer(index_->get_block_size() * index_->get_max_size() * 2)
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
  move_cache_window(pos.val() / (block_size*max_size /index_size));
  return Value(cache.ptr + (pos.val() % (block_size*max_size/index_size))*index_size);
}


template< typename Key, typename Value >
void Random_File< Key, Value >::put(Key pos, const Value& val)
{
  if (!index->writeable())
    throw File_Error(0, index->get_map_file_name(), "Random_File:2");
  
  move_cache_window(pos.val() / (block_size*max_size/index_size));
  val.to_data(cache.ptr + (pos.val() % (block_size*max_size/index_size))*index_size);
  changed = true;
}


template< typename Key, typename Value >
void Random_File< Key, Value >::move_cache_window(uint32 pos)
{
  // The cache already contains the needed position.
  if ((pos == cache_pos) && (cache_pos != index->npos))
    return;

  if (changed)
  {
    uint32 data_size = max_size;
    void* target = cache.ptr;

    if (index->compression_method == Random_File_Index::ZLIB_COMPRESSION)
    {
      target = buffer.ptr;
      uint32 compressed_size = Zlib_Deflate(1)
          .compress(cache.ptr, block_size * max_size, target, block_size * index->max_size);
      data_size = (compressed_size - 1) / block_size + 1;
      zero_padding((uint8*)target + compressed_size, block_size * data_size - compressed_size); 
    }

    uint32 disk_pos = allocate_block(data_size);
    
    // Save the found position to the index.
    if (index->blocks.size() <= cache_pos)
      index->blocks.resize(cache_pos+1, Random_File_Index_Entry(index->npos, 1));
    Random_File_Index_Entry entry(disk_pos, data_size);
    index->blocks[cache_pos] = entry;
    
    // Write the data at the found position.
    val_file.seek((int64)disk_pos*block_size, "Random_File:21");
    val_file.write((uint8*)target, block_size * data_size, "Random_File:22");
  }
  changed = false;
  
  if (pos == index->npos)
    return;
  
  if ((index->blocks.size() <= pos) || (index->blocks[pos].pos == index->npos))
  {
    // Reset the whole cache to zero.
    for (uint32 i = 0; i < block_size * max_size; ++i)
      *(cache.ptr + i) = 0;
  }
  else
  {
    val_file.seek((int64)(index->blocks[pos].pos)*block_size, "Random_File:23");
    if (index->compression_method == Random_File_Index::NO_COMPRESSION)
      val_file.read(cache.ptr, block_size * index->blocks[pos].size, "Random_File:24");
    else if (index->compression_method == Random_File_Index::ZLIB_COMPRESSION)
    {
      val_file.read(buffer.ptr, block_size * index->blocks[pos].size, "Random_File:24");
      Zlib_Inflate().decompress
          (buffer.ptr, block_size * index->blocks[pos].size, cache.ptr, block_size * index->max_size);
    }
  }
  cache_pos = pos;
}


// Finds an appropriate block, removes it from the list of available blocks, and returns it
template< typename Key, typename Value >
uint32 Random_File< Key, Value >::allocate_block(uint32 data_size)
{
  uint32 result = this->index->block_count;

  if (this->index->void_blocks.empty())
    this->index->block_count += data_size;
  else
  {
    std::vector< std::pair< uint32, uint32 > >::iterator pos_it
    = std::lower_bound(this->index->void_blocks.begin(), this->index->void_blocks.end(),
        std::make_pair(data_size, uint32(0)));

    if (pos_it != this->index->void_blocks.end() && pos_it->first == data_size)
    {
      // We have a gap of exactly the needed size.
      result = pos_it->second;
      this->index->void_blocks.erase(pos_it);
    }
    else
    {
      pos_it = --(this->index->void_blocks.end());
      uint32 last_size = pos_it->first;
      while (pos_it != this->index->void_blocks.begin() && last_size > data_size)
      {
        --pos_it;
        if (last_size == pos_it->first)
        {
          // We have a gap size that appears twice (or more often).
          // This is a good heuristic choice.
          result = pos_it->second;
          pos_it->first -= data_size;
          pos_it->second += data_size;
          rearrange_block(this->index->void_blocks.begin(), pos_it, *pos_it);
          return result;
        }
        last_size = pos_it->first;
      }

      pos_it = --(this->index->void_blocks.end());
      if (pos_it->first >= data_size)
      {
        // If no really matching block exists then we choose the largest one.
        result = pos_it->second;
        pos_it->first -= data_size;
        pos_it->second += data_size;
        rearrange_block(this->index->void_blocks.begin(), pos_it, *pos_it);
      }
      else
        this->index->block_count += data_size;
    }
  }

  return result;
}


#endif
