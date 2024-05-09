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

#ifndef DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_WRITE_H
#define DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_WRITE_H

#include "block_backend.h"

#include <type_traits>


template< typename Index, typename Object, typename File_Blocks, typename Iterator >
void flush_if_necessary_and_write_obj(
    uint64* start_ptr, uint8*& insert_ptr, File_Blocks& file_blocks, Iterator& file_it,
    const Index& idx, const Object& obj, uint32 block_size, const std::string& data_filename)
{
  uint32 idx_size = idx.size_of();
  uint32 obj_size = obj.size_of();

  if (insert_ptr - (uint8*)start_ptr + obj_size > block_size)
  {
    uint bytes_written = insert_ptr - (uint8*)start_ptr;
    if (bytes_written > 8 + idx_size)
    {
      *(uint32*)start_ptr = bytes_written;
      *(((uint32*)start_ptr)+1) = bytes_written;
      file_it = file_blocks.insert_block(file_it, start_ptr);
    }
    if (idx_size + obj_size + 8 > block_size)
    {
      if (obj_size > 64*1024*1024)
          throw File_Error(0, data_filename, "Block_Backend: an item's size exceeds limit of 64 MiB.");

      uint buf_scale = (idx_size + obj_size + 7)/block_size + 1;
      Void64_Pointer< uint64 > large_buf(buf_scale * block_size);
      *(uint32*)large_buf.ptr = block_size;
      *(((uint32*)large_buf.ptr)+1) = idx_size + obj_size + 8;
      memcpy(large_buf.ptr+1, start_ptr+1, idx_size);
      obj.to_data(((uint8*)large_buf.ptr) + 8 + idx_size);

      for (uint i = 0; i+1 < buf_scale; ++i)
      {
        file_it = file_blocks.insert_block(file_it, large_buf.ptr + i*block_size/8, block_size, idx);
      }
      file_it = file_blocks.insert_block(
          file_it, large_buf.ptr + (buf_scale-1)*block_size/8, idx_size + obj_size + 8 - block_size*(buf_scale-1),
          idx);

      insert_ptr = ((uint8*)start_ptr) + 8 + idx_size;
      return;
    }
    else
      insert_ptr = ((uint8*)start_ptr) + 8 + idx_size;
  }

  obj.to_data(insert_ptr);
  insert_ptr = insert_ptr + obj_size;
}


template< typename Index, typename File_Blocks >
struct File_Handler
{
  File_Handler(
      File_Blocks& file_blocks_, const std::vector< Index >& relevant_idxs,
      uint32 block_size_, const std::string& data_filename_)
      : file_blocks(file_blocks_),
        file_it(file_blocks.write_begin(relevant_idxs.begin(), relevant_idxs.end(), true)),
        block_size(block_size_), data_filename(data_filename_) {}

  void insert_block(uint64* ptr)
  { file_it = file_blocks.insert_block(file_it, ptr); }
  void replace_block(uint64* ptr)
  {
    file_it = file_blocks.replace_block(file_it, ptr);
    ++file_it;
  }
  void erase_block()
  { file_it = file_blocks.erase_block(file_it); }

  File_Blocks& file_blocks;
  typename File_Blocks::Write_Iterator file_it;
  uint32 block_size;
  std::string data_filename;
};


template< typename Container >
uint32 total_size_of(const Container& container)
{
  uint32 total_size = 0;
  for (const auto& i : container)
    total_size += i.size_of();
  return total_size;
}


struct Offset_Size
{
  uint32 offset;
  uint32 size;
};


template< typename Cont_Entry >
struct Idx_Block_To_Write
{
  Idx_Block_To_Write(std::vector< Offset_Size >&& existing_, uint32 existing_size)
      : existing(existing_), to_insert(nullptr), size(4 + existing_size) {}
  Idx_Block_To_Write(std::vector< Offset_Size >&& existing_, uint32 existing_size, const Cont_Entry& to_insert_)
      : existing(existing_), to_insert(&to_insert_), size(4 + existing_size + total_size_of(to_insert_.second)) {}
  Idx_Block_To_Write(const Cont_Entry& to_insert_)
      : to_insert(&to_insert_), size(4 + to_insert_.first.size_of() + total_size_of(to_insert_.second)) {}

  std::vector< Offset_Size > existing;
  Cont_Entry* to_insert;
  uint32 size;
};


