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

#ifndef DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_INDEX_H
#define DE__OSM3S___TEMPLATE_DB__RANDOM_FILE_INDEX_H

#include "types.h"
#include "void_blocks.h"

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
	      Access_Mode access_mode, bool use_shadow,
	      const std::string& db_dir, const std::string& file_name_extension,
              int compression_method_ = File_Blocks_Index_Base::USE_DEFAULT);
  ~Random_File_Index();
  bool writeable() const { return (empty_index_file_name != ""); }
  const std::string& file_name_extension() const { return file_name_extension_; }

  std::string get_map_file_name() const { return map_file_name; }
  uint64 get_block_size() const { return block_size_; }
  uint32 get_compression_factor() const { return compression_factor; }
  uint32 get_compression_method() const { return compression_method; }

  std::vector< Random_File_Index_Entry >& get_blocks()
  {
    return blocks;
  }
  Void_Blocks& get_void_blocks()
  {
    if (!void_blocks_initialized)
      init_void_blocks();
    return void_blocks;
  }

  static const int FILE_FORMAT_VERSION = 1007053000;
  const uint32 npos;

private:
  std::string index_file_name;
  std::string empty_index_file_name;
  std::string map_file_name;
  std::string file_name_extension_;

  std::vector< Random_File_Index_Entry > blocks;
  Void_Blocks void_blocks;
  bool void_blocks_initialized;

  uint64 block_size_;
  uint32 compression_factor;
  int compression_method;

  void init_void_blocks();

public:
  uint32 block_count;
};


inline std::vector< bool > get_map_index_footprint
    (const File_Properties& file_prop, std::string db_dir, bool use_shadow);


/** Implementation Random_File_Index: ---------------------------------------*/

static const int VOID_BLOCK_ENTRY_SIZE = 8;


inline Random_File_Index::Random_File_Index
    (const File_Properties& file_prop,
     Access_Mode access_mode, bool use_shadow,
     const std::string& db_dir, const std::string& file_name_extension, int compression_method_) :
    npos(std::numeric_limits< uint32 >::max()),
    index_file_name(db_dir + file_prop.get_file_name_trunk()
        + file_name_extension + file_prop.get_id_suffix()
        + file_prop.get_index_suffix()
	+ (use_shadow ? file_prop.get_shadow_suffix() : "")),
    empty_index_file_name(access_mode == Access_Mode::writeable || access_mode == Access_Mode::truncate
        ? db_dir + file_prop.get_file_name_trunk() + file_prop.get_id_suffix() + file_prop.get_shadow_suffix()
        : ""),
    map_file_name(db_dir + file_prop.get_file_name_trunk()
        + file_name_extension + file_prop.get_id_suffix()),
    file_name_extension_(file_name_extension),
    void_blocks_initialized(false),
    block_size_(file_prop.get_map_block_size()),
    compression_factor(file_prop.get_map_compression_factor()),
    compression_method(compression_method_ == File_Blocks_Index_Base::USE_DEFAULT ?
        file_prop.get_map_compression_method() : compression_method_),
    block_count(0)
{
  uint64 file_size = 0;
  try
  {
    if (access_mode == Access_Mode::truncate)
      Raw_File(map_file_name, O_WRONLY|O_TRUNC, S_666, "Random_File:20");
    else
    {
      Raw_File val_file(map_file_name, O_RDONLY, S_666, "Random_File:8");
      file_size = val_file.size("Random_File:9");
    }
  }
  catch (File_Error e)
  {
    if (e.error_number != ENOENT)
      throw;
  }

  try
  {
    if (access_mode == Access_Mode::truncate)
      Raw_File(index_file_name, O_WRONLY|O_TRUNC, S_666, "Random_File:19");
    else
    {
      Raw_File source_file(
          index_file_name,
          access_mode == Access_Mode::writeable || access_mode == Access_Mode::truncate ? O_RDONLY|O_CREAT : O_RDONLY,
          S_666, "Random_File:6");

      // read index file
      uint32 index_size = source_file.size("Random_File:10");
      Void_Pointer< uint8 > index_buf(index_size);
      source_file.read(index_buf.ptr, index_size, "Random_File:14");

      bool read_old_format = (file_name_extension == ".legacy" ||
        (index_size > 0 && *(int32*)index_buf.ptr < 7512));
        // We support this way the old format although it has no version marker.

      if (!read_old_format && index_size > 0)
      {
        uint8 block_exp = *(uint8*)(index_buf.ptr + 4);
        uint8 compression_exp = *(uint8*)(index_buf.ptr + 5);
        uint16 guessed_compression_method = *(uint16*)(index_buf.ptr + 6);
        uint32 guessed_compression_factor = 1u<<compression_exp;

        if (block_exp < 32 && compression_exp < 32 && guessed_compression_method < 3)
        {
          block_count = file_size / (1ull<<block_exp);

          uint32 pos = 8;
          while (pos < index_size)
          {
            Random_File_Index_Entry entry(*(uint32*)(index_buf.ptr + pos),
                *(uint32*)(index_buf.ptr + pos + 4));

            blocks.push_back(entry);

            if (entry.size > guessed_compression_factor * 2) // increased buffer size for lz4
            {
              read_old_format = true;
              break;
            }

            if (entry.pos != npos && entry.pos >= block_count)
            {
              read_old_format = true;
              break;
            }

            pos += 8;
          }

          if (read_old_format)
            blocks.clear();
          else
          {
            block_size_ = 1ull<<block_exp;
            compression_factor = guessed_compression_factor;
            compression_method = guessed_compression_method;
          }
        }
        else
          read_old_format = true;
      }

      if (read_old_format)
      {
        block_count = file_size/block_size_;

        uint32 pos = 0;
        while (pos < index_size)
        {
          Random_File_Index_Entry entry(*(uint32*)(index_buf.ptr + pos),
              compression_factor); //block size is always 1 in the legacy format
          if (entry.pos != npos)
            entry.pos *= compression_factor;
          blocks.push_back(entry);

          if (entry.pos != npos && entry.pos >= block_count)
            throw File_Error(0, index_file_name, "Random_File: bad pos in index file");
          pos += 4;
        }
      }
    }
  }
  catch (File_Error e)
  {
    if (e.error_number != ENOENT)
      throw;
  }

  if (empty_index_file_name != "")
    init_void_blocks();
}


