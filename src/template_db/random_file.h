#ifndef DE_OSM3S__BACKEND__RANDOM_FILE
#define DE_OSM3S__BACKEND__RANDOM_FILE

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <limits>
#include <map>
#include <vector>

#include "types.h"

/**
 *
 *
 * TVal must offer
 *
 * static uint32 max_size_of()
 */

using namespace std;

struct Random_File_Index
{
  public:
    Random_File_Index(string source_index_file_name,
		      string dest_index_file_name,
		      uint32 block_count);
    ~Random_File_Index();
    
    typedef uint32 size_t;
    
  private:
    string dest_index_file_name;
    
  public:
    vector< size_t > blocks;
    vector< size_t > void_blocks;
    size_t block_count;    

    const size_t npos;    
};

template< class TVal >
struct Random_File
{
private:
  Random_File(const Random_File& f) {}
  
public:
  typedef uint32 size_t;
  
  Random_File(const File_Properties& file_prop, bool writeable);  
  ~Random_File();
  
  TVal get(size_t pos);
  void put(size_t pos, const TVal& index);
  
private:
  string id_file_name;
  bool writeable;
  uint32 index_size;
  
  Raw_File val_file;
  Random_File_Index index;
  Void_Pointer< uint8 > cache;
  size_t cache_pos, block_size;
  
  void move_cache_window(size_t pos);
};

/** Implementation Random_File_Index: ---------------------------------------*/

inline Random_File_Index::Random_File_Index
    (string source_index_file_name, string dest_index_file_name_,
     uint32 block_count_) :
     dest_index_file_name(dest_index_file_name_), block_count(block_count_),
     npos(numeric_limits< size_t >::max())
{
  vector< bool > is_referred(block_count, false);
  
  bool writeable = (dest_index_file_name != "");
  Raw_File source_file(source_index_file_name,
		       writeable ? O_RDONLY|O_CREAT : O_RDONLY,
		       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
		       "Random_File:6");
  
  // read index file
  uint32 index_size = lseek64(source_file.fd, 0, SEEK_END);
  Void_Pointer< uint8 > index_buf(index_size);
  lseek64(source_file.fd, 0, SEEK_SET);
  uint32 foo(read(source_file.fd, index_buf.ptr, index_size)); foo = 0;
  
  uint32 pos = 0;
  while (pos < index_size)
  {
    size_t* entry = (size_t*)(index_buf.ptr+pos);
    blocks.push_back(*entry);
    if (*entry != npos)
    {
      if (*entry > block_count)
	throw File_Error(0, source_index_file_name,
			 "Random_File: bad pos in index file");
      else
	is_referred[*entry] = true;
    }
    pos += sizeof(size_t);
  }
  
  // determine void_blocks
  for (uint32 i(0); i < block_count; ++i)
  {
    if (!(is_referred[i]))
      void_blocks.push_back(i);
  }
}

inline Random_File_Index::~Random_File_Index()
{
  if (dest_index_file_name == "")
    return;

  uint32 index_size = blocks.size()*sizeof(size_t);
  uint32 pos = 0;
 
  Void_Pointer< uint8 > index_buf(index_size);
  
  for (vector< size_t >::const_iterator it(blocks.begin()); it != blocks.end();
      ++it)
  {
    *(size_t*)(index_buf.ptr+pos) = *it;
    pos += sizeof(size_t);
  }

  Raw_File dest_file(dest_index_file_name, O_RDWR|O_CREAT,
		     S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, "Random_File:7");

  if (index_size < lseek64(dest_file.fd, 0, SEEK_END))
  {
    int foo(ftruncate64(dest_file.fd, index_size)); foo = 0;
  }
  lseek64(dest_file.fd, 0, SEEK_SET);
  uint32 foo(write(dest_file.fd, index_buf.ptr, index_size)); foo = 0;
}

/** Implementation Random_File: ---------------------------------------------*/

template< class TVal >
inline Random_File< TVal >::Random_File(const File_Properties& file_prop, bool writeable)
: index_size(TVal::max_size_of()),
  val_file(file_prop.get_file_base_name() + file_prop.get_id_suffix(),
	   writeable ? O_RDWR|O_CREAT : O_RDONLY,
	   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
	   "Random_File:1"),
  index(file_prop.get_file_base_name() + file_prop.get_id_suffix()
        + file_prop.get_index_suffix(),
	writeable ? file_prop.get_file_base_name() + file_prop.get_id_suffix()
	+ file_prop.get_index_suffix() : "",
	lseek64(val_file.fd, 0, SEEK_END)/file_prop.get_map_block_size()/index_size),
  cache(file_prop.get_map_block_size()*sizeof(size_t)), cache_pos(index.npos),
  block_size(file_prop.get_map_block_size())
{
  id_file_name = file_prop.get_file_base_name() + file_prop.get_id_suffix();
  this->writeable = writeable;
}

template< class TVal >
inline Random_File< TVal >::~Random_File()
{
  move_cache_window(index.npos);  
}

template< class TVal >
inline TVal Random_File< TVal >::get(size_t pos)
{
  move_cache_window(pos / block_size);  
  return TVal(cache.ptr + (pos % block_size)*index_size);
}

template< class TVal >
inline void Random_File< TVal >::put(size_t pos, const TVal& val)
{
  if (!writeable)
    throw File_Error(0, id_file_name, "Random_File:2");
  
  move_cache_window(pos / block_size);  
  val.to_data(cache.ptr + (pos % block_size)*index_size);
}

template< class TVal >
inline void Random_File< TVal >::move_cache_window(size_t pos)
{
  // The cache already contains the needed position.
  if ((pos == cache_pos) && (cache_pos != index.npos))
    return;
  
  if ((writeable) && (cache_pos != index.npos))
  {
    // Find an empty position.
    uint32 disk_pos;
    if (index.void_blocks.empty())
    {
      disk_pos = index.block_count;
      ++(index.block_count);
    }
    else
    {
      disk_pos = index.void_blocks.back();
      index.void_blocks.pop_back();
    }
    
    // Save the found position to the index.
    if (index.blocks.size() <= cache_pos)
      index.blocks.resize(cache_pos+1, index.npos);
    index.blocks[cache_pos] = disk_pos;
    
    // Write the data at the found position.
    lseek64(val_file.fd, (int64)disk_pos*block_size*index_size, SEEK_SET);
    uint32 foo(write(val_file.fd, cache.ptr, block_size*index_size)); foo = 0;
  }

  if (pos == index.npos)
    return;
  
  if ((index.blocks.size() <= pos) || (index.blocks[pos] == index.npos))
  {
    // Reset the whole cache to zero.
    for (uint32 i = 0; i < block_size*index_size; ++i)
      *(cache.ptr + i) = 0;
  }
  else
  {
    lseek64(val_file.fd, (int64)index.blocks[pos]*block_size*index_size, SEEK_SET);
    uint32 foo(read(val_file.fd, cache.ptr, block_size*index_size)); foo = 0;
  }
  cache_pos = pos;
}

#endif
