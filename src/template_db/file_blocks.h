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
  File_Blocks_Basic_Iterator(const File_Blocks_Index_Iterator< TIndex >& begin)
      : block_it(begin) {}

  File_Blocks_Basic_Iterator(const File_Blocks_Basic_Iterator& a)
      : block_it(a.block_it) {}

  bool is_end() const { return block_it.is_end(); }

  const File_Blocks_Index_Iterator< TIndex >& block() const { return block_it; }

protected:
  File_Blocks_Index_Iterator< TIndex > block_it;
};


template< typename TIndex >
struct File_Blocks_Flat_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Flat_Iterator(const File_Blocks_Index_Iterator< TIndex >& begin)
    : File_Blocks_Basic_Iterator< TIndex >(begin) {}

  File_Blocks_Flat_Iterator(const File_Blocks_Flat_Iterator& a)
    : File_Blocks_Basic_Iterator< TIndex >(a) {}

  ~File_Blocks_Flat_Iterator() {}

  const File_Blocks_Flat_Iterator& operator=
      (const File_Blocks_Flat_Iterator& a);
  bool operator==(const File_Blocks_Flat_Iterator& a) const;
  File_Blocks_Flat_Iterator& operator++();
};


template< typename TIndex, typename TIterator >
struct File_Blocks_Discrete_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Discrete_Iterator
      (TIterator const& index_it_, TIterator const& index_end_,
       const File_Blocks_Index_Iterator< TIndex >& begin)
    : File_Blocks_Basic_Iterator< TIndex >(begin),
      index_lower(index_it_), index_upper(index_it_), index_end(index_end_)
  {
    find_next_block();
//     if (this->block_it.is_end())
//       std::cout<<"DEBUG Disc_Ctor end\n";
//     else
//       std::cout<<"DEBUG Disc_Ctor "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
  }

  File_Blocks_Discrete_Iterator
      (const File_Blocks_Index_Iterator< TIndex >& end)
    : File_Blocks_Basic_Iterator< TIndex >(end) {}

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

private:
  TIterator index_lower;
  TIterator index_upper;
  TIterator index_end;

  void find_next_block();
};


template< typename TIndex, typename TRangeIterator >
struct File_Blocks_Range_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Range_Iterator
      (const File_Blocks_Index_Iterator< TIndex >& begin,
       const TRangeIterator& index_it_,  const TRangeIterator& index_end_)
    : File_Blocks_Basic_Iterator< TIndex >(begin),
      index_it(index_it_), index_end(index_end_), index_equals_last_index(false)
  {
    find_next_block();
//     if (this->block_it.is_end())
//       std::cout<<"DEBUG Range_Ctor end\n";
//     else
//       std::cout<<"DEBUG Range_Ctor "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
  }

  File_Blocks_Range_Iterator
      (const File_Blocks_Index_Iterator< TIndex >& end)
    : File_Blocks_Basic_Iterator< TIndex >(end) {}

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
    index.get_void_blocks().release_block(block_it->pos, block_it->size);
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

  Write_Iterator insert_block(const Write_Iterator& it, uint64* buf);
  Write_Iterator insert_block(
      const Write_Iterator& it, uint64* buf, uint32 payload_size, const TIndex& block_idx);
  Write_Iterator replace_block(const Write_Iterator& it, uint64* buf);
  Write_Iterator replace_block(
      Write_Iterator it, uint64* buf, uint32 payload_size, const TIndex& block_idx);
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
  ++(this->block_it);
  find_next_block();
//   if (this->block_it.is_end())
//     std::cout<<"DEBUG Disc_++ end\n";
//   else
//     std::cout<<"DEBUG Disc_++ "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
  return *this;
}


