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

#ifndef DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_H
#define DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_H

#include "file_blocks.h"
#include "ranges.h"
#include "types.h"

#include <cstring>
#include <map>
#include <set>


template< typename Object >
struct Idx_Handle
{
  Idx_Handle()
      : obj(0), ptr_to_raw(0) {}
  ~Idx_Handle()
  {
    delete obj;
  }
  void set_ptr(uint8* ptr)
  {
    delete obj;
    obj = 0;
    ptr_to_raw = ptr;
  }
  const Object& object() const
  {
    if (!obj)
      obj = new Object(ptr_to_raw);
    return *obj;
  }
  uint8* get_ptr_to_raw() const
  {
    return ptr_to_raw;
  }

  Idx_Handle(const Idx_Handle& rhs)
      : obj(0), ptr_to_raw(rhs.ptr_to_raw) {}
  Idx_Handle& operator=(const Idx_Handle& rhs)
  {
    set_ptr(rhs.ptr_to_raw);
  }

private:
  mutable Object* obj;
  uint8* ptr_to_raw;
};


template< typename Object >
struct Handle : Idx_Handle< Object >
{
  typename Object::Id_Type id() const
  {
    return Object::get_id(this->get_ptr_to_raw());
  }
};


//-----------------------------------------------------------------------------


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
struct Block_Backend_Basic_Iterator
{
  Block_Backend_Basic_Iterator(
      uint32 block_size, const File_Handle& file_handle, const Idx_Assessor& idx_assessor);
  Block_Backend_Basic_Iterator(const Block_Backend_Basic_Iterator& rhs);
  const Block_Backend_Basic_Iterator& operator=(const Block_Backend_Basic_Iterator& rhs);

  Block_Backend_Basic_Iterator& operator++();

  const Index& index()
  {
    return idx_cache.object();
  }
  const Object& object()
  {
    return obj_cache.object();
  }
  const Handle< Object >& handle()
  { 
    return obj_cache;
  }

  bool is_end() const
  {
    return obj_offset == 0 || idx_block_offset == 0;
  }

  bool operator==(const Block_Backend_Basic_Iterator& rhs) const
  {
    return obj_offset == rhs.obj_offset && file_handle == rhs.file_handle;
  }

private:
  uint32 block_size;
  Void64_Pointer< uint64 > buffer;
  uint32 buffer_size;
  uint32 idx_block_offset; // Points to the entry that contains the jump offset
  uint32 obj_offset;
  Idx_Handle< Index > idx_cache;
  Handle< Object > obj_cache;
  File_Handle file_handle;
  Idx_Assessor idx_assessor;

  uint32 next_idx_block_offset() const
  {
    return *(uint32*)(((uint8*)buffer.ptr) + idx_block_offset);
  }
  uint8* idx_ptr() const
  {
    return ((uint8*)buffer.ptr) + idx_block_offset + 4;
  }
  uint32 total_payload_size() const
  {
    return *(uint32*)buffer.ptr;
  }