template< typename Cont_Entry, typename File_Handler >
void build_dest_block(
    const std::vector< Idx_Block_To_Write< Cont_Entry > >& to_write, uint32 from, uint32 until,
    const uint8* source, uint8* dest, const File_Handler& handler)
{
  uint32 total_size = 4;
  for (uint32 i = from; i < until; ++i)
    total_size += to_write[i].size;
  *(uint32*)dest = total_size;
  dest += 4;

  if (total_size > handler.block_size)
    throw File_Error(0, handler.data_filename, "build_dest_block: total_size bigger than block_size");

  total_size = 4;
  for (uint32 i = from; i < until; ++i)
  {
    total_size += to_write[i].size;
    *(uint32*)dest = total_size;
    dest += 4;
    if (source && !to_write[i].existing.empty())
    {
      // if not empty then the first entry is the index, thus catered for
      for (const auto& j : to_write[i].existing)
      {
        memcpy(dest, source + j.offset, j.size);
        dest += j.size;
      }
    }
    else if (to_write[i].to_insert)
    {
      to_write[i].to_insert->first.to_data(dest);
      dest += to_write[i].to_insert->first.size_of();
    }

    if (to_write[i].to_insert)
    {
      for (const auto& j : to_write[i].to_insert->second)
      {
        j.to_data(dest);
        dest += j.size_of();
      }
    }
  }
}


// If the elements fit within one block then compile and return that block.
// If the elements do not fit within one block then compile and flush a block and return the remainder.
template< typename Cont_Entry, typename File_Handler >
void force_flush_group(
    const std::vector< Idx_Block_To_Write< Cont_Entry > >& to_write, uint32 from, uint32 until,
    const uint8* source, uint8* dest, uint32 total_size,
    File_Handler& handler)
{
  if (total_size < handler.block_size - 4)
    build_dest_block(to_write, from, until, source, dest, handler);
  else
  {
    while (total_size >= (handler.block_size - 4)*2)
    {
      uint32 split = from;
      uint32 partial_size = 0;
      while (split < until && partial_size <= (handler.block_size - 4)*2/3)
        partial_size += to_write[split++].size;
      if (partial_size > handler.block_size - 4)
        partial_size -= to_write[--split].size;

      build_dest_block(to_write, from, split, source, dest, handler);
      handler.insert_block((uint64*)dest);
      from = split;
      total_size -= partial_size;
    }

    uint32 split = from;
    uint32 partial_size = 0;
    while (split < until && partial_size*2 <= total_size)
      partial_size += to_write[split++].size;

    bool two_blocks = false;
    if (2*partial_size < total_size + to_write[split-1].size)
    {
      if (partial_size <= handler.block_size - 4)
        two_blocks = true;
    }
    else
    {
      if (total_size - partial_size + to_write[split-1].size <= handler.block_size - 4)
      {
        --split;
        partial_size -= to_write[split].size;
        two_blocks = true;
      }
    }

    if (two_blocks)
    {
      build_dest_block(to_write, from, split, source, dest, handler);
      handler.insert_block((uint64*)dest);
      build_dest_block(to_write, split, until, source, dest, handler);
    }
    else
    {
      uint32 partial_size_l = partial_size - to_write[split-1].size;
      uint32 split_l = split-1;
      while (from+1 < split_l && partial_size_l - to_write[split_l-1].size > total_size*2/3)
        partial_size_l -= to_write[--split_l].size;

      uint32 partial_size_r = total_size - partial_size;
      uint32 split_r = split;
      while (split_r+1 < until && partial_size_r - to_write[split_r].size > total_size*2/3)
        partial_size_r -= to_write[split_r++].size;

      build_dest_block(to_write, from, split_l, source, dest, handler);
      handler.insert_block((uint64*)dest);
      build_dest_block(to_write, split_l, split_r, source, dest, handler);
      handler.insert_block((uint64*)dest);
      build_dest_block(to_write, split_r, until, source, dest, handler);
    }
  }
}


