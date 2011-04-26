#ifndef DE_OSM3S__BACKEND__RANDOM_FILE
#define DE_OSM3S__BACKEND__RANDOM_FILE

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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
  
/*  void flush_cache();
  void apply_written_cache();*/
  
private:
  string id_file_name;
  string shadow_file_name;
  bool writeable;
  uint32 index_size;
  uint64 file_size;
  map< size_t, TVal > write_cache;
  
  Raw_File val_file;
  Random_File_Index index;
  Void_Pointer< uint8 > cache;
  size_t cache_pos, block_size;
  void* zero;
};

/** Implementation Random_File_Index: ---------------------------------------*/

inline Random_File_Index::Random_File_Index
    (string source_index_file_name, string dest_index_file_name_,
     uint32 block_count_) :
     dest_index_file_name(dest_index_file_name_), block_count(block_count_)
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
    if (*entry > block_count)
      throw File_Error(0, source_index_file_name, "Random_File: bad pos in index file");
    else
      is_referred[*entry] = true;
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
	lseek64(val_file.fd, 0, SEEK_END)/file_prop.get_map_block_size()),
  cache(file_prop.get_map_block_size()*sizeof(size_t)), cache_pos(index.block_count),
  block_size(file_prop.get_map_block_size())
{
  id_file_name = file_prop.get_file_base_name()
      + file_prop.get_id_suffix();
  shadow_file_name = id_file_name + file_prop.get_shadow_suffix();
  this->writeable = writeable;
  
  // reserve buffer for read/write actions
  zero = calloc(index_size, 64*1024);
  
  // determine the data file's size
  file_size = lseek64(val_file.fd, 0, SEEK_END);
}

template< class TVal >
inline Random_File< TVal >::~Random_File()
{
/*  flush_cache();
  apply_written_cache();*/
  
  free(zero);
}

template< class TVal >
inline TVal Random_File< TVal >::get(size_t pos)
{
  if ((cache_pos == index.block_count) || (pos / block_size != cache_pos))
  {
    if (writeable)
    {
      if (cache_pos != index.block_count)
      {
        lseek64(val_file.fd, (int64)cache_pos*block_size*index_size, SEEK_SET);
	uint32 foo(write(val_file.fd, cache.ptr, block_size*index_size)); foo = 0;
      }
    }
    cache_pos = pos / block_size;
    if (cache_pos < index.block_count)
    {
      lseek64(val_file.fd, (int64)cache_pos*block_size*index_size, SEEK_SET);
      uint32 foo(read(val_file.fd, cache.ptr, block_size*index_size)); foo = 0;
    }
    else
    {
      // Reset the whole cache to zero.
      for (uint32 i = 0; i < block_size*index_size; ++i)
	*(cache.ptr + i) = 0;
      if (writeable)
      {
	//TODO
      }
    }
  }
  
  return TVal(((TVal*)cache.ptr) + pos % block_size);
}

template< class TVal >
inline void Random_File< TVal >::put(size_t pos, const TVal& index)
{
  if (!writeable)
    throw File_Error(0, id_file_name, "Random_File:2");

  typename map< size_t, TVal >::iterator it = write_cache.lower_bound(pos);
  if (it->first == pos)
    it->second = index;
  else
    write_cache.insert(it, make_pair(pos, index));
}

// template< class TVal >
// inline void Random_File< TVal >::flush_cache()
// {
//   if (!writeable)
//     return;
//   
//   // open data file
//   int data_fd = open64
//       (shadow_file_name.c_str(),
//        O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
//   if (data_fd < 0)
//     throw File_Error(errno, id_file_name, "Random_File:3");
//   
//   lseek64(data_fd, 0, SEEK_SET);
//   
//   for (typename map< size_t, TVal >::const_iterator it = write_cache.begin();
//       it != write_cache.end(); ++it)
//   {
//     *(size_t*)buffer = it->first;
//     *(TVal*)((size_t*)buffer+1) = it->second;
//     uint32 foo = write(data_fd, buffer, index_size + sizeof(size_t)); foo = 0;
//   }
//   
//   close(data_fd);
// }

// template< class TVal >
// inline void Random_File< TVal >::apply_written_cache()
// {
//   if (!writeable)
//     return;
//   
//   // open data file
//   int source_fd = open64
//       (shadow_file_name.c_str(),
//        O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
//   if (source_fd < 0)
//     throw File_Error(errno, id_file_name, "Random_File:4");
//   int dest_fd = open64
//       (id_file_name.c_str(),
//        O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
//   if (dest_fd < 0)
//     throw File_Error(errno, id_file_name, "Random_File:5");
//    
//   // determine the data file's size
//   uint64 source_file_size = lseek64(source_fd, 0, SEEK_END);
//   lseek64(source_fd, 0, SEEK_SET);
//   
//   for (int64 source_pos = 0; source_pos < source_file_size;
//       source_pos += index_size + sizeof(size_t))
//   {
//     uint32 foo = read(source_fd, buffer, index_size + sizeof(size_t)); foo = 0;
//     uint64 pos = *(size_t*)buffer;
// 
//     if (pos*(index_size) > file_size)
//     {
//       lseek64(dest_fd, file_size, SEEK_SET);
//       while (pos*(index_size) > file_size)
//       {
// 	uint32 foo(write(dest_fd, zero, index_size*64*1024)); foo = 0;
// 	file_size += 64*1024*index_size;
//       }
//     }
//     if (pos*(index_size) == file_size)
//       file_size += index_size;
//     
//     lseek64(dest_fd, (int64)pos*(index_size), SEEK_SET);
//     foo = write(dest_fd, (uint8*)buffer+sizeof(size_t), index_size); foo = 0;
//   }
//   
//   close(source_fd);
//   close(dest_fd);
// 
//   remove(shadow_file_name.c_str());
// }

#endif
