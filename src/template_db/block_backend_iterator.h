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

#ifndef DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_ITERATOR_H
#define DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_ITERATOR_H

#include "file_blocks.h"

#include <cstring>
#include <map>
#include <set>


template< class TIndex >
struct Default_Range_Iterator : std::set< std::pair< TIndex, TIndex > >::const_iterator
{
  Default_Range_Iterator
      (const typename std::set< std::pair< TIndex, TIndex > >::const_iterator it)
  : std::set< std::pair< TIndex, TIndex > >::const_iterator(it) {}
  
  Default_Range_Iterator() {}
  
  const TIndex& lower_bound() const { return (*this)->first; }
  const TIndex& upper_bound() const { return (*this)->second; }
};


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
struct Block_Backend_Basic_Iterator
{
  Block_Backend_Basic_Iterator(uint32 block_size_, bool is_end);
  Block_Backend_Basic_Iterator(const Block_Backend_Basic_Iterator& it);
  ~Block_Backend_Basic_Iterator();
  
  bool advance();  
  const TIndex& index();
  const TObject& object();
  
  int read_whole_key(std::vector< TObject >& result_values);
  
  uint32 block_size;
  uint32 pos;
  uint32* current_idx_pos;
  TIndex* current_index;
  TObject* current_object;
  
  Void_Pointer< uint8 > buffer;
};


template< class TIndex, class TObject, class TIterator >
struct Block_Backend_Flat_Iterator : Block_Backend_Basic_Iterator< TIndex, TObject >
{
  typedef File_Blocks< TIndex, TIterator, Default_Range_Iterator< TIndex > > File_Blocks_;
  
  Block_Backend_Flat_Iterator
      (File_Blocks_& file_blocks_, uint32 block_size_, bool is_end = false);
  
  Block_Backend_Flat_Iterator(const Block_Backend_Flat_Iterator& it)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(it),
      file_blocks(it.file_blocks), file_it(it.file_it), file_end(it.file_end) {}
      
  ~Block_Backend_Flat_Iterator() {}
  
  const Block_Backend_Flat_Iterator& operator=(const Block_Backend_Flat_Iterator& it);
  bool operator==(const Block_Backend_Flat_Iterator& it) const;
  const Block_Backend_Flat_Iterator& operator++();
  
  const File_Blocks< TIndex, TIterator, Default_Range_Iterator< TIndex > >& file_blocks;
  typename File_Blocks_::Flat_Iterator file_it;
  typename File_Blocks_::Flat_Iterator file_end;
  
private:
  // returns true if we have found something
  bool search_next_index();
  
  // returns true if we are done
  // if we have loaded a new block, returns false to trigger search_next_index()
  bool read_block();
};


template< class TIndex, class TObject, class TIterator >
struct Block_Backend_Discrete_Iterator : Block_Backend_Basic_Iterator< TIndex, TObject >
{
  typedef File_Blocks< TIndex, TIterator, Default_Range_Iterator< TIndex > > File_Blocks_;
  
  Block_Backend_Discrete_Iterator
      (File_Blocks_& file_blocks_,
       const TIterator& index_it_,
       const TIterator& index_end_, uint32 block_size_);
  
  Block_Backend_Discrete_Iterator
      (const File_Blocks_& file_blocks_, uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, true),
      file_blocks(file_blocks_), file_it(file_blocks_.discrete_end()),
      file_end(file_blocks_.discrete_end()), index_it(), index_end() {}
  
  Block_Backend_Discrete_Iterator(const Block_Backend_Discrete_Iterator& it)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(it),
      file_blocks(it.file_blocks), file_it(it.file_it), file_end(it.file_end),
      index_it(it.index_it), index_end(it.index_end) {}
  
  ~Block_Backend_Discrete_Iterator() {}
  
  const Block_Backend_Discrete_Iterator& operator=
      (const Block_Backend_Discrete_Iterator& it);  
  bool operator==(const Block_Backend_Discrete_Iterator& it) const;
  const Block_Backend_Discrete_Iterator& operator++();
  
  const File_Blocks< TIndex, TIterator, Default_Range_Iterator< TIndex > >& file_blocks;
  typename File_Blocks_::Discrete_Iterator file_it;
  typename File_Blocks_::Discrete_Iterator file_end;
  TIterator index_it;
  TIterator index_end;
  
private:
  bool search_next_index();
  bool read_block();
};


