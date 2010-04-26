#ifndef DE_OSM3S__BACKEND__BLOCK_BACKEND
#define DE_OSM3S__BACKEND__BLOCK_BACKEND

#include <map>
#include <set>

#include "../dispatch/settings.h"
#include "file_blocks.h"

/**
 * Block_Backend: a template to write and read database data blockwise
 *
 *
 * Block_Backend< TIndex, TObject, TIterator, TRangeIterator > offers the following methods and fields:
 *
 * typedef Read_Iterator;
 * typedef Range_Iterator;
 *
 * Block_Backend(int32 FILE_PROPERTIES, bool writeable);
 * ~Block_Backend();
 *
 * Block_Backend::Flat_Iterator flat_begin();
 * Block_Backend::Flat_Iterator flat_end();
 * Block_Backend::Discrete_Iterator discrete_begin(TIterator begin, TIterator end);
 * Block_Backend::Discrete_Iterator discrete_end();
 * Block_Backend::Range_Iterator select_blocks(TRangeIterator begin, TRangeIterator end);
 * Block_Backend::Range_Iterator range_end();
 *
 * void update
 *     (const map< TIndex, set< TObject > >& to_delete,
 *      const map< TIndex, set< TObject > >& to_insert);
 *
 *
 * where File_Blocks::Flat_Iterator offers the following methods and fields:
 *
 * bool operator== const(const Block_Backend::Read_Iterator&) const;
 * Block_Backend::Read_Iterator& Block_Backend::Read_Iterator::operator++();
 * const TIndex& index();
 * const TObject& object();
 *
 *
 * where File_Blocks::Discrete_Iterator offers the following methods and fields:
 *
 * bool operator== const(const Block_Backend::Read_Iterator&) const;
 * Block_Backend::Read_Iterator& Block_Backend::Read_Iterator::operator++();
 * const TIndex& index();
 * const TObject& object();
 *
 *
 * and File_Blocks::Range_Iterator offers the following methods and fields:
 *
 * bool operator== const(const Block_Backend::Range_Iterator&) const;
 * Block_Backend::Read_Iterator& Block_Backend::Range_Iterator::operator++();
 * const TIndex& index();
 * const TObject& object();
 *
 *
 * Block_Backend< TIndex, TObject, TIterator, TRangeIterator > requires the following methods
 * and fields of TIndex:
 *
 * TIndex(void* data);
 * uint32 size_of() const;
 * static uint32 size_of(void* data);
 * void to_data(void* data) const;
 *
 * bool operator<(const TIndex&, const TIndex&);
 *
 *
 * Block_Backend< TIndex, TObject, TIterator, TRangeIterator > requires the following methods
 * and fields of TObject:
 *
 * TObject(void* data, void* index);
 * uint32 size_of() const;
 * static uint32 size_of(void* data);
 * void to_data(void* data) const;
 *
 * bool operator<(const TObject&, const TObject&);
 * bool operator==(const TObject&, const TObject&);
 *
 *
 * Block_Backend< TIndex, TObject, TIterator, TRangeIterator > requires the following methods
 * and fields of TIterator:
 *
 * bool operator==(const TIterator&, const TIterator&);
 * bool operator++(const TIterator&);
 * const TIndex& operator*(const TIterator&);
 *
 *
 * Block_Backend< TIndex, TObject, TIterator, TRangeIterator > requires the following methods
 * and fields of TRangeIterator:
 *
 * bool operator==(const TRangeIterator&, const TRangeIterator&);
 * bool operator++(const TRangeIterator&);
 * const TIndex& lower_bound(const TRangeIterator&);
 * const TIndex& upper_bound(const TRangeIterator&);
 */

using namespace std;

template< class TIndex >
struct Default_Range_Iterator : set< pair< TIndex, TIndex > >::const_iterator
{
  Default_Range_Iterator
      (const typename set< pair< TIndex, TIndex > >::const_iterator it)
  : set< pair< TIndex, TIndex > >::const_iterator(it) {}
  
  Default_Range_Iterator() {}
  
  const TIndex& lower_bound() const
  {
    return (*this)->first;
  }
  
  const TIndex& upper_bound() const
  {
    return (*this)->second;
  }
};