// If the elements fit within one block then compile and return that block.
// If the elements do not fit within one block then compile and flush a block and return the remainder.
template< typename Cont_Entry, typename File_Handler >
void flush_segment(
    const Idx_Block_To_Write< Cont_Entry >& to_write, const uint8* source, uint8* dest, File_Handler& handler)
{
  uint8* dest_pos = dest + 8;

  if (source && !to_write.existing.empty())
  {
    for (const auto& j : to_write.existing)
    {
      memcpy(dest_pos, source + j.offset, j.size);
      dest_pos += j.size;
    }
  }
  else if (to_write.to_insert)
  {
    to_write.to_insert->first.to_data(dest_pos);
    dest_pos += to_write.to_insert->first.size_of();
  }

  bool fit_found = false;
  bool oversized_found = false;
  if (to_write.to_insert)
  {
    auto idx_size = to_write.to_insert->first.size_of();

    auto it = to_write.to_insert->second.begin();
    while (it != to_write.to_insert->second.end())
    {
      auto obj_size = it->size_of();
      if (obj_size + idx_size + 8 < handler.block_size)
      {
        if ((uint32)(dest_pos - dest) + obj_size > handler.block_size)
        {
          *(uint32*)dest = (uint32)(dest_pos - dest);
          *(uint32*)(dest + 4) = (uint32)(dest_pos - dest);
          handler.insert_block((uint64*)dest);

          dest_pos = dest + idx_size + 8;
        }

        it->to_data((void*)dest_pos);
        dest_pos += obj_size;
        fit_found = true;
      }
      else
        oversized_found = true;
      ++it;
    }
  }

  if (!to_write.existing.empty() || fit_found)
  {
    *(uint32*)dest = (uint32)(dest_pos - dest);
    *(uint32*)(dest + 4) = (uint32)(dest_pos - dest);
    handler.insert_block((uint64*)dest);
  }

  if (oversized_found)
  {
    auto idx_size = to_write.to_insert->first.size_of();

    for (auto it = to_write.to_insert->second.begin(); it != to_write.to_insert->second.end(); ++it)
    {
      auto obj_size = it->size_of();
      if (obj_size + idx_size + 8 >= handler.block_size)
      {
        if (obj_size > 64*1024*1024)
            throw File_Error(0, handler.data_filename, "Block_Backend: an item's size exceeds limit of 64 MiB.");

        uint buf_scale = (idx_size + obj_size + 7)/handler.block_size + 1;
        Void64_Pointer< uint64 > large_buf(buf_scale * handler.block_size);
        *(uint32*)large_buf.ptr = handler.block_size;
        *(((uint32*)large_buf.ptr)+1) = idx_size + obj_size + 8;
        memcpy(large_buf.ptr+1, dest+8, idx_size);
        it->to_data(((uint8*)large_buf.ptr) + 8 + idx_size);

        for (uint i = 0; i+1 < buf_scale; ++i)
          handler.file_it = handler.file_blocks.insert_block(
              handler.file_it,
              large_buf.ptr + i*handler.block_size/8, handler.block_size, to_write.to_insert->first);

        handler.file_it = handler.file_blocks.insert_block(
            handler.file_it,
            large_buf.ptr + (buf_scale-1)*handler.block_size/8, idx_size + obj_size + 8 - handler.block_size*(buf_scale-1),
            to_write.to_insert->first);
      }
    }
  }
}


template< typename Object >
class Object_Set_Predicate
{
public:
  Object_Set_Predicate(const std::set< Object >& data_) : data(&data_) {}

  bool match(const Object& obj) const
  { return data->find(obj) != data->end(); }

  typedef Object Base_Object;

private:
  const std::set< Object >* data;
};


