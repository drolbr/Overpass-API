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
  File_Block_Index_Entry(const TIndex& i, uint32 pos_, uint32 max_keysize_)
    : index(i), pos(pos_), max_keysize(max_keysize_) {}
  
  TIndex index;
  uint32 pos;
  uint32 max_keysize;
};

template< class TIndex, class TIterator >
struct File_Blocks_Iterator
{
  File_Blocks_Iterator
    (const typename list< File_Block_Index_Entry< TIndex > >::iterator& begin,
     const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
    : index_it(0), block_it(begin),
      index_end(0), block_end(end) {}
  
  File_Blocks_Iterator
  (TIterator const& idx_it,
   const typename list< File_Block_Index_Entry< TIndex > >::iterator& begin,
   TIterator const& idx_end,
   const typename list< File_Block_Index_Entry< TIndex > >::iterator& end)
   : index_it(0), block_it(begin), index_end(0), block_end(end)
  {
    index_it = new TIterator(idx_it);
    index_end = new TIterator(idx_end);
    
    if (block_it == block_end)
      return;
    if (*index_it == *index_end)
    {
      block_it = block_end;
      return;
    }
    if (!(block_it->index < **index_it))
      return;
    typename list< File_Block_Index_Entry< TIndex > >::iterator
        next_block(block_it);
    ++next_block;
    
    // The right block fulfills
    //   block_it == index_it || block_it < index_it < next_block
    while ((next_block != block_end) &&
      (!(**index_it < next_block->index)))
    {
      ++block_it;
      if (!(block_it->index < **index_it))
	return;
      ++next_block;
    }
  }
  
  File_Blocks_Iterator(const File_Blocks_Iterator& a)
    : index_it(a.index_it), block_it(a.block_it),
      index_end(a.index_end), block_end(a.block_end)
  {
    if (index_it != 0)
      index_it = new TIterator(*index_it);
    if (index_end != 0)
      index_end = new TIterator(*index_end);
    
    block_it = a.block_it;
    block_end = a.block_end;
  }
  
  ~File_Blocks_Iterator()
  {
    if (index_it != 0)
      delete index_it;
    if (index_end != 0)
      delete index_end;
  }
  
  const File_Blocks_Iterator& operator=(const File_Blocks_Iterator& b)
  {
    if (this == &b)
      return *this;
  
    if (index_it != 0)
      delete index_it;
    if (index_end != 0)
      delete index_end;
    index_it = 0;
    index_end = 0;
    
    if (b.index_it != 0)
      this->index_it = new TIterator(*(b.index_it));
    if (b.index_end != 0)
      this->index_end = new TIterator(*(b.index_end));
    
    block_it = b.block_it;
    block_end = b.block_end;
  }
  
  bool operator==
    (const File_Blocks_Iterator& b) const
  {
    return (this->block_it == b.block_it);
  }
   
  File_Blocks_Iterator& operator++()
  {
    ++block_it;
    if (index_it == 0)
      return *this;
    
    if (block_it == block_end)
      return *this;
    while ((*index_it != *index_end) &&
        (**index_it < block_it->index))
      ++(*index_it);
    if (*index_it == *index_end)
    {
      block_it = block_end;
      return *this;
    }
    if (!(block_it->index < **index_it))
      return *this;
    typename list< File_Block_Index_Entry< TIndex > >::iterator
      next_block(block_it);
    ++next_block;
    
    // The right block fulfills
    //   block_it == index_it || block_it < index_it < next_block
    while ((next_block != block_end) &&
      (!(**index_it < next_block->index)))
    {
      ++block_it;
      if (!(block_it->index < **index_it))
	return *this;
      ++next_block;
    }
    return *this;
  }

  TIterator* index_it;
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_it;
  
  TIterator* index_end;
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_end;
};

template< class TIndex, class TRangeIterator >
struct File_Blocks_Range_Iterator
{
  File_Blocks_Range_Iterator
      (typename list< File_Block_Index_Entry< TIndex > >::iterator begin,
       typename list< File_Block_Index_Entry< TIndex > >::iterator end)
         : block_it(begin), block_end(end), index_it(0), index_end(0) {}
   