template< class TIndex, class TObject, class TIterator >
struct Block_Backend_Range_Iterator : Block_Backend_Basic_Iterator< TIndex, TObject >
{
  typedef File_Blocks< TIndex, TIterator, Default_Range_Iterator< TIndex > > File_Blocks_;
  
  Block_Backend_Range_Iterator
      (File_Blocks_& file_blocks_,
       const Default_Range_Iterator< TIndex >& index_it_,
       const Default_Range_Iterator< TIndex >& index_end_, uint32 block_size_);
  
  Block_Backend_Range_Iterator(const File_Blocks_& file_blocks_, uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, true),
      file_blocks(file_blocks_), file_it(file_blocks_.range_end()),
      file_end(file_blocks_.range_end()), index_it(), index_end() {}
  
  Block_Backend_Range_Iterator(const Block_Backend_Range_Iterator& it)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(it),
      file_blocks(it.file_blocks), file_it(it.file_it), file_end(it.file_end),
      index_it(it.index_it), index_end(it.index_end) {}
  
  const Block_Backend_Range_Iterator& operator=
      (const Block_Backend_Range_Iterator& it);
  bool operator==(const Block_Backend_Range_Iterator& it) const;
  const Block_Backend_Range_Iterator& operator++();
  
  const File_Blocks_& file_blocks;
  typename File_Blocks_::Range_Iterator file_it;
  typename File_Blocks_::Range_Iterator file_end;
  Default_Range_Iterator< TIndex > index_it;
  Default_Range_Iterator< TIndex > index_end;
  
private:
  // returns true if we have found something
  bool search_next_index();
  
  // returns true if we have found something
  bool read_block();
};


/** Implementation Block_Backend_Basic_Iterator: ----------------------------*/

template< class TIndex, class TObject >
Block_Backend_Basic_Iterator< TIndex, TObject >::
    Block_Backend_Basic_Iterator(uint32 block_size_, bool is_end)
: block_size(block_size_), pos(0), current_idx_pos(0), current_index(0),
  current_object(0), buffer(block_size)
{
  if (is_end)
    return;
}


template< class TIndex, class TObject >
Block_Backend_Basic_Iterator< TIndex, TObject >::
    Block_Backend_Basic_Iterator(const Block_Backend_Basic_Iterator& it)
: block_size(it.block_size), pos(it.pos),
  current_idx_pos(0), current_index(0), current_object(0), buffer(block_size)
{
  memcpy(buffer.ptr, it.buffer.ptr, block_size);
  current_idx_pos = (uint32*)(buffer.ptr + ((uint8*)it.current_idx_pos - it.buffer.ptr));
}


template< class TIndex, class TObject >
Block_Backend_Basic_Iterator< TIndex, TObject >::~Block_Backend_Basic_Iterator()
{
  if (current_index != 0)
  {
    delete current_index;
    current_index = 0;
  }
  if (current_object != 0)
  {
    delete current_object;
    current_object = 0;
  }
}


template< class TIndex, class TObject >
bool Block_Backend_Basic_Iterator< TIndex, TObject >::advance()
{
  pos += TObject::size_of((void*)(buffer.ptr + pos));
  
  // invalidate cached object
  if (current_object != 0)
  {
    delete current_object;
    current_object = 0;
  }
  
  // if we have still the same index, we're done
  if (pos < *current_idx_pos)
    return true;
  
  // invalidate cached index
  if (current_index != 0)
  {
    delete current_index;
    current_index = 0;
  }
  
  return false;
}


template< class TIndex, class TObject >
const TIndex& Block_Backend_Basic_Iterator< TIndex, TObject >::index()
{
  if (current_index == 0)
    current_index = new TIndex((void*)(current_idx_pos + 1));
  return *current_index;
}


template< class TIndex, class TObject >
const TObject& Block_Backend_Basic_Iterator< TIndex, TObject >::object()
{
  if (current_object == 0)
    current_object = new TObject((void*)(buffer.ptr + pos));
  return *current_object;
}


/** Implementation Block_Backend_Flat_Iterator: ----------------------------*/

template< class TIndex, class TObject, class TIterator >
Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >::
    Block_Backend_Flat_Iterator
    (File_Blocks_& file_blocks_, uint32 block_size_, bool is_end)
  : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, is_end),
    file_blocks(file_blocks_), file_it(file_blocks_.flat_begin()),
    file_end(file_blocks_.flat_end())
{
  if (is_end)
  {
    file_it = file_end;
    return;
  }
  
  if (read_block())
    return;
  while (true)
  {
    if (search_next_index())
      return;
    
    ++file_it;
    if (read_block())
      return;
  }
}