template< class TIndex, class TObject >
struct Block_Backend_Basic_Iterator
{
  Block_Backend_Basic_Iterator(uint32 block_size_, bool is_end)
    : buffer(0), block_size(block_size_), pos(0),
      current_idx_pos(0), current_index(0), current_object(0)
  {
    if (is_end)
      return;
    
    buffer = (uint8*)malloc(block_size);
  }
  
  Block_Backend_Basic_Iterator(const Block_Backend_Basic_Iterator& it)
    : buffer(0), block_size(it.block_size), pos(it.pos),
      current_idx_pos(0), current_index(0), current_object(0)
  {
    if (it.buffer != 0)
    {
      buffer = (uint8*)malloc(block_size);
      memcpy(buffer, it.buffer, block_size);
      current_idx_pos = (uint32*)(buffer + ((uint32)it.current_idx_pos - (uint32)it.buffer));
    }
  }
  
  ~Block_Backend_Basic_Iterator()
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
    if (buffer != 0)
      free(buffer);
  }
  
  bool advance()
  {
    pos += TObject::size_of((void*)(buffer + pos));
    
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
  
  const TIndex& index()
  {
    if (current_index == 0)
      current_index = new TIndex((void*)(current_idx_pos + 1));
    return *current_index;
  }
  
  const TObject& object()
  {
    if (current_object == 0)
      current_object = new TObject((void*)(buffer + pos));
    return *current_object;
  }
  
  uint32 block_size;
  uint32 pos;
  uint32* current_idx_pos;
  TIndex* current_index;
  TObject* current_object;
  
  uint8* buffer;
};

template< class TIndex, class TObject, class TIterator >
struct Block_Backend_Flat_Iterator : Block_Backend_Basic_Iterator< TIndex, TObject >
{
  typedef File_Blocks
      < TIndex, typename set< TIndex >::const_iterator,
      Default_Range_Iterator< TIndex > > File_Blocks_;
  
  Block_Backend_Flat_Iterator
      (const File_Blocks_& file_blocks_, uint32 block_size_, bool is_end = false)
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
  
  Block_Backend_Flat_Iterator(const Block_Backend_Flat_Iterator& it)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(it),
      file_blocks(it.file_blocks), file_it(it.file_it), file_end(it.file_end) {}
  
  ~Block_Backend_Flat_Iterator() {}
  
  const Block_Backend_Flat_Iterator& operator=(const Block_Backend_Flat_Iterator& it)
  {
    if (this == &it)
      return *this;
    this->~Block_Backend_Flat_Iterator();
    new (this) Block_Backend_Flat_Iterator(it);
  }
  
  bool operator==(const Block_Backend_Flat_Iterator& it) const
  {
    bool res((this->pos == it.pos) && (file_it == it.file_it));
    return (res);
  }
  
  const Block_Backend_Flat_Iterator& operator++()
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
  
  const File_Blocks< TIndex, typename set< TIndex >::const_iterator,
      Default_Range_Iterator< TIndex > >& file_blocks;
  typename File_Blocks_::Flat_Iterator file_it;
  typename File_Blocks_::Flat_Iterator file_end;
  
private:
  // returns true if we have found something
  bool search_next_index()
  {
    // search for the next suitable index
    this->current_idx_pos = (uint32*)((this->buffer) + this->pos);
    if (this->pos < *(uint32*)(this->buffer))
    {
      this->pos += 4;
      this->pos += TIndex::size_of((void*)((this->buffer) + this->pos));
      return true;
    }
    
    return false;
  }
  
  // returns true if we are done
  // if we have loaded a new block, returns false to trigger search_next_index()
  bool read_block()
  {
    if (file_it == file_end)
    {
      // there is no block left
      this->pos = 0;
      return true;
    }
    this->pos = 4;
    file_blocks.read_block(file_it, this->buffer);
    
    return false;
  }
};

template< class TIndex, class TObject, class TIterator >
struct Block_Backend_Discrete_Iterator : Block_Backend_Basic_Iterator< TIndex, TObject >
{
  typedef File_Blocks
      < TIndex, typename set< TIndex >::const_iterator,
      Default_Range_Iterator< TIndex > > File_Blocks_;
  
  Block_Backend_Discrete_Iterator
      (File_Blocks_& file_blocks_,
       const typename set< TIndex >::const_iterator& index_it_,
       const typename set< TIndex >::const_iterator& index_end_, uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, false),
      file_blocks(file_blocks_), file_it(file_blocks_.discrete_begin(index_it_, index_end_)),
      file_end(file_blocks_.discrete_end()), index_it(0), index_end(0)
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
  
