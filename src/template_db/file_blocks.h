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

#ifndef DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_H
#define DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_H

#include "file_blocks_index.h"
#include "types.h"
#include "lz4_wrapper.h"
#include "zlib_wrapper.h"

#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <list>
#include <sstream>


/** Declarations: -----------------------------------------------------------*/


template< typename TIndex >
struct File_Blocks_Basic_Iterator
{
  File_Blocks_Basic_Iterator(
      const File_Blocks_Index_Iterator< TIndex >& begin, const File_Blocks_Index_Iterator< TIndex >& end)
      : block_it(begin), block_end(end) {}

  File_Blocks_Basic_Iterator(const File_Blocks_Basic_Iterator& a)
      : block_it(a.block_it), block_end(a.block_end) {}

  bool is_end() const { return block_it == block_end; }

  const File_Blocks_Index_Iterator< TIndex >& block() const { return block_it; }

protected:
  File_Blocks_Index_Iterator< TIndex > block_it;
  File_Blocks_Index_Iterator< TIndex > block_end;
};


template< typename TIndex >
struct File_Blocks_Flat_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Flat_Iterator
  (const File_Blocks_Index_Iterator< TIndex >& begin, const File_Blocks_Index_Iterator< TIndex >& end)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end) {}

  File_Blocks_Flat_Iterator(const File_Blocks_Flat_Iterator& a)
    : File_Blocks_Basic_Iterator< TIndex >(a) {}

  ~File_Blocks_Flat_Iterator() {}

  const File_Blocks_Flat_Iterator& operator=
      (const File_Blocks_Flat_Iterator& a);
  bool operator==(const File_Blocks_Flat_Iterator& a) const;
  File_Blocks_Flat_Iterator& operator++();
  bool is_out_of_range(const TIndex& index);
};


template< typename TIndex, typename TIterator >
struct File_Blocks_Discrete_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Discrete_Iterator
      (TIterator const& index_it_, TIterator const& index_end_,
       const File_Blocks_Index_Iterator< TIndex >& begin, const File_Blocks_Index_Iterator< TIndex >& end)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end),
      index_lower(index_it_), index_upper(index_it_), index_end(index_end_)
  {
    find_next_block();
  }

  File_Blocks_Discrete_Iterator
      (const File_Blocks_Index_Iterator< TIndex >& end)
    : File_Blocks_Basic_Iterator< TIndex >(end, end) {}

  File_Blocks_Discrete_Iterator(const File_Blocks_Discrete_Iterator& a)
    : File_Blocks_Basic_Iterator< TIndex >(a),
      index_lower(a.index_lower), index_upper(a.index_upper),
      index_end(a.index_end) {}

  ~File_Blocks_Discrete_Iterator() {}

  const File_Blocks_Discrete_Iterator& operator=
      (const File_Blocks_Discrete_Iterator& a);
  bool operator==
      (const File_Blocks_Discrete_Iterator& a) const;
  File_Blocks_Discrete_Iterator& operator++();

  const TIterator& lower_bound() const { return index_lower; }
  const TIterator& upper_bound() const { return index_upper; }

  TIterator index_lower;
  TIterator index_upper;
  TIterator index_end;

private:
  void find_next_block();
};


template< typename TIndex, typename TRangeIterator >
struct File_Blocks_Range_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Range_Iterator
      (const File_Blocks_Index_Iterator< TIndex >& begin, const File_Blocks_Index_Iterator< TIndex >& end,
       const TRangeIterator& index_it_,  const TRangeIterator& index_end_)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end),
      index_it(index_it_), index_end(index_end_), index_equals_last_index(false)
  {
    find_next_block();
  }

  File_Blocks_Range_Iterator
      (const File_Blocks_Index_Iterator< TIndex >& end)
    : File_Blocks_Basic_Iterator< TIndex >(end, end) {}

  File_Blocks_Range_Iterator(const File_Blocks_Range_Iterator& a)
    : File_Blocks_Basic_Iterator< TIndex >(a),
      index_it(a.index_it), index_end(a.index_end) {}

  ~File_Blocks_Range_Iterator() {}

  const File_Blocks_Range_Iterator& operator=
      (const File_Blocks_Range_Iterator& a);
  bool operator==(const File_Blocks_Range_Iterator& b) const;
  File_Blocks_Range_Iterator& operator++();

private:
  TRangeIterator index_it;
  TRangeIterator index_end;
  bool index_equals_last_index;

  void find_next_block();
};


template< typename TIndex, typename TIterator >
struct File_Blocks_Write_Iterator
{
  File_Blocks_Write_Iterator
      (TIterator const& index_it_, TIterator const& index_end_,
       const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& begin,
       const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end, bool is_empty_ = false)
    : index_lower(index_it_), index_upper(index_it_), index_end(index_end_),
      is_empty(is_empty_ && begin == end), segments_mode(false),
      block_begin(begin), block_it(begin), block_end(end)
  {
    find_next_block();
    if (is_empty_ && this->block_it == this->block_end && index_it_ != index_end_)
      is_empty = true;
  }