template< typename TIndex, typename TIterator >
void File_Blocks_Discrete_Iterator< TIndex, TIterator >::find_next_block()
{
  while (index_lower != index_end)
  {
//     if (this->block_it.is_end())
//       std::cout<<"DEBUG find_next_block_A end\n";
//     else
//       std::cout<<"DEBUG find_next_block_A "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
    this->block_it.seek(*index_lower);
//     if (this->block_it.is_end())
//       std::cout<<"DEBUG find_next_block_B end\n";
//     else
//       std::cout<<"DEBUG find_next_block_B "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
    if (this->block_it.is_end())
    {
      index_upper = index_lower;
      while (index_upper != index_end)
        ++index_upper;
      return;
    }
    if (!index_lower->less(this->block_it.idx_ptr()))
    {
      if (!(*index_lower < *index_upper))
      {
        index_upper = index_lower;
        decltype(this->block_it) next = this->block_it;
        ++next;
        if (next.is_end())
        {
          while (index_upper != index_end)
            ++index_upper;
        }
        else
        {
          while (index_upper != index_end && index_upper->less(next.idx_ptr()))
            ++index_upper;
        }
      }
      return;
    }
    ++index_lower;
  }
//   if (this->block_it.is_end())
//     std::cout<<"DEBUG find_next_block_C end\n";
//   else
//     std::cout<<"DEBUG find_next_block_C "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
  this->block_it.set_end();
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


template< typename TIndex, typename TRangeIterator >
File_Blocks_Range_Iterator< TIndex, TRangeIterator >&
File_Blocks_Range_Iterator< TIndex, TRangeIterator >::operator++()
{
  ++(this->block_it);
  find_next_block();
//   if (this->block_it.is_end())
//     std::cout<<"DEBUG Range_++ end\n";
//   else
//     std::cout<<"DEBUG Range_++ "<<(void*)this->block_it.idx_ptr()<<' '<<std::hex<<this->block_it.pos()<<'\n';
  return *this;
}


// template< typename TIndex >
// void debug_dump(const TIndex& idx)
// {
//   static char buf[1024];
//   static const char* hex = "0123456789abcdef";
//   idx.to_data((void*)&buf);
//   int size = idx.size_of();
//   std::cout<<"DEBUG debug_dump";
//   for (unsigned int i = 0; i < size; ++i)
//     std::cout<<' '<<hex[(((int)buf[i])>>4)&0xf]<<hex[((int)buf[i])&0xf];
//   std::cout<<'\n';
// }


template< typename TIndex, typename TRangeIterator >
void File_Blocks_Range_Iterator< TIndex, TRangeIterator >::find_next_block()
{
  while (index_it != index_end)
  {
//     debug_dump(index_it.lower_bound());
    this->block_it.seek(index_it.lower_bound());
    if (this->block_it.is_end() || !index_it.upper_bound().leq(this->block_it.idx_ptr()))
      return;
    ++index_it;
  }
  this->block_it.set_end();
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
  index.get_void_blocks().release_block(to_delete->pos, to_delete->size);
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
  {
    index.get_void_blocks().release_block(block_it->pos, block_it->size);
    block_it = index.erase(block_it);
  }
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
    return Flat_Iterator(rd_idx->begin());
  return Flat_Iterator(wr_idx->begin());
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Flat_Iterator
    File_Blocks< TIndex, TIterator >::flat_end()
{
  if (rd_idx)
    return Flat_Iterator(rd_idx->end());
  return Flat_Iterator(wr_idx->end());
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator >::discrete_begin
    (const TIterator& begin, const TIterator& end)
{
  if (rd_idx)
    return File_Blocks_Discrete_Iterator< TIndex, TIterator >
        (begin, end, rd_idx->begin());
  return File_Blocks_Discrete_Iterator< TIndex, TIterator >
      (begin, end, wr_idx->begin());
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
        (rd_idx->begin(), begin, end);
  return File_Blocks_Range_Iterator< TIndex, Range_Iterator >
      (wr_idx->begin(), begin, end);
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
    posix_madvise(addr, length, POSIX_MADV_WILLNEED);
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
  if (sigterm_status())
    throw File_Error(0, "-", "SIGTERM received");

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


inline std::pair< uint64_t*, uint32_t > prepare_block_for_write(
    uint64_t* input, uint64_t* buffer, uint32_t payload_size,
    uint32_t block_size, uint32_t compression_factor, int compression_method)
{
  if (sigterm_status())
    throw File_Error(0, "-", "SIGTERM received");

  if (payload_size < block_size * compression_factor)
    memset(((uint8*)input) + payload_size, 0, block_size * compression_factor - payload_size);

  uint64_t* payload = input;
  if (compression_method == File_Blocks_Index_Base::ZLIB_COMPRESSION)
  {
    payload = buffer;
    payload_size = Zlib_Deflate(1).compress(input, payload_size, payload, block_size * compression_factor);
  }
  else if (compression_method == File_Blocks_Index_Base::LZ4_COMPRESSION)
  {
    payload = buffer;
    payload_size = LZ4_Deflate().compress(input, payload_size, payload, block_size * compression_factor * 2);
  }

  // compute block_count
  return { payload, payload_size == 0 ? 0 : (payload_size - 1) / block_size + 1 };
}


inline void write_prepared_block(
    Raw_File& data_file, std::pair< uint64_t*, uint32_t > buf_size, int64 pos_in_file, uint32_t block_size)
{
  data_file.seek(pos_in_file * block_size, "File_Blocks::write_block::1");
  data_file.write(buf_size.first, block_size * buf_size.second, "File_Blocks::write_block::2");
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::insert_block
    (const Write_Iterator& it, uint64* buf)
{
  return insert_block(it, buf, *(uint32*)buf, TIndex((void*)(buf+1)));
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::insert_block
    (const Write_Iterator& it, uint64* buf, uint32 payload_size, const TIndex& block_idx)
{
  if (buf == 0)
    return it;

  auto buf_size = prepare_block_for_write(
      (uint64_t*)buf, (uint64_t*)buffer.ptr, payload_size, block_size, compression_factor, compression_method);

  uint32_t pos = wr_idx->get_void_blocks().allocate_block(buf_size.second);
  write_prepared_block(data_file, buf_size, pos, block_size);

  Write_Iterator return_it = it;
  return_it.insert_block(*wr_idx, File_Block_Index_Entry< TIndex >(block_idx, pos, buf_size.second));
  return_it.is_empty = it.is_empty;
  return return_it;
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::replace_block
    (const Write_Iterator& it, uint64* buf)
{
  return replace_block(it, buf, *(uint32*)buf, TIndex((void*)(buf+1)));
}


template< typename TIndex, typename TIterator >
typename File_Blocks< TIndex, TIterator >::Write_Iterator
    File_Blocks< TIndex, TIterator >::replace_block
    (Write_Iterator it, uint64* buf, uint32 payload_size, const TIndex& block_idx)
{
  if (!buf)
    return erase_block(it);

  auto buf_size = prepare_block_for_write(
      (uint64_t*)buf, (uint64_t*)buffer.ptr, payload_size, block_size, compression_factor, compression_method);

  uint32_t pos = wr_idx->get_void_blocks().allocate_block(buf_size.second);
  write_prepared_block(data_file, buf_size, pos, block_size);

  it.set_block(*wr_idx, File_Block_Index_Entry< TIndex >(block_idx, pos, buf_size.second));
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
