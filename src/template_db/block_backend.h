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


template< class Index >
struct Default_Range_Iterator : std::set< std::pair< Index, Index > >::const_iterator
{
  Default_Range_Iterator
      (const typename std::set< std::pair< Index, Index > >::const_iterator it)
  : std::set< std::pair< Index, Index > >::const_iterator(it) {}

  Default_Range_Iterator() {}

  const Index& lower_bound() const { return (*this)->first; }
  const Index& upper_bound() const { return (*this)->second; }
};


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
        Flat_File_Handle< File_Blocks< Index, Iterator, Default_Range_Iterator< Index > >,
            typename File_Blocks< Index, Iterator, Default_Range_Iterator< Index > >::Flat_Iterator > >
{
  typedef File_Blocks< Index, Iterator, Default_Range_Iterator< Index > > File_Blocks_;
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
          Discrete_File_Handle< File_Blocks< Index, Iterator, Default_Range_Iterator< Index > >,
              typename File_Blocks< Index, Iterator, Default_Range_Iterator< Index > >::Discrete_Iterator > >
{
  typedef File_Blocks< Index, Iterator, Default_Range_Iterator< Index > > File_Blocks_;
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


template< typename File_Blocks, typename File_Iterator >
struct Range_File_Handle
{
  Range_File_Handle(File_Blocks& file_blocks_, const File_Iterator& file_it_)
      : file_blocks(&file_blocks_), file_it(file_it_), file_end(file_blocks_.range_end()) {}

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
  File_Iterator file_it;
  File_Iterator file_end;
};


template< typename Index, typename Object, typename Iterator >
struct Block_Backend_Range_Iterator
    : Block_Backend_Basic_Iterator< Index, Object, Range_Idx_Assessor< Index, Default_Range_Iterator< Index > >,
        Range_File_Handle< File_Blocks< Index, Iterator, Default_Range_Iterator< Index > >,
            typename File_Blocks< Index, Iterator, Default_Range_Iterator< Index > >::Range_Iterator > >
{
  typedef File_Blocks< Index, Iterator, Default_Range_Iterator< Index > > File_Blocks_;
  typedef Range_File_Handle< File_Blocks_, typename File_Blocks_::Range_Iterator > File_Handle_;

  Block_Backend_Range_Iterator(
      File_Blocks_& file_blocks,
      const Default_Range_Iterator< Index >& index_it, const Default_Range_Iterator< Index >& index_end,
      uint32 block_size)
      : Block_Backend_Basic_Iterator< Index, Object,
          Range_Idx_Assessor< Index, Default_Range_Iterator< Index > >, File_Handle_ >(
          block_size, File_Handle_(file_blocks, file_blocks.range_begin(index_it, index_end)),
          Range_Idx_Assessor< Index, Default_Range_Iterator< Index > >(index_it, index_end)) {}

  Block_Backend_Range_Iterator(File_Blocks_& file_blocks, uint32 block_size)
      : Block_Backend_Basic_Iterator< Index, Object,
          Range_Idx_Assessor< Index, Default_Range_Iterator< Index > >, File_Handle_ >(
          block_size, File_Handle_(file_blocks, file_blocks.range_end()),
          Range_Idx_Assessor< Index, Default_Range_Iterator< Index > >(
              Default_Range_Iterator< Index >(), Default_Range_Iterator< Index >())) {}

  Block_Backend_Range_Iterator(const Block_Backend_Range_Iterator& it)
    : Block_Backend_Basic_Iterator< Index, Object,
        Range_Idx_Assessor< Index, Default_Range_Iterator< Index > >, File_Handle_ >(it) {}
};


//-----------------------------------------------------------------------------


template< class TIndex, class TObject >
struct Index_Collection
{
  Index_Collection(uint8* source_begin_, uint8* source_end_,
		   const typename std::map< TIndex, std::set< TObject > >::const_iterator& delete_it_,
		   const typename std::map< TIndex, std::set< TObject > >::const_iterator& insert_it_)
      : source_begin(source_begin_), source_end(source_end_),
        delete_it(delete_it_), insert_it(insert_it_) {}

  uint8* source_begin;
  uint8* source_end;
  typename std::map< TIndex, std::set< TObject > >::const_iterator delete_it;
  typename std::map< TIndex, std::set< TObject > >::const_iterator insert_it;
};


template< class TIndex, class TObject >
struct Empty_Update_Logger
{
public:
  void deletion(const TIndex&, const TObject&) {}
};


template< class TIndex, class TObject, class TIterator = typename std::set< TIndex >::const_iterator >
struct Block_Backend
{
    typedef Block_Backend_Flat_Iterator< TIndex, TObject, TIterator > Flat_Iterator;
    typedef Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator > Discrete_Iterator;
    typedef Block_Backend_Range_Iterator< TIndex, TObject, TIterator > Range_Iterator;

    typedef File_Blocks< TIndex, TIterator, Default_Range_Iterator< TIndex > > File_Blocks_;

    Block_Backend(File_Blocks_Index_Base* index_);
    ~Block_Backend();

    Flat_Iterator flat_begin() { return Flat_Iterator(file_blocks, block_size, false); }
    const Flat_Iterator& flat_end() const { return *flat_end_it; }

    Discrete_Iterator discrete_begin(TIterator begin, TIterator end)
        { return Discrete_Iterator(file_blocks, begin, end, block_size); }
    const Discrete_Iterator& discrete_end() const { return *discrete_end_it; }

    Range_Iterator range_begin
        (Default_Range_Iterator< TIndex > begin,
         Default_Range_Iterator< TIndex > end)
        { return Range_Iterator(file_blocks, begin, end, block_size); }
    const Range_Iterator& range_end() const { return *range_end_it; }

    template< class Update_Logger >
    void update
        (const std::map< TIndex, std::set< TObject > >& to_delete,
         const std::map< TIndex, std::set< TObject > >& to_insert,
	 Update_Logger& update_logger);

    void update
        (const std::map< TIndex, std::set< TObject > >& to_delete,
         const std::map< TIndex, std::set< TObject > >& to_insert)
    {
      Empty_Update_Logger< TIndex, TObject> empty_logger;
      update< Empty_Update_Logger< TIndex, TObject> >(to_delete, to_insert, empty_logger);
    }

    uint read_count() const { return file_blocks.read_count(); }
    void reset_read_count() const { file_blocks.reset_read_count(); }

  private:
    File_Blocks_ file_blocks;
    Flat_Iterator* flat_end_it;
    Discrete_Iterator* discrete_end_it;
    Range_Iterator* range_end_it;
    uint32 block_size;
    std::set< TIndex > relevant_idxs;
    std::string data_filename;

    void calc_split_idxs
        (std::vector< TIndex >& split,
         const std::vector< uint32 >& sizes,
         typename std::set< TIndex >::const_iterator it,
         const typename std::set< TIndex >::const_iterator& end);

    void flush_if_necessary_and_write_obj(
        uint64* start_ptr, uint8*& insert_ptr, typename File_Blocks_::Write_Iterator& file_it,
        const TIndex& idx, const TObject& obj);

    void create_from_scratch
        (typename File_Blocks_::Write_Iterator& file_it,
         const std::map< TIndex, std::set< TObject > >& to_insert);

    template< class Update_Logger >
    void update_group
        (typename File_Blocks_::Write_Iterator& file_it,
         const std::map< TIndex, std::set< TObject > >& to_delete,
         const std::map< TIndex, std::set< TObject > >& to_insert,
	 Update_Logger& update_logger);

    template< class Update_Logger >
    void copy_and_delete_on_the_fly(
        uint64* source_start_ptr, uint64* dest_start_ptr,
        typename File_Blocks_::Write_Iterator& file_it, uint32 idx_size,
        const std::map< TIndex, std::set< TObject > >& to_delete,
        typename std::map< TIndex, std::set< TObject > >::const_iterator& delete_it,
        bool& block_modified, uint8*& insert_ptr,
        Update_Logger& update_logger);

    template< class Update_Logger >
    uint32 skip_deleted_objects(
        uint64* source_start_ptr, uint64* dest_start_ptr,
        const std::set< TObject >& objs_to_delete, uint32 idx_size,
        Update_Logger& update_logger, const TIndex& idx);

    bool read_block_or_blocks(
        typename File_Blocks_::Write_Iterator& file_it, Void64_Pointer< uint64 >& source, uint32& buffer_size);

    void flush_or_delete_block(
        uint64* start_ptr, uint8* insert_ptr, typename File_Blocks_::Write_Iterator& file_it,
        uint32 idx_size);

    template< class Update_Logger >
    void update_segments
        (typename File_Blocks_::Write_Iterator& file_it,
         const std::map< TIndex, std::set< TObject > >& to_delete,
         const std::map< TIndex, std::set< TObject > >& to_insert,
	 Update_Logger& update_logger);
};


template< class TIndex, class TObject, class TIterator >
Block_Backend< TIndex, TObject, TIterator >::Block_Backend(File_Blocks_Index_Base* index_)
  : file_blocks(index_),
    block_size(((File_Blocks_Index< TIndex >*)index_)->get_block_size()
        * ((File_Blocks_Index< TIndex >*)index_)->get_compression_factor()),
    data_filename
      (((File_Blocks_Index< TIndex >*)index_)->get_data_file_name())
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

template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
void Block_Backend< TIndex, TObject, TIterator >::update
    (const std::map< TIndex, std::set< TObject > >& to_delete,
     const std::map< TIndex, std::set< TObject > >& to_insert,
     Update_Logger& update_logger)
{
  relevant_idxs.clear();
  for (typename std::map< TIndex, std::set< TObject > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
    relevant_idxs.insert(it->first);
  for (typename std::map< TIndex, std::set< TObject > >::const_iterator
      it(to_insert.begin()); it != to_insert.end(); ++it)
    relevant_idxs.insert(it->first);

  typename File_Blocks_::Write_Iterator file_it
      = file_blocks.write_begin(relevant_idxs.begin(), relevant_idxs.end(), true);

  while (file_it.lower_bound() != relevant_idxs.end())
  {
    if (file_it.block_type() == File_Block_Index_Entry< TIndex >::EMPTY)
      create_from_scratch(file_it, to_insert);
    else if (file_it.block_type() == File_Block_Index_Entry< TIndex >::GROUP)
      update_group(file_it, to_delete, to_insert, update_logger);
    else //if (file_it.block_type() == File_Block_Index_Entry< TIndex >::SEGMENT)
      update_segments(file_it, to_delete, to_insert, update_logger);
  }
}

template< class TIndex, class TObject, class TIterator >
void Block_Backend< TIndex, TObject, TIterator >::calc_split_idxs
    (std::vector< TIndex >& split,
     const std::vector< uint32 >& sizes,
     typename std::set< TIndex >::const_iterator it,
     const typename std::set< TIndex >::const_iterator& end)
{
  std::vector< uint32 > vsplit;
  std::vector< uint64 > min_split_pos;

  // calc total size
  uint64 total_size(0);
  for (uint i(0); i < sizes.size(); ++i)
    total_size += sizes[i];

  // calc minimal splitting points
  uint64 cur_size(0), sum_size(0);
  if (sizes.size() > 0)
    cur_size = sizes[sizes.size() - 1];
  for (int i(sizes.size()-2); i >= 0; --i)
  {
    cur_size += sizes[i];
    if (cur_size <= block_size-4)
      continue;
    sum_size += cur_size - sizes[i];
    min_split_pos.push_back(total_size - sum_size);
    cur_size = sizes[i];
  }

  std::vector< uint64 > oversize_splits;
  // find oversized blocks and force splits there
  sum_size = 0;
  if (sizes.size() > 0)
    sum_size = sizes[0];
  bool split_after((sizes.size() > 0) && (sizes[0] > block_size - 4));
  for (uint i(1); i < sizes.size(); ++i)
  {
    if (sizes[i] > block_size - 4)
    {
      oversize_splits.push_back(sum_size);
      split_after = true;
    }
    else if (split_after)
    {
      oversize_splits.push_back(sum_size);
      split_after = false;
    }
    sum_size += sizes[i];
  }
  oversize_splits.push_back(sum_size);

  std::vector< std::pair< uint64, uint32 > > forced_splits;
  // find splitting points where the average is below the minimum
  // - here needs the fitting to be corrected
  sum_size = 0;
  int min_split_i(min_split_pos.size());
  for (std::vector< uint64 >::const_iterator oit(oversize_splits.begin());
  oit != oversize_splits.end(); ++oit)
  {
    int block_count(1);
    while ((min_split_i - block_count >= 0) &&
      (min_split_pos[min_split_i - block_count] < *oit))
      ++block_count;
    // correct the fitting if necessary
    uint32 used_blocks(0);
    for (int j(1); j < block_count; ++j)
    {
      if ((*oit - sum_size)*j/block_count + sum_size
	<= min_split_pos[min_split_i - j])
      {
	forced_splits.push_back
	  (std::make_pair(min_split_pos[min_split_i - j], j - used_blocks));
	used_blocks = j;
      }
    }
    forced_splits.push_back(std::make_pair(*oit, block_count - used_blocks));
    min_split_i = min_split_i - block_count;
    sum_size = *oit;
  }

  std::vector< std::pair< uint64, uint32 > >::const_iterator forced_it(forced_splits.begin());
  // calculate the real splitting positions
  sum_size = 0;
  uint64 min_sum_size(0);
  uint32 cur_block(0);
  uint64 next_limit(forced_it->first/forced_it->second);
  for (uint i(0); i < sizes.size(); ++i)
  {
    sum_size += sizes[i];
    if (sum_size > forced_it->first)
    {
      vsplit.push_back(i);
      cur_block = 0;
      min_sum_size = forced_it->first;
      ++forced_it;
      next_limit = (forced_it->first - min_sum_size)/
      (forced_it->second - cur_block) + min_sum_size;
      uint j(min_split_pos.size() - 1 - vsplit.size());
      if ((vsplit.size() < min_split_pos.size()) &&
	(min_split_pos[j] > next_limit))
	next_limit = min_split_pos[j];
    }
    else if (sum_size > next_limit)
    {
      vsplit.push_back(i);
      ++cur_block;
      min_sum_size = sum_size - sizes[i];
      next_limit = (forced_it->first - min_sum_size)/
      (forced_it->second - cur_block) + min_sum_size;
      uint j(min_split_pos.size() - 1 - vsplit.size());
      if ((vsplit.size() < min_split_pos.size()) &&
	(min_split_pos[j] > next_limit))
	next_limit = min_split_pos[j];
    }
  }

  // This converts the result in a more convienient form
  uint i(0), j(0);
  while ((j < vsplit.size()) && (it != end))
  {
    if (vsplit[j] == i)
    {
      split.push_back(*it);
      ++j;
    }

    ++i;
    ++it;
  }
}


template< class Index, class Object, class Iterator >
void Block_Backend< Index, Object, Iterator >::flush_if_necessary_and_write_obj(
    uint64* start_ptr, uint8*& insert_ptr, typename File_Blocks_::Write_Iterator& file_it,
    const Index& idx, const Object& obj)
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
      file_it = file_blocks.insert_block(file_it, start_ptr, bytes_written);
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
        file_it = file_blocks.insert_block(
            file_it, large_buf.ptr + i*block_size/8, block_size,
            i == 0 ? idx_size + obj_size + 4 : 0, idx);
      }
      file_it = file_blocks.insert_block(
          file_it, large_buf.ptr + (buf_scale-1)*block_size/8, idx_size + obj_size + 8 - block_size*(buf_scale-1),
          0, idx);

      insert_ptr = ((uint8*)start_ptr) + 8 + idx_size;
      return;
    }
    else
      insert_ptr = ((uint8*)start_ptr) + 8 + idx_size;
  }

  obj.to_data(insert_ptr);
  insert_ptr = insert_ptr + obj_size;
}


