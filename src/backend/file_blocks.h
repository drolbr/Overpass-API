#ifndef DE_OSM3S__BACKEND__FILE_BLOCKS
#define DE_OSM3S__BACKEND__FILE_BLOCKS

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <vector>

#include "../dispatch/settings.h"

/**
 * File_Blocks: a template to write and read database data blockwise
 *
 *
 * File_Blocks offers the following methods and fields:
 *
 * typedef File_Blocks_Iterator< TIndex, TIterator > Iterator;
 *
 * File_Blocks(int32 FILE_PROPERTIES, bool writeable);
 * ~File_Blocks();
 *
 * File_Blocks::Iterator begin();
 * File_Blocks::Iterator end();
 * File_Blocks::Iterator select_blocks(TIterator begin, TIterator end);
 *
 * void* read_block(const File_Blocks::Iterator& it);
 * int64 answer_size(const File_Blocks::Iterator& it);
 * File_Blocks::Iterator insert_block
 *     (File_Blocks::Iterator it, void* buf, uint32 max_keysize);
 * void replace_block(File_Blocks::Iterator it, void* buf, uint32 max_keysize);
 *
 *
 * where File_Blocks::Iterator offers the following methods and fields:
 *
 * bool File_Blocks::Iterator::operator== const(const File_Blocks::Iterator&);
 * File_Blocks::Iterator& operator++(File_Blocks::Iterator& it);
 *
 *
 * File_Blocks< TIndex, TIterator, TRangeIterator > requires the following methods
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
 * File_Blocks< TIndex, TIterator, TRangeIterator > requires the following methods
 * and fields of TIterator:
 *
 * bool operator==(const TIterator&, const TIterator&);
 * bool operator++(const TIterator&);
 * const TIndex& operator*(const TIterator&);
 *
 *
 * File_Blocks< TIndex, TIterator, TRangeIterator > requires the following methods
 * and fields of TRangeIterator:
 *
 * bool operator==(const TRangeIterator&, const TRangeIterator&);
 * bool operator++(const TRangeIterator&);
 * const TIndex& lower_bound(const TRangeIterator&);
 * const TIndex& upper_bound(const TRangeIterator&);
 */

using namespace std;

struct File_Error
{
  File_Error(uint32 errno_, string filename_, string origin_)
  : error_number(errno_), filename(filename_), origin(origin_) {}
  
  uint32 error_number;
  string filename;
  string origin;
};

template< class TIndex >
struct File_Block_Index_Entry
{
  static const int EMPTY = 1;
  static const int GROUP = 2;
  static const int SEGMENT = 3;
  static const int LAST_SEGMENT = 4;
  
  File_Block_Index_Entry
      (const TIndex& i, uint32 pos_, uint32 max_keysize_)
    : index(i), pos(pos_), max_keysize(max_keysize_)
  {}
  
  TIndex index;
  uint32 pos;
  uint32 max_keysize;
};

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
    
  int block_type() const
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
      (const File_Blocks_Flat_Iterator& a)
  {
    if (this == &a)
      return *this;
    this->~File_Blocks_Flat_Iterator();
    new (this) File_Blocks_Flat_Iterator(a);
  }
  
  bool operator==(const File_Blocks_Flat_Iterator& a) const
  {
    return (this->block_it == a.block_it);
  }
  
  File_Blocks_Flat_Iterator& operator++()
  {
    ++(this->block_it);
  }
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
      index_end(a.index_end), just_inserted(a.just_inserted)
  {}
    
  ~File_Blocks_Discrete_Iterator() {}
    
  const File_Blocks_Discrete_Iterator& operator=
      (const File_Blocks_Discrete_Iterator& a)
  {
    if (this == &a)
      return *this;
    this->~File_Blocks_Discrete_Iterator();
    new (this) File_Blocks_Discrete_Iterator(a);
  }
    
  bool operator==
      (const File_Blocks_Discrete_Iterator& a) const
  {
    return ((this->block_it == a.block_it) && (this->is_empty == a.is_empty));
  }
  
  File_Blocks_Discrete_Iterator& operator++()
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
  
  const TIterator& lower_bound() const
  {
    return index_lower;
  }
  
  const TIterator& upper_bound() const
  {
    return index_upper;
  }
  
  TIterator index_lower;
  TIterator index_upper;
  TIterator index_end;
  bool just_inserted;
  
