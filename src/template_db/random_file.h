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

template< class TVal >
struct Random_File
{
private:
  Random_File(const Random_File& f) {}
  
public:
  typedef uint32 size_t;
  
  Random_File(const File_Properties& file_prop, bool writeable);  
  ~Random_File();
  
  TVal get(size_t pos) const;
  void put(size_t pos, const TVal& index);
  
  void flush_cache();
  void apply_written_cache();
  
private:
  string id_file_name;
  string shadow_file_name;
  bool writeable;
  uint32 index_size;
  uint64 file_size;
  map< size_t, TVal > write_cache;
  
  Raw_File val_file;
  void* buffer;
  void* zero;
};

/** Implementation Random_File_Index: ---------------------------------------*/

// template< class TIndex >
// File_Blocks_Index< TIndex >::File_Blocks_Index
//     (string source_index_file_name, string dest_index_file_name_,
//      uint32 block_count_) :
//      dest_index_file_name(dest_index_file_name_), block_count(block_count_)
// {
//   vector< bool > is_referred(block_count, false);
//   
//   bool writeable = (dest_index_file_name != "");
//   Raw_File source_file(source_index_file_name,
// 		       writeable ? O_RDONLY|O_CREAT : O_RDONLY,
// 		       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
// 		       "File_Blocks:2");
//   
//   // read index file
//   uint32 index_size(lseek64(source_file.fd, 0, SEEK_END));
//   Void_Pointer< uint8 > index_buf(index_size);
//   lseek64(source_file.fd, 0, SEEK_SET);
//   uint32 foo(read(source_file.fd, index_buf.ptr, index_size)); foo = 0;
//   
//   uint32 pos(0);
//   while (pos < index_size)
//   {
//     TIndex index(index_buf.ptr+pos);
//     File_Block_Index_Entry< TIndex >
//         entry(index,
// 	*(uint32*)(index_buf.ptr + (pos + TIndex::size_of(index_buf.ptr+pos))),
// 	*(uint32*)(index_buf.ptr + (pos + TIndex::size_of(index_buf.ptr+pos) + 4)));
//     blocks.push_back(entry);
//     if (entry.pos > block_count)
//       throw File_Error(0, source_index_file_name, "File_Blocks: bad pos in index file");
//     else
//       is_referred[entry.pos] = true;
//     pos += TIndex::size_of(index_buf.ptr+pos) + 2*sizeof(uint32);
//   }
//   
//   // determine void_blocks
//   for (uint32 i(0); i < block_count; ++i)
//   {
//     if (!(is_referred[i]))
//       void_blocks.push_back(i);
//   }
// }
// 
// template< class TIndex >
// File_Blocks_Index< TIndex >::~File_Blocks_Index()
// {
//   if (dest_index_file_name == "")
//     return;
// 
//   uint32 index_size(0), pos(0);
//   for (typename list< File_Block_Index_Entry< TIndex > >::const_iterator
//       it(blocks.begin()); it != blocks.end(); ++it)
//     index_size += 2*sizeof(uint32) + it->index.size_of();
//   
//   Void_Pointer< uint8 > index_buf(index_size);
//   
//   for (typename list< File_Block_Index_Entry< TIndex > >::const_iterator
//       it(blocks.begin()); it != blocks.end(); ++it)
//   {
//     it->index.to_data(index_buf.ptr+pos);
//     pos += it->index.size_of();
//     *(uint32*)(index_buf.ptr+pos) = it->pos;
//     pos += sizeof(uint32);
//     *(uint32*)(index_buf.ptr+pos) = it->max_keysize;
//     pos += sizeof(uint32);
//   }
// 
//   Raw_File dest_file(dest_index_file_name, O_RDWR|O_CREAT,
// 		     S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, "File_Blocks:3");
// 
//   if (index_size < lseek64(dest_file.fd, 0, SEEK_END))
//   {
//     int foo(ftruncate64(dest_file.fd, index_size)); foo = 0;
//   }
//   lseek64(dest_file.fd, 0, SEEK_SET);
//   uint32 foo(write(dest_file.fd, index_buf.ptr, index_size)); foo = 0;
// }

