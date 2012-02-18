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
* along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_INDEX_H
#define DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_INDEX_H

#include "types.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <vector>

using namespace std;

struct Random_File_Index
{
  public:
    Random_File_Index(const File_Properties& file_prop,
		      bool writeable, bool use_shadow, string db_dir);
    ~Random_File_Index();
    bool writeable() const { return (empty_index_file_name != ""); }
    
    string get_map_file_name() const { return map_file_name; }
    uint64 get_block_size() const { return block_size_; }
    
    typedef uint32 size_t;
    
  private:
    string index_file_name;
    string empty_index_file_name;
    string map_file_name;
    
  public:
    vector< size_t > blocks;
    vector< size_t > void_blocks;
    size_t block_count;    
    uint64 block_size_;
    
    const size_t npos;
    
    uint count;
};

inline vector< bool > get_map_index_footprint
    (const File_Properties& file_prop, string db_dir, bool use_shadow);

/** Implementation Random_File_Index: ---------------------------------------*/

inline Random_File_Index::Random_File_Index
    (const File_Properties& file_prop,
     bool writeable, bool use_shadow, string db_dir) :
    index_file_name(db_dir + file_prop.get_file_name_trunk()
        + file_prop.get_id_suffix()
        + file_prop.get_index_suffix()
	+ (use_shadow ? file_prop.get_shadow_suffix() : "")),
    empty_index_file_name(writeable ? db_dir + file_prop.get_file_name_trunk()
        + file_prop.get_id_suffix()
        + file_prop.get_shadow_suffix() : ""),
    map_file_name(db_dir + file_prop.get_file_name_trunk()
        + file_prop.get_id_suffix()),
    block_count(0),
    block_size_(file_prop.get_map_block_size()),
    npos(numeric_limits< size_t >::max()), count(0)
{
  try
  {
    Raw_File val_file(map_file_name, O_RDONLY, S_666, "Random_File:8");
    block_count = val_file.size("Random_File:9")/block_size_;
  }
  catch (File_Error e)
  {
    if (e.error_number != 2)
      throw e;
    block_count = 0;
  }
  
  vector< bool > is_referred(block_count, false);

  try
  {
    Raw_File source_file
        (index_file_name, writeable ? O_RDONLY|O_CREAT : O_RDONLY, S_666,
	 "Random_File:6");
     
    // read index file
    uint32 index_size = source_file.size("Random_File:10");
    Void_Pointer< uint8 > index_buf(index_size);
    source_file.read(index_buf.ptr, index_size, "Random_File:14");
    
    uint32 pos = 0;
    while (pos < index_size)
    {
      size_t* entry = (size_t*)(index_buf.ptr+pos);
      blocks.push_back(*entry);
      if (*entry != npos)
      {
	if (*entry > block_count)
	  throw File_Error
	      (0, index_file_name, "Random_File: bad pos in index file");
	else
	  is_referred[*entry] = true;
      }
      pos += sizeof(size_t);
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
	  void_blocks.push_back(*(uint32*)(index_buf.ptr + 4*i));
	empty_index_file_used = true;
      }
      catch (File_Error e) {}
    }
    
    if (!empty_index_file_used)
    {
      // determine void_blocks
      for (uint32 i(0); i < block_count; ++i)
      {
	if (!(is_referred[i]))
	  void_blocks.push_back(i);
      }
    }
  }
}

inline Random_File_Index::~Random_File_Index()
{
  if (empty_index_file_name == "")
    return;

  // Write index file
  uint32 index_size = blocks.size()*sizeof(size_t);
  uint32 pos = 0;
 
  Void_Pointer< uint8 > index_buf(index_size);
  
  for (vector< size_t >::const_iterator it(blocks.begin()); it != blocks.end();
      ++it)
  {
    *(size_t*)(index_buf.ptr+pos) = *it;
    pos += sizeof(size_t);
  }

  Raw_File dest_file(index_file_name, O_RDWR|O_CREAT, S_666, "Random_File:7");

  if (index_size < dest_file.size("Random_File:12"))
    dest_file.resize(index_size, "Random_File:13");
  dest_file.write(index_buf.ptr, index_size, "Random_File:17");
  
  // Write void blocks
  Void_Pointer< uint8 > void_index_buf(void_blocks.size()*sizeof(uint32));
  uint32* it_ptr = (uint32*)void_index_buf.ptr;
  for (vector< size_t >::const_iterator it(void_blocks.begin());
      it != void_blocks.end(); ++it)
    *(it_ptr++) = *it;
  try
  {
    Raw_File void_file(empty_index_file_name, O_RDWR|O_TRUNC, S_666, "Random_File:5");
    void_file.write(void_index_buf.ptr, void_blocks.size()*sizeof(uint32), "Random_File:18");
  }
  catch (File_Error e) {}
}

/** Implementation non-members: ---------------------------------------------*/

inline vector< bool > get_map_index_footprint
    (const File_Properties& file_prop, string db_dir, bool use_shadow = false)
{
  Random_File_Index index(file_prop, false, use_shadow, db_dir);
  
  vector< bool > result(index.block_count, true);
  for (vector< uint32 >::const_iterator it(index.void_blocks.begin());
       it != index.void_blocks.end(); ++it)
    result[*it] = false;
  return result;
}

#endif
