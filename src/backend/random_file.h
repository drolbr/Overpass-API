#ifndef DE_OSM3S__BACKEND__RANDOM_FILE
#define DE_OSM3S__BACKEND__RANDOM_FILE

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <vector>

#include "types.h"

/**
 *
 *
 * TIndex must offer
 *
 * static uint32 max_size_of()
 */

using namespace std;

template< class TIndex >
struct Random_File
{
private:
  Random_File(const Random_File& f) {}
  
public:
  Random_File(const File_Properties& file_prop, bool writeable)
    : index_size(TIndex::max_size_of())
  {
    id_file_name = file_prop.get_file_base_name()
        + file_prop.get_id_suffix();
    this->writeable = writeable;
    
    // open data file
    if (writeable)
      data_fd = open64
	  (id_file_name.c_str(),
	   O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    else
      data_fd = open64
	  (id_file_name.c_str(), O_RDONLY);
    if (data_fd < 0)
      throw File_Error(errno, id_file_name, "Random_File:1");
    
    // reserve buffer for read/write actions
    buffer = malloc(index_size);
    zero = calloc(index_size, 64*1024);

    // determine the data file's size
    file_size = lseek64(data_fd, 0, SEEK_END);
  }
  
  ~Random_File()
  {
    free(buffer);
    free(zero);
    close(data_fd);
  }

  TIndex get(uint32 pos) const
  {
    uint8* result((uint8*)buffer);
    
    if ((uint64)pos*(index_size) >= file_size)
      result = (uint8*)zero;
    else
    {
      lseek64(data_fd, (int64)pos*(index_size), SEEK_SET);
      uint32 foo(read(data_fd, buffer, index_size)); foo = 0;
    }
    
    return TIndex(result);
  }

  void put(uint32 pos, const TIndex& index)
  {
    index.to_data(buffer);
    
    if ((uint64)pos*(index_size) > file_size)
    {
      lseek64(data_fd, file_size, SEEK_SET);
      while ((uint64)pos*(index_size) > file_size)
      {
	uint32 foo(write(data_fd, zero, index_size*64*1024)); foo = 0;
	file_size += 64*1024*index_size;
      }
    }
    if ((uint64)pos*(index_size) == file_size)
      file_size += index_size;
    
    lseek64(data_fd, (int64)pos*(index_size), SEEK_SET);
    uint32 foo(write(data_fd, buffer, index_size)); foo = 0;
  }
  
private:
  string id_file_name;
  bool writeable;
  uint32 index_size;
  uint64 file_size;
  
  int data_fd;
  void* buffer;
  void* zero;
};

#endif
