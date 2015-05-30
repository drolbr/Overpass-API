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

#ifndef DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_INDEX_H
#define DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_INDEX_H

#include "types.h"

#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <vector>


struct Random_File_Index_Entry
{
  Random_File_Index_Entry(uint32 pos_, uint32 size_)
    : pos(pos_), size(size_) {}
//   Random_File_Index_Entry(const uint32 index_)
//     : index(index_), pos(i), size(i) {}

  uint32 pos;
  uint32 size;
};


struct Random_File_Index
{
  public:
    Random_File_Index(const File_Properties& file_prop,
		      bool writeable, bool use_shadow,
		      const std::string& db_dir, const std::string& file_name_extension);
    ~Random_File_Index();
    bool writeable() const { return (empty_index_file_name != ""); }
    const std::string& file_name_extension() const { return file_name_extension_; }
    
    std::string get_map_file_name() const { return map_file_name; }
    uint64 get_block_size() const { return block_size_; }
    uint32 get_max_size() const { return max_size; }
    uint32 get_compression_method() const { return compression_method; }
    
  private:
    std::string index_file_name;
    std::string empty_index_file_name;
    std::string map_file_name;
    std::string file_name_extension_;
    
  public:
    std::vector< Random_File_Index_Entry > blocks;
    std::vector< std::pair< uint32, uint32 > > void_blocks;
    uint32 block_count;
    uint64 block_size_;
    
    const uint32 npos;
    
    uint count;
    uint32 max_size;
    int compression_method;
    
    static const int FILE_FORMAT_VERSION = 7512;
    static const int NO_COMPRESSION = 0;
    static const int ZLIB_COMPRESSION = 1;
    static const int LZ4_COMPRESSION = 2;
};


inline std::vector< bool > get_map_index_footprint
    (const File_Properties& file_prop, std::string db_dir, bool use_shadow);

    
/** Implementation Random_File_Index: ---------------------------------------*/


inline Random_File_Index::Random_File_Index
    (const File_Properties& file_prop,
     bool writeable, bool use_shadow,
     const std::string& db_dir, const std::string& file_name_extension) :
    index_file_name(db_dir + file_prop.get_file_name_trunk()
        + file_prop.get_id_suffix()
        + file_prop.get_index_suffix()
	+ (use_shadow ? file_prop.get_shadow_suffix() : "")),
    empty_index_file_name(writeable ? db_dir + file_prop.get_file_name_trunk()
        + file_prop.get_id_suffix()
        + file_prop.get_shadow_suffix() : ""),
    map_file_name(db_dir + file_prop.get_file_name_trunk()
        + file_prop.get_id_suffix()),
    file_name_extension_(file_name_extension),
    block_count(0),
    block_size_(file_prop.get_map_block_size()),
    npos(std::numeric_limits< uint32 >::max()),
    count(0),
    max_size(file_prop.get_map_max_size()),
    compression_method(file_prop.get_compression_method())
{
  uint64 file_size = 0;
  try
  {
    Raw_File val_file(map_file_name, O_RDONLY, S_666, "Random_File:8");
    file_size = val_file.size("Random_File:9");
  }
  catch (File_Error e)
  {
    if (e.error_number != 2)
      throw e;
  }
  
  std::vector< bool > is_referred;

  try
  {
    Raw_File source_file
        (index_file_name, writeable ? O_RDONLY|O_CREAT : O_RDONLY, S_666,
	 "Random_File:6");
     
    // read index file
    uint32 index_size = source_file.size("Random_File:10");
    Void_Pointer< uint8 > index_buf(index_size);
    source_file.read(index_buf.ptr, index_size, "Random_File:14");
    
    if (file_name_extension == ".legacy")
      // We support this way the old format although it has no version marker.
    {
      block_count = file_size/block_size_;
      is_referred.resize(block_count, false);

      uint32 pos = 0;
      while (pos < index_size)
      {
        Random_File_Index_Entry entry(*(uint32*)(index_buf.ptr + pos),
            1); //block size is always 1 in the legacy format
        blocks.push_back(entry);
        if (entry.pos != npos)
        {
          if (entry.pos > block_count)
            throw File_Error
            (0, index_file_name, "Random_File: bad pos in index file");
          else
            is_referred[entry.pos] = true;
        }
        pos += 4;
      }
    }
    else if (index_size > 0)
    {
      if (*(int32*)index_buf.ptr != FILE_FORMAT_VERSION)
        throw File_Error(0, index_file_name, "Random_File_Index: Unsupported index file format version");
      block_size_ = 1ull<<*(uint8*)(index_buf.ptr + 4);
      max_size = 1u<<*(uint8*)(index_buf.ptr + 5);
      compression_method = *(uint16*)(index_buf.ptr + 6);

      block_count = file_size / block_size_;
      is_referred.resize(block_count, false);

      uint32 pos = 8;
      while (pos < index_size)
      {
        Random_File_Index_Entry entry(*(uint32*)(index_buf.ptr + pos),
            *(uint32*)(index_buf.ptr + pos + 4));

        blocks.push_back(entry);

        if (entry.pos != npos)
        {
          if (entry.pos > block_count)
            throw File_Error
            (0, index_file_name, "Random_File: bad pos in index file");
          else
            for (uint32 i = 0; i < entry.size; ++i)
              is_referred[entry.pos + i] = true;
        }
        
        pos += 8;
      }
    }  
  }
  catch (File_Error e)
  {
    if (e.error_number != 2)
      throw e;
  }
  
  //if (writeable)
  {
    bool empty_index_file_used = false;
    if (empty_index_file_name != "")
    {
      try
      {
	Raw_File void_blocks_file(empty_index_file_name, O_RDONLY, S_666, "");
	uint32 void_index_size = void_blocks_file.size("Random_File:11");
	Void_Pointer< uint8 > index_buf(void_index_size);
	void_blocks_file.read(index_buf.ptr, void_index_size, "Random_File:15");
	for (uint32 i = 0; i < void_index_size/sizeof(uint32); ++i)
	  void_blocks.push_back(*(std::pair< uint32, uint32 >*)(index_buf.ptr + 8*i));
	empty_index_file_used = true;
      }
      catch (File_Error e) {}
    }
    
    if (!empty_index_file_used)
    {
      // determine void_blocks
      uint32 last_start = 0;
      for (uint32 i(0); i < block_count; ++i)
      {
	if (is_referred[i])
        {
          if (last_start < i)
            void_blocks.push_back(std::make_pair(i - last_start, last_start));
          last_start = i+1;
        }
      }
      if (last_start < block_count)
        void_blocks.push_back(std::make_pair(block_count - last_start, last_start));
    }
  }
  
  std::stable_sort(void_blocks.begin(), void_blocks.end());
}


