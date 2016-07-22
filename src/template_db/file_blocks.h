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

#ifndef DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_H
#define DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_H

#include "file_blocks_index.h"
#include "types.h"
#include "lz4_wrapper.h"
#include "zlib_wrapper.h"

#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <list>

/** Declarations: -----------------------------------------------------------*/


template< typename TIndex >
struct File_Blocks_Basic_Iterator
{
  File_Blocks_Basic_Iterator
  (const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& begin,
   const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : block_begin(begin), block_it(begin), block_end(end), is_empty(false) {}
    
  File_Blocks_Basic_Iterator(const File_Blocks_Basic_Iterator& a)
  : block_begin(a.block_begin), block_it(a.block_it), block_end(a.block_end),
    is_empty(a.is_empty) {}
    
  int block_type() const;
   
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator block_begin;
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator block_it;
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator block_end;
  bool is_empty;
};


template< typename TIndex >
struct File_Blocks_Flat_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Flat_Iterator
  (const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& begin,
   const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end)
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
       const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& begin,
       const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end),
      index_lower(index_it_), index_upper(index_it_), index_end(index_end_),
      just_inserted(false)
  {
    find_next_block();
  }
  
  File_Blocks_Discrete_Iterator
      (const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : File_Blocks_Basic_Iterator< TIndex >(end, end) {}
  
  File_Blocks_Discrete_Iterator(const File_Blocks_Discrete_Iterator& a)
    : File_Blocks_Basic_Iterator< TIndex >(a),
      index_lower(a.index_lower), index_upper(a.index_upper),
      index_end(a.index_end), just_inserted(a.just_inserted) {}
    
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
  bool just_inserted;
  
private:
  void find_next_block();
};


template< typename TIndex, typename TRangeIterator >
struct File_Blocks_Range_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Range_Iterator
      (const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& begin,
       const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end,
       const TRangeIterator& index_it_,  const TRangeIterator& index_end_)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end),
      index_it(index_it_), index_end(index_end_)
  {
    find_next_block();
  }
  
  File_Blocks_Range_Iterator
      (const typename std::list< File_Block_Index_Entry< TIndex > >::iterator& end)
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

  void find_next_block();
};


template< typename TIndex, typename TIterator, typename TRangeIterator >
struct File_Blocks
{
  typedef File_Blocks_Flat_Iterator< TIndex > Flat_Iterator;
  typedef File_Blocks_Discrete_Iterator< TIndex, TIterator > Discrete_Iterator;
  typedef File_Blocks_Range_Iterator< TIndex, TRangeIterator > Range_Iterator;
  
private:
  File_Blocks(const File_Blocks& f) {}
  
public:
  File_Blocks(File_Blocks_Index_Base* index);  
  ~File_Blocks();
  
  Flat_Iterator flat_begin();
  const Flat_Iterator& flat_end() const { return *flat_end_it; }  
  Discrete_Iterator discrete_begin(const TIterator& begin, const TIterator& end);
  const Discrete_Iterator& discrete_end() const { return *discrete_end_it; }
  Range_Iterator range_begin(const TRangeIterator& begin, const TRangeIterator& end);
  const Range_Iterator& range_end() const { return *range_end_it; }
  
  void* read_block(const File_Blocks_Basic_Iterator< TIndex >& it) const;  
  void* read_block
      (const File_Blocks_Basic_Iterator< TIndex >& it, void* buffer) const;
      
  uint32 answer_size(const Flat_Iterator& it) const
  {
    return (block_size * it.block_it->size - sizeof(uint32));
  }
  uint32 answer_size(const Discrete_Iterator& it) const;  
  uint32 answer_size(const Range_Iterator& it) const
  {
    return (block_size * it.block_it->size - sizeof(uint32));
  }
  
  uint read_count() const { return read_count_; }
  void reset_read_count() { read_count_ = 0; }
  
  Discrete_Iterator insert_block
      (const Discrete_Iterator& it, void* buf, uint32 max_keysize);
  Discrete_Iterator replace_block(Discrete_Iterator it, void* buf, uint32 max_keysize);
  
  const File_Blocks_Index< TIndex >& get_index() const { return *index; }
  
private:
  File_Blocks_Index< TIndex >* index;
  uint32 block_size;
  uint32 max_size;
  int compression_method;
  bool writeable;
  mutable uint read_count_;
  
  Flat_Iterator* flat_end_it;
  Discrete_Iterator* discrete_end_it;
  Range_Iterator* range_end_it;

  Raw_File data_file;
  Void_Pointer< void > buffer;
  
  uint32 allocate_block(uint32 data_size);
};


/** Implementation File_Blocks_Basic_Iterator: ------------------------------*/

