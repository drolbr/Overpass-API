#ifndef RAW_FILE_DB
#define RAW_FILE_DB

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <set>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

const int32 RAW_DB_LESS = 1;
const int32 RAW_DB_GREATER = 2;
const int32 RAW_DB_EQUAL_SKIP = 3;
const int32 RAW_DB_EQUAL_REPLACE = 4;

struct File_Error
{
  File_Error(uint32 errno_) : error_number(errno_) {}
  
  uint32 error_number;
  set< uint16 > bla;
};

// constraints:
// T::size_of_buf(T::to_buf()) == T::size_of()
// T::index_of_buf(T::to_buf()) == T::index_of()
// T::size_of() < T::blocksize()
// the block_index is coherent with the file content and size
// ...
template< class T >
void flush_data(T& env, typename T::Iterator elem_begin, typename T::Iterator elem_end)
{
  if (elem_begin == elem_end)
    return;
  
  const uint32 BLOCKSIZE(env.blocksize());
  multimap< typename T::Index, uint16 >& block_index(env.block_index());
  
  int next_block_id(block_index.size());
  
  if (block_index.empty())
  {
    int dest_fd = open64(env.data_file(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    close(dest_fd);
  
    dest_fd = open64(env.data_file(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (dest_fd < 0)
      throw File_Error(errno);
  
    uint8* dest_buf = (uint8*) malloc(BLOCKSIZE);
    unsigned int i(sizeof(uint32));
    
    block_index.insert(make_pair< typename T::Index, uint16 >
	(env.index_of(elem_begin), next_block_id++));
    
    typename T::Iterator it(elem_begin);
    while (it != elem_end)
    {
      if (i >= BLOCKSIZE - env.size_of(it))
      {
	((uint32*)dest_buf)[0] = i - sizeof(uint32);
	write(dest_fd, dest_buf, BLOCKSIZE);
        
        block_index.insert(make_pair< typename T::Index, uint16 >
	    (env.index_of(it), next_block_id++));
	i = sizeof(uint32);
      }
      
      env.to_buf(&(dest_buf[i]), it);
      i += env.size_of(it);
      ++it;
    }
    if (i > 1)
    {
      ((uint32*)dest_buf)[0] = i - sizeof(uint32);
      write(dest_fd, dest_buf, BLOCKSIZE);
    }

    free(dest_buf);
    close(dest_fd);
  }
  else
  {
    int dest_fd = open64(env.data_file(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (dest_fd < 0)
      throw File_Error(errno);
  
    uint8* source_buf = (uint8*) malloc(BLOCKSIZE);
    uint8* dest_buf = (uint8*) malloc(BLOCKSIZE);
    
    typename T::Iterator elem_it(elem_begin);
    typename multimap< typename T::Index, uint16 >::const_iterator block_it(block_index.begin());
    unsigned int cur_block((block_it++)->second);
    while (elem_it != elem_end)
    {
      while ((block_it != block_index.end()) && (block_it->first <= env.index_of(elem_it)))
	cur_block = (block_it++)->second;

      uint32 new_byte_count(0);
      typename T::Iterator elem_it2(elem_it);
      if (block_it != block_index.end())
      {
	while ((elem_it2 != elem_end) && (block_it->first > env.index_of(elem_it2)))
	{
	  new_byte_count += env.size_of(elem_it2);
	  ++elem_it2;
	}
      }
      else
      {
	while (elem_it2 != elem_end)
	{
	  new_byte_count += env.size_of(elem_it2);
	  ++elem_it2;
	}
      }
      
      lseek64(dest_fd, (int64)cur_block*(BLOCKSIZE), SEEK_SET);
      read(dest_fd, source_buf, BLOCKSIZE);
      new_byte_count += ((uint32*)source_buf)[0];
      
      uint32 i(sizeof(uint32));
      while (new_byte_count > BLOCKSIZE)
      {
	uint32 blocksize(new_byte_count/(new_byte_count/BLOCKSIZE + 1));
        
	uint32 j(sizeof(uint32));
	while ((j < blocksize) &&
		 (elem_it != elem_it2) && (i < ((uint32*)source_buf)[0]) &&
		 (j < BLOCKSIZE - env.size_of(elem_it)) &&
		       (j < BLOCKSIZE - env.size_of_buf(&(source_buf[i]))))
	{
	  int cmp_val(env.compare(elem_it, &(source_buf[i])));
	  if (cmp_val == RAW_DB_LESS)
	  {
	    env.to_buf(&(dest_buf[j]), elem_it);
	    j += env.size_of(elem_it);
	    ++elem_it;
	  }
  	  else if (cmp_val == RAW_DB_GREATER)
  	  {
	    memcpy(&(dest_buf[j]), &(source_buf[i]), env.size_of_buf(&(source_buf[i])));
	    j += env.size_of_buf(&(source_buf[i]));
	    i += env.size_of_buf(&(source_buf[i]));
	  }
	  else if (cmp_val == RAW_DB_EQUAL_SKIP)
	  {
	    memcpy(&(dest_buf[j]), &(source_buf[i]), env.size_of_buf(&(source_buf[i])));
	    j += env.size_of_buf(&(source_buf[i]));
	    i += env.size_of_buf(&(source_buf[i]));
	    ++elem_it;
	  }
	  else if (cmp_val == RAW_DB_EQUAL_REPLACE)
	  {
	    env.to_buf(&(dest_buf[j]), elem_it);
	    j += env.size_of(elem_it);
	    ++elem_it;
	    i += env.size_of_buf(&(source_buf[i]));
	  }
	}
	while ((j < blocksize) &&
	       (elem_it != elem_it2) && (j < BLOCKSIZE - env.size_of(elem_it)) &&
	       ((i >= ((uint32*)source_buf)[0]) || (env.compare(elem_it, &(source_buf[i])) == RAW_DB_LESS)))
	{
	  env.to_buf(&(dest_buf[j]), elem_it);
	  j += env.size_of(elem_it);
	  ++elem_it;
	}
	while ((j < blocksize) &&
	       (i < ((uint32*)source_buf)[0]) && (j < BLOCKSIZE - env.size_of_buf(&(source_buf[i]))) &&
	       ((elem_it == elem_it2) || (env.compare(elem_it, &(source_buf[i])) == RAW_DB_GREATER)))
	{
	  memcpy(&(dest_buf[j]), &(source_buf[i]), env.size_of_buf(&(source_buf[i])));
	  j += env.size_of_buf(&(source_buf[i]));
	  i += env.size_of_buf(&(source_buf[i]));
	}

	lseek64(dest_fd, (int64)cur_block*(BLOCKSIZE), SEEK_SET);
	((uint32*)dest_buf)[0] = j - sizeof(uint32);
	write(dest_fd, dest_buf, BLOCKSIZE);

	cur_block = next_block_id;
	if ((i >= ((uint32*)source_buf)[0]) || (env.compare(elem_it, &(source_buf[i])) == RAW_DB_LESS))
	  block_index.insert(make_pair< typename T::Index, uint16 >(env.index_of(elem_it), next_block_id++));
	else
	  block_index.insert(make_pair< typename T::Index, uint16 >
	      (env.index_of_buf(&(source_buf[i])), next_block_id++));
	new_byte_count -= (j - sizeof(uint32));
      }
      
      uint32 j(sizeof(uint32));
      while ((elem_it != elem_it2) && (i < ((uint32*)source_buf)[0]))
      {
	int cmp_val(env.compare(elem_it, &(source_buf[i])));
	if (cmp_val == RAW_DB_LESS)
	{
	  env.to_buf(&(dest_buf[j]), elem_it);
	  j += env.size_of(elem_it);
	  ++elem_it;
	}
	else if (cmp_val == RAW_DB_GREATER)
	{
	  memcpy(&(dest_buf[j]), &(source_buf[i]), env.size_of_buf(&(source_buf[i])));
	  j += env.size_of_buf(&(source_buf[i]));
	  i += env.size_of_buf(&(source_buf[i]));
	}
	else if (cmp_val == RAW_DB_EQUAL_SKIP)
	{
	  memcpy(&(dest_buf[j]), &(source_buf[i]), env.size_of_buf(&(source_buf[i])));
	  j += env.size_of_buf(&(source_buf[i]));
	  i += env.size_of_buf(&(source_buf[i]));
	  ++elem_it;
	}
	else if (cmp_val == RAW_DB_EQUAL_REPLACE)
	{
	  env.to_buf(&(dest_buf[j]), elem_it);
	  j += env.size_of(elem_it);
	  ++elem_it;
	  i += env.size_of_buf(&(source_buf[i]));
	}
      }
      while (elem_it != elem_it2)
      {
	env.to_buf(&(dest_buf[j]), elem_it);
	j += env.size_of(elem_it);
	++elem_it;
      }
      while (i < ((uint32*)source_buf)[0])
      {
	memcpy(&(dest_buf[j]), &(source_buf[i]), env.size_of_buf(&(source_buf[i])));
	j += env.size_of_buf(&(source_buf[i]));
	i += env.size_of_buf(&(source_buf[i]));
      }
      lseek64(dest_fd, (int64)cur_block*(BLOCKSIZE), SEEK_SET);
      ((uint32*)dest_buf)[0] = j - sizeof(uint32);
      write(dest_fd, dest_buf, BLOCKSIZE);
    }
    
    free(source_buf);
    free(dest_buf);

    close(dest_fd);
  }
}

template< class T >
    void make_block_index(const T& env)
{
  const multimap< typename T::Index, uint16 >& block_index(env.block_index());
  
  uint8* buf =
    (uint8*) malloc((env.size_of_Index() + sizeof(uint16))*block_index.size());
  
  uint32 i(0);
  for (typename multimap< typename T::Index, uint16 >::const_iterator it(block_index.begin());
       it != block_index.end(); ++it)
  {
    env.index_to_buf(&(buf[i]), it->first);
    i += env.size_of_Index();
    *((uint16*)&(buf[i])) = it->second;
    i += sizeof(uint16);
  }
  
  int dest_fd = open64(env.index_file(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno);
  write(dest_fd, buf, (env.size_of_Index() + sizeof(uint16))*block_index.size());
  close(dest_fd);
  
  free(buf);
}

template < class T >
    void select_with_idx(T& env)
{
  const char* IDX_FILE(env.index_file());
  const char* DATA_FILE(env.data_file());
  const uint32 BLOCKSIZE(env.blocksize());
  
  static vector< uint32 > idx_boundaries;
  static vector< uint16 > block_idx;
  
  if ((idx_boundaries.empty()) || (block_idx.empty()))
  {
    idx_boundaries.clear();
    block_idx.clear();
    
    int idx_fd = open64(IDX_FILE, O_RDONLY);
    if (idx_fd < 0)
      throw File_Error(errno);
  
    uint32 idx_file_size(lseek64(idx_fd, 0, SEEK_END));
    char* idx_buf = (char*) malloc(idx_file_size);
    lseek64(idx_fd, 0, SEEK_SET);
    read(idx_fd, idx_buf, idx_file_size);
    uint32 i(0);
    while (i < idx_file_size)
    {
      idx_boundaries.push_back(*((uint32*)&(idx_buf[i])));
      i += sizeof(uint32);
      block_idx.push_back(*(uint16*)&(idx_buf[i]));
      i += sizeof(uint16);
    }
    free(idx_buf);
  
    close(idx_fd);
  }
  
  set< uint16 > block_ids;
  uint32 i(1);
  typename T::Index_Iterator it(env.idxs_begin());
  while (it != env.idxs_end())
  {
    while ((i < idx_boundaries.size()) && (idx_boundaries[i] < *it))
      ++i;
    block_ids.insert(block_idx[i-1]);
    while ((i < idx_boundaries.size()) && (idx_boundaries[i] <= *it))
    {
      block_ids.insert(block_idx[i]);
      ++i;
    }
    ++it;
  }

  int data_fd = open64(DATA_FILE, O_RDONLY);
  if (data_fd < 0)
    throw File_Error(errno);
  
  uint8* data_buf = (uint8*) malloc(BLOCKSIZE);
  for (set< uint16 >::const_iterator it(block_ids.begin());
       it != block_ids.end(); ++it)
  {
    lseek64(data_fd, ((uint64)(*it))*BLOCKSIZE, SEEK_SET);
    read(data_fd, data_buf, BLOCKSIZE);
    uint32 pos(sizeof(uint32));
    while (pos < *((uint32*)data_buf) + sizeof(uint32))
    {
      env.process(&(data_buf[pos]));
      pos += env.size_of_buf(&(data_buf[pos]));
    }
  }
  free(data_buf);
  
  close(data_fd);
}

#endif