  File_Blocks_Write_Iterator
      (const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : is_empty(false), segments_mode(false), block_begin(end), block_it(end), block_end(end) {}

  File_Blocks_Write_Iterator(const File_Blocks_Write_Iterator& a)
    : index_lower(a.index_lower), index_upper(a.index_upper),
      index_end(a.index_end), is_empty(a.is_empty), segments_mode(a.segments_mode),
      block_begin(a.block_begin), block_it(a.block_it), block_end(a.block_end) {}

  ~File_Blocks_Write_Iterator() {}

  bool is_end() const { return block_it == block_end && !is_empty; }
  int block_type() const;
  void start_segments_mode() { segments_mode = true; }
  void end_segments_mode()
  {
    segments_mode = false;
    find_next_block();
  }

  const File_Blocks_Write_Iterator& operator=
      (const File_Blocks_Write_Iterator& a);
  bool operator==
      (const File_Blocks_Write_Iterator& a) const;
  File_Blocks_Write_Iterator& operator++();

  const TIterator& lower_bound() const { return index_lower; }
  const TIterator& upper_bound() const { return index_upper; }

  const File_Block_Index_Entry< TIndex >& block() const { return *block_it; }
  void set_block(Writeable_File_Blocks_Index< TIndex >& index, const File_Block_Index_Entry< TIndex >& rhs)
  {
    index.drop_block_array();
    *block_it = rhs;
  }
  void insert_block(Writeable_File_Blocks_Index< TIndex >& index, const File_Block_Index_Entry< TIndex >& entry);
  void erase_block(Writeable_File_Blocks_Index< TIndex >& index);
  void erase_blocks(Writeable_File_Blocks_Index< TIndex >& index, const File_Blocks_Write_Iterator& upper_limit);

  TIterator index_lower;
  TIterator index_upper;
  TIterator index_end;
  bool is_empty;
  bool segments_mode;

private:
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator block_begin;
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator block_it;
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator block_end;

  void find_next_block();
};


template< typename TIndex, typename TIterator >
struct File_Blocks
{
  typedef File_Blocks_Flat_Iterator< TIndex > Flat_Iterator;
  typedef File_Blocks_Discrete_Iterator< TIndex, TIterator > Discrete_Iterator;
  //typedef File_Blocks_Range_Iterator< TIndex, TRangeIterator > Range_Iterator;
  typedef File_Blocks_Write_Iterator< TIndex, TIterator > Write_Iterator;

private:
  File_Blocks(const File_Blocks& f) {}

public:
  File_Blocks(File_Blocks_Index_Base* index);
  ~File_Blocks() {}

  Flat_Iterator flat_begin();
  Flat_Iterator flat_end();
  Discrete_Iterator discrete_begin(const TIterator& begin, const TIterator& end);
  Discrete_Iterator discrete_end();

  template< typename Range_Iterator >
  File_Blocks_Range_Iterator< TIndex, Range_Iterator > range_begin(const Range_Iterator& begin, const Range_Iterator& end);
  template< typename Range_Iterator >
  File_Blocks_Range_Iterator< TIndex, Range_Iterator > range_end();

  Write_Iterator write_begin(const TIterator& begin, const TIterator& end, bool is_empty = false);
  Write_Iterator write_end();

  uint64* read_block(const File_Blocks_Basic_Iterator< TIndex >& it, bool check_idx = true) const;
  uint64* read_block(
      const File_Blocks_Basic_Iterator< TIndex >& it, uint64* buffer, bool check_idx = true) const;
  uint64* read_block(const File_Blocks_Write_Iterator< TIndex, TIterator >& it, bool check_idx = true) const;
  uint64* read_block(
      const File_Blocks_Write_Iterator< TIndex, TIterator >& it, uint64* buffer, bool check_idx = true) const;

  uint read_count() const { return read_count_; }
  void reset_read_count() const { read_count_ = 0; }

  Write_Iterator insert_block(const Write_Iterator& it, uint64* buf, uint32 max_keysize);
  Write_Iterator insert_block(
      const Write_Iterator& it, uint64* buf, uint32 payload_size, uint32 max_keysize, const TIndex& block_idx);
  Write_Iterator replace_block(const Write_Iterator& it, uint64* buf, uint32 max_keysize);
  Write_Iterator replace_block(
      Write_Iterator it, uint64* buf, uint32 payload_size, uint32 max_keysize, const TIndex& block_idx);
  Write_Iterator erase_block(Write_Iterator it);
  void erase_blocks(
      Write_Iterator& block_it, const Write_Iterator& it);