/** Implementation Random_File: ---------------------------------------------*/

template< class TVal >
inline Random_File< TVal >::Random_File(const File_Properties& file_prop, bool writeable)
: index_size(TVal::max_size_of()),
  val_file(file_prop.get_file_base_name() + file_prop.get_id_suffix(),
	   writeable ? O_RDWR|O_CREAT : O_RDONLY,
	   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
	   "Random_File:1")
{
  id_file_name = file_prop.get_file_base_name()
      + file_prop.get_id_suffix();
  shadow_file_name = id_file_name + file_prop.get_shadow_suffix();
  this->writeable = writeable;
  
  // reserve buffer for read/write actions
  buffer = malloc(index_size);
  zero = calloc(index_size, 64*1024);
  
  // determine the data file's size
  file_size = lseek64(val_file.fd, 0, SEEK_END);
}

template< class TVal >
inline Random_File< TVal >::~Random_File()
{
  flush_cache();
  apply_written_cache();
  
  free(buffer);
  free(zero);
}

template< class TVal >
inline TVal Random_File< TVal >::get(size_t pos) const
{
  if (writeable)
  {
    typename map< size_t, TVal >::const_iterator it = write_cache.find(pos);
    if (it != write_cache.end())
      return it->second;
  }
  uint8* result((uint8*)buffer);
  
  if ((uint64)pos*(index_size) >= file_size)
    result = (uint8*)zero;
  else
  {
    lseek64(val_file.fd, (int64)pos*(index_size), SEEK_SET);
    uint32 foo(read(val_file.fd, buffer, index_size)); foo = 0;
  }
  
  return TVal(result);
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

template< class TVal >
inline void Random_File< TVal >::flush_cache()
{
  if (!writeable)
    return;
  
  // open data file
  int data_fd = open64
      (shadow_file_name.c_str(),
       O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (data_fd < 0)
    throw File_Error(errno, id_file_name, "Random_File:3");
  
  lseek64(data_fd, 0, SEEK_SET);
  
  for (typename map< size_t, TVal >::const_iterator it = write_cache.begin();
      it != write_cache.end(); ++it)
  {
    *(size_t*)buffer = it->first;
    *(TVal*)((size_t*)buffer+1) = it->second;
    uint32 foo = write(data_fd, buffer, index_size + sizeof(size_t)); foo = 0;
  }
  
  close(data_fd);
}

template< class TVal >
inline void Random_File< TVal >::apply_written_cache()
{
  if (!writeable)
    return;
  
  // open data file
  int source_fd = open64
      (shadow_file_name.c_str(),
       O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (source_fd < 0)
    throw File_Error(errno, id_file_name, "Random_File:4");
  int dest_fd = open64
      (id_file_name.c_str(),
       O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno, id_file_name, "Random_File:5");
   
  // determine the data file's size
  uint64 source_file_size = lseek64(source_fd, 0, SEEK_END);
  lseek64(source_fd, 0, SEEK_SET);
  
  for (int64 source_pos = 0; source_pos < source_file_size;
      source_pos += index_size + sizeof(size_t))
  {
    uint32 foo = read(source_fd, buffer, index_size + sizeof(size_t)); foo = 0;
    uint64 pos = *(size_t*)buffer;

    if (pos*(index_size) > file_size)
    {
      lseek64(dest_fd, file_size, SEEK_SET);
      while (pos*(index_size) > file_size)
      {
	uint32 foo(write(dest_fd, zero, index_size*64*1024)); foo = 0;
	file_size += 64*1024*index_size;
      }
    }
    if (pos*(index_size) == file_size)
      file_size += index_size;
    
    lseek64(dest_fd, (int64)pos*(index_size), SEEK_SET);
    foo = write(dest_fd, (uint8*)buffer+sizeof(size_t), index_size); foo = 0;
  }
  
  close(source_fd);
  close(dest_fd);

  remove(shadow_file_name.c_str());
}

#endif
