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

#ifndef DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_H
#define DE__OSM3S___TEMPLATE_DB__BLOCK_BACKEND_H

#include "file_blocks.h"

#include <cstring>
#include <map>
#include <set>

using namespace std;

template< class TIndex >
struct Default_Range_Iterator : set< pair< TIndex, TIndex > >::const_iterator
{
  Default_Range_Iterator
      (const typename set< pair< TIndex, TIndex > >::const_iterator it)
  : set< pair< TIndex, TIndex > >::const_iterator(it) {}
  
  Default_Range_Iterator() {}
  
  const TIndex& lower_bound() const { return (*this)->first; }
  const TIndex& upper_bound() const { return (*this)->second; }
};

template< class TIndex, class TObject >
struct Index_Collection
{
  Index_Collection(uint8* source_begin_, uint8* source_end_,
		   const typename map< TIndex, set< TObject > >::const_iterator& delete_it_,
		   const typename map< TIndex, set< TObject > >::const_iterator& insert_it_)
      : source_begin(source_begin_), source_end(source_end_),
        delete_it(delete_it_), insert_it(insert_it_) {}
  
  uint8* source_begin;
  uint8* source_end;
  typename map< TIndex, set< TObject > >::const_iterator delete_it;
  typename map< TIndex, set< TObject > >::const_iterator insert_it;
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
  
  template< typename T >
  T apply_func(T(*)(const void *));

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

template< class TIndex, class TObject >
struct Empty_Update_Logger
{
public:
  void deletion(const TIndex&, const TObject&) {}
};

template< class TIndex, class TObject, class TIterator = typename set< TIndex >::const_iterator >
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
        (const map< TIndex, set< TObject > >& to_delete,
         const map< TIndex, set< TObject > >& to_insert,
	 Update_Logger& update_logger);
	
    void update
        (const map< TIndex, set< TObject > >& to_delete,
         const map< TIndex, set< TObject > >& to_insert)
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
    set< TIndex > relevant_idxs;
    string data_filename;
  
    void calc_split_idxs
        (vector< TIndex >& split,
         const vector< uint32 >& sizes,
         typename set< TIndex >::const_iterator it,
         const typename set< TIndex >::const_iterator& end);
  
    void create_from_scratch
        (typename File_Blocks_::Discrete_Iterator& file_it,
         const map< TIndex, set< TObject > >& to_insert);
	 
    template< class Update_Logger >
    void update_group
        (typename File_Blocks_::Discrete_Iterator& file_it,
         const map< TIndex, set< TObject > >& to_delete,
         const map< TIndex, set< TObject > >& to_insert,
	 Update_Logger& update_logger);
	 