template< typename TIndex >
int File_Blocks_Basic_Iterator< TIndex >::block_type() const
{
  if ((block_it == block_end) || (is_empty))
    return File_Block_Index_Entry< TIndex >::EMPTY;
  typename std::list< File_Block_Index_Entry< TIndex > >::const_iterator
      it(block_it);
  if (block_it == block_begin)
  {
    if (++it == block_end)
      return File_Block_Index_Entry< TIndex >::GROUP;
    else if (block_it->index == it->index)
      return File_Block_Index_Entry< TIndex >::SEGMENT;
    else
      return File_Block_Index_Entry< TIndex >::GROUP;
  }
  ++it;
  if (it == block_end)
  {
    --it;
    --it;
    if (it->index == block_it->index)
      return File_Block_Index_Entry< TIndex >::LAST_SEGMENT;
    else
      return File_Block_Index_Entry< TIndex >::GROUP;
  }
  if (it->index == block_it->index)
    return File_Block_Index_Entry< TIndex >::SEGMENT;
  --it;
  --it;
  if (it->index == block_it->index)
    return File_Block_Index_Entry< TIndex >::LAST_SEGMENT;
  else
    return File_Block_Index_Entry< TIndex >::GROUP;
}


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
  return (this->block_it == a.block_it);
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
  typename std::list< File_Block_Index_Entry< TIndex > >::iterator
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
  return ((this->block_it == a.block_it) && (this->is_empty == a.is_empty));
}


template< typename TIndex, typename TIterator >
File_Blocks_Discrete_Iterator< TIndex, TIterator >&
File_Blocks_Discrete_Iterator< TIndex, TIterator >::operator++()
{
  if (just_inserted)
  {
    just_inserted = false;
    ++(this->block_it);
    return *this;
  }
  
  int block_type(this->block_type());
  if (block_type == File_Block_Index_Entry< TIndex >::EMPTY)
  {
    this->is_empty = false;
    find_next_block();
    return *this;
  }
  if (block_type == File_Block_Index_Entry< TIndex >::SEGMENT)
  {
    ++(this->block_it);
    return *this;
  }
  
  ++(this->block_it);
  find_next_block();
  return *this;
}


template< typename TIndex, typename TIterator >
void File_Blocks_Discrete_Iterator< TIndex, TIterator >::find_next_block()
{
  index_lower = index_upper;
  if (index_lower == index_end)
  {
    this->block_it = this->block_end;
    return;
  }
  
  if (this->block_it == this->block_end)
  {
    this->is_empty = true;
    index_upper = index_end;
    return;
  }
  
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
  
  if (this->block_type() == File_Block_Index_Entry< TIndex >::LAST_SEGMENT)
  {
    while ((!(index_upper == index_end)) && (*index_upper < next_block->index))
      ++index_upper;
    ++(this->block_it);
    this->is_empty = true;
  }
  
  while ((index_upper != index_end) && (*index_upper < next_block->index))
    ++index_upper;
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
  return *this;
}


template< typename TIndex, typename TRangeIterator >
void File_Blocks_Range_Iterator< TIndex, TRangeIterator >::find_next_block()
{
  if (this->block_it == this->block_end)
    // We are done - there are no more file blocks left
    return;
  
  while (true)
  {
    while ((index_it != index_end) &&
      (!(this->block_it->index < index_it.upper_bound())))
      ++index_it;
    
    if (index_it == index_end)
    {
      // We are done - there are no more indices left
      this->block_it = this->block_end;
      return;
    }
    
    typename std::list< File_Block_Index_Entry< TIndex > >::const_iterator
    next_block(this->block_it);
    ++next_block;
    while ((next_block != this->block_end) &&
      (!(index_it.lower_bound() < next_block->index)))
    {
      if (!(this->block_it->index < index_it.lower_bound()))
	// We have found a relevant block that is a segment
	return;
      ++(this->block_it);
      ++next_block;
    }
    
    if ((this->block_type() != File_Block_Index_Entry< TIndex >::LAST_SEGMENT)
      || (!(this->block_it->index < index_it.lower_bound())))
      break;
    
    ++(this->block_it);
    if (this->block_it == this->block_end)
      break;
  }
}


/** Implementation File_Blocks: ---------------------------------------------*/