  File_Blocks_Range_Iterator
       (TRangeIterator const& idx_it,
        typename list< File_Block_Index_Entry< TIndex > >::iterator begin,
        TRangeIterator const& idx_end,
        typename list< File_Block_Index_Entry< TIndex > >::iterator end)
          : index_it(0), block_it(begin), index_end(0), block_end(end)
  {
    index_it = new TRangeIterator(idx_it);
    index_end = new TRangeIterator(idx_end);
    
    if (block_it == block_end)
      return;
    if (*index_it == *index_end)
    {
      block_it = block_end;
      return;
    }
    if (!(block_it->index < index_it->lower_bound()))
      return;
    typename list< File_Block_Index_Entry< TIndex > >::iterator
    next_block(block_it);
    ++next_block;
    
    // The right block fulfills
    //   block_it == index_it->lower || block_it < index_it->lower < next_block
    while ((next_block != block_end) &&
      (!(index_it->lower_bound() < next_block->index)))
    {
      ++block_it;
      if (!(block_it->index < index_it->lower_bound()))
	return;
      ++next_block;
    }
  }
  
  File_Blocks_Range_Iterator(const File_Blocks_Range_Iterator& a)
    : index_it(a.index_it), block_it(a.block_it),
      index_end(a.index_end), block_end(a.block_end)
  {
    if (index_it != 0)
      index_it = new TRangeIterator(*index_it);
    if (index_end != 0)
      index_end = new TRangeIterator(*index_end);
    
    block_it = a.block_it;
    block_end = a.block_end;
  }
  
  ~File_Blocks_Range_Iterator()
  {
    if (index_it != 0)
      delete index_it;
    if (index_end != 0)
      delete index_end;
  }
  
  const File_Blocks_Range_Iterator& operator=(const File_Blocks_Range_Iterator& b)
  {
    if (this == &b)
      return *this;
    
    if (index_it != 0)
      delete index_it;
    if (index_end != 0)
      delete index_end;
    index_it = 0;
    index_end = 0;
    
    if (b.index_it != 0)
      this->index_it = new TRangeIterator(*(b.index_it));
    if (b.index_end != 0)
      this->index_end = new TRangeIterator(*(b.index_end));
    
    block_it = b.block_it;
    block_end = b.block_end;
  }
  
  bool operator==(const File_Blocks_Range_Iterator& b) const
  {
    return (this->block_it == b.block_it);
  }
  
  File_Blocks_Range_Iterator& operator++()
  {
    ++block_it;
    if (index_it == 0)
      return *this;
    
    if (block_it == block_end)
      return *this;
    if (block_it->index < index_it->upper_bound())
      return *this;
    while ((*index_it != *index_end) &&
      (!(block_it->index < index_it->upper_bound())))
      ++(*index_it);
    if (*index_it == *index_end)
    {
      block_it = block_end;
      return *this;
    }
    if (!(block_it->index < index_it->upper_bound()))
      return *this;
    typename list< File_Block_Index_Entry< TIndex > >::iterator
      next_block(block_it);
    ++next_block;
    
    // The right block fulfills
    //   block_it == index_it || block_it < index_it < next_block
    while ((next_block != block_end) &&
      (!(index_it->lower_bound() < next_block->index)))
    {
      ++block_it;
      if (!(block_it->index < index_it->lower_bound()))
	return *this;
      ++next_block;
    }
    return *this;
  }
  
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_it;
  typename list< File_Block_Index_Entry< TIndex > >::iterator block_end;
  
  TRangeIterator* index_it;
  TRangeIterator* index_end;
};

template< class TIndex, class TIterator, class TRangeIterator >
struct File_Blocks
{
  typedef File_Blocks_Iterator< TIndex, TIterator > Iterator;
  typedef File_Blocks_Range_Iterator< TIndex, TRangeIterator > Range_Iterator;
  
private:
  File_Blocks(const File_Blocks& f) {}
  
public:
  File_Blocks(int32 FILE_PROPERTIES, bool writeable)
  {
    index_file_name = get_file_base_name(FILE_PROPERTIES) + get_index_suffix(FILE_PROPERTIES);
    data_file_name = get_file_base_name(FILE_PROPERTIES) + get_data_suffix(FILE_PROPERTIES);
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
      File_Block_Index_Entry< TIndex >
        entry(TIndex(index_buf + pos),
	      *(uint32*)(index_buf + (pos + TIndex::size_of(index_buf+pos))),
	      *(uint32*)(index_buf + (pos + TIndex::size_of(index_buf+pos)
	        + sizeof(uint32))));
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
	ftruncate64(index_fd, index_size);
      lseek64(index_fd, 0, SEEK_SET);
      uint32 foo(write(index_fd, index_buf, index_size));
      
      free(index_buf);
    }
    