  Block_Backend_Discrete_Iterator
      (const File_Blocks_& file_blocks_, uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, true),
      file_blocks(file_blocks_), file_it(file_blocks_.discrete_end()),
      file_end(file_blocks_.discrete_end()), index_it(), index_end() {}
  
  Block_Backend_Discrete_Iterator(const Block_Backend_Discrete_Iterator& it)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(it),
      file_blocks(it.file_blocks), file_it(it.file_it), file_end(it.file_end),
      index_it(0), index_end(0) {}
  
  ~Block_Backend_Discrete_Iterator() {}
  
  const Block_Backend_Discrete_Iterator& operator=
      (const Block_Backend_Discrete_Iterator& it)
  {
    if (this == &it)
      return *this;
    this->~Block_Backend_Discrete_Iterator();
    new (this) Block_Backend_Discrete_Iterator(it);
  }
  
  bool operator==(const Block_Backend_Discrete_Iterator& it) const
  {
    bool res((this->pos == it.pos) && (file_it == it.file_it));
    return (res);
  }
  
  const Block_Backend_Discrete_Iterator& operator++()
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
  
  const File_Blocks< TIndex, typename set< TIndex >::const_iterator,
      Default_Range_Iterator< TIndex > >& file_blocks;
  typename File_Blocks_::Discrete_Iterator file_it;
  typename File_Blocks_::Discrete_Iterator file_end;
  typename set< TIndex >::const_iterator index_it;
  typename set< TIndex >::const_iterator index_end;
  
private:
  // returns true if we have found something
  bool search_next_index()
  {
    // search for the next suitable index
    this->current_idx_pos = (uint32*)((this->buffer) + this->pos);
    while (this->pos < *(uint32*)(this->buffer))
    {
      this->pos += 4;
      
      this->current_index = new TIndex((void*)((this->buffer) + this->pos));
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
	this->pos += TIndex::size_of((void*)((this->buffer) + this->pos));
	return true;
      }
      delete this->current_index;
      this->current_index = 0;
      
      this->pos = *(this->current_idx_pos);
      this->current_idx_pos = (uint32*)((this->buffer) + this->pos);
    }
    
    return false;
  }
  
  // returns true if we have found something
  bool read_block()
  {
    // we need to load a new block
    if (file_it == file_end)
    {
      // there is no block left
      this->pos = 0;
      return true;
    }
    this->pos = 4;
    file_blocks.read_block(file_it, this->buffer);
    
    return false;
  }
};

template< class TIndex, class TObject, class TIterator >
struct Block_Backend_Range_Iterator : Block_Backend_Basic_Iterator< TIndex, TObject >
{
  typedef File_Blocks
      < TIndex, typename set< TIndex >::const_iterator,
      Default_Range_Iterator< TIndex > > File_Blocks_;
  
  Block_Backend_Range_Iterator
      (File_Blocks_& file_blocks_,
       const Default_Range_Iterator< TIndex >& index_it_,
       const Default_Range_Iterator< TIndex >& index_end_, uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, false),
      file_blocks(file_blocks_), file_it(file_blocks_.range_begin(index_it_, index_end_)),
      file_end(file_blocks_.range_end()), index_it(), index_end()
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
  
  Block_Backend_Range_Iterator
      (const File_Blocks_& file_blocks_, uint32 block_size_)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(block_size_, true),
      file_blocks(file_blocks_), file_it(file_blocks_.range_end()),
      file_end(file_blocks_.range_end()), index_it(), index_end() {}
  
  Block_Backend_Range_Iterator(const Block_Backend_Range_Iterator& it)
    : Block_Backend_Basic_Iterator< TIndex, TObject >(it),
      file_blocks(it.file_blocks), file_it(it.file_it), file_end(it.file_end),
      index_it(), index_end() {}
  
  const Block_Backend_Range_Iterator& operator=
      (const Block_Backend_Range_Iterator& it)
  {
    if (this == &it)
      return *this;
    this->~Block_Backend_Range_Iterator();
    new (this) Block_Backend_Range_Iterator(it);
  }
  
  bool operator==(const Block_Backend_Range_Iterator& it) const
  {
    return ((this->pos == it.pos) && (file_it == it.file_it));
  }
  
  const Block_Backend_Range_Iterator& operator++()
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
  