inline void Random_File_Index::init_void_blocks()
{
  void_blocks.set_size(block_count);

  bool empty_index_file_used = false;
  if (empty_index_file_name != "")
  {
    try
    {
      Raw_File void_blocks_file(empty_index_file_name, O_RDONLY, S_666, "");
      uint32 void_index_size = void_blocks_file.size("Random_File:11");
      Void_Pointer< uint8 > index_buf(void_index_size);
      void_blocks_file.read(index_buf.ptr, void_index_size, "Random_File:15");
      void_blocks.read_dump(index_buf.ptr, void_index_size/(sizeof(uint32_t)*2));
      empty_index_file_used = true;
    }
    catch (File_Error e) {
      empty_index_file_used = false;
    }
  }

  if (!empty_index_file_used)
  {
    for (const auto& i : blocks)
    {
      if (i.pos != npos)
        void_blocks.set_reserved(i.pos, i.size);
    }
  }

  void_blocks_initialized = true;
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
  *(uint8*)(index_buf.ptr + 5) = shift_log(compression_factor);
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
  try
  {
    uint32_t buf_size = void_blocks.num_distinct_blocks() * 8;
    Void_Pointer< uint8 > void_index_buf(buf_size);
    void_blocks.dump_distinct_blocks(void_index_buf.ptr);

    Raw_File void_file(empty_index_file_name, O_RDWR|O_TRUNC, S_666, "Random_File:5");
    void_file.write(void_index_buf.ptr, buf_size, "Random_File:18");
  }
  catch (...) {}
}


/** Implementation non-members: ---------------------------------------------*/


inline std::vector< bool > get_map_index_footprint
    (const File_Properties& file_prop, std::string db_dir, bool use_shadow = false)
{
  Random_File_Index index(file_prop, Access_Mode::readonly, use_shadow, db_dir, "");  
  return index.get_void_blocks().get_map_index_footprint();
}

#endif