template< typename Index, typename Container, typename File_Blocks >
void create_from_scratch(
    File_Handler< Index, File_Blocks >& file_handler,
    const std::map< Index, Container >& to_insert,
    const std::vector< Index >& relevant_idxs,
    std::map< Index, Delta_Count >* obj_count)
{
  Void_Pointer< uint8 > dest(file_handler.block_size);

  std::vector< Idx_Block_To_Write< typename std::remove_reference< decltype(*to_insert.begin()) >::type > > to_write;

  auto to_insert_begin = to_insert.lower_bound(*(file_handler.file_it.lower_bound()));
  auto to_insert_end = to_insert.end();
  if (file_handler.file_it.upper_bound() != relevant_idxs.end())
    to_insert_end = to_insert.lower_bound(*(file_handler.file_it.upper_bound()));

  uint32 total_size = 0;
  uint32 cur_from = 0;
  for (auto it = to_insert_begin; it != to_insert_end; ++it)
  {
    if (!it->second.empty())
    {
      to_write.push_back({ *it });

      if (to_write.back().size >= file_handler.block_size - 4)
      {
        if (cur_from + 1 < to_write.size())
        {
          force_flush_group(
              to_write, cur_from, to_write.size()-1, nullptr, dest.ptr, total_size, file_handler);
          file_handler.insert_block((uint64*)dest.ptr);
        }

        flush_segment(to_write.back(), nullptr, dest.ptr, file_handler);

        to_write.clear();
        cur_from = 0;
        total_size = 0;
      }
      else if (to_write.size() - cur_from > 1
          && to_write.back().size + to_write[to_write.size()-2].size > file_handler.block_size - 4)
      {
        force_flush_group(
            to_write, cur_from, to_write.size()-1, nullptr, dest.ptr, total_size, file_handler);
        file_handler.insert_block((uint64*)dest.ptr);

        to_write.erase(to_write.begin(), to_write.end()-1);
        cur_from = 0;
        total_size = to_write.back().size;
      }
      else
        total_size += to_write.back().size;
    }

    while (total_size >= (file_handler.block_size - 4)*2)
    {
      uint32 j = cur_from;
      uint32 partial_size = 0;
      while (j < to_write.size() && partial_size <= (file_handler.block_size - 4)*2/3)
        partial_size += to_write[j++].size;
      if (partial_size > file_handler.block_size - 4)
        partial_size -= to_write[--j].size;

      build_dest_block(to_write, cur_from, j, nullptr, dest.ptr, file_handler);
      file_handler.insert_block((uint64*)dest.ptr);
      cur_from = j;
      total_size -= partial_size;
    }

    if (file_handler.block_size < cur_from * 8)
    {
      to_write.erase(to_write.begin(), to_write.begin() + cur_from);
      cur_from = 0;
    }
  }

  if (total_size > 0)
  {
    force_flush_group(
        to_write, cur_from, to_write.size(), nullptr, dest.ptr, total_size, file_handler);
    file_handler.insert_block((uint64*)dest.ptr);
  }
  ++file_handler.file_it;
}


template< typename Index, typename Object_Predicate >
struct Existing_Idx_Info
{
  Existing_Idx_Info(const uint8* src_start, const uint8* pos, const Object_Predicate* pred)
      : size(0)
  {
    const uint8* src_end = src_start + *(uint32*)pos;
    pos += 4;
    if (src_end > pos)
    {
      uint32 idx_size = Index::size_of((void*)pos);
      existing.push_back({ (uint32)(pos - src_start), idx_size });
      pos += idx_size;
      size += idx_size;

      while (pos < src_end)
      {
        uint32 obj_size = Object_Predicate::Base_Object::size_of((void*)pos);
        if (!pred || !pred->match(typename Object_Predicate::Base_Object((void*)pos)))
        {
          if (existing.empty() || existing.back().offset + existing.back().size < pos - src_start)
            existing.push_back({ (uint32)(pos - src_start), obj_size });
          else
            existing.back().size += obj_size;
          ++count.after;
          size += obj_size;
        }
        pos += obj_size;
        ++count.before;
      }
    }
  }

  std::vector< Offset_Size > existing;
  uint32 size;
  Delta_Count count;
};


template< typename Index, typename Object_Predicate >
class To_Delete_Matcher
{
  typedef typename std::map< Index, Object_Predicate >::const_iterator Iterator;

public:
  To_Delete_Matcher(const std::map< Index, Object_Predicate >& to_delete)
      : it(to_delete.begin()), end(to_delete.end()), last_seen_equal(false) {}

  const Object_Predicate* seek(uint8* data)
  {
    while (it != end && it->first.less(data))
      ++it;
    last_seen_equal = (it != end && it->first.equal(data));
    return last_seen_equal ? &it->second : nullptr;
  }

  bool was_equal() const { return last_seen_equal; }
  Index idx() { return it->first; }

private:
  Iterator it;
  Iterator end;
  bool last_seen_equal;
};