template< class TIndex, class TObject, class TIterator >
const Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >& Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >::operator=
    (const Block_Backend_Flat_Iterator& it)
{
  if (this == &it)
    return *this;
  this->~Block_Backend_Flat_Iterator();
  new (this) Block_Backend_Flat_Iterator(it);
  return *this;
}


template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >::operator==
    (const Block_Backend_Flat_Iterator& it) const
{
  bool res((this->pos == it.pos) && (file_it == it.file_it));
  return (res);
}


template< class TIndex, class TObject, class TIterator >
const Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >&
    Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >::operator++()
{
  if (this->advance())
    return *this;
  
  while (true)
  {
    if (search_next_index())
      return *this;
    
    ++file_it;
    if (read_block())
      return *this;
  }
}


// returns true if we have found something
template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >::search_next_index()
{
  // search for the next suitable index
  this->current_idx_pos = (uint32*)((this->buffer.ptr) + this->pos);
  if (this->pos < *(uint32*)(this->buffer.ptr))
  {
    if (this->current_index)
      delete this->current_index;
    this->current_index = new TIndex((void*)(this->current_idx_pos + 1));
    typename File_Blocks_::Flat_Iterator next_it(file_it);
    if (file_it.is_out_of_range(*this->current_index))
      throw File_Error(file_it.block_it->pos,
		file_blocks.get_index().get_data_file_name(),
	        "Block_Backend: index out of range.");
    this->pos += 4;
    this->pos += TIndex::size_of((void*)((this->buffer.ptr) + this->pos));
    return true;
  }
    
  return false;
}


// returns true if we are done
// if we have loaded a new block, returns false to trigger search_next_index()
template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Flat_Iterator< TIndex, TObject, TIterator >::read_block()
{
  if (file_it == file_end)
  {
    // there is no block left
    this->pos = 0;
    return true;
  }
  this->pos = 4;
  file_blocks.read_block(file_it, this->buffer.ptr);
  
  return false;
}


/** Implementation Block_Backend_Discrete_Iterator: -------------------------*/

template< class TIndex, class TObject, class TIterator >
Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >::
    Block_Backend_Discrete_Iterator
    (File_Blocks_& file_blocks_,
     const TIterator& index_it_, const TIterator& index_end_,
     uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, false),
      file_blocks(file_blocks_),
      file_it(file_blocks_.discrete_begin(index_it_, index_end_)),
      file_end(file_blocks_.discrete_end()),
      index_it(index_it_), index_end(index_end_)
{
  if (read_block())
    return;
  while (true)
  {
    if (search_next_index())
      return;
    
    ++file_it;
    if (read_block())
      return;
  }
}


template< class TIndex, class TObject, class TIterator >
const Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >&
    Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >::
    operator=(const Block_Backend_Discrete_Iterator& it)
{
  if (this == &it)
    return *this;
  this->~Block_Backend_Discrete_Iterator();
  new (this) Block_Backend_Discrete_Iterator(it);
  return *this;
}


template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >::operator==
    (const Block_Backend_Discrete_Iterator& it) const
{
  bool res((this->pos == it.pos) && (file_it == it.file_it));
  return (res);
}
  

template< class TIndex, class TObject, class TIterator >
const Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >&
    Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >::operator++()
{
  if (this->advance())
    return *this;
  
  while (true)
  {
    if (search_next_index())
      return *this;
    
    ++file_it;
    if (read_block())
      return *this;
  }
}


// returns true if we have found something
template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >::search_next_index()
{
  // search for the next suitable index
  this->current_idx_pos = (uint32*)((this->buffer.ptr) + this->pos);
  while (this->pos < *(uint32*)(this->buffer.ptr))
  {
    this->pos += 4;
    
    if (this->current_index)
      delete this->current_index;
    this->current_index = new TIndex((void*)((this->buffer.ptr) + this->pos));
    while ((index_it != index_end) && (*index_it < *(this->current_index)))
      ++index_it;
    if (index_it == index_end)
    {
      // there cannot be data anymore, because there is no valid index left
      file_it = file_end;
      this->pos = 0;
      return true;
    }
    if (*index_it == *(this->current_index))
    {
      // we have reached the next valid index
      this->pos += TIndex::size_of((void*)((this->buffer.ptr) + this->pos));
      return true;
    }
    delete this->current_index;
    this->current_index = 0;
    
    this->pos = *(this->current_idx_pos);
    this->current_idx_pos = (uint32*)((this->buffer.ptr) + this->pos);
  }
  
  return false;
}