  const Readonly_File_Blocks_Index< TIndex >& get_rd_idx() const { return *rd_idx; }
  const Writeable_File_Blocks_Index< TIndex >& get_wr_idx() const { return *wr_idx; }

private:
  Readonly_File_Blocks_Index< TIndex >* rd_idx;
  Writeable_File_Blocks_Index< TIndex >* wr_idx;  
  uint32 block_size;
  uint32 compression_factor;
  int compression_method;
  mutable uint read_count_;

  Raw_File data_file;
  Void64_Pointer< uint64 > buffer;

  template< typename File_Blocks_Iterator >
  uint64* read_block_(
      const File_Blocks_Iterator& it, uint64* buffer_, bool check_idx) const;
  uint32 allocate_block(uint32 data_size);
  void write_block(uint64* buf, uint32 uncompressed_size, uint32& data_size, uint32& pos);
};


/** Implementation File_Blocks_Flat_Iterator: -------------------------------*/

template< typename TIndex >
const File_Blocks_Flat_Iterator< TIndex >& File_Blocks_Flat_Iterator< TIndex >::operator=
(const File_Blocks_Flat_Iterator& a)
{
  if (this == &a)
    return *this;
  this->~File_Blocks_Flat_Iterator();
  new (this) File_Blocks_Flat_Iterator(a);
  return *this;
}


template< typename TIndex >
bool File_Blocks_Flat_Iterator< TIndex >::operator==
(const File_Blocks_Flat_Iterator& a) const
{
  return this->block_it == a.block_it;
}


template< typename TIndex >
File_Blocks_Flat_Iterator< TIndex >& File_Blocks_Flat_Iterator< TIndex >::operator++()
{
  ++(this->block_it);
  return *this;
}


template< typename TIndex >
bool File_Blocks_Flat_Iterator< TIndex >::is_out_of_range(const TIndex& index)
{
  if (this->block_it == this->block_end)
    return true;
  if (index == this->block_it->index)
    return false;
  if (index < this->block_it->index)
    return true;
  typename std::vector< File_Block_Index_Entry< TIndex > >::iterator
      next_it(this->block_it);
  if (++next_it == this->block_end)
    return false;
  if (!(index < next_it->index))
    return true;
  return false;
}


/** Implementation File_Blocks_Discrete_Iterator: ---------------------------*/

template< typename TIndex, typename TIterator >
const File_Blocks_Discrete_Iterator< TIndex, TIterator >&
File_Blocks_Discrete_Iterator< TIndex, TIterator >::operator=
(const File_Blocks_Discrete_Iterator& a)
{
  if (this == &a)
    return *this;
  this->~File_Blocks_Discrete_Iterator();
  new (this) File_Blocks_Discrete_Iterator(a);
  return *this;
}


template< typename TIndex, typename TIterator >
bool File_Blocks_Discrete_Iterator< TIndex, TIterator >::operator==
(const File_Blocks_Discrete_Iterator< TIndex, TIterator >& a) const
{
  return this->block_it == a.block_it;
}


template< typename TIndex, typename TIterator >
File_Blocks_Discrete_Iterator< TIndex, TIterator >&
File_Blocks_Discrete_Iterator< TIndex, TIterator >::operator++()
{
  auto it = this->block_it;
  ++(this->block_it);
  if (!this->is_end() && !TIndex::equal(this->block_it.idx_ptr(), it.idx_ptr()))
    find_next_block();
  return *this;
}


template< typename TIndex, typename TIterator >
void File_Blocks_Discrete_Iterator< TIndex, TIterator >::find_next_block()
{
  index_lower = index_upper;

  while (!this->is_end())
  {
    while (index_lower != index_end && index_lower->less(this->block_it.idx_ptr()))
      ++index_lower;
    if (index_lower == index_end)
    {
      index_upper = index_end;
      this->block_it = this->block_end;
      return;
    }

    auto next_block = this->block_it;
    ++next_block;

    if (next_block == this->block_end)
    {
      index_upper = index_end;
      return;
    }
    else if (index_lower->less(next_block.idx_ptr()))
    {
      index_upper = index_lower;
      while (index_upper != index_end && index_upper->less(next_block.idx_ptr()))
        ++index_upper;
      return;
    }
    else if (index_lower->equal(this->block_it.idx_ptr())) // implies: this->block_it->index == next_block->index
    {
      index_upper = index_lower;
      ++index_upper;
      return;
    }
    // seek
    else if (TIndex::equal(this->block_it.idx_ptr(), next_block.idx_ptr()))
    {
      while (!this->is_end() && TIndex::equal(this->block_it.idx_ptr(), next_block.idx_ptr()))
        ++this->block_it;
    }
    else
      ++this->block_it;
  }

  index_upper = index_end;
}


/** Implementation File_Blocks_Range_Iterator: ------------------------------*/

template< typename TIndex, typename TRangeIterator >
const File_Blocks_Range_Iterator< TIndex, TRangeIterator >&
File_Blocks_Range_Iterator< TIndex, TRangeIterator >::operator=
(const File_Blocks_Range_Iterator< TIndex, TRangeIterator >& a)
{
  if (this == &a)
    return *this;
  this->~File_Blocks_Range_Iterator();
  new (this) File_Blocks_Range_Iterator(a);
  return *this;
}


template< typename TIndex, typename TRangeIterator >
bool File_Blocks_Range_Iterator< TIndex, TRangeIterator >::operator==(const File_Blocks_Range_Iterator< TIndex, TRangeIterator >& b) const
{
  return (this->block_it == b.block_it);
}


template< typename Index, typename Iterator >
bool index_equals_next_index(Iterator it, Iterator end)
{
  if (it == end)
    return false;
  Iterator next = it;
  ++next;
  if (next == end)
    return false;
  return Index::equal(it.idx_ptr(), next.idx_ptr());
}


template< typename TIndex, typename TRangeIterator >
File_Blocks_Range_Iterator< TIndex, TRangeIterator >&
File_Blocks_Range_Iterator< TIndex, TRangeIterator >::operator++()
{
  index_equals_last_index = index_equals_next_index
      < TIndex, File_Blocks_Index_Iterator< TIndex > >(this->block_it, this->block_end);
  ++(this->block_it);
  if (!index_equals_last_index)
    find_next_block();
  return *this;
}


template< typename TIndex, typename TRangeIterator >
void File_Blocks_Range_Iterator< TIndex, TRangeIterator >::find_next_block()
{
  while (!this->is_end())
  {
    while (index_it != index_end && index_it.upper_bound().leq(this->block_it.idx_ptr()))
      ++index_it;

    if (index_it == index_end)
    {
      // We are done - there are no more indices left
      this->block_it = this->block_end;
      return;
    }

    auto next_block = this->block_it;
    ++next_block;
    //seek
    while ((!(next_block == this->block_end)) && (!index_it.lower_bound().less(next_block.idx_ptr())))
    {
      if (index_it.lower_bound().leq(this->block_it.idx_ptr()))
	// We have found a relevant block that is a segment
	return;
      index_equals_last_index = index_equals_next_index
          < TIndex, File_Blocks_Index_Iterator< TIndex > >(this->block_it, this->block_end);
      ++(this->block_it);
      ++next_block;
    }

    // i.e. it is not a last part of a sequence of segments
    if (!index_equals_last_index || index_it.lower_bound().leq(this->block_it.idx_ptr()))
      break;

    index_equals_last_index = index_equals_next_index
        < TIndex, File_Blocks_Index_Iterator< TIndex > >(this->block_it, this->block_end);
    ++(this->block_it);
  }
}


/** Implementation File_Blocks_Write_Iterator: ---------------------------*/

template< typename TIndex, typename TIterator >
int File_Blocks_Write_Iterator< TIndex, TIterator >::block_type() const
{
  if ((this->block_it == this->block_end) || (this->is_empty))
    return File_Block_Index_Entry< TIndex >::EMPTY;
  typename std::list< File_Block_Index_Entry< TIndex > >::const_iterator
      it(this->block_it);
  if (this->block_it == this->block_begin)
  {
    if (++it == this->block_end)
      return File_Block_Index_Entry< TIndex >::GROUP;
    else if (this->block_it->index == it->index)
      return File_Block_Index_Entry< TIndex >::SEGMENT;
    else
      return File_Block_Index_Entry< TIndex >::GROUP;
  }
  ++it;
  if (it == this->block_end)
  {
    --it;
    --it;
    if (it->index == this->block_it->index)
      return File_Block_Index_Entry< TIndex >::LAST_SEGMENT;
    else
      return File_Block_Index_Entry< TIndex >::GROUP;
  }
  if (it->index == this->block_it->index)
    return File_Block_Index_Entry< TIndex >::SEGMENT;
  --it;
  --it;
  if (it->index == this->block_it->index)
    return File_Block_Index_Entry< TIndex >::LAST_SEGMENT;
  else
    return File_Block_Index_Entry< TIndex >::GROUP;
}


template< typename TIndex, typename TIterator >
const File_Blocks_Write_Iterator< TIndex, TIterator >&
File_Blocks_Write_Iterator< TIndex, TIterator >::operator=
(const File_Blocks_Write_Iterator& a)
{
  if (this == &a)
    return *this;
  this->~File_Blocks_Write_Iterator();
  new (this) File_Blocks_Write_Iterator(a);
  return *this;
}


template< typename TIndex, typename TIterator >
bool File_Blocks_Write_Iterator< TIndex, TIterator >::operator==
(const File_Blocks_Write_Iterator< TIndex, TIterator >& a) const
{
  return ((this->block_it == a.block_it) && (this->is_empty == a.is_empty));
}


template< typename TIndex, typename TIterator >
File_Blocks_Write_Iterator< TIndex, TIterator >&
File_Blocks_Write_Iterator< TIndex, TIterator >::operator++()
{
  int block_type(this->block_type());
  if (block_type == File_Block_Index_Entry< TIndex >::EMPTY)
  {
    this->is_empty = false;
    find_next_block();
    return *this;
  }
  if (segments_mode || block_type == File_Block_Index_Entry< TIndex >::SEGMENT)
  {
    ++(this->block_it);
    return *this;
  }

  ++(this->block_it);
  find_next_block();
  return *this;
}


template< typename TIndex, typename TIterator >
void File_Blocks_Write_Iterator< TIndex, TIterator >::insert_block(
    Writeable_File_Blocks_Index< TIndex >& index, const File_Block_Index_Entry< TIndex >& entry)
{
  index.drop_block_array();
  if (block_it == block_begin)
  {
    block_it = index.insert(block_it, entry);
    block_begin = block_it;
  }
  else
    block_it = index.insert(block_it, entry);
  ++block_it;
}


template< typename TIndex, typename TIterator >
void File_Blocks_Write_Iterator< TIndex, TIterator >::erase_block(Writeable_File_Blocks_Index< TIndex >& index)
{
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator to_delete = block_it;
  operator++();

  index.drop_block_array();
  if (to_delete == block_begin)
  {
    to_delete = index.erase(to_delete);
    block_begin = to_delete;
  }
  else
    to_delete = index.erase(to_delete);
}


template< typename TIndex, typename TIterator >
void File_Blocks_Write_Iterator< TIndex, TIterator >::erase_blocks(
    Writeable_File_Blocks_Index< TIndex >& index, const File_Blocks_Write_Iterator< TIndex, TIterator >& rhs)
{
  index.drop_block_array();
  while (block_it != rhs.block_it)
    block_it = index.erase(block_it);
}


template< typename TIndex, typename TIterator >
void File_Blocks_Write_Iterator< TIndex, TIterator >::find_next_block()
{
//   std::cout<<"DEBUG N "<<this->is_empty<<' '<<(index_upper == index_end)
//       <<' '<<(this->block_it == this->block_end ? 0xffffffff : this->block_it->pos)<<'\n';
  index_lower = index_upper;
  if (index_lower == index_end)
  {
    this->block_it = this->block_end;
    this->is_empty = false;
    return;
  }

  if (this->block_it == this->block_end)
  {
    this->is_empty = false;
    index_upper = index_end;
    return;
  }

//   std::cout<<"DEBUG O "<<this->is_empty<<' '<<(index_upper == index_end)
//       <<' '<<(this->block_it == this->block_end ? 0xffffffff : this->block_it->pos)<<'\n';
  if ((this->block_type() == File_Block_Index_Entry< TIndex >::SEGMENT)
    && (*index_lower < this->block_it->index))
  {
    this->is_empty = true;
    while ((!(index_upper == index_end)) && (*index_upper < this->block_it->index))
      ++index_upper;
  }

  typename std::list< File_Block_Index_Entry< TIndex > >::const_iterator
  next_block(this->block_it);
  ++next_block;
  while ((next_block != this->block_end) &&
    (!(*index_lower < next_block->index)))
  {
    if (this->block_it->index == *index_lower)
    {
      // We have found a relevant block that is a segment
      ++index_upper;
      return;
    }
    ++(this->block_it);
    ++next_block;
  }
//   std::cout<<"DEBUG P "<<this->is_empty<<' '<<(index_upper == index_end)
//       <<' '<<(this->block_it == this->block_end ? 0xffffffff : this->block_it->pos)<<'\n';

  if (next_block == this->block_end)
  {
    if (this->block_type() == File_Block_Index_Entry< TIndex >::LAST_SEGMENT)
    {
      ++(this->block_it);
      this->is_empty = true;
    }
    index_upper = index_end;
    return;
  }
//   std::cout<<"DEBUG Q "<<this->is_empty<<' '<<(index_upper == index_end)
//       <<' '<<(this->block_it == this->block_end ? 0xffffffff : this->block_it->pos)<<'\n';

  if (this->block_type() == File_Block_Index_Entry< TIndex >::LAST_SEGMENT)
  {
    while ((!(index_upper == index_end)) && (*index_upper < next_block->index))
      ++index_upper;
    ++(this->block_it);
    this->is_empty = true;
  }

  while ((index_upper != index_end) && (*index_upper < next_block->index))
    ++index_upper;
//   std::cout<<"DEBUG R "<<this->is_empty<<' '<<(index_upper == index_end)
//       <<' '<<(this->block_it == this->block_end ? 0xffffffff : this->block_it->pos)<<'\n';
}


/** Implementation File_Blocks: ---------------------------------------------*/

template< typename TIndex, typename TIterator >
File_Blocks< TIndex, TIterator >::File_Blocks
    (File_Blocks_Index_Base* index_) :
     rd_idx(dynamic_cast< Readonly_File_Blocks_Index< TIndex >* >(index_)),
     wr_idx(dynamic_cast< Writeable_File_Blocks_Index< TIndex >* >(index_)),
     block_size(index_->get_block_size()),
     compression_factor(index_->get_compression_factor()),
     compression_method(index_->get_compression_method()),
     read_count_(0),
     data_file(index_->get_data_file_name(),
	       wr_idx ? O_RDWR|O_CREAT : O_RDONLY,
	       S_666, "File_Blocks::File_Blocks::1"),
     buffer(index_->get_block_size() * index_->get_compression_factor() * 2)      // increased buffer size for lz4
{}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Flat_Iterator
    File_Blocks< TIndex, TIterator >::flat_begin()
{
  if (rd_idx)
    return Flat_Iterator(rd_idx->begin(), rd_idx->end());
  return Flat_Iterator(wr_idx->begin(), wr_idx->end());
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Flat_Iterator
    File_Blocks< TIndex, TIterator >::flat_end()
{
  if (rd_idx)
    return Flat_Iterator(rd_idx->end(), rd_idx->end());
  return Flat_Iterator(wr_idx->end(), wr_idx->end());
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator >::discrete_begin
    (const TIterator& begin, const TIterator& end)
{
  if (rd_idx)
    return File_Blocks_Discrete_Iterator< TIndex, TIterator >
        (begin, end, rd_idx->begin(), rd_idx->end());
  return File_Blocks_Discrete_Iterator< TIndex, TIterator >
      (begin, end, wr_idx->begin(), wr_idx->end());
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator >::discrete_end()
{
  if (rd_idx)
    return Discrete_Iterator(rd_idx->end());
  return Discrete_Iterator(wr_idx->end());
}


template< typename TIndex, typename TIterator >
template< typename Range_Iterator >
File_Blocks_Range_Iterator< TIndex, Range_Iterator > File_Blocks< TIndex, TIterator >::range_begin(
    const Range_Iterator& begin, const Range_Iterator& end)
{
  if (rd_idx)
    return File_Blocks_Range_Iterator< TIndex, Range_Iterator >
        (rd_idx->begin(), rd_idx->end(), begin, end);
  return File_Blocks_Range_Iterator< TIndex, Range_Iterator >
      (wr_idx->begin(), wr_idx->end(), begin, end);
}


template< typename TIndex, typename TIterator >
template< typename Range_Iterator >
File_Blocks_Range_Iterator< TIndex, Range_Iterator > File_Blocks< TIndex, TIterator >::range_end()
{
  if (rd_idx)
    return File_Blocks_Range_Iterator< TIndex, Range_Iterator >(rd_idx->end());
  return File_Blocks_Range_Iterator< TIndex, Range_Iterator >(wr_idx->end());
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::write_begin
    (const TIterator& begin, const TIterator& end, bool is_empty)
{
  return File_Blocks_Write_Iterator< TIndex, TIterator >
      (begin, end, wr_idx->wr_begin(), wr_idx->wr_end(), is_empty);
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::write_end()
{
  return File_Blocks_Write_Iterator< TIndex, TIterator >(wr_idx->wr_end());
}


class Mmap
{
public:
  Mmap(int fd, off_t offset, size_t length_, const std::string& file_name, const std::string& origin)
      : addr(0), length(length_)
  {
    if (length > 0)
      addr = mmap(0, length, PROT_READ, MAP_PRIVATE, fd, offset);
    if (addr == (void*)(-1))
      throw File_Error(errno, file_name, origin);
  }
  ~Mmap()
  { 
    if (addr)
      munmap(addr, length);
  }
  
  uint64* ptr() { return (uint64*)addr; }
  
private:
  void* addr;
  size_t length;
};


template< typename TIndex, typename TIterator >
template< typename File_Block_Entry >
uint64* File_Blocks< TIndex, TIterator >::read_block_
    (const File_Block_Entry& block, uint64* buffer_, bool check_idx) const
{
  if (compression_method == File_Blocks_Index_Base::NO_COMPRESSION)
  {
    data_file.seek((int64)(block.pos()) * block_size, "File_Blocks::read_block::1");
    data_file.read((uint8*)buffer_, block_size * block.size(), "File_Blocks::read_block::2");
  }
  else if (compression_method == File_Blocks_Index_Base::ZLIB_COMPRESSION)
  {
    Mmap raw_block(
        data_file.fd(), (int64)(block.pos()) * block_size, block_size * block.size(),
        rd_idx ? rd_idx->get_data_file_name() : wr_idx->get_data_file_name(), "File_Blocks::read_block::3");
    try
    {
      Zlib_Inflate().decompress(
          raw_block.ptr(), block_size * block.size(), buffer_, block_size * compression_factor);
    }
    catch (const Zlib_Inflate::Error& e)
    {
      std::ostringstream out;
      out<<"File_Blocks::read_block: Zlib_Inflate::Error "<<e.error_code
          <<" at offset "<<((int64)(block.pos()) * block_size + 8)<<"; "
          <<" in_size: "<<(block_size * block.size())<<", "
          <<" out_size: "<<(block_size * compression_factor);
      throw File_Error(block.pos(), rd_idx ? rd_idx->get_data_file_name() : wr_idx->get_data_file_name(), out.str());
    }
  }
  else if (compression_method == File_Blocks_Index_Base::LZ4_COMPRESSION)
  {
    Mmap raw_block(
        data_file.fd(), (int64)(block.pos()) * block_size, block_size * block.size(),
        rd_idx ? rd_idx->get_data_file_name() : wr_idx->get_data_file_name(), "File_Blocks::read_block::4");
    try
    {
      LZ4_Inflate().decompress(
          raw_block.ptr(), block_size * block.size(), buffer_, block_size * compression_factor);
    }
    catch (const LZ4_Inflate::Error& e)
    {
      std::ostringstream out;
      out<<"File_Blocks::read_block: LZ4_Inflate::Error "<<e.error_code
          <<" at offset "<<((int64)(block.pos()) * block_size + 8)<<"; "
          <<" in_size: "<<(block_size * block.size())<<", "
          <<" out_size: "<<(block_size * compression_factor);
      throw File_Error(block.pos(), rd_idx ? rd_idx->get_data_file_name() : wr_idx->get_data_file_name(), out.str());
    }
  }

  if (check_idx && !(block.index() ==
        TIndex(((uint8*)buffer_)+(sizeof(uint32)+sizeof(uint32)))))
  {
    std::ostringstream out;
    out<<"File_Blocks::read_block: Index inconsistent at offset "<<((int64)(block.pos()) * block_size + 8);
    throw File_Error(block.pos(), rd_idx ? rd_idx->get_data_file_name() : wr_idx->get_data_file_name(), out.str());
  }
  ++read_count_;
  ++global_read_counter();
  return buffer_;
}


template< typename TIndex, typename TIterator >
uint64* File_Blocks< TIndex, TIterator >::read_block
    (const File_Blocks_Basic_Iterator< TIndex >& it, bool check_idx) const
{
  return read_block_(it.block(), buffer.ptr, check_idx);
}


template< typename TIndex, typename TIterator >
uint64* File_Blocks< TIndex, TIterator >::read_block
    (const File_Blocks_Basic_Iterator< TIndex >& it, uint64* buffer_, bool check_idx) const
{
  return read_block_(it.block(), buffer_, check_idx);
}


template< typename Iterator, typename Index >
struct Write_Iterator_Adapter
{
  Write_Iterator_Adapter(const Iterator& it_) : it(it_) {}

  Index index() const { return it.block().index; }
  uint32 pos() const { return it.block().pos; }
  uint32 size() const { return it.block().size; }
  uint32 max_keysize() const { return it.block().max_keysize; }
  
private:
  Iterator it;
};


template< typename TIndex, typename TIterator >
uint64* File_Blocks< TIndex, TIterator >::read_block
    (const File_Blocks_Write_Iterator< TIndex, TIterator >& it, bool check_idx) const
{
  return read_block_(
      Write_Iterator_Adapter< File_Blocks_Write_Iterator< TIndex, TIterator >, TIndex >(it),
      buffer.ptr, check_idx);
}


template< typename TIndex, typename TIterator >
uint64* File_Blocks< TIndex, TIterator >::read_block
    (const File_Blocks_Write_Iterator< TIndex, TIterator >& it, uint64* buffer_, bool check_idx) const
{
  return read_block_(
      Write_Iterator_Adapter< File_Blocks_Write_Iterator< TIndex, TIterator >, TIndex >(it),
      buffer_, check_idx);
}


// Finds an appropriate block, removes it from the list of available blocks, and returns it
template< typename TIndex, typename TIterator >
uint32 File_Blocks< TIndex, TIterator >::allocate_block(uint32 data_size)
{
  uint32 result = this->wr_idx->get_block_count();

  if (this->wr_idx->get_void_blocks().empty())
    this->wr_idx->increase_block_count(data_size);
  else
  {
    std::vector< std::pair< uint32, uint32 > >::iterator pos_it
        = std::lower_bound(this->wr_idx->get_void_blocks().begin(), this->wr_idx->get_void_blocks().end(),
			   std::make_pair(data_size, uint32(0)));

    if (pos_it != this->wr_idx->get_void_blocks().end() && pos_it->first == data_size)
    {
      // We have a gap of exactly the needed size.
      result = pos_it->second;
      this->wr_idx->get_void_blocks().erase(pos_it);
    }
    else
    {
      pos_it = --(this->wr_idx->get_void_blocks().end());
      uint32 last_size = pos_it->first;
      while (pos_it != this->wr_idx->get_void_blocks().begin() && last_size > data_size)
      {
	--pos_it;
	if (last_size == pos_it->first)
	{
	  // We have a gap size that appears twice (or more often).
	  // This is a good heuristic choice.
	  result = pos_it->second;
	  pos_it->first -= data_size;
	  pos_it->second += data_size;
	  rearrange_block(this->wr_idx->get_void_blocks().begin(), pos_it, *pos_it);
	  return result;
	}
	last_size = pos_it->first;
      }

      pos_it = --(this->wr_idx->get_void_blocks().end());
      if (pos_it->first >= data_size)
      {
	// If no really matching block exists then we choose the largest one.
	result = pos_it->second;
	pos_it->first -= data_size;
	pos_it->second += data_size;
	rearrange_block(this->wr_idx->get_void_blocks().begin(), pos_it, *pos_it);
      }
      else
	this->wr_idx->increase_block_count(data_size);
    }
  }

  return result;
}


template< typename TIndex, typename TIterator >
void File_Blocks< TIndex, TIterator >::write_block(uint64* buf, uint32 payload_size, uint32& block_count, uint32& pos)
{
  void* payload = buf;
  if (compression_method == File_Blocks_Index_Base::ZLIB_COMPRESSION)
  {
    payload = buffer.ptr;
    block_count = (
        Zlib_Deflate(1).compress(buf, payload_size, payload, block_size * compression_factor)
        - 1) / block_size + 1;
  }
  else if (compression_method == File_Blocks_Index_Base::LZ4_COMPRESSION)
  {
    payload = buffer.ptr;
    block_count = (
        LZ4_Deflate().compress(buf, payload_size, payload, block_size * compression_factor * 2)
        - 1) / block_size + 1;
  }

  pos = allocate_block(block_count);

  data_file.seek(((int64)pos)*block_size, "File_Blocks::write_block::1");
  data_file.write((uint8*)payload, block_size * block_count, "File_Blocks::write_block::2");
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::insert_block
    (const Write_Iterator& it, uint64* buf, uint32 max_keysize)
{
  return insert_block(it, buf, *(uint32*)buf, max_keysize, TIndex((void*)(buf+1)));
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::insert_block
    (const Write_Iterator& it, uint64* buf, uint32 payload_size, uint32 max_keysize, const TIndex& block_idx)
{
  if (buf == 0)
    return it;

  uint32 data_size = payload_size == 0 ? 0 : (payload_size - 1) / block_size + 1;
  uint32 pos;
  if (payload_size < block_size * compression_factor)
    memset(((uint8*)buf) + payload_size, 0, block_size * compression_factor - payload_size);
  write_block(buf, payload_size, data_size, pos);

  Write_Iterator return_it = it;
  return_it.insert_block(*wr_idx, File_Block_Index_Entry< TIndex >(block_idx, pos, data_size, max_keysize));
  return_it.is_empty = it.is_empty;
  return return_it;
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::replace_block
    (const Write_Iterator& it, uint64* buf, uint32 max_keysize)
{
  return replace_block(it, buf, *(uint32*)buf, max_keysize, TIndex((void*)(buf+1)));
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::replace_block
    (Write_Iterator it, uint64* buf, uint32 payload_size, uint32 max_keysize, const TIndex& block_idx)
{
  if (!buf)
    return erase_block(it);

  uint32 data_size = payload_size == 0 ? 0 : (payload_size - 1) / block_size + 1;
  if (payload_size < block_size * compression_factor)
    memset(((uint8*)buf) + payload_size, 0, block_size * compression_factor - payload_size);
  uint32 pos = 0;
  write_block(buf, payload_size, data_size, pos);

  it.set_block(*wr_idx, File_Block_Index_Entry< TIndex >(block_idx, pos, data_size, max_keysize));
  return it;
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::erase_block(Write_Iterator it)
{
  it.erase_block(*wr_idx);
  return it;
}


template< typename TIndex, typename TIterator >
void File_Blocks< TIndex, TIterator >::erase_blocks(
    Write_Iterator& block_it, const Write_Iterator& it)
{
  block_it.erase_blocks(*wr_idx, it);
}


#endif