inline Random_File_Index::~Random_File_Index()
{
  if (empty_index_file_name == "")
    return;

  // Keep space for file version and size information
  uint32 index_size = 8 + 8 * blocks.size();
  uint32 pos = 8;
 
  Void_Pointer< uint8 > index_buf(index_size);
  
  *(uint32*)index_buf.ptr = FILE_FORMAT_VERSION;
  *(uint8*)(index_buf.ptr + 4) = shift_log(block_size_);
  *(uint8*)(index_buf.ptr + 5) = shift_log(max_size);
  *(uint16*)(index_buf.ptr + 6) = compression_method;

  for (std::vector< Random_File_Index_Entry >::const_iterator
      it = blocks.begin(); it != blocks.end(); ++it)
  {
    *(uint32*)(index_buf.ptr+pos) = it->pos;
    pos += 4;
    *(uint32*)(index_buf.ptr+pos) = it->size;
    pos += 4;
  }

  Raw_File dest_file(index_file_name, O_RDWR|O_CREAT, S_666, "Random_File:7");

  if (index_size < dest_file.size("Random_File:12"))
    dest_file.resize(index_size, "Random_File:13");
  dest_file.write(index_buf.ptr, index_size, "Random_File:17");
  
  // Write void blocks
  Void_Pointer< uint8 > void_index_buf(void_blocks.size() * 8);
  std::pair< uint32, uint32 >* it_ptr = (std::pair< uint32, uint32 >*)(void_index_buf.ptr);
  for (std::vector< std::pair< uint32, uint32 > >::const_iterator it(void_blocks.begin());
      it != void_blocks.end(); ++it)
    *(it_ptr++) = *it;
  try
  {
    Raw_File void_file(empty_index_file_name, O_RDWR|O_TRUNC, S_666, "Random_File:5");
    void_file.write(void_index_buf.ptr, void_blocks.size() * 8, "Random_File:18");
  }
  catch (File_Error e) {}
}


/** Implementation non-members: ---------------------------------------------*/


inline std::vector< bool > get_map_index_footprint
    (const File_Properties& file_prop, std::string db_dir, bool use_shadow = false)
{
  Random_File_Index index(file_prop, false, use_shadow, db_dir, "");
  
  std::vector< bool > result(index.block_count, true);
  for (typename std::vector< std::pair< uint32, uint32 > >::const_iterator
      it = index.void_blocks.begin(); it != index.void_blocks.end(); ++it)
  {
    for (uint32 i = 0; i < it->first; ++i)
      result[it->second + i] = false;
  }
  return result;
}

#endif
