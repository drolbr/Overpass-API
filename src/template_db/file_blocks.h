#ifndef DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_H
#define DE__OSM3S___TEMPLATE_DB__FILE_BLOCKS_H

#include "file_blocks_index.h"
#include "types.h"

#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <list>
#include <vector>

/** Declarations: -----------------------------------------------------------*/

using namespace std;

template< class TIndex >
struct File_Blocks_Basic_Iterator
{
  File_Blocks_Basic_Iterator
  (const typename list< File_Block_Index_Entry< TIndex > >::iterator& begin,
   const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : block_begin(begin), block_it(begin), block_end(end), is_empty(false) {}
    
  File_Blocks_Basic_Iterator(const File_Blocks_Basic_Iterator& a)
  : block_begin(a.block_begin), block_it(a.block_it), block_end(a.block_end),
    is_empty(a.is_empty) {}
    
  int block_type() const;
   
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_begin;
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_it;
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_end;
  bool is_empty;
};

template< class TIndex >
struct File_Blocks_Flat_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Flat_Iterator
  (const typename list< File_Block_Index_Entry< TIndex > >::iterator& begin,
   const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
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

template< class TIndex, class TIterator >
struct File_Blocks_Discrete_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Discrete_Iterator
      (TIterator const& index_it_, TIterator const& index_end_,
       const typename list< File_Block_Index_Entry< TIndex > >::iterator& begin,
       const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end),
      index_lower(index_it_), index_upper(index_it_), index_end(index_end_),
      just_inserted(false)
  {
    find_next_block();
  }
  
  File_Blocks_Discrete_Iterator
      (const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
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

template< class TIndex, class TRangeIterator >
struct File_Blocks_Range_Iterator : File_Blocks_Basic_Iterator< TIndex >
{
  File_Blocks_Range_Iterator
      (const typename list< File_Block_Index_Entry< TIndex > >::iterator& begin,
       const typename list< File_Block_Index_Entry< TIndex > >::iterator& end,
       const TRangeIterator& index_it_,  const TRangeIterator& index_end_)
    : File_Blocks_Basic_Iterator< TIndex >(begin, end),
      index_it(index_it_), index_end(index_end_)
  {
    find_next_block();
  }
  
  File_Blocks_Range_Iterator
      (const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
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

template< class TIndex, class TIterator, class TRangeIterator >
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
    return (block_size - sizeof(uint32));
  }
  uint32 answer_size(const Discrete_Iterator& it) const;  
  uint32 answer_size(const Range_Iterator& it) const
  {
    return (block_size - sizeof(uint32));
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
  bool writeable;
  mutable uint read_count_;
  
  Flat_Iterator* flat_end_it;
  Discrete_Iterator* discrete_end_it;
  Range_Iterator* range_end_it;

  Raw_File data_file;
  Void_Pointer< void > buffer;
};

/** Implementation File_Blocks_Basic_Iterator: ------------------------------*/

template< class TIndex >
int File_Blocks_Basic_Iterator< TIndex >::block_type() const
{
  if ((block_it == block_end) || (is_empty))
    return File_Block_Index_Entry< TIndex >::EMPTY;
  typename list< File_Block_Index_Entry< TIndex > >::const_iterator
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

template< class TIndex >
const File_Blocks_Flat_Iterator< TIndex >& File_Blocks_Flat_Iterator< TIndex >::operator=
(const File_Blocks_Flat_Iterator& a)
{
  if (this == &a)
    return *this;
  this->~File_Blocks_Flat_Iterator();
  new (this) File_Blocks_Flat_Iterator(a);
  return *this;
}

template< class TIndex >
bool File_Blocks_Flat_Iterator< TIndex >::operator==
(const File_Blocks_Flat_Iterator& a) const
{
  return (this->block_it == a.block_it);
}

template< class TIndex >
File_Blocks_Flat_Iterator< TIndex >& File_Blocks_Flat_Iterator< TIndex >::operator++()
{
  ++(this->block_it);
  return *this;
}

template< class TIndex >
bool File_Blocks_Flat_Iterator< TIndex >::is_out_of_range(const TIndex& index)
{
  if (this->block_it == this->block_end)
    return true;
  if (index == this->block_it->index)
    return false;
  if (index < this->block_it->index)
    return true;
  typename list< File_Block_Index_Entry< TIndex > >::iterator
      next_it(this->block_it);
  if (++next_it == this->block_end)
    return false;
  if (!(index < next_it->index))
    return true;
  return false;
}

/** Implementation File_Blocks_Discrete_Iterator: ---------------------------*/

template< class TIndex, class TIterator >
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

template< class TIndex, class TIterator >
bool File_Blocks_Discrete_Iterator< TIndex, TIterator >::operator==
(const File_Blocks_Discrete_Iterator< TIndex, TIterator >& a) const
{
  return ((this->block_it == a.block_it) && (this->is_empty == a.is_empty));
}

template< class TIndex, class TIterator >
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

template< class TIndex, class TIterator >
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
  
  typename list< File_Block_Index_Entry< TIndex > >::const_iterator
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

template< class TIndex, class TRangeIterator >
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

template< class TIndex, class TRangeIterator >
bool File_Blocks_Range_Iterator< TIndex, TRangeIterator >::operator==(const File_Blocks_Range_Iterator< TIndex, TRangeIterator >& b) const
{
  return (this->block_it == b.block_it);
}

template< class TIndex, class TRangeIterator >
File_Blocks_Range_Iterator< TIndex, TRangeIterator >&
File_Blocks_Range_Iterator< TIndex, TRangeIterator >::operator++()
{
  ++(this->block_it);
  find_next_block();
  return *this;
}

template< class TIndex, class TRangeIterator >
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
    
    typename list< File_Block_Index_Entry< TIndex > >::const_iterator
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

template< class TIndex, class TIterator, class TRangeIterator >
File_Blocks< TIndex, TIterator, TRangeIterator >::File_Blocks
    (File_Blocks_Index_Base* index_) : 
     index((File_Blocks_Index< TIndex >*)index_),
     block_size(index->get_block_size()),
     writeable(index->writeable()),
     read_count_(0),
     data_file(index->get_data_file_name(),
	       writeable ? O_RDWR|O_CREAT : O_RDONLY,
	       S_666, "File_Blocks::File_Blocks::1"),
     buffer(index->get_block_size())
{
  // cerr<<"  "<<index->get_data_file_name()<<'\n'; //Debug
  
  // prepare standard iterators
  flat_end_it = new Flat_Iterator(index->blocks.end(), index->blocks.end());
  discrete_end_it = new Discrete_Iterator(index->blocks.end());
  range_end_it = new Range_Iterator(index->blocks.end());
}

template< class TIndex, class TIterator, class TRangeIterator >
File_Blocks< TIndex, TIterator, TRangeIterator >::~File_Blocks()
{
  delete flat_end_it;
  delete discrete_end_it;
  delete range_end_it;

  // cerr<<"~ "<<index->get_data_file_name()<<'\n'; //Debug
}

template< class TIndex, class TIterator, class TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Flat_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::flat_begin()
{
  return Flat_Iterator(index->blocks.begin(), index->blocks.end());
}

template< class TIndex, class TIterator, class TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::discrete_begin
    (const TIterator& begin, const TIterator& end)
{
  return File_Blocks_Discrete_Iterator< TIndex, TIterator >
      (begin, end, index->blocks.begin(), index->blocks.end());
}

template< class TIndex, class TIterator, class TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Range_Iterator
File_Blocks< TIndex, TIterator, TRangeIterator >::range_begin(const TRangeIterator& begin, const TRangeIterator& end)
{
  return File_Blocks_Range_Iterator< TIndex, TRangeIterator >
      (index->blocks.begin(), index->blocks.end(), begin, end);
}

template< class TIndex, class TIterator, class TRangeIterator >
void* File_Blocks< TIndex, TIterator, TRangeIterator >::read_block
    (const File_Blocks_Basic_Iterator< TIndex >& it) const
{
  data_file.seek((int64)(it.block_it->pos)*(block_size), "File_Blocks::read_block::1");
  data_file.read((uint8*)buffer.ptr, block_size, "File_Blocks::read_block::2");
  ++read_count_;
  return buffer.ptr;
}

template< class TIndex, class TIterator, class TRangeIterator >
void* File_Blocks< TIndex, TIterator, TRangeIterator >::read_block
    (const File_Blocks_Basic_Iterator< TIndex >& it, void* buffer) const
{
  data_file.seek((int64)(it.block_it->pos)*(block_size), "File_Blocks::read_block::3");
  data_file.read((uint8*)buffer, block_size, "File_Blocks::read_block::4");
  if (!(it.block_it->index ==
        TIndex(((uint8*)buffer)+(sizeof(uint32)+sizeof(uint32)))))
    throw File_Error(it.block_it->pos, index->get_data_file_name(),
		     "File_Blocks::read_block: Index inconsistent");
  ++read_count_;
  return buffer;
}

template< class TIndex, class TIterator, class TRangeIterator >
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
  
  if (count*(it.block_it->max_keysize) > block_size-sizeof(uint32))
    return (block_size - sizeof(uint32));
  else
    return count*(it.block_it->max_keysize);
}

template< class TIndex, class TIterator, class TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::insert_block
    (const Discrete_Iterator& it, void* buf, uint32 max_keysize)
{
  if (buf == 0)
    return it;
  
  uint32 pos;
  if (this->index->void_blocks.empty())
  {
    pos = this->index->block_count;
    ++(this->index->block_count);
  }
  else
  {
    pos = index->void_blocks.back();
    this->index->void_blocks.pop_back();
  }
  
  // cerr<<dec<<pos<<"\t0x"; //Debug
  // for (uint i = 0; i < TIndex::size_of(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32))); ++i)
  //   cerr<<' '<<hex<<setw(2)<<setfill('0')
  //       <<int(*(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32))+i)); // Debug
  // cerr<<'\n';
  
  data_file.seek(((int64)pos)*block_size, "File_Blocks::insert_block::1");
  data_file.write((uint8*)buf, block_size, "File_Blocks::insert_block::2");
  
  TIndex index(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32)));
  File_Block_Index_Entry< TIndex > entry(index, pos, max_keysize);
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

template< class TIndex, class TIterator, class TRangeIterator >
typename File_Blocks< TIndex, TIterator, TRangeIterator >::Discrete_Iterator
    File_Blocks< TIndex, TIterator, TRangeIterator >::replace_block
    (Discrete_Iterator it, void* buf, uint32 max_keysize)
{
  if (buf != 0)
  {
    if (this->index->void_blocks.empty())
    {
      it.block_it->pos = this->index->block_count;
      ++(this->index->block_count);
    }
    else
    {
      it.block_it->pos = this->index->void_blocks.back();
      this->index->void_blocks.pop_back();
    }
    data_file.seek(((int64)it.block_it->pos)*block_size, "File_Blocks::replace_block::1");
    data_file.write((uint8*)buf, block_size, "File_Blocks::replace_block::2");
    
    it.block_it->index = TIndex((uint8*)buf+(sizeof(uint32)+sizeof(uint32)));
    it.block_it->max_keysize = max_keysize;
    
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