private:
  void find_next_block()
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
	++(this->block_it);
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
      (const File_Blocks_Range_Iterator& a)
  {
    if (this == &a)
      return *this;
    this->~File_Blocks_Range_Iterator();
    new (this) File_Blocks_Range_Iterator(a);
  }
  
  bool operator==(const File_Blocks_Range_Iterator& b) const
  {
    return (this->block_it == b.block_it);
  }
  
  File_Blocks_Range_Iterator& operator++()
  {
    ++(this->block_it);
    find_next_block();
    return *this;
  }
  
private:
  TRangeIterator index_it;
  TRangeIterator index_end;

  void find_next_block()
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
  File_Blocks(int32 FILE_PROPERTIES, bool writeable, string file_name_extension = "")
  {
    index_file_name = get_file_base_name(FILE_PROPERTIES) + file_name_extension
	+ get_index_suffix(FILE_PROPERTIES);
    data_file_name = get_file_base_name(FILE_PROPERTIES) + file_name_extension
	+ get_data_suffix(FILE_PROPERTIES);
    block_size = get_block_size(FILE_PROPERTIES);
    this->writeable = writeable;
    
    // open data file
    if (writeable)
      data_fd = open64
	  (data_file_name.c_str(),
	   O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    else
      data_fd = open64
	  (data_file_name.c_str(), O_RDONLY);
    if (data_fd < 0)
      throw File_Error(errno, data_file_name, "File_Blocks:1");
    
    // determine block_count from the data file's size
    block_count = lseek64(data_fd, 0, SEEK_END)/block_size;
    vector< bool > is_referred(block_count, false);

    // open index file
    if (writeable)
      index_fd = open64
	  (index_file_name.c_str(),
	   O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    else
      index_fd = open64
	  (index_file_name.c_str(), O_RDONLY);
    if (index_fd < 0)
      throw File_Error(errno, index_file_name, "File_Blocks:2");
    
    // read index file
    uint32 index_size(lseek64(index_fd, 0, SEEK_END));
    uint8* index_buf = (uint8*)malloc(index_size);
    lseek64(index_fd, 0, SEEK_SET);
    uint32 foo(read(index_fd, index_buf, index_size));
    
    uint32 pos(0);
    while (pos < index_size)
    {
      TIndex index(index_buf+pos);
      File_Block_Index_Entry< TIndex >
        entry(index,
	      *(uint32*)(index_buf + (pos + TIndex::size_of(index_buf+pos))),
	      *(uint32*)(index_buf + (pos + TIndex::size_of(index_buf+pos)
	        + 4)));
      block_index.push_back(entry);
      if (entry.pos > block_count)
	throw File_Error(0, index_file_name, "File_Blocks: bad pos in index file");
      else
	is_referred[entry.pos] = true;
      pos += TIndex::size_of(index_buf+pos) + 2*sizeof(uint32);
    }
    
    free(index_buf);
    
    // determine void_blocks
    for (uint32 i(0); i < block_count; ++i)
    {
      if (!(is_referred[i]))
	void_blocks.push_back(i);
    }
    
    // prepare standard iterators
    flat_end_it = new Flat_Iterator(block_index.end(), block_index.end());
    discrete_end_it = new Discrete_Iterator(block_index.end());
    range_end_it = new Range_Iterator(block_index.end());
    
    // reserve buffer for read/write actions
    buffer = malloc(block_size);
  }
  
  ~File_Blocks()
  {
    if (writeable)
    {
      // write back block index
      uint32 index_size(0), pos(0);
      for (typename list< File_Block_Index_Entry< TIndex > >::const_iterator
	  it(block_index.begin()); it != block_index.end(); ++it)
	index_size += 2*sizeof(uint32) + it->index.size_of();
      uint8* index_buf = (uint8*)malloc(index_size);
      
      for (typename list< File_Block_Index_Entry< TIndex > >::const_iterator
	   it(block_index.begin()); it != block_index.end(); ++it)
      {
	it->index.to_data(index_buf+pos);
	pos += it->index.size_of();
	*(uint32*)(index_buf+pos) = it->pos;
	pos += sizeof(uint32);
	*(uint32*)(index_buf+pos) = it->max_keysize;
	pos += sizeof(uint32);
      }
      if (index_size < lseek64(index_fd, 0, SEEK_END))
	int foo(ftruncate64(index_fd, index_size));
      lseek64(index_fd, 0, SEEK_SET);
      uint32 foo(write(index_fd, index_buf, index_size));
      
      free(index_buf);
    }
    
    free(buffer);
    
    delete flat_end_it;
    delete discrete_end_it;
    delete range_end_it;
    
    close(index_fd);
    close(data_fd);
  }
  
  Flat_Iterator flat_begin()
  {
    return Flat_Iterator(block_index.begin(), block_index.end());
  }
  
  const Flat_Iterator& flat_end() const
  {
    return *flat_end_it;
  }
  
  Discrete_Iterator discrete_begin(const TIterator& begin, const TIterator& end)
  {
    return File_Blocks_Discrete_Iterator< TIndex, TIterator >
	(begin, end, block_index.begin(), block_index.end());
  }
  
  const Discrete_Iterator& discrete_end() const
  {
    return *discrete_end_it;
  }
  
  Range_Iterator range_begin(const TRangeIterator& begin, const TRangeIterator& end)
  {
    return File_Blocks_Range_Iterator< TIndex, TRangeIterator >
	(block_index.begin(), block_index.end(), begin, end);
  }
  
  const Range_Iterator& range_end() const
  {
    return *range_end_it;
  }
  
  void* read_block(const File_Blocks_Basic_Iterator< TIndex >& it) const
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(read(data_fd, buffer, block_size));
    return buffer;
  }
  
  void* read_block
      (const File_Blocks_Basic_Iterator< TIndex >& it, void* buffer) const
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(read(data_fd, buffer, block_size));
    return buffer;
  }
  
  uint32 answer_size(const Flat_Iterator& it) const
  {
    return (block_size - sizeof(uint32));
  }
  
  uint32 answer_size(const Discrete_Iterator& it) const
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
  
  uint32 answer_size(const Range_Iterator& it) const
  {
    return (block_size - sizeof(uint32));
  }
  
  Discrete_Iterator insert_block(const Discrete_Iterator& it, void* buf, uint32 max_keysize)
  {
    if (buf == 0)
      return it;
    
    uint32 pos;
    if (void_blocks.empty())
    {
      pos = block_count;
      ++block_count;
    }
    else
    {
      pos = void_blocks.back();
      void_blocks.pop_back();
    }
    
    lseek64(data_fd, (int64)pos*(block_size), SEEK_SET);
    uint32 foo(write(data_fd, buf, block_size));
    
    TIndex index(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32)));
    File_Block_Index_Entry< TIndex > entry
	(index, pos, max_keysize);
    Discrete_Iterator return_it(it);
    if (return_it.block_it == return_it.block_begin)
    {
      return_it.block_it = block_index.insert(return_it.block_it, entry);
      return_it.block_begin = return_it.block_it;
    }
    else
      return_it.block_it = block_index.insert(return_it.block_it, entry);
    return_it.just_inserted = true;
    return_it.is_empty = it.is_empty;
    return return_it;
  }
  
  Discrete_Iterator replace_block(Discrete_Iterator it, void* buf, uint32 max_keysize)
  {
    if (buf != 0)
    {
      lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
      uint32 foo(write(data_fd, buf, block_size));
    
      it.block_it->index = TIndex((uint8*)buf+(sizeof(uint32)+sizeof(uint32)));
      it.block_it->max_keysize = max_keysize;
      
      return it;
    }
    else
    {
      void_blocks.push_back(it.block_it->pos);
      Discrete_Iterator return_it(it);
      ++return_it;
      if (it.block_it == it.block_begin)
      {
	it.block_it = block_index.erase(it.block_it);
	it.block_begin = it.block_it;
      }
      else
	it.block_it = block_index.erase(it.block_it);
      return return_it;
    }
  }
  
  private:
    string index_file_name;
    string data_file_name;
    uint32 block_size;
    bool writeable;
    
    list< File_Block_Index_Entry< TIndex > > block_index;
    vector< uint32 > void_blocks;
    uint32 block_count;
    
    Flat_Iterator* flat_end_it;
    Discrete_Iterator* discrete_end_it;
    Range_Iterator* range_end_it;
    
    int index_fd, data_fd;
    void* buffer;
};

#endif