  const File_Blocks_& file_blocks;
  typename File_Blocks_::Range_Iterator file_it;
  typename File_Blocks_::Range_Iterator file_end;
  Default_Range_Iterator< TIndex > index_it;
  Default_Range_Iterator< TIndex > index_end;
  
private:
  // returns true if we have found something
  bool search_next_index()
  {
    // search for the next suitable index
    this->current_idx_pos = (uint32*)((this->buffer) + this->pos);
    while (this->pos < *(uint32*)(this->buffer))
    {
      this->pos += 4;
      
      this->current_index = new TIndex((void*)((this->buffer) + this->pos));
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
	this->pos += TIndex::size_of((void*)((this->buffer) + this->pos));
	return true;
      }
      delete this->current_index;
      this->current_index = 0;
      
      this->pos = *(this->current_idx_pos);
      this->current_idx_pos = (uint32*)((this->buffer) + this->pos);
    }
    
    return false;
  }
  
  // returns true if we have found something
  bool read_block()
  {
    // we need to load a new block
    if (file_it == file_end)
    {
        // there is no block left
      this->pos = 0;
      return true;
    }
    this->pos = 4;
    file_blocks.read_block(file_it, this->buffer);
    
    return false;
  }
};

template< class TIndex, class TObject >
struct Block_Backend
{
  typedef Block_Backend_Flat_Iterator< TIndex, TObject, typename set< TIndex >::const_iterator >
      Flat_Iterator;
  typedef Block_Backend_Discrete_Iterator< TIndex, TObject, typename set< TIndex >::const_iterator >
      Discrete_Iterator;
  typedef Block_Backend_Range_Iterator< TIndex, TObject, Default_Range_Iterator< TIndex > >
      Range_Iterator;
  
  Block_Backend(int32 FILE_PROPERTIES, bool writeable)
    : file_blocks(FILE_PROPERTIES, writeable), block_size(get_block_size(FILE_PROPERTIES))
  {
  }
  
  Flat_Iterator flat_begin() const
  {
    return Flat_Iterator(file_blocks, false);
  }
  
  Flat_Iterator flat_end() const
  {
    return Flat_Iterator(file_blocks, true);
  }
  
  Discrete_Iterator discrete_begin
      (typename set< TIndex >::const_iterator begin,
       typename set< TIndex >::const_iterator end)
  {
    return Discrete_Iterator(file_blocks, begin, end, block_size);
  }
  
  Discrete_Iterator discrete_end() const
  {
    return Discrete_Iterator(file_blocks, block_size);
  }
  
  Range_Iterator range_begin
      (Default_Range_Iterator< TIndex > begin,
       Default_Range_Iterator< TIndex > end)
  {
    return Range_Iterator(file_blocks, begin, end, block_size);
  }
  
  Range_Iterator range_end()
  {
    return Range_Iterator(file_blocks, block_size);
  }
  
  void update
      (const map< TIndex, set< TObject > >& to_delete,
       const map< TIndex, set< TObject > >& to_insert)
  {
    set< TIndex > relevant_idxs;
    for (typename map< TIndex, set< TObject > >::const_iterator
        it(to_delete.begin()); it != to_delete.end(); ++it)
      relevant_idxs.insert(it->first);
    for (typename map< TIndex, set< TObject > >::const_iterator
        it(to_insert.begin()); it != to_insert.end(); ++it)
      relevant_idxs.insert(it->first);
    
    typename File_Blocks
        < TIndex,
	  typename set< TIndex >::const_iterator,
	  Default_Range_Iterator< TIndex > >::Discrete_Iterator
      file_it(file_blocks.discrete_begin
        (relevant_idxs.begin(), relevant_idxs.end()));

    if (file_it == file_blocks.discrete_end())
    {
      if (!to_insert.empty())
	create_from_scratch(to_insert);
      return;
    }
    typename File_Blocks
        < TIndex,
	  typename set< TIndex >::const_iterator,
	  Default_Range_Iterator< TIndex > >::Discrete_Iterator
      file_it_2(file_it);
    while (!(file_it_2 == file_blocks.discrete_end()))
    {
      return;
/*      if (file_it.index() == file_it_2.index())
      {
	//TODO
	TIndex current_idx(file_it.index());
	while ((!(file_it_2 == file_blocks.end())) && (current_idx == file_it_2.index()))
	{
	  ++file_it;
	  ++file_it_2;
	}
      }
      else
      {
	//TODO
	++file_it;
	++file_it_2;
      }*/
    }
  }
  
private:
  File_Blocks< TIndex, typename set< TIndex >::const_iterator, Default_Range_Iterator< TIndex > >
    file_blocks;
  uint32 block_size;
  