  void increment_idx();
  void increment_block();
};


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >::
    Block_Backend_Basic_Iterator(
        uint32 block_size_, const File_Handle& file_handle_, const Idx_Assessor& idx_assessor_)
    : block_size(block_size_), buffer(block_size_), buffer_size(block_size_),
    idx_block_offset(0), obj_offset(0), file_handle(file_handle_), idx_assessor(idx_assessor_)
{
  increment_block();

  if (idx_block_offset == 0)
    obj_offset = 0;
  else if (idx_assessor.is_relevant(idx_ptr()))
    obj_offset = idx_block_offset + 4 + Index::size_of(idx_ptr());
  else
    obj_offset = next_idx_block_offset();

  while (obj_offset > 0 && obj_offset >= next_idx_block_offset())
    increment_idx();

  idx_cache.set_ptr(idx_ptr());
  obj_cache.set_ptr(((uint8*)buffer.ptr) + obj_offset);
}


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >::
    Block_Backend_Basic_Iterator(const Block_Backend_Basic_Iterator& rhs)
    : block_size(rhs.block_size), buffer(rhs.buffer_size), buffer_size(rhs.buffer_size),
    idx_block_offset(rhs.idx_block_offset), obj_offset(rhs.obj_offset),
    file_handle(rhs.file_handle), idx_assessor(rhs.idx_assessor)
{
  memcpy(buffer.ptr, rhs.buffer.ptr, buffer_size);
  idx_cache.set_ptr(((uint8*)buffer.ptr) + idx_block_offset + 4);
  obj_cache.set_ptr(((uint8*)buffer.ptr) + obj_offset);
}


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
const Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >&
    Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >::
    operator=(const Block_Backend_Basic_Iterator& rhs)
{
  if (buffer_size != rhs.buffer_size)
  {
    Void64_Pointer< uint64 > new_buffer(rhs.buffer_size);
    buffer.swap(new_buffer);
  }

  block_size = rhs.block_size;
  buffer_size = rhs.buffer_size;
  idx_block_offset = rhs.idx_block_offset;
  obj_offset = rhs.obj_offset;
  file_handle = rhs.file_handle;
  idx_assessor = rhs.idx_assessor;

  memcpy(buffer.ptr, rhs.buffer.ptr, buffer_size);
  idx_cache.set_ptr(((uint8*)buffer.ptr) + idx_block_offset + 4);
  obj_cache.set_ptr(((uint8*)buffer.ptr) + obj_offset);

  return *this;
}


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >&
    Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >::operator++()
{
  obj_offset += Object::size_of(((uint8*)buffer.ptr) + obj_offset);
  while (obj_offset > 0 && obj_offset >= next_idx_block_offset())
    increment_idx();
  obj_cache.set_ptr(((uint8*)buffer.ptr) + obj_offset);
  return *this;
}


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
void Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >::increment_idx()
{
  do
  {
    idx_block_offset = next_idx_block_offset();
    if (idx_block_offset >= total_payload_size())
      increment_block();
  }
  while (idx_block_offset > 0 && !idx_assessor.is_relevant(idx_ptr()));

  if (idx_block_offset == 0)
    obj_offset = 0;
  else
    obj_offset = idx_block_offset + 4 + Index::size_of(idx_ptr());

  idx_cache.set_ptr(idx_ptr());
}


template< typename Index, typename Object, typename Idx_Assessor, typename File_Handle >
void Block_Backend_Basic_Iterator< Index, Object, Idx_Assessor, File_Handle >::increment_block()
{
  if (!file_handle.next(buffer.ptr))
  {
    idx_block_offset = 0;
    return;
  }
  idx_block_offset = 4;

  if (next_idx_block_offset() > block_size)
  {
    uint32 new_buffer_size = (next_idx_block_offset()/block_size + 1) * block_size;
    if (buffer_size < new_buffer_size)
    {
      Void64_Pointer< uint64 > new_buffer(new_buffer_size);
      memcpy(new_buffer.ptr, buffer.ptr, block_size);
      buffer.swap(new_buffer);
      buffer_size = new_buffer_size;
    }
    for (uint i_offset = block_size; i_offset < new_buffer_size; i_offset += block_size)
      file_handle.next(buffer.ptr + i_offset/8, false);
  }
}


//-----------------------------------------------------------------------------


struct Flat_Idx_Assessor
{
  bool is_relevant(uint8*)
  {
    return true;
  }
};


template< typename File_Blocks, typename File_Iterator >
struct Flat_File_Handle
{
  Flat_File_Handle(File_Blocks& file_blocks_, bool is_end)
      : file_blocks(&file_blocks_), file_it(is_end ? file_blocks_.flat_end() : file_blocks_.flat_begin()),
      file_end(file_blocks_.flat_end()) {}

  bool next(uint64* ptr, bool check_idx = true)
  {
    if (file_it == file_end)
      return false;
    file_blocks->read_block(file_it, ptr, check_idx);
    ++file_it;
    return true;
  }

  bool operator==(const Flat_File_Handle& rhs) const
  {
    return file_it == rhs.file_it;
  }

private:
  const File_Blocks* file_blocks;
  File_Iterator file_it;
  File_Iterator file_end;
};


template< typename Index, typename Object, typename Iterator >
struct Block_Backend_Flat_Iterator
    : Block_Backend_Basic_Iterator< Index, Object, Flat_Idx_Assessor,
        Flat_File_Handle< File_Blocks< Index, Iterator >, typename File_Blocks< Index, Iterator >::Flat_Iterator > >
{
  typedef File_Blocks< Index, Iterator > File_Blocks_;
  typedef Flat_File_Handle< File_Blocks_, typename File_Blocks_::Flat_Iterator > File_Handle_;

  Block_Backend_Flat_Iterator(File_Blocks_& file_blocks, uint32 block_size, bool is_end = false)
      : Block_Backend_Basic_Iterator< Index, Object, Flat_Idx_Assessor, File_Handle_ >(
          block_size, File_Handle_(file_blocks, is_end), Flat_Idx_Assessor()) {}

  Block_Backend_Flat_Iterator(const Block_Backend_Flat_Iterator& rhs)
      : Block_Backend_Basic_Iterator< Index, Object, Flat_Idx_Assessor, File_Handle_ >(rhs) {}
};