template< class TIndex, class TObject, class TIterator >
void Block_Backend< TIndex, TObject, TIterator >::create_from_scratch
    (typename File_Blocks_::Write_Iterator& file_it,
     const std::map< TIndex, std::set< TObject > >& to_insert)
{
  std::map< TIndex, uint32 > sizes;
  std::vector< TIndex > split;
  std::vector< uint32 > vsizes;
  Void_Pointer< uint8 > buffer(block_size);

  // compute the distribution over different blocks
  for (typename std::set< TIndex >::const_iterator fit(file_it.lower_bound());
      fit != file_it.upper_bound(); ++fit)
  {
    typename std::map< TIndex, std::set< TObject > >::const_iterator
        it(to_insert.find(*fit));

    uint32 current_size(4);
    if ((it == to_insert.end()) || (it->second.empty()))
      current_size = 0;
    else
    {
      // only add nonempty indices
      current_size += fit->size_of();
      for (typename std::set< TObject >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        current_size += it2->size_of();
    }

    sizes[*fit] += current_size;
    vsizes.push_back(current_size);
  }
  calc_split_idxs(split, vsizes, file_it.lower_bound(), file_it.upper_bound());

  // really write data
  typename std::vector< TIndex >::const_iterator split_it(split.begin());
  uint8* pos(buffer.ptr + 4);
  uint32 max_size(0);
  typename std::set< TIndex >::const_iterator upper_bound(file_it.upper_bound());
  for (typename std::set< TIndex >::const_iterator fit(file_it.lower_bound());
      fit != upper_bound; ++fit)
  {
    typename std::map< TIndex, std::set< TObject > >::const_iterator
        it(to_insert.find(*fit));

    if ((split_it != split.end()) && (*fit == *split_it))
    {
      if (pos > buffer.ptr + 4)
      {
        *(uint32*)buffer.ptr = pos - buffer.ptr;
        file_it = file_blocks.insert_block(file_it, (uint64*)buffer.ptr, max_size);
        pos = buffer.ptr + 4;
      }
      ++split_it;
      max_size = 0;
    }

    if (sizes[*fit] > max_size)
      max_size = sizes[*fit];

    if (sizes[*fit] == 0)
      continue;
    else if (sizes[*fit] < block_size - 4)
    {
      uint8* current_pos(pos);
      fit->to_data(pos + 4);
      pos = pos + fit->size_of() + 4;
      if (it != to_insert.end())
      {
        for (typename std::set< TObject >::const_iterator
          it2(it->second.begin()); it2 != it->second.end(); ++it2)
        {
          it2->to_data(pos);
          pos = pos + it2->size_of();
        }
      }
      *(uint32*)current_pos = pos - buffer.ptr;
    }
    else
    {
      fit->to_data(pos + 4);
      pos = pos + fit->size_of() + 4;

      if (it != to_insert.end())
      {
        for (typename std::set< TObject >::const_iterator
            it2 = it->second.begin(); it2 != it->second.end(); ++it2)
          flush_if_necessary_and_write_obj(
              (uint64*)buffer.ptr, pos, file_it, *fit, *it2);
      }

      if (pos - buffer.ptr > fit->size_of() + 8)
      {
        *(uint32*)(buffer.ptr+4) = pos - buffer.ptr;
        max_size = (*(uint32*)(buffer.ptr + 4)) - 4;
      }
      else
        pos = buffer.ptr + 4;
    }
  }
  if (pos > buffer.ptr + 4)
  {
    *(uint32*)buffer.ptr = pos - buffer.ptr;
    file_it = file_blocks.insert_block(file_it, (uint64*)buffer.ptr, max_size);
  }
  ++file_it;
}


template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
void Block_Backend< TIndex, TObject, TIterator >::update_group
    (typename File_Blocks_::Write_Iterator& file_it,
     const std::map< TIndex, std::set< TObject > >& to_delete,
     const std::map< TIndex, std::set< TObject > >& to_insert,
     Update_Logger& update_logger)
{
  std::map< TIndex, Index_Collection< TIndex, TObject > > index_values;
  std::map< TIndex, uint32 > sizes;
  std::vector< TIndex > split;
  std::vector< uint32 > vsizes;
  Void_Pointer< uint8 > source(block_size);
  Void_Pointer< uint8 > dest(block_size);

  file_blocks.read_block(file_it, (uint64*)source.ptr);

  // prepare a unified iterator over all indices, from file, to_delete
  // and to_insert
  uint8* pos(source.ptr + 4);
  uint8* source_end(source.ptr + *(uint32*)source.ptr);
  while (pos < source_end)
  {
    index_values.insert(std::make_pair(TIndex(pos + 4), Index_Collection< TIndex, TObject >
        (pos, source.ptr + *(uint32*)pos, to_delete.end(), to_insert.end())));
    pos = source.ptr + *(uint32*)pos;
  }
  typename std::map< TIndex, std::set< TObject > >::const_iterator
      to_delete_begin(to_delete.lower_bound(*(file_it.lower_bound())));
  typename std::map< TIndex, std::set< TObject > >::const_iterator
      to_delete_end(to_delete.end());
  if (file_it.upper_bound() != relevant_idxs.end())
    to_delete_end = to_delete.lower_bound(*(file_it.upper_bound()));
  for (typename std::map< TIndex, std::set< TObject > >::const_iterator
    it(to_delete_begin); it != to_delete_end; ++it)
  {
    typename std::map< TIndex, Index_Collection< TIndex, TObject > >::iterator
        ic_it(index_values.find(it->first));
    if (ic_it == index_values.end())
    {
      index_values.insert(std::make_pair(it->first,
	  Index_Collection< TIndex, TObject >(0, 0, it, to_insert.end())));
    }
    else
      ic_it->second.delete_it = it;
  }

  typename std::map< TIndex, std::set< TObject > >::const_iterator
      to_insert_begin(to_insert.lower_bound(*(file_it.lower_bound())));
  typename std::map< TIndex, std::set< TObject > >::const_iterator
      to_insert_end(to_insert.end());
  if (file_it.upper_bound() != relevant_idxs.end())
    to_insert_end = to_insert.lower_bound(*(file_it.upper_bound()));
  for (typename std::map< TIndex, std::set< TObject > >::const_iterator
      it(to_insert_begin); it != to_insert_end; ++it)
  {
    typename std::map< TIndex, Index_Collection< TIndex, TObject > >::iterator
        ic_it(index_values.find(it->first));
    if (ic_it == index_values.end())
    {
      index_values.insert(std::make_pair(it->first,
	  Index_Collection< TIndex, TObject >(0, 0, to_delete.end(), it)));
    }
    else
      ic_it->second.insert_it = it;
  }

  // compute the distribution over different blocks
  // and log all objects that will be deleted
  for (typename std::map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
      it(index_values.begin()); it != index_values.end(); ++it)
  {
    uint32 current_size(0);

    if (it->second.source_begin != 0)
    {
      uint8* pos(it->second.source_begin + 4);
      pos = pos + TIndex::size_of((it->second.source_begin) + 4);
      while (pos < it->second.source_end)
      {
	TObject obj(pos);
	if ((it->second.delete_it == to_delete.end()) ||
	  (it->second.delete_it->second.find(obj) == it->second.delete_it->second.end()))
	  current_size += obj.size_of();
	else
	  update_logger.deletion(it->first, obj);
	pos = pos + obj.size_of();
      }
      if (current_size > 0)
	current_size += TIndex::size_of((it->second.source_begin) + 4) + 4;
    }

    if ((it->second.insert_it != to_insert.end()) &&
      (!(it->second.insert_it->second.empty())))
    {
      // only add nonempty indices
      if (current_size == 0)
	current_size += it->first.size_of() + 4;
      for (typename std::set< TObject >::const_iterator
	it2(it->second.insert_it->second.begin());
      it2 != it->second.insert_it->second.end(); ++it2)
	current_size += it2->size_of();
    }

    sizes[it->first] += current_size;
    vsizes.push_back(current_size);
  }

  std::set< TIndex > index_values_set;
  for (typename std::map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
      it(index_values.begin()); it != index_values.end(); ++it)
    index_values_set.insert(it->first);
  calc_split_idxs(split, vsizes, index_values_set.begin(), index_values_set.end());

  // really write data
  typename std::vector< TIndex >::const_iterator split_it(split.begin());
  pos = (dest.ptr + 4);
  uint32 max_size(0);
  for (typename std::map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
    it(index_values.begin()); it != index_values.end(); ++it)
  {
    if ((split_it != split.end()) && (it->first == *split_it))
    {
      *(uint32*)dest.ptr = pos - dest.ptr;
      file_it = file_blocks.insert_block(file_it, (uint64*)dest.ptr, max_size);
      ++split_it;
      pos = dest.ptr + 4;
      max_size = 0;
    }

    if (sizes[it->first] > max_size)
      max_size = sizes[it->first];

    if (sizes[it->first] == 0)
      continue;
    else if (sizes[it->first] < block_size - 4)
    {
      uint8* current_pos(pos);
      it->first.to_data(pos + 4);
      pos += it->first.size_of() + 4;

      if (it->second.source_begin != 0)
      {
	uint8* spos(it->second.source_begin + 4);
	spos = spos + TIndex::size_of((it->second.source_begin) + 4);
	while (spos < it->second.source_end)
	{
	  TObject obj(spos);
	  if ((it->second.delete_it == to_delete.end()) ||
	    (it->second.delete_it->second.find(obj) == it->second.delete_it->second.end()))
	  {
	    memcpy(pos, spos, obj.size_of());
	    pos = pos + obj.size_of();
	  }
	  spos = spos + obj.size_of();
	}
      }

      if ((it->second.insert_it != to_insert.end()) &&
	(!(it->second.insert_it->second.empty())))
      {
	// only add nonempty indices
	for (typename std::set< TObject >::const_iterator
	  it2(it->second.insert_it->second.begin());
	it2 != it->second.insert_it->second.end(); ++it2)
	{
	  it2->to_data(pos);
	  pos = pos + it2->size_of();
	}
      }
      *(uint32*)current_pos = pos - dest.ptr;
    }
    else
    {
      it->first.to_data(pos + 4);
      pos += it->first.size_of() + 4;

      // can never overflow - we have read only one block
      if (it->second.source_begin != 0)
      {
	uint8* spos(it->second.source_begin + 4);
	spos = spos + TIndex::size_of((it->second.source_begin) + 4);
	while (spos < it->second.source_end)
	{
	  TObject obj(spos);
	  if ((it->second.delete_it == to_delete.end()) ||
	    (it->second.delete_it->second.find(obj) == it->second.delete_it->second.end()))
	  {
	    memcpy(pos, spos, obj.size_of());
	    pos = pos + obj.size_of();
	  }
	  spos = spos + obj.size_of();
	}
      }

      if ((it->second.insert_it != to_insert.end()) &&
	(!(it->second.insert_it->second.empty())))
      {
        for (typename std::set< TObject >::const_iterator
            it2(it->second.insert_it->second.begin());
            it2 != it->second.insert_it->second.end(); ++it2)
          flush_if_necessary_and_write_obj(
              (uint64*)dest.ptr, pos, file_it, it->first, *it2);
      }

      if ((uint32)(pos - dest.ptr) == it->first.size_of() + 8)
	// the block is in fact empty
	pos = dest.ptr + 4;

      *(uint32*)(dest.ptr+4) = pos - dest.ptr;
      max_size = (*(uint32*)(dest.ptr + 4)) - 4;
    }
  }

  if (pos > dest.ptr + 4)
  {
    *(uint32*)dest.ptr = pos - dest.ptr;
    file_it = file_blocks.replace_block(file_it, (uint64*)dest.ptr, max_size);
    ++file_it;
  }
  else
    file_it = file_blocks.erase_block(file_it);
}


template< class TIndex, class TObject, class TIterator >
void Block_Backend< TIndex, TObject, TIterator >::flush_or_delete_block(
    uint64* start_ptr, uint8* insert_ptr, typename File_Blocks_::Write_Iterator& file_it, uint32 idx_size)
{
  uint bytes_written = insert_ptr - (uint8*)start_ptr;
  if (bytes_written > 8 + idx_size)
  {
    *(uint32*)start_ptr = bytes_written;
    *(((uint32*)start_ptr)+1) = bytes_written;
    file_it = file_blocks.replace_block(file_it, start_ptr, bytes_written);
    ++file_it;
  }
  else
    file_it = file_blocks.erase_block(file_it);
}


template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
void Block_Backend< TIndex, TObject, TIterator >::copy_and_delete_on_the_fly(
    uint64* source_start_ptr, uint64* dest_start_ptr,
    typename File_Blocks_::Write_Iterator& file_it, uint32 idx_size,
    const std::map< TIndex, std::set< TObject > >& to_delete,
    typename std::map< TIndex, std::set< TObject > >::const_iterator& delete_it,
    bool& block_modified, uint8*& insert_ptr,
    Update_Logger& update_logger)
{
  file_blocks.read_block(file_it, source_start_ptr);

  block_modified = false;
  if (delete_it == to_delete.end())
  {
    memcpy(dest_start_ptr, source_start_ptr, *(uint32*)source_start_ptr);
    insert_ptr = ((uint8*)dest_start_ptr) + *(uint32*)source_start_ptr;
    return;
  }

  if (idx_size == 0)
    idx_size = TIndex::size_of(source_start_ptr+1);
  uint8* spos = ((uint8*)source_start_ptr) + 8 + idx_size;
  insert_ptr = ((uint8*)dest_start_ptr) + 8 + idx_size;
  memcpy(dest_start_ptr, source_start_ptr, spos - (uint8*)source_start_ptr);

  //copy everything that is not deleted yet
  if (*(uint32*)source_start_ptr != *(((uint32*)source_start_ptr)+1))
    throw File_Error(0, data_filename, "Block_Backend: one index expected - several found.");

  while ((uint32)(spos - (uint8*)source_start_ptr) < *(uint32*)source_start_ptr)
  {
    TObject obj(spos);
    if (delete_it->second.find(obj) == delete_it->second.end())
    {
      memcpy(insert_ptr, spos, obj.size_of());
      insert_ptr = insert_ptr + obj.size_of();
    }
    else
    {
      block_modified = true;
      update_logger.deletion(delete_it->first, obj);
    }
    spos = spos + obj.size_of();
  }
}


template< class TIndex, class TObject, class TIterator >
bool Block_Backend< TIndex, TObject, TIterator >::read_block_or_blocks(
    typename File_Blocks_::Write_Iterator& file_it, Void64_Pointer< uint64 >& source, uint32& buffer_size)
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


template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
uint32 Block_Backend< TIndex, TObject, TIterator >::skip_deleted_objects(
    uint64* source_start_ptr, uint64* dest_start_ptr,
    const std::set< TObject >& objs_to_delete, uint32 idx_size,
    Update_Logger& update_logger, const TIndex& idx)
{
  uint32 src_obj_offset = 8 + idx_size;
  uint32 dest_obj_offset = src_obj_offset;
  uint32 src_size = *(uint32*)source_start_ptr;
  memcpy(((uint8*)dest_start_ptr) + 8, ((uint8*)source_start_ptr) + 8, idx_size);

  while (src_obj_offset < src_size)
  {
    TObject obj(((uint8*)source_start_ptr) + src_obj_offset);
    if (objs_to_delete.find(obj) == objs_to_delete.end())
    {
      memcpy(
          ((uint8*)dest_start_ptr) + dest_obj_offset, ((uint8*)source_start_ptr) + src_obj_offset,
          obj.size_of());
      dest_obj_offset += obj.size_of();
    }
    else
      update_logger.deletion(idx, obj);

    src_obj_offset += obj.size_of();
  }

  *(uint32*)dest_start_ptr = dest_obj_offset;
  *(((uint32*)dest_start_ptr)+1) = dest_obj_offset;
  memcpy(dest_start_ptr + 1, source_start_ptr + 1, idx_size);

  return src_obj_offset == dest_obj_offset ? 0 : dest_obj_offset;
}


template< typename Object >
void append_insertables(
    uint64* dest_start_ptr, uint32 block_size,
    typename std::set< Object >::const_iterator& cur_insert,
    const typename std::set< Object >::const_iterator& cur_end)
{
  uint32 obj_append_offset = *(uint32*)dest_start_ptr;

  while ((cur_insert != cur_end) && (obj_append_offset + cur_insert->size_of() < block_size))
  {
    cur_insert->to_data(((uint8*)dest_start_ptr) + obj_append_offset);
    obj_append_offset += cur_insert->size_of();
    ++cur_insert;
  }

  *(uint32*)dest_start_ptr = obj_append_offset;
  *(((uint32*)dest_start_ptr)+1) = obj_append_offset;
}


template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
void Block_Backend< TIndex, TObject, TIterator >::update_segments
      (typename File_Blocks_::Write_Iterator& file_it,
       const std::map< TIndex, std::set< TObject > >& to_delete,
       const std::map< TIndex, std::set< TObject > >& to_insert,
       Update_Logger& update_logger)
{
  uint32 buffer_size = block_size;
  Void64_Pointer< uint64 > source(buffer_size);
  Void64_Pointer< uint64 > dest(buffer_size);
  TIndex idx = file_it.block_it->index;
  typename std::map< TIndex, std::set< TObject > >::const_iterator
      delete_it(to_delete.find(idx));
  typename std::map< TIndex, std::set< TObject > >::const_iterator
      insert_it(to_insert.find(idx));
  uint32 idx_size = idx.size_of();
  bool last_segment_belongs_to_oversized = false;

  typename std::set< TObject >::const_iterator cur_insert;
  if (insert_it != to_insert.end())
    cur_insert = insert_it->second.begin();

  while (!(file_it == file_blocks.write_end())
      && file_it.block_type() == File_Block_Index_Entry< TIndex >::SEGMENT)
  {
    typename std::list< File_Block_Index_Entry< TIndex > >::iterator delta_it = file_it.block_it;
    bool oversized = read_block_or_blocks(file_it, source, buffer_size);
    if (oversized)
    {
      last_segment_belongs_to_oversized =
          file_it.block_type() == File_Block_Index_Entry< TIndex >::LAST_SEGMENT;
      if (!last_segment_belongs_to_oversized)
        ++file_it;
      TObject obj(((uint8*)source.ptr) + 8 + idx_size);
      if (delete_it != to_delete.end() && delete_it->second.find(obj) != delete_it->second.end())
        file_blocks.erase_blocks(delta_it, file_it);
    }
    else
    {
      uint32 obj_append_offset = 0;
      if (delete_it != to_delete.end())
        obj_append_offset = skip_deleted_objects(
            source.ptr, dest.ptr, delete_it->second, idx_size, update_logger, idx);
      else
        memcpy(dest.ptr, source.ptr, *(uint32*)source.ptr);

      if (obj_append_offset)
      {
        if (insert_it != to_insert.end())
          append_insertables< TObject >(dest.ptr, block_size, cur_insert, insert_it->second.end());
        flush_or_delete_block(dest.ptr, ((uint8*)dest.ptr) + *(uint32*)dest.ptr, file_it, idx_size);
      }
      else if (insert_it != to_insert.end() && *(uint32*)source.ptr < block_size/2)
      {
        append_insertables< TObject >(dest.ptr, block_size, cur_insert, insert_it->second.end());
        flush_or_delete_block(dest.ptr, ((uint8*)dest.ptr) + *(uint32*)dest.ptr, file_it, idx_size);
      }
      else
        ++file_it;
    }
  }

  uint32 obj_append_offset = 0;
  bool is_modified = true;
  if (!(file_it == file_blocks.write_end()) && !last_segment_belongs_to_oversized
      && file_it.block_type() == File_Block_Index_Entry< TIndex >::LAST_SEGMENT)
  {
    typename std::list< File_Block_Index_Entry< TIndex > >::iterator delta_it = file_it.block_it;
    read_block_or_blocks(file_it, source, buffer_size);

    if (delete_it != to_delete.end())
      obj_append_offset = skip_deleted_objects(
          source.ptr, dest.ptr, delete_it->second, idx_size, update_logger, idx);
    else
    {
      memcpy(dest.ptr, source.ptr, *(uint32*)source.ptr);
      is_modified = false;
    }

    if (is_modified && obj_append_offset)
    {
      if (insert_it != to_insert.end())
        append_insertables< TObject >(dest.ptr, block_size, cur_insert, insert_it->second.end());
    }
    else if (insert_it != to_insert.end() && *(uint32*)source.ptr < block_size/2)
      append_insertables< TObject >(dest.ptr, block_size, cur_insert, insert_it->second.end());
    else
      is_modified = false;

    obj_append_offset = *(uint32*)dest.ptr;
  }
  else
  {
    idx.to_data(((uint8*)dest.ptr) + 8);
    obj_append_offset = 8 + idx_size;
  }

  uint8* pos = ((uint8*)dest.ptr) + obj_append_offset;
  if (insert_it != to_insert.end())
  {
    is_modified = true;
    while (cur_insert != insert_it->second.end())
    {
      flush_if_necessary_and_write_obj(dest.ptr, pos, file_it, idx, *cur_insert);
      ++cur_insert;
    }
  }

  if (is_modified)
    flush_or_delete_block(dest.ptr, pos, file_it, idx_size);
  else
    ++file_it;
}


#endif
