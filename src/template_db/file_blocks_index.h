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

#ifndef DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_INDEX_H
#define DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_INDEX_H

#include "types.h"

#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <list>
#include <vector>

/** Declarations: -----------------------------------------------------------*/

using namespace std;

template< class TIndex >
struct File_Block_Index_Entry
{
  static const int EMPTY = 1;
  static const int GROUP = 2;
  static const int SEGMENT = 3;
  static const int LAST_SEGMENT = 4;
  
  File_Block_Index_Entry(const TIndex& i, uint32 pos_, uint32 max_keysize_)
    : index(i), pos(pos_), max_keysize(max_keysize_) {}
  
  TIndex index;
  uint32 pos;
  uint32 max_keysize;
};

template< class TIndex >
struct File_Blocks_Index : public File_Blocks_Index_Base
{
  public:
    File_Blocks_Index(const File_Properties& file_prop,
		      bool writeable, bool use_shadow,
		      string db_dir, string file_name_extension);
    virtual ~File_Blocks_Index();
    bool writeable() const { return (empty_index_file_name != ""); }
    const string& file_name_extension() const { return file_name_extension_; }
    
    string get_data_file_name() const { return data_file_name; }
    uint64 get_block_size() const { return block_size_; }
    
  private:
    string index_file_name;
    string empty_index_file_name;
    string data_file_name;
    string file_name_extension_;
    
  public:
    list< File_Block_Index_Entry< TIndex > > blocks;
    vector< uint32 > void_blocks;
    uint32 block_count;
    uint64 block_size_;
};
 
template< class TIndex >
vector< bool > get_data_index_footprint(const File_Properties& file_prop,
					string db_dir);

/** Implementation File_Blocks_Index: ---------------------------------------*/

template< class TIndex >
File_Blocks_Index< TIndex >::File_Blocks_Index
    (const File_Properties& file_prop, bool writeable, bool use_shadow,
     string db_dir, string file_name_extension) :
     index_file_name(db_dir + file_prop.get_file_name_trunk()
         + file_name_extension + file_prop.get_data_suffix()
         + file_prop.get_index_suffix()
	 + (use_shadow ? file_prop.get_shadow_suffix() : "")),
     empty_index_file_name(writeable ? db_dir + file_prop.get_file_name_trunk()
         + file_name_extension + file_prop.get_data_suffix()
         + file_prop.get_shadow_suffix() : ""),
     data_file_name(db_dir + file_prop.get_file_name_trunk()
         + file_name_extension + file_prop.get_data_suffix()),
     file_name_extension_(file_name_extension),
     block_count(0),
     block_size_(file_prop.get_block_size())
{
  try
  {
    Raw_File val_file(data_file_name, O_RDONLY, S_666, "File_Blocks_Index::File_Blocks_Index::1");
    block_count = val_file.size("File_Blocks_Index::File_Blocks_Index::2")/block_size_;
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
    Raw_File source_file(index_file_name, O_RDONLY, S_666,
			 "File_Blocks_Index::File_Blocks_Index::3");
			 
    // read index file
    uint32 index_size = source_file.size("File_Blocks_Index::File_Blocks_Index::4");
    Void_Pointer< uint8 > index_buf(index_size);
    source_file.read(index_buf.ptr, index_size, "File_Blocks_Index::File_Blocks_Index::5");
    
    uint32 pos(0);
    while (pos < index_size)
    {
      TIndex index(index_buf.ptr+pos);
      File_Block_Index_Entry< TIndex >
          entry(index,
	  *(uint32*)(index_buf.ptr + (pos + TIndex::size_of(index_buf.ptr+pos))),
	  *(uint32*)(index_buf.ptr + (pos + TIndex::size_of(index_buf.ptr+pos) + 4)));
      blocks.push_back(entry);
      if (entry.pos > block_count)
	throw File_Error(0, index_file_name, "File_Blocks_Index: bad pos in index file");
      else
	is_referred[entry.pos] = true;
      pos += TIndex::size_of(index_buf.ptr+pos) + 2*sizeof(uint32);
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
	uint32 void_index_size = void_blocks_file.size("File_Blocks_Index::File_Blocks_Index::6");
	Void_Pointer< uint8 > index_buf(void_index_size);
	void_blocks_file.read(index_buf.ptr, void_index_size,
			      "File_Blocks_Index::File_Blocks_Index::7");
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

template< class TIndex >
File_Blocks_Index< TIndex >::~File_Blocks_Index()
{
  if (empty_index_file_name == "")
    return;

  uint32 index_size(0), pos(0);
  for (typename list< File_Block_Index_Entry< TIndex > >::const_iterator
      it(blocks.begin()); it != blocks.end(); ++it)
    index_size += 2*sizeof(uint32) + it->index.size_of();
  
  Void_Pointer< uint8 > index_buf(index_size);
  
  for (typename list< File_Block_Index_Entry< TIndex > >::const_iterator
      it(blocks.begin()); it != blocks.end(); ++it)
  {
    it->index.to_data(index_buf.ptr+pos);
    pos += it->index.size_of();
    *(uint32*)(index_buf.ptr+pos) = it->pos;
    pos += sizeof(uint32);
    *(uint32*)(index_buf.ptr+pos) = it->max_keysize;
    pos += sizeof(uint32);
  }

  Raw_File dest_file(index_file_name, O_RDWR|O_CREAT, S_666,
		     "File_Blocks_Index::~File_Blocks_Index::1");

  if (index_size < dest_file.size("File_Blocks_Index::~File_Blocks_Index::2"))
    dest_file.resize(index_size, "File_Blocks_Index::~File_Blocks_Index::3");
  dest_file.write(index_buf.ptr, index_size, "File_Blocks_Index::~File_Blocks_Index::4");
  
  // Write void blocks
  Void_Pointer< uint8 > void_index_buf(void_blocks.size()*sizeof(uint32));
  uint32* it_ptr = (uint32*)void_index_buf.ptr;
  for (vector< uint32 >::const_iterator it(void_blocks.begin());
      it != void_blocks.end(); ++it)
    *(it_ptr++) = *it;
  try
  {
    Raw_File void_file(empty_index_file_name, O_RDWR|O_TRUNC, S_666,
		       "File_Blocks_Index::~File_Blocks_Index::5");
    void_file.write(void_index_buf.ptr, void_blocks.size()*sizeof(uint32),
		    "File_Blocks_Index::~File_Blocks_Index::6");
  }
  catch (File_Error e) {}
}

/** Implementation non-members: ---------------------------------------------*/

template< class TIndex >
vector< bool > get_data_index_footprint
    (const File_Properties& file_prop, string db_dir)
{
  File_Blocks_Index< TIndex > index(file_prop, false, false, db_dir, "");
  
  vector< bool > result(index.block_count, true);
  for (typename vector< uint32 >::const_iterator
      it(index.void_blocks.begin()); it != index.void_blocks.end(); ++it)
    result[*it] = false;
  return result;
}

#endif