template< typename Index, typename Object_Predicate, typename Container, typename File_Blocks >
void update_group(
    File_Handler< Index, File_Blocks >& file_handler,
    const std::map< Index, Object_Predicate >& to_delete, const std::map< Index, Container >& to_insert,
    const std::vector< Index >& relevant_idxs,
    std::map< Index, Delta_Count >* obj_count)
{
  Void_Pointer< uint8 > source(file_handler.block_size);
  Void_Pointer< uint8 > dest(file_handler.block_size);

  file_handler.file_blocks.read_block(file_handler.file_it, (uint64*)source.ptr);

  std::vector< Idx_Block_To_Write< typename std::remove_reference< decltype(*to_insert.begin()) >::type > > to_write;

  auto to_insert_begin = to_insert.lower_bound(*(file_handler.file_it.lower_bound()));
  auto to_insert_end = to_insert.end();
  if (file_handler.file_it.upper_bound() != relevant_idxs.end())
    to_insert_end = to_insert.lower_bound(*(file_handler.file_it.upper_bound()));

  uint8* src_pos(source.ptr + 4);
  uint8* source_end(source.ptr + *(uint32*)source.ptr);

  To_Delete_Matcher< Index, Object_Predicate > del_match(to_delete);

  uint32 total_size = 0;
  uint32 cur_from = 0;
  for (auto it = to_insert_begin; it != to_insert_end; ++it)
  {
    while (src_pos < source_end && !it->first.leq(src_pos + 4))
    {
      Existing_Idx_Info< Index, Object_Predicate > info(source.ptr, src_pos, del_match.seek(src_pos + 4));
      if (obj_count && del_match.was_equal())
        (*obj_count)[del_match.idx()] = info.count;
      if (info.count.after > 0)
      {
        to_write.push_back({ std::move(info.existing), info.size });
        total_size += to_write.back().size;
      }
      src_pos = source.ptr + *(uint32*)src_pos;
    }

    bool entry_created = false;
    if (src_pos < source_end && it->first.equal(src_pos + 4))
    {
      Existing_Idx_Info< Index, Object_Predicate > info(source.ptr, src_pos, del_match.seek(src_pos + 4));
      info.count.after += it->second.size();
      if (obj_count)
        (*obj_count)[it->first] = info.count;
      if (info.count.after > 0)
      {
        to_write.push_back({ std::move(info.existing), info.size, *it });
        entry_created = true;
      }
      src_pos = source.ptr + *(uint32*)src_pos;
    }
    else if (!it->second.empty())
    {
      to_write.push_back({ *it });
      entry_created = true;
    }

    if (entry_created)
    {
      if (to_write.back().size >= file_handler.block_size - 4)
      {
        if (cur_from + 1 < to_write.size())
        {
          force_flush_group(
              to_write, cur_from, to_write.size()-1, source.ptr, dest.ptr, total_size, file_handler);
          file_handler.insert_block((uint64*)dest.ptr);
        }

        flush_segment(to_write.back(), source.ptr, dest.ptr, file_handler);

        to_write.clear();
        cur_from = 0;
        total_size = 0;
      }
      else if (to_write.size() - cur_from > 1
          && to_write.back().size + to_write[to_write.size()-2].size > file_handler.block_size - 4)
      {
        force_flush_group(
            to_write, cur_from, to_write.size()-1, source.ptr, dest.ptr, total_size, file_handler);
        file_handler.insert_block((uint64*)dest.ptr);

        to_write.erase(to_write.begin(), to_write.end()-1);
        cur_from = 0;
        total_size = to_write.back().size;
      }
      else
        total_size += to_write.back().size;
    }

    while (total_size >= (file_handler.block_size - 4)*2)
    {
      uint32 j = cur_from;
      uint32 partial_size = 0;
      while (j < to_write.size() && partial_size <= (file_handler.block_size - 4)*2/3)
        partial_size += to_write[j++].size;
      if (partial_size > file_handler.block_size - 4)
        partial_size -= to_write[--j].size;

      build_dest_block(to_write, cur_from, j, source.ptr, dest.ptr, file_handler);
      file_handler.insert_block((uint64*)dest.ptr);
      cur_from = j;
      total_size -= partial_size;
    }

    if (file_handler.block_size < cur_from * 8)
    {
      to_write.erase(to_write.begin(), to_write.begin() + cur_from);
      cur_from = 0;
    }
  }
  while (src_pos < source_end)
  {
    Existing_Idx_Info< Index, Object_Predicate > info(source.ptr, src_pos, del_match.seek(src_pos + 4));
    if (obj_count && del_match.was_equal())
      (*obj_count)[del_match.idx()] = info.count;
    if (info.count.after > 0)
    {
      to_write.push_back({ std::move(info.existing), info.size });
      total_size += to_write.back().size;
    }
    src_pos = source.ptr + *(uint32*)src_pos;
  }

  if (total_size > 0)
  {
    force_flush_group(
        to_write, cur_from, to_write.size(), source.ptr, dest.ptr, total_size, file_handler);
    file_handler.replace_block((uint64*)dest.ptr);
  }
  else
    file_handler.erase_block();
}