//-----------------------------------------------------------------------------


template< typename Index, typename Iterator >
struct Discrete_Idx_Assessor
{
  Discrete_Idx_Assessor(const Iterator& index_it_, const Iterator& index_end_)
      : index_it(index_it_), index_end(index_end_) {}

  bool is_relevant(uint8* ptr)
  {
    Index idx((void*)ptr);
    while (index_it != index_end && *index_it < idx)
      ++index_it;
    return index_it != index_end && *index_it == idx;
  }

private:
  Iterator index_it;
  Iterator index_end;
};


template< typename File_Blocks, typename File_Iterator >
struct Discrete_File_Handle
{
  Discrete_File_Handle(File_Blocks& file_blocks_, const File_Iterator& file_it_)
      : file_blocks(&file_blocks_), file_it(file_it_), file_end(file_blocks_.discrete_end()) {}

  bool next(uint64* ptr, bool check_idx = true)
  {
    if (file_it == file_end)
      return false;
    file_blocks->read_block(file_it, ptr, check_idx);
    ++file_it;
    return true;
  }

  bool operator==(const Discrete_File_Handle& rhs) const
  {
    return file_it == rhs.file_it;
  }

private:
  const File_Blocks* file_blocks;
  File_Iterator file_it;
  File_Iterator file_end;
};


template< typename Index, typename Object, typename Iterator >
struct Block_Backend_Discrete_Iterator
    : Block_Backend_Basic_Iterator< Index, Object, Discrete_Idx_Assessor< Index, Iterator >,
          Discrete_File_Handle< File_Blocks< Index, Iterator >,
              typename File_Blocks< Index, Iterator >::Discrete_Iterator > >
{
  typedef File_Blocks< Index, Iterator > File_Blocks_;
  typedef Discrete_File_Handle< File_Blocks_, typename File_Blocks_::Discrete_Iterator > File_Handle_;

  Block_Backend_Discrete_Iterator
      (File_Blocks_& file_blocks, const Iterator& index_it, const Iterator& index_end, uint32 block_size)
      : Block_Backend_Basic_Iterator< Index, Object, Discrete_Idx_Assessor< Index, Iterator >, File_Handle_ >(
          block_size, File_Handle_(file_blocks, file_blocks.discrete_begin(index_it, index_end)),
          Discrete_Idx_Assessor< Index, Iterator >(index_it, index_end)) {}

  Block_Backend_Discrete_Iterator(File_Blocks_& file_blocks, uint32 block_size)
      : Block_Backend_Basic_Iterator< Index, Object, Discrete_Idx_Assessor< Index, Iterator >, File_Handle_ >(
          block_size, File_Handle_(file_blocks, file_blocks.discrete_end()),
          Discrete_Idx_Assessor< Index, Iterator >(Iterator(), Iterator())) {}

  Block_Backend_Discrete_Iterator(const Block_Backend_Discrete_Iterator& it)
    : Block_Backend_Basic_Iterator< Index, Object, Discrete_Idx_Assessor< Index, Iterator >, File_Handle_ >(it) {}
};


//-----------------------------------------------------------------------------


template< typename Index, typename Iterator >
struct Range_Idx_Assessor
{
  Range_Idx_Assessor(const Iterator& index_it_, const Iterator& index_end_)
      : index_it(index_it_), index_end(index_end_) {}

  bool is_relevant(uint8* ptr)
  {
    Index idx((void*)ptr);
    while (index_it != index_end && !(idx < index_it.upper_bound()))
      ++index_it;
    return index_it != index_end && !(idx < index_it.lower_bound()) && idx < index_it.upper_bound();
  }

private:
  Iterator index_it;
  Iterator index_end;
};


template< typename File_Blocks, typename Index >
struct Range_File_Handle
{
  Range_File_Handle(File_Blocks& file_blocks_,
      const File_Blocks_Range_Iterator< Index, typename Ranges< Index >::Iterator >& file_it_)
      : file_blocks(&file_blocks_), file_it(file_it_),
      file_end(file_blocks_.template range_end< typename Ranges< Index >::Iterator >()) {}

  bool next(uint64* ptr, bool check_idx = true)
  {
    if (file_it == file_end)
      return false;
    file_blocks->read_block(file_it, ptr, check_idx);
    ++file_it;
    return true;
  }

  bool operator==(const Range_File_Handle& rhs) const
  {
    return file_it == rhs.file_it;
  }

private:
  const File_Blocks* file_blocks;
  File_Blocks_Range_Iterator< Index, typename Ranges< Index >::Iterator > file_it;
  File_Blocks_Range_Iterator< Index, typename Ranges< Index >::Iterator > file_end;
};