    free(buffer);
    
    close(index_fd);
    close(data_fd);
  }
  
  Iterator begin()
  {
    return File_Blocks_Iterator< TIndex, TIterator >
	(block_index.begin(), block_index.end());
  }
  
  Iterator end()
  {
    return File_Blocks_Iterator< TIndex, TIterator >
	(block_index.end(), block_index.end());
  }
  
  Range_Iterator range_end()
  {
    return File_Blocks_Range_Iterator< TIndex, TRangeIterator >
        (block_index.end(), block_index.end());
  }
  
  Iterator select_blocks(const TIterator& begin, const TIterator& end)
  {
    return File_Blocks_Iterator< TIndex, TIterator >
	(begin, block_index.begin(), end, block_index.end());
  }
  
  Range_Iterator select_blocks(const TRangeIterator& begin, const TRangeIterator& end)
  {
    return File_Blocks_Range_Iterator< TIndex, TRangeIterator >
        (begin, block_index.begin(), end, block_index.end());
  }
  
  void* read_block(const Iterator& it) const
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(read(data_fd, buffer, block_size));
    return buffer;
  }
  
  void* read_block(const Iterator& it, void* buffer) const
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(read(data_fd, buffer, block_size));
    return buffer;
  }
  
  void* read_block(const Range_Iterator& it) const
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(read(data_fd, buffer, block_size));
    return buffer;
  }
  
  void* read_block(const Range_Iterator& it, void* buffer) const
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(read(data_fd, buffer, block_size));
    return buffer;
  }
  
  uint32 answer_size(const Iterator& it) const
  {
    if (it.index_it == 0)
      return (block_size - sizeof(uint32));
    
    TIterator index_it(*(it.index_it));
    while ((index_it != *(it.index_end)) && (*index_it < it.block_it->index))
      ++index_it;
    uint32 count(0);
    
    typename list< File_Block_Index_Entry< TIndex > >::iterator
      next_block(it.block_it);
    ++next_block;
    if (next_block == it.block_end)
    {
      while (index_it != *(it.index_end))
      {
	++index_it;
	++count;
      }
    }
    else
    {
      while ((index_it != *(it.index_end)) && (*index_it < next_block->index))
      {
	++index_it;
	++count;
      }
    }
    if (count == 0)
      return it.block_it->max_keysize;
    else if (count*(it.block_it->max_keysize) > block_size-sizeof(uint32))
      return (block_size - sizeof(uint32));
    else
      return count*(it.block_it->max_keysize);
  }
  
  uint32 answer_size(const Range_Iterator& it) const
  {
    return (block_size - sizeof(uint32));
  }
  
  Iterator insert_block(Iterator it, void* buf, uint32 max_keysize)
  {
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
    
    if (*(uint32*)buf != 0)
    {
      File_Block_Index_Entry< TIndex > entry
          (TIndex(((uint8*)buf)+(sizeof(uint32)+sizeof(uint32))),
	   pos, max_keysize);
      it.block_it = block_index.insert(it.block_it, entry);
    }
    else
      void_blocks.push_back(pos);
    
    return it;
  }
  
  void replace_block(Iterator it, void* buf, uint32 max_keysize)
  {
    lseek64(data_fd, (int64)(it.block_it->pos)*(block_size), SEEK_SET);
    uint32 foo(write(data_fd, buf, block_size));
    
    if (*(uint32*)buf != 0)
    {
      it.block_it->index = TIndex((uint8*)buf+(sizeof(uint32)+sizeof(uint32)));
      it.block_it->max_keysize = max_keysize;
    }
    else
    {
      void_blocks.push_back(it.block_it->pos);
      it.block_it = block_index.erase(it.block_it);
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
    
    int index_fd, data_fd;
    void* buffer;
};

#endif