template< typename File_Handler >
void flush_or_delete_block(
    uint64* start_ptr, uint bytes_written, File_Handler& file_handler, uint32 idx_size)
{
  if (bytes_written > 8 + idx_size)
  {
    *(uint32*)start_ptr = bytes_written;
    *(((uint32*)start_ptr)+1) = bytes_written;
    file_handler.replace_block(start_ptr);
  }
  else
    file_handler.erase_block();
}


template< typename File_Blocks, typename Iterator >
bool read_block_or_blocks(
    File_Blocks& file_blocks, Iterator& file_it, uint32 block_size,
    Void64_Pointer< uint64 >& source, uint32& buffer_size)
{
  file_blocks.read_block(file_it, source.ptr);

  if (*(((uint32*)source.ptr) + 1) > block_size)
  {
    uint32 new_buffer_size = (*(((uint32*)source.ptr) + 1)/block_size + 1) * block_size;
    if (buffer_size < new_buffer_size)
    {
      Void64_Pointer< uint64 > new_source_buffer(new_buffer_size);
      memcpy(new_source_buffer.ptr, source.ptr, block_size);
      source.swap(new_source_buffer);

      buffer_size = new_buffer_size;
    }
    for (uint i_offset = block_size; i_offset < new_buffer_size; i_offset += block_size)
    {
      ++file_it;
      file_blocks.read_block(file_it, source.ptr + i_offset/8, false);
    }

    return true;
  }
  return false;
}


template< typename Object_Predicate >
Delta_Count skip_deleted_objects(
    uint64* source_start_ptr, uint64* dest_start_ptr,
    const Object_Predicate& objs_to_delete, uint32 idx_size)
{
  Delta_Count count;

  uint32 src_obj_offset = 8 + idx_size;
  uint32 dest_obj_offset = src_obj_offset;
  uint32 src_size = *(uint32*)source_start_ptr;
  memcpy(((uint8*)dest_start_ptr) + 8, ((uint8*)source_start_ptr) + 8, idx_size);

  while (src_obj_offset < src_size)
  {
    typename Object_Predicate::Base_Object obj(((uint8*)source_start_ptr) + src_obj_offset);
    uint32 obj_size = obj.size_of();

    if (!objs_to_delete.match(obj))
    {
      memcpy(
          ((uint8*)dest_start_ptr) + dest_obj_offset, ((uint8*)source_start_ptr) + src_obj_offset, obj_size);
      dest_obj_offset += obj_size;
      ++count.after;
    }
    src_obj_offset += obj_size;
    ++count.before;
  }

  *(uint32*)dest_start_ptr = dest_obj_offset;
  *(((uint32*)dest_start_ptr)+1) = dest_obj_offset;
  memcpy(dest_start_ptr + 1, source_start_ptr + 1, idx_size);

  return count;
}


template< typename Iterator >
uint64 append_insertables(
    uint64* dest_start_ptr, uint32 block_size, Iterator& cur_insert, const Iterator& cur_end)
{
  uint32 obj_append_offset = *(uint32*)dest_start_ptr;
  uint64 counter = 0;

  while ((cur_insert != cur_end) && (obj_append_offset + cur_insert->size_of() < block_size))
  {
    cur_insert->to_data(((uint8*)dest_start_ptr) + obj_append_offset);
    obj_append_offset += cur_insert->size_of();
    ++cur_insert;
    ++counter;
  }

  *(uint32*)dest_start_ptr = obj_append_offset;
  *(((uint32*)dest_start_ptr)+1) = obj_append_offset;

  return counter;
}