template< typename Index, typename Object, typename Iterator >
struct Block_Backend_Range_Iterator
    : Block_Backend_Basic_Iterator< Index, Object, Range_Idx_Assessor< Index, typename Ranges< Index >::Iterator >,
        Range_File_Handle< File_Blocks< Index, Iterator >, Index > >
{
  typedef File_Blocks< Index, Iterator > File_Blocks_;
  typedef Range_File_Handle< File_Blocks_, Index > File_Handle_;

  Block_Backend_Range_Iterator(
      File_Blocks_& file_blocks,
      const typename Ranges< Index >::Iterator& index_it, const typename Ranges< Index >::Iterator& index_end,
      uint32 block_size)
      : Block_Backend_Basic_Iterator< Index, Object,
          Range_Idx_Assessor< Index, typename Ranges< Index >::Iterator >, File_Handle_ >(
          block_size, File_Handle_(file_blocks, file_blocks.range_begin(index_it, index_end)),
          Range_Idx_Assessor< Index, typename Ranges< Index >::Iterator >(index_it, index_end)) {}

  Block_Backend_Range_Iterator(File_Blocks_& file_blocks, uint32 block_size)
      : Block_Backend_Basic_Iterator< Index, Object,
          Range_Idx_Assessor< Index, typename Ranges< Index >::Iterator >, File_Handle_ >(
              block_size, File_Handle_(file_blocks, file_blocks.template range_end< typename Ranges< Index >::Iterator >()),
          Range_Idx_Assessor< Index, typename Ranges< Index >::Iterator >(
              typename Ranges< Index >::Iterator(), typename Ranges< Index >::Iterator())) {}

  Block_Backend_Range_Iterator(const Block_Backend_Range_Iterator& it)
    : Block_Backend_Basic_Iterator< Index, Object,
        Range_Idx_Assessor< Index, typename Ranges< Index >::Iterator >, File_Handle_ >(it) {}
};


//-----------------------------------------------------------------------------


template< class TIndex, class TObject, class TIterator = typename std::set< TIndex >::const_iterator >
struct Block_Backend
{
  typedef Block_Backend_Flat_Iterator< TIndex, TObject, TIterator > Flat_Iterator;
  typedef Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator > Discrete_Iterator;
  typedef Block_Backend_Range_Iterator< TIndex, TObject, TIterator > Range_Iterator;

  typedef File_Blocks< TIndex, TIterator > File_Blocks_;

  Block_Backend(File_Blocks_Index_Base* index_);
  ~Block_Backend();

  Flat_Iterator flat_begin() { return Flat_Iterator(file_blocks, block_size, false); }
  const Flat_Iterator& flat_end() const { return *flat_end_it; }

  Discrete_Iterator discrete_begin(TIterator begin, TIterator end)
      { return Discrete_Iterator(file_blocks, begin, end, block_size); }
  const Discrete_Iterator& discrete_end() const { return *discrete_end_it; }

  Range_Iterator range_begin(const Ranges< TIndex >& arg)
  { return Range_Iterator(file_blocks, arg.begin(), arg.end(), block_size); }

  const Range_Iterator& range_end() const { return *range_end_it; }

  void update(
      const std::map< TIndex, std::set< TObject > >& to_delete,
      const std::map< TIndex, std::set< TObject > >&   to_insert,
      std::map< TIndex, uint64 >* obj_count = nullptr);

  uint read_count() const { return file_blocks.read_count(); }
  void reset_read_count() const { file_blocks.reset_read_count(); }

private:
  File_Blocks_ file_blocks;
  Flat_Iterator* flat_end_it;
  Discrete_Iterator* discrete_end_it;
  Range_Iterator* range_end_it;
  uint32 block_size;
  std::string data_filename;
};


template< class TIndex, class TObject, class TIterator >
Block_Backend< TIndex, TObject, TIterator >::Block_Backend(File_Blocks_Index_Base* index_)
  : file_blocks(index_),
    block_size(index_->get_block_size() * index_->get_compression_factor()),
    data_filename(index_->get_data_file_name())
{
  flat_end_it = new Flat_Iterator(file_blocks, block_size, true);
  discrete_end_it = new Discrete_Iterator(file_blocks, block_size);
  range_end_it = new Range_Iterator(file_blocks, block_size);
}


template< class TIndex, class TObject, class TIterator >
Block_Backend< TIndex, TObject, TIterator >::~Block_Backend()
{
  delete flat_end_it;
  delete discrete_end_it;
  delete range_end_it;
}


#endif