  void calc_split_idxs
      (vector< typename map< TIndex, set< TObject > >::const_iterator >& split,
       const vector< uint32 >& sizes,
       const map< TIndex, set< TObject > >& data)
  {
    //TODO: This is just a greedy algorithm and should be replace by something smarter
    vector< uint > vsplit;
    uint32 pos(4);
    for (uint i(0); i < sizes.size(); ++i)
    {
      pos += sizes[i];
      if (pos <= block_size)
	continue;
      if (i == 0)
	continue;
      vsplit.push_back(i-1);
      pos = 4 + sizes[i];
    }
    
    // This converts the result in a more convienient form
    uint i(0), j(0);
    typename map< TIndex, set< TObject > >::const_iterator it(data.begin());
    while ((j < vsplit.size()) && (it != data.end()))
    {
      if (vsplit[j] == i)
      {
	split.push_back(it);
	++j;
      }
      ++i;
      ++it;
    }
    split.push_back(data.end());
  }
  
  void create_from_scratch
      (const map< TIndex, set< TObject > >& to_insert)
  {
    // compute the distribution over different blocks
    map< TIndex, uint32 > sizes;
    vector< uint32 > vsizes;
    for (typename map< TIndex, set< TObject > >::const_iterator it(to_insert.begin());
	 it != to_insert.end(); ++it)
    {
      // only add nonempty indices
      if (it->second.empty())
	continue;
      
      uint32 current_size(4);
      current_size += it->first.size_of();
      for (typename set< TObject >::const_iterator it2(it->second.begin());
	   it2 != it->second.end(); ++it2)
	current_size += it2->size_of();
      
      sizes[it->first] = current_size;
      vsizes.push_back(current_size);
    }
    vector< typename map< TIndex, set< TObject > >::const_iterator > split;
    calc_split_idxs(split, vsizes, to_insert);
    
    // really write data
    uint8* buffer = (uint8*)malloc(block_size);
    typename map< TIndex, set< TObject > >::const_iterator idx_it(to_insert.begin());
    typename set< TObject >::const_iterator obj_it(idx_it->second.begin());
    for (typename vector< typename map< TIndex, set< TObject > >::const_iterator >::const_iterator
	 split_it(split.begin()); split_it != split.end(); ++split_it)
    {
      uint32 pos(4), max_size(0);
      if (sizes[idx_it->first] <= block_size - 4)
      {
	for (; idx_it != *split_it; ++idx_it)
	{
	  if (sizes[idx_it->first] == 0)
	    continue;
	  if (sizes[idx_it->first] > max_size)
	    max_size = sizes[idx_it->first];
	  *(uint32*)(buffer + pos) = pos + sizes[idx_it->first];
	  pos += 4;
	  idx_it->first.to_data(buffer + pos);
	  pos += idx_it->first.size_of();
	  for (typename set< TObject >::const_iterator it2(idx_it->second.begin());
	       it2 != idx_it->second.end(); ++it2)
	  {
	    it2->to_data(buffer + pos);
	    pos += it2->size_of();
	  }
	}
	*(uint32*)buffer = pos;
      }
      else
      {
	pos += 4;
	idx_it->first.to_data(buffer + pos);
	pos += idx_it->first.size_of();	
	for (typename set< TObject >::const_iterator it2(idx_it->second.begin());
		    it2 != idx_it->second.end(); ++it2)
	{
	  if (pos + it2->size_of() > block_size)
	  {
	    *(uint32*)buffer = pos;
	    *(uint32*)(buffer + 4) = pos;
	    file_blocks.insert_block(file_blocks.discrete_end(), buffer, max_size);
	    pos = 8 + idx_it->first.size_of();
	  }
	  it2->to_data(buffer + pos);
	  pos += it2->size_of();
	}
	++idx_it;
	*(uint32*)buffer = pos;
	*(uint32*)(buffer + 4) = pos;
      }
      file_blocks.insert_block(file_blocks.discrete_end(), buffer, max_size);
    }
    free(buffer);
  }
};

#endif