template< typename Index, typename Del_Container, typename Ins_Container, typename File_Blocks >
void update_segments(
    File_Handler< Index, File_Blocks >& file_handler,
    const std::map< Index, Del_Container >& to_delete, const std::map< Index, Ins_Container >& to_insert,
    std::map< Index, Delta_Count >* obj_count)
{
  file_handler.file_it.start_segments_mode();

  uint32 buffer_size = file_handler.block_size;
  Void64_Pointer< uint64 > source(buffer_size);
  Void64_Pointer< uint64 > dest(buffer_size);

  Index idx = file_handler.file_it.block().index;
  auto delete_it = to_delete.find(idx);
  auto insert_it = to_insert.find(idx);
  uint32 idx_size = idx.size_of();

  Delta_Count count;

  decltype(insert_it->second.begin()) cur_insert;
  if (insert_it != to_insert.end())
    cur_insert = insert_it->second.begin();

  while (!file_handler.file_it.is_end() && file_handler.file_it.block().index == idx)
  {
    auto delta_it = file_handler.file_it;
    bool oversized = read_block_or_blocks(
        file_handler.file_blocks, file_handler.file_it, file_handler.block_size, source, buffer_size);
    if (oversized)
    {
      ++file_handler.file_it;
      typename Del_Container::Base_Object obj(((uint8*)source.ptr) + 8 + idx_size);
      if (delete_it != to_delete.end() && delete_it->second.match(obj))
        file_handler.file_blocks.erase_blocks(delta_it, file_handler.file_it);
      else
        ++count.after;
      ++count.before;
    }
    else
    {
      if (*(uint32*)source.ptr < 8 + idx_size)
      { // something has seriously gone wrong - such a block shuld not exist
        ++file_handler.file_it;
        continue;
      }

      if (delete_it != to_delete.end())
        count += skip_deleted_objects(source.ptr, dest.ptr, delete_it->second, idx_size);
      else
        memcpy(dest.ptr, source.ptr, *(uint32*)source.ptr);

      if (*(uint32*)source.ptr != *(uint32*)dest.ptr
          || (insert_it != to_insert.end() && *(uint32*)source.ptr < file_handler.block_size/2))
      {
        if (insert_it != to_insert.end())
          count.after += append_insertables(dest.ptr, file_handler.block_size, cur_insert, insert_it->second.end());
        flush_or_delete_block(dest.ptr, *(uint32*)dest.ptr, file_handler, idx_size);
      }
      else
        ++file_handler.file_it;
    }
  }

  uint8* pos = ((uint8*)dest.ptr) + idx_size + 8;
  memcpy(dest.ptr+1, source.ptr+1, idx_size);
  if (insert_it != to_insert.end())
  {
    while (cur_insert != insert_it->second.end())
    {
      flush_if_necessary_and_write_obj(
          dest.ptr, pos, file_handler.file_blocks, file_handler.file_it, idx, *cur_insert, file_handler.block_size, file_handler.data_filename);
      ++cur_insert;
      ++count.after;
    }
  }
  if (pos > ((uint8*)dest.ptr) + idx_size + 8)
  {
    *(uint32*)dest.ptr = pos - (uint8*)dest.ptr;
    *(((uint32*)dest.ptr)+1) = pos - (uint8*)dest.ptr;
    file_handler.insert_block(dest.ptr);
  }

  if (obj_count)
    (*obj_count)[idx] += count;
  file_handler.file_it.end_segments_mode();
}


template< typename Index, typename Object, typename Iterator >
template< typename Container >
void Block_Backend< Index, Object, Iterator >::update(
    const std::map< Index, std::set< Object > >& to_delete,
    const std::map< Index, Container >& to_insert,
    std::map< Index, Delta_Count >* obj_count)
{
  std::vector< Index > relevant_idxs;
  auto iit = to_insert.begin();
  for (auto dit = to_delete.begin(); dit != to_delete.end(); ++dit)
  {
    while (iit != to_insert.end() && iit->first < dit->first)
      relevant_idxs.push_back((iit++)->first);
    if (iit != to_insert.end() && iit->first == dit->first)
      ++iit;
    relevant_idxs.push_back(dit->first);
  }
  while (iit != to_insert.end())
    relevant_idxs.push_back((iit++)->first);

  File_Handler< Index, File_Blocks_ > handler(file_blocks, relevant_idxs, block_size, data_filename);

  std::map< Index, Object_Set_Predicate< Object > > to_delete_;
  for (const auto& i : to_delete)
    to_delete_.insert(std::make_pair(i.first, Object_Set_Predicate< Object >(i.second)));

  while (handler.file_it.lower_bound() != relevant_idxs.end())
  {
    if (handler.file_it.block_type() == File_Block_Index_Entry< Index >::EMPTY)
      create_from_scratch(handler, to_insert, relevant_idxs, obj_count);
    else if (handler.file_it.block_type() == File_Block_Index_Entry< Index >::GROUP)
      update_group(handler, to_delete_, to_insert, relevant_idxs, obj_count);
    else //if (file_it.block_type() == File_Block_Index_Entry< Index >::SEGMENT)
      update_segments(handler, to_delete_, to_insert, obj_count);
  }
}


#endif
