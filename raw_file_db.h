#ifndef RAW_FILE_DB
#define RAW_FILE_DB

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// constraints:
// T::size_of_buf(T::to_buf()) == T::size_of()
// T::index_of_buf(T::to_buf()) == T::index_of()
// T::size_of() < T::blocksize()
// the block_index is coherent with the file content and size
template< class T, class Iterator >
    void flush_data(Iterator elem_begin, Iterator elem_end)
{
  if (elem_begin == elem_end)
    return;
  
  const uint32 BLOCKSIZE(T::blocksize());
  multimap< int32, uint16 >& block_index(T::block_index());
  
  int next_block_id(block_index.size());
  
  if (block_index.empty())
  {
    int dest_fd = open64(T::data_file(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    close(dest_fd);
  
    dest_fd = open64(T::data_file(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (dest_fd < 0)
    {
      cerr<<"open64: "<<errno<<'\n';
      exit(0);
    }
  
    uint8* dest_buf = (uint8*) malloc(BLOCKSIZE);
    unsigned int i(sizeof(uint32));
    
    block_index.insert(make_pair< int32, uint32 >
	(T::index_of(elem_begin), next_block_id++));
    
    Iterator it(elem_begin);
    while (it != elem_end)
    {
      if (i >= BLOCKSIZE - T::size_of(it))
      {
	((uint32*)dest_buf)[0] = i - sizeof(uint32);
	write(dest_fd, dest_buf, BLOCKSIZE);
        
	block_index.insert(make_pair< int, unsigned int >
	    (T::index_of(it), next_block_id++));
	i = sizeof(uint32);
      }
      
      T::to_buf(&(dest_buf[i]), it);
      i += T::size_of(it);
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
    int dest_fd = open64(T::data_file(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (dest_fd < 0)
    {
      cerr<<"open64: "<<errno<<'\n';
      exit(0);
    }
  
    uint8* source_buf = (uint8*) malloc(BLOCKSIZE);
    uint8* dest_buf = (uint8*) malloc(BLOCKSIZE);
    
    Iterator elem_it(elem_begin);
    multimap< int32, uint16 >::const_iterator block_it(block_index.begin());
    unsigned int cur_block((block_it++)->second);
    while (elem_it != elem_end)
    {
      while ((block_it != block_index.end()) && (block_it->first <= T::index_of(elem_it)))
	cur_block = (block_it++)->second;

      uint32 new_byte_count(0);
      vector< uint32* >::const_iterator elem_it2(elem_it);
      if (block_it != block_index.end())
      {
	while ((elem_it2 != elem_end) && (block_it->first > T::index_of(elem_it2)))
	{
	  new_byte_count += T::size_of(elem_it2);
	  ++elem_it2;
	}
      }
      else
      {
	while (elem_it2 != elem_end)
	{
	  new_byte_count += T::size_of(elem_it2);
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
		 (j < BLOCKSIZE - T::size_of(elem_it)) &&
		       (j < BLOCKSIZE - T::size_of_buf(&(source_buf[i]))))
	{
	  if (T::elem_less_buf(elem_it, &(source_buf[i])))
	  {
	    T::to_buf(&(dest_buf[j]), elem_it);
	    j += T::size_of(elem_it);
	    ++elem_it;
	  }
	  else
	  {
	    memcpy(&(dest_buf[j]), &(source_buf[i]), T::size_of_buf(&(source_buf[i])));
	    j += T::size_of_buf(&(source_buf[i]));
	    i += T::size_of_buf(&(source_buf[i]));
	  }
	}
	while ((j < blocksize) && (elem_it != elem_it2) && (j < BLOCKSIZE - T::size_of(elem_it)))
	{
	  T::to_buf(&(dest_buf[j]), elem_it);
	  j += T::size_of(elem_it);
	  ++elem_it;
	}
	while ((j < blocksize) && (i < ((uint32*)source_buf)[0]) &&
		       (j < BLOCKSIZE - T::size_of_buf(&(source_buf[i]))))
	{
	  memcpy(&(dest_buf[j]), &(source_buf[i]), T::size_of_buf(&(source_buf[i])));
	  j += T::size_of_buf(&(source_buf[i]));
	  i += T::size_of_buf(&(source_buf[i]));
	}

	lseek64(dest_fd, (int64)cur_block*(BLOCKSIZE), SEEK_SET);
	((uint32*)dest_buf)[0] = j - sizeof(uint32);
	write(dest_fd, dest_buf, BLOCKSIZE);

	cur_block = next_block_id;
	if ((i >= ((uint32*)source_buf)[0]) || (T::elem_less_buf(elem_it, &(source_buf[i]))))
	  block_index.insert(make_pair< int, unsigned int >(T::index_of(elem_it), next_block_id++));
	else
	  block_index.insert(make_pair< int, unsigned int >
	      (T::index_of_buf(&(source_buf[i])), next_block_id++));
	new_byte_count -= (j - sizeof(uint32));
      }
      
      uint32 j(sizeof(uint32));
      while ((elem_it != elem_it2) && (i < ((uint32*)source_buf)[0]))
      {
	if (T::elem_less_buf(elem_it, &(source_buf[i])))
	{
	  T::to_buf(&(dest_buf[j]), elem_it);
	  j += T::size_of(elem_it);
	  ++elem_it;
	}
	else
	{
	  memcpy(&(dest_buf[j]), &(source_buf[i]), T::size_of_buf(&(source_buf[i])));
	  j += T::size_of_buf(&(source_buf[i]));
	  i += T::size_of_buf(&(source_buf[i]));
	}
      }
      while (elem_it != elem_it2)
      {
	T::to_buf(&(dest_buf[j]), elem_it);
	j += T::size_of(elem_it);
	++elem_it;
      }
      while (i < ((uint32*)source_buf)[0])
      {
	memcpy(&(dest_buf[j]), &(source_buf[i]), T::size_of_buf(&(source_buf[i])));
	j += T::size_of_buf(&(source_buf[i]));
	i += T::size_of_buf(&(source_buf[i]));
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
    void make_block_index()
{
  multimap< int32, uint16 >& block_index(T::block_index());
  
  pair< int32, uint16 >* buf =
      (pair< int32, uint16 >*) malloc(sizeof(int)*2*block_index.size());
      
  if (!buf)
  {
    cerr<<"malloc: "<<errno<<'\n';
    exit(0);
  }
  
  unsigned int i(0);
  for (multimap< int32, uint16 >::const_iterator it(block_index.begin());
       it != block_index.end(); ++it)
    buf[i++] = *it;
  
  int dest_fd = open64(T::index_file(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  write(dest_fd, buf, sizeof(int)*2*block_index.size());
  close(dest_fd);
  
  free(buf);
}

#endif