template< typename TIndex, typename TIterator, typename TRangeIterator >
File_Blocks< TIndex, TIterator, TRangeIterator >::File_Blocks
    (File_Blocks_Index_Base* index_) : 
     index((File_Blocks_Index< TIndex >*)index_),
     block_size(index->get_block_size()),
     max_size(index->get_max_size()),
     compression_method(index->get_compression_method()),
     writeable(index->writeable()),
     read_count_(0),
     data_file(index->get_data_file_name(),
	       writeable ? O_RDWR|O_CREAT : O_RDONLY,
	       S_666, "File_Blocks::File_Blocks::1"),
     buffer(index->get_block_size() * index->get_max_size() * 2)      // increased buffer size for lz4
{
  // cerr<<"  "<<index->get_data_file_name()<<'\n'; //Debug
  
  // prepare standard iterators
  flat_end_it = new Flat_Iterator(index->blocks.end(), index->blocks.end());
  discrete_end_it = new Discrete_Iterator(index->blocks.end());
  range_end_it = new Range_Iterator(index->blocks.end());
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
File_Blocks< TIndex, TIterator, TRangeIterator >::~File_Blocks()
{
  delete flat_end_it;
  delete discrete_end_it;
  delete range_end_it;

  // cerr<<"~ "<<index->get_data_file_name()<<'\n'; //Debug
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Flat_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::flat_begin()
{
  return Flat_Iterator(index->blocks.begin(), index->blocks.end());
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::discrete_begin
    (const TIterator& begin, const TIterator& end)
{
  return File_Blocks_Discrete_Iterator< TIndex, TIterator >
      (begin, end, index->blocks.begin(), index->blocks.end());
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Range_Iterator
File_Blocks< TIndex, TIterator, TRangeIterator >::range_begin(const TRangeIterator& begin, const TRangeIterator& end)
{
  return File_Blocks_Range_Iterator< TIndex, TRangeIterator >
      (index->blocks.begin(), index->blocks.end(), begin, end);
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
void* File_Blocks< TIndex, TIterator, TRangeIterator >::read_block
    (const File_Blocks_Basic_Iterator< TIndex >& it) const
{
  data_file.seek((int64)(it.block_it->pos) * block_size, "File_Blocks::read_block::1");
  if (compression_method == File_Blocks_Index< TIndex >::NO_COMPRESSION)
    data_file.read((uint8*)buffer.ptr, block_size * it.block_it->size, "File_Blocks::read_block::2");
  else if (compression_method == File_Blocks_Index< TIndex >::ZLIB_COMPRESSION)
  {
    Void_Pointer< void > input(block_size * it.block_it->size);
    data_file.read((uint8*)input.ptr, block_size * it.block_it->size, "File_Blocks::read_block::2");
    Zlib_Inflate().decompress(input.ptr, block_size * it.block_it->size, buffer.ptr, block_size * max_size);
  }
  else if (compression_method == File_Blocks_Index< TIndex >::LZ4_COMPRESSION)
  {
    Void_Pointer< void > input(block_size * it.block_it->size);
    data_file.read((uint8*)input.ptr, block_size * it.block_it->size, "File_Blocks::read_block::2");
    LZ4_Inflate().decompress(input.ptr, block_size * it.block_it->size, buffer.ptr, block_size * max_size);
  }

  ++read_count_;
  ++global_read_counter();
  return buffer.ptr;
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
void* File_Blocks< TIndex, TIterator, TRangeIterator >::read_block
    (const File_Blocks_Basic_Iterator< TIndex >& it, void* buffer_) const
{
  data_file.seek((int64)(it.block_it->pos) * block_size, "File_Blocks::read_block::3");
  
  if (compression_method == File_Blocks_Index< TIndex >::NO_COMPRESSION)
    data_file.read((uint8*)buffer_, block_size * it.block_it->size, "File_Blocks::read_block::4");
  else if (compression_method == File_Blocks_Index< TIndex >::ZLIB_COMPRESSION)
  {
    data_file.read((uint8*)buffer.ptr, block_size * it.block_it->size, "File_Blocks::read_block::4");
    Zlib_Inflate().decompress(buffer.ptr, block_size * it.block_it->size, buffer_, block_size * max_size);
  }
  else if (compression_method == File_Blocks_Index< TIndex >::LZ4_COMPRESSION)
  {
    data_file.read((uint8*)buffer.ptr, block_size * it.block_it->size, "File_Blocks::read_block::4");
    LZ4_Inflate().decompress(buffer.ptr, block_size * it.block_it->size, buffer_, block_size * max_size);
  }
  
  if (!(it.block_it->index ==
        TIndex(((uint8*)buffer_)+(sizeof(uint32)+sizeof(uint32)))))
    throw File_Error(it.block_it->pos, index->get_data_file_name(),
		     "File_Blocks::read_block: Index inconsistent");
  ++read_count_;
  ++global_read_counter();
  return buffer_;
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
uint32 File_Blocks< TIndex, TIterator, TRangeIterator >::answer_size
    (const Discrete_Iterator& it) const
{
  if (it.is_empty)
    return 0;
  
  uint32 count(0);
  TIterator index_it(it.lower_bound());
  while (index_it != it.upper_bound())
  {
    ++index_it;
    ++count;
  }
  
  if (count*(it.block_it->max_keysize) > block_size * it.block_it->size - sizeof(uint32))
    return (block_size * it.block_it->size - sizeof(uint32));
  else
    return count*(it.block_it->max_keysize);
}


// Finds an appropriate block, removes it from the list of available blocks, and returns it
template< typename TIndex, typename TIterator, typename TRangeIterator >
uint32 File_Blocks< TIndex, TIterator, TRangeIterator >::allocate_block(uint32 data_size)
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


template< typename TIndex, typename TIterator, typename TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::insert_block
    (const Discrete_Iterator& it, void* buf, uint32 max_keysize)
{
  if (buf == 0)
    return it;
  
  uint32 data_size = *(uint32*)buf == 0 ? 0 : ((*(uint32*)buf) - 1) / block_size + 1;
  
  void* target = buf;
  if (compression_method == File_Blocks_Index< TIndex >::ZLIB_COMPRESSION)
  {
    target = buffer.ptr;
    uint32 compressed_size = Zlib_Deflate(1).compress(buf, *(uint32*)buf, target, block_size * max_size);
    data_size = (compressed_size - 1) / block_size + 1;
    zero_padding((uint8*)target + compressed_size, block_size * data_size - compressed_size); 
  }
  else if (compression_method == File_Blocks_Index< TIndex >::LZ4_COMPRESSION)
  {
    target = buffer.ptr;
    uint32 compressed_size = LZ4_Deflate().compress(buf, *(uint32*)buf, target, block_size * max_size * 2);
    data_size = (compressed_size - 1) / block_size + 1;
    zero_padding((uint8*)target + compressed_size, block_size * data_size - compressed_size);
  }
    
  uint32 pos = allocate_block(data_size);
  
  // cerr<<dec<<pos<<"\t0x"; //Debug
  // for (uint i = 0; i < TIndex::size_of(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32))); ++i)
  //   cerr<<' '<<hex<<setw(2)<<setfill('0')
  //       <<int(*(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32))+i)); // Debug
  // cerr<<'\n';
  
  data_file.seek(((int64)pos)*block_size, "File_Blocks::insert_block::1");
  data_file.write((uint8*)target, block_size * data_size, "File_Blocks::insert_block::2");
  
  TIndex index(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32)));
  File_Block_Index_Entry< TIndex > entry(index, pos, data_size, max_keysize);
  Discrete_Iterator return_it(it);
  if (return_it.block_it == return_it.block_begin)
  {
    return_it.block_it = this->index->blocks.insert(return_it.block_it, entry);
    return_it.block_begin = return_it.block_it;
  }
  else
    return_it.block_it = this->index->blocks.insert(return_it.block_it, entry);
  return_it.just_inserted = true;
  return_it.is_empty = it.is_empty;
  return return_it;
}


template< typename TIndex, typename TIterator, typename TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::replace_block
    (Discrete_Iterator it, void* buf, uint32 max_keysize)
{
  if (buf != 0)
  {
    uint32 data_size = *(uint32*)buf == 0 ? 0 : ((*(uint32*)buf) - 1) / block_size + 1;
    
    void* target = buf;
    if (compression_method == File_Blocks_Index< TIndex >::ZLIB_COMPRESSION)
    {
      target = buffer.ptr;
      data_size = (Zlib_Deflate(1).compress(buf, *(uint32*)buf, target, block_size * max_size) - 1) / block_size + 1;
    }
    else if (compression_method == File_Blocks_Index< TIndex >::LZ4_COMPRESSION)
    {
      target = buffer.ptr;
      data_size = (LZ4_Deflate().compress(buf, *(uint32*)buf, target, block_size * max_size * 2) - 1) / block_size + 1;
    }
    
    it.block_it->pos = allocate_block(data_size);
    
    data_file.seek(((int64)it.block_it->pos)*block_size, "File_Blocks::replace_block::1");
    data_file.write((uint8*)target, block_size * data_size, "File_Blocks::replace_block::2");
    
    it.block_it->index = TIndex((uint8*)buf+(sizeof(uint32)+sizeof(uint32)));
    it.block_it->max_keysize = max_keysize;
    it.block_it->size = data_size;
    
    return it;
  }
  else
  {
    Discrete_Iterator return_it(it);
    ++return_it;
    if (it.block_it == it.block_begin)
    {
      it.block_it = index->blocks.erase(it.block_it);
      it.block_begin = it.block_it;
    }
    else
      it.block_it = index->blocks.erase(it.block_it);
    return return_it;
  }
}

#endif