// returns true if we have found something
template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Discrete_Iterator< TIndex, TObject, TIterator >::read_block()
{
  // we need to load a new block
  // skip empty blocks
  while ((!(file_it == file_end)) &&
    (file_it.block_type() == File_Block_Index_Entry< TIndex >::EMPTY))
    ++file_it;
  if (file_it == file_end)
  {
    // there is no block left
    this->pos = 0;
    return true;
  }
  this->pos = 4;
  file_blocks.read_block(file_it, this->buffer.ptr);
  
  return false;
}


/** Implementation Block_Backend_Range_Iterator: -------------------------*/

template< class TIndex, class TObject, class TIterator >
Block_Backend_Range_Iterator< TIndex, TObject, TIterator >::Block_Backend_Range_Iterator
    (File_Blocks_& file_blocks_,
     const Default_Range_Iterator< TIndex >& index_it_,
     const Default_Range_Iterator< TIndex >& index_end_, uint32 block_size_)
  : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, false),
    file_blocks(file_blocks_),
    file_it(file_blocks_.range_begin(index_it_, index_end_)),
    file_end(file_blocks_.range_end()),
    index_it(index_it_), index_end(index_end_)
{
  if (read_block())
    return;
  while (true)
  {
    if (search_next_index())
      return;
    
    ++file_it;
    if (read_block())
      return;
  }
}


template< class TIndex, class TObject, class TIterator >
const Block_Backend_Range_Iterator< TIndex, TObject, TIterator >& 
    Block_Backend_Range_Iterator< TIndex, TObject, TIterator >::operator=
    (const Block_Backend_Range_Iterator& it)
{
  if (this == &it)
    return *this;
  this->~Block_Backend_Range_Iterator();
  new (this) Block_Backend_Range_Iterator(it);
  return *this;
}


template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Range_Iterator< TIndex, TObject, TIterator >::operator==
    (const Block_Backend_Range_Iterator& it) const
{
  return ((this->pos == it.pos) && (file_it == it.file_it));
}


template< class TIndex, class TObject, class TIterator >
const Block_Backend_Range_Iterator< TIndex, TObject, TIterator >&
    Block_Backend_Range_Iterator< TIndex, TObject, TIterator >::operator++()
{
  if (this->advance())
    return *this;
  
  while (true)
  {
    if (search_next_index())
      return *this;
    
    ++file_it;
    if (read_block())
      return *this;
  }
}


// returns true if we have found something
template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Range_Iterator< TIndex, TObject, TIterator >::search_next_index()
{
  // search for the next suitable index
  this->current_idx_pos = (uint32*)((this->buffer.ptr) + this->pos);
  while (this->pos < *(uint32*)(this->buffer.ptr))
  {
    this->pos += 4;
    
    if (this->current_index)
      delete this->current_index;
    this->current_index = new TIndex((void*)((this->buffer.ptr) + this->pos));
    while ((index_it != index_end) &&
      (!(*(this->current_index) < index_it.upper_bound())))
      ++(index_it);
    if (index_it == index_end)
    {
      // there cannot be data anymore, because there is no valid index left
      file_it = file_end;
      this->pos = 0;
      return true;
    }
    if (!(*(this->current_index) < index_it.lower_bound()))
    {
      // we have reached the next valid index
      this->pos += TIndex::size_of((void*)((this->buffer.ptr) + this->pos));
      return true;
    }
    delete this->current_index;
    this->current_index = 0;
    
    this->pos = *(this->current_idx_pos);
    this->current_idx_pos = (uint32*)((this->buffer.ptr) + this->pos);
  }
  
  return false;
}


// returns true if we have found something
template< class TIndex, class TObject, class TIterator >
bool Block_Backend_Range_Iterator< TIndex, TObject, TIterator >::read_block()
{
  // we need to load a new block
  if (file_it == file_end)
  {
    // there is no block left
    this->pos = 0;
    return true;
  }
  this->pos = 4;
  file_blocks.read_block(file_it, this->buffer.ptr);
  
  return false;
}


#endif