    template< class Update_Logger >
    void update_segments
        (typename File_Blocks_::Discrete_Iterator& file_it,
         const map< TIndex, set< TObject > >& to_delete,
         const map< TIndex, set< TObject > >& to_insert,
	 Update_Logger& update_logger);
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

template< class TIndex, class TObject >
template< typename T >
inline T Block_Backend_Basic_Iterator< TIndex, TObject >::apply_func(T(*f)(const void *))
{
  return f((void*)(buffer.ptr + pos));
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

/** Implementation Block_Backend: -------------------------------------------*/

template< class TIndex, class TObject, class TIterator >
Block_Backend< TIndex, TObject, TIterator >::Block_Backend(File_Blocks_Index_Base* index_)
  : file_blocks(index_),
    block_size(((File_Blocks_Index< TIndex >*)index_)->get_block_size() * ((File_Blocks_Index< TIndex >*)index_)->get_max_size()),
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
    (const map< TIndex, set< TObject > >& to_delete,
     const map< TIndex, set< TObject > >& to_insert,
     Update_Logger& update_logger)
{
  relevant_idxs.clear();
  for (typename map< TIndex, set< TObject > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
    relevant_idxs.insert(it->first);
  for (typename map< TIndex, set< TObject > >::const_iterator
      it(to_insert.begin()); it != to_insert.end(); ++it)
    relevant_idxs.insert(it->first);
  
  typename File_Blocks_::Discrete_Iterator
      file_it(file_blocks.discrete_begin
      (relevant_idxs.begin(), relevant_idxs.end()));
   
  while (!(file_it == file_blocks.discrete_end()))
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
    (vector< TIndex >& split,
     const vector< uint32 >& sizes,
     typename set< TIndex >::const_iterator it,
     const typename set< TIndex >::const_iterator& end)
{
  vector< uint32 > vsplit;
  vector< uint64 > min_split_pos;
  
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
  
  vector< uint64 > oversize_splits;
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
  
  vector< pair< uint64, uint32 > > forced_splits;
  // find splitting points where the average is below the minimum
  // - here needs the fitting to be corrected
  sum_size = 0;
  int min_split_i(min_split_pos.size());
  for (vector< uint64 >::const_iterator oit(oversize_splits.begin());
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
	  (make_pair(min_split_pos[min_split_i - j], j - used_blocks));
	used_blocks = j;
      }
    }
    forced_splits.push_back(make_pair(*oit, block_count - used_blocks));
    min_split_i = min_split_i - block_count;
    sum_size = *oit;
  }
  
  vector< pair< uint64, uint32 > >::const_iterator forced_it(forced_splits.begin());
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

template< class TIndex, class TObject, class TIterator >
void Block_Backend< TIndex, TObject, TIterator >::create_from_scratch
    (typename File_Blocks_::Discrete_Iterator& file_it,
     const map< TIndex, set< TObject > >& to_insert)
{
  map< TIndex, uint32 > sizes;
  vector< TIndex > split;
  vector< uint32 > vsizes;
  Void_Pointer< uint8 > buffer(block_size);
  
  // compute the distribution over different blocks
  for (typename set< TIndex >::const_iterator fit(file_it.lower_bound());
      fit != file_it.upper_bound(); ++fit)
  {
    typename map< TIndex, set< TObject > >::const_iterator
        it(to_insert.find(*fit));
    
    uint32 current_size(4);
    if ((it == to_insert.end()) || (it->second.empty()))
      current_size = 0;
    else
    {
      // only add nonempty indices
      current_size += it->first.size_of();
      for (typename set< TObject >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        current_size += it2->size_of();
    }
    
    sizes[it->first] += current_size;
    vsizes.push_back(current_size);
  }
  calc_split_idxs(split, vsizes, file_it.lower_bound(), file_it.upper_bound());
  
  // really write data
  typename vector< TIndex >::const_iterator split_it(split.begin());
  uint8* pos(buffer.ptr + 4);
  uint32 max_size(0);
  typename set< TIndex >::const_iterator upper_bound(file_it.upper_bound());
  for (typename set< TIndex >::const_iterator fit(file_it.lower_bound());
      fit != upper_bound; ++fit)
  {
    typename map< TIndex, set< TObject > >::const_iterator
        it(to_insert.find(*fit));
     
    if ((split_it != split.end()) && (it->first == *split_it))
    {
      *(uint32*)buffer.ptr = pos - buffer.ptr;
      file_it = file_blocks.insert_block(file_it, buffer.ptr, max_size);
      ++file_it;
      ++split_it;
      pos = buffer.ptr + 4;
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
      pos = pos + it->first.size_of() + 4;
      for (typename set< TObject >::const_iterator
	it2(it->second.begin()); it2 != it->second.end(); ++it2)
      {
	it2->to_data(pos);
	pos = pos + it2->size_of();
      }
      *(uint32*)current_pos = pos - buffer.ptr;
    }
    else
    {
      it->first.to_data(pos + 4);
      pos = pos + it->first.size_of() + 4;
      
      if (!(it->second.empty()))
      {
	for (typename set< TObject >::const_iterator
	  it2(it->second.begin()); it2 != it->second.end(); ++it2)
	{
	  if (pos - buffer.ptr + it2->size_of() > block_size)
	  {
	    *(uint32*)buffer.ptr = pos - buffer.ptr;
	    *(uint32*)(buffer.ptr+4) = *(uint32*)buffer.ptr;
	    file_it = file_blocks.insert_block(file_it, buffer.ptr, (*(uint32*)(buffer.ptr+4)) - 4);
	    ++file_it;
	    pos = buffer.ptr + 8 + it->first.size_of();
	    if (it->first.size_of() + it2->size_of() + 8 > block_size)
	      throw File_Error
	       (0, data_filename,
		"Block_Backend: an item's size exceeds block size.");
	  }
	   
	  it2->to_data(pos);
	  pos = pos + it2->size_of();
	}
      }
       
      *(uint32*)(buffer.ptr+4) = pos - buffer.ptr;
      max_size = (*(uint32*)(buffer.ptr + 4)) - 4;
    }
  }
  if (pos > buffer.ptr + 4)
  {
    *(uint32*)buffer.ptr = pos - buffer.ptr;
    file_it = file_blocks.insert_block(file_it, buffer.ptr, max_size);
    ++file_it;
  }
  ++file_it;
}

template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
void Block_Backend< TIndex, TObject, TIterator >::update_group
    (typename File_Blocks_::Discrete_Iterator& file_it,
     const map< TIndex, set< TObject > >& to_delete,
     const map< TIndex, set< TObject > >& to_insert,
     Update_Logger& update_logger)
{
  map< TIndex, Index_Collection< TIndex, TObject > > index_values;
  map< TIndex, uint32 > sizes;
  vector< TIndex > split;
  vector< uint32 > vsizes;
  Void_Pointer< uint8 > source(block_size);
  Void_Pointer< uint8 > dest(block_size);
  
  file_blocks.read_block(file_it, source.ptr);
  
  // prepare a unified iterator over all indices, from file, to_delete
  // and to_insert
  uint8* pos(source.ptr + 4);
  uint8* source_end(source.ptr + *(uint32*)source.ptr);
  while (pos < source_end)
  {
    index_values.insert(make_pair(TIndex(pos + 4), Index_Collection< TIndex, TObject >
        (pos, source.ptr + *(uint32*)pos, to_delete.end(), to_insert.end())));
    pos = source.ptr + *(uint32*)pos;
  }
  typename map< TIndex, set< TObject > >::const_iterator
      to_delete_begin(to_delete.lower_bound(*(file_it.lower_bound())));
  typename map< TIndex, set< TObject > >::const_iterator
      to_delete_end(to_delete.end());
  if (file_it.upper_bound() != relevant_idxs.end())
    to_delete_end = to_delete.lower_bound(*(file_it.upper_bound()));
  for (typename map< TIndex, set< TObject > >::const_iterator
    it(to_delete_begin); it != to_delete_end; ++it)
  {
    typename map< TIndex, Index_Collection< TIndex, TObject > >::iterator
        ic_it(index_values.find(it->first));
    if (ic_it == index_values.end())
    {
      index_values.insert(make_pair(it->first,
	  Index_Collection< TIndex, TObject >(0, 0, it, to_insert.end())));
    }
    else
      ic_it->second.delete_it = it;
  }
  
  typename map< TIndex, set< TObject > >::const_iterator
      to_insert_begin(to_insert.lower_bound(*(file_it.lower_bound())));
  typename map< TIndex, set< TObject > >::const_iterator
      to_insert_end(to_insert.end());
  if (file_it.upper_bound() != relevant_idxs.end())
    to_insert_end = to_insert.lower_bound(*(file_it.upper_bound()));
  for (typename map< TIndex, set< TObject > >::const_iterator
      it(to_insert_begin); it != to_insert_end; ++it)
  {
    typename map< TIndex, Index_Collection< TIndex, TObject > >::iterator
        ic_it(index_values.find(it->first));
    if (ic_it == index_values.end())
    {
      index_values.insert(make_pair(it->first,
	  Index_Collection< TIndex, TObject >(0, 0, to_delete.end(), it)));
    }
    else
      ic_it->second.insert_it = it;
  }
  
  // compute the distribution over different blocks
  // and log all objects that will be deleted
  for (typename map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
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
      for (typename set< TObject >::const_iterator
	it2(it->second.insert_it->second.begin());
      it2 != it->second.insert_it->second.end(); ++it2)
	current_size += it2->size_of();
    }
    
    sizes[it->first] += current_size;
    vsizes.push_back(current_size);
  }
  
  set< TIndex > index_values_set;
  for (typename map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
      it(index_values.begin()); it != index_values.end(); ++it)
    index_values_set.insert(it->first);
  calc_split_idxs(split, vsizes, index_values_set.begin(), index_values_set.end());
    
  // really write data
  typename vector< TIndex >::const_iterator split_it(split.begin());
  pos = (dest.ptr + 4);
  uint32 max_size(0);
  for (typename map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
    it(index_values.begin()); it != index_values.end(); ++it)
  {
    typename map< TIndex, Index_Collection< TIndex, TObject > >::const_iterator
    debug_it(it);
    
    if ((split_it != split.end()) && (it->first == *split_it))
    {
      *(uint32*)dest.ptr = pos - dest.ptr;
      file_it = file_blocks.insert_block(file_it, dest.ptr, max_size);
      ++file_it;
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
	for (typename set< TObject >::const_iterator
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
	for (typename set< TObject >::const_iterator
	  it2(it->second.insert_it->second.begin());
	it2 != it->second.insert_it->second.end(); ++it2)
	{
	  if (pos - dest.ptr + it2->size_of() > block_size)
	  {
	    *(uint32*)dest.ptr = pos - dest.ptr;
	    *(uint32*)(dest.ptr+4) = *(uint32*)dest.ptr;
	    file_it = file_blocks.insert_block(file_it, dest.ptr, (*(uint32*)dest.ptr) - 4);
	    ++file_it;
	    pos = dest.ptr + 8 + it->first.size_of();
	    if (it->first.size_of() + it2->size_of() + 8 > block_size)
	      throw File_Error
		    (0, data_filename,
		     "Block_Backend: an item's size exceeds block size.");
	  }
	    
	    it2->to_data(pos);
	    pos = pos + it2->size_of();
	}
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
    file_it = file_blocks.replace_block(file_it, dest.ptr, max_size);
    ++file_it;
    
  }
  else
    file_it = file_blocks.replace_block(file_it, 0, 0);
}

template< class TIndex, class TObject, class TIterator >
template< class Update_Logger >
void Block_Backend< TIndex, TObject, TIterator >::update_segments
      (typename File_Blocks_::Discrete_Iterator& file_it,
       const map< TIndex, set< TObject > >& to_delete,
       const map< TIndex, set< TObject > >& to_insert,
       Update_Logger& update_logger)
{
  Void_Pointer< uint8 > source(block_size);
  Void_Pointer< uint8 > dest(block_size);
  typename map< TIndex, set< TObject > >::const_iterator
      delete_it(to_delete.find(*(file_it.lower_bound())));
  typename map< TIndex, set< TObject > >::const_iterator
      insert_it(to_insert.find(*(file_it.lower_bound())));
      
  typename set< TObject >::const_iterator cur_insert;
  if (insert_it != to_insert.end())
    cur_insert = insert_it->second.begin();
      
  while (file_it.block_type() == File_Block_Index_Entry< TIndex >::SEGMENT)
  {
    bool block_modified(false);
    
    file_blocks.read_block(file_it, source.ptr);
    
    uint8* spos(source.ptr + 8 + TIndex::size_of(source.ptr + 8));
    uint8* pos(dest.ptr + 8 + TIndex::size_of(source.ptr + 8));
    memcpy(dest.ptr, source.ptr, spos - source.ptr);
    
    //copy everything that is not deleted yet
    if (*(uint32*)source.ptr != *((uint32*)(source.ptr + 4)))
      throw File_Error(0, data_filename,
	    "Block_Backend::1: one index expected - several found.");	 
    while ((uint32)(spos - source.ptr) < *(uint32*)source.ptr)
    {
      TObject obj(spos);
      if ((delete_it == to_delete.end()) ||
	(delete_it->second.find(obj) == delete_it->second.end()))
      {
	memcpy(pos, spos, obj.size_of());
	pos = pos + obj.size_of();
      }
      else
      {
	block_modified = true;
	update_logger.deletion(delete_it->first, obj);
      }
      spos = spos + obj.size_of();
    }
    
    // if nothing is modified, we keep the block untouched
    if (!block_modified)
    {
      ++file_it;
      continue;
    }
    
    // fill block with new data if any
    if (insert_it != to_insert.end())
    {
      while ((cur_insert != insert_it->second.end())
	&& (pos + cur_insert->size_of() < dest.ptr + block_size))
      {
	cur_insert->to_data(pos);
	pos = pos + cur_insert->size_of();
	++cur_insert;
      }
    }
    
    if (pos > dest.ptr + 8 + TIndex::size_of(source.ptr + 8))
    {
      *(uint32*)dest.ptr = pos - dest.ptr;
      *(uint32*)(dest.ptr+4) = *(uint32*)dest.ptr;
      file_it = file_blocks.replace_block(file_it, dest.ptr, (*(uint32*)dest.ptr) - 4);
      ++file_it;
    }
    else
      file_it = file_blocks.replace_block(file_it, 0, 0);
  }
  
  file_blocks.read_block(file_it, source.ptr);
  
  uint8* spos(source.ptr + 8 + TIndex::size_of(source.ptr + 8));
  uint8* pos(dest.ptr + 8 + TIndex::size_of(source.ptr + 8));
  memcpy(dest.ptr, source.ptr, spos - source.ptr);
  
  //copy everything that is not deleted yet
  if (*(uint32*)source.ptr != *((uint32*)(source.ptr + 4)))
      throw File_Error(0, data_filename,
	  "Block_Backend::2: one index expected - several found.");	 
  while ((uint32)(spos - source.ptr) < *(uint32*)source.ptr)
  {
    TObject obj(spos);
    if ((delete_it == to_delete.end()) ||
      (delete_it->second.find(obj) == delete_it->second.end()))
    {
      memcpy(pos, spos, obj.size_of());
      pos = pos + obj.size_of();
    }
    else
      update_logger.deletion(delete_it->first, obj);
    spos = spos + obj.size_of();
  }
  
  // fill block with new data if any
  if (insert_it != to_insert.end())
  {
    while (cur_insert != insert_it->second.end())
    {
      if (pos - dest.ptr + cur_insert->size_of() + TIndex::size_of(source.ptr + 8)
	>= block_size)
      {
	*(uint32*)dest.ptr = pos - dest.ptr;
	*(uint32*)(dest.ptr+4) = *(uint32*)dest.ptr;
	file_it = file_blocks.insert_block(file_it, dest.ptr, (*(uint32*)dest.ptr) - 4);
	++file_it;
	pos = dest.ptr + 8 + TIndex::size_of(source.ptr + 8);
	if (TIndex::size_of(source.ptr + 8) + cur_insert->size_of() + 8 > block_size)
	  throw File_Error
	  (0, data_filename,
	   "Block_Backend: an item's size exceeds block size.");
      }
      
      cur_insert->to_data(pos);
      pos = pos + cur_insert->size_of();
      ++cur_insert;
    }
  }
  
  if (pos > dest.ptr + 8 + TIndex::size_of(source.ptr + 8))
  {
    *(uint32*)dest.ptr = pos - dest.ptr;
    *(uint32*)(dest.ptr+4) = *(uint32*)dest.ptr;
    file_it = file_blocks.replace_block(file_it, dest.ptr, (*(uint32*)dest.ptr) - 4);
    ++file_it;
    
  }
  else
    file_it = file_blocks.replace_block(file_it, 0, 0);
}

#endif
