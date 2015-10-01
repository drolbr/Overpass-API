/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Template_DB.
*
* Template_DB is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Template_DB is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Template_DB.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___TEMPLATE_DB__TYPES_H
#define DE__OSM3S___TEMPLATE_DB__TYPES_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef NATIVE_LARGE_FILES
#define ftruncate64 ftruncate
#define lseek64 lseek
#define open64 open
#endif


typedef unsigned int uint;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;


const int S_666 = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;


struct File_Error
{
  File_Error(uint32 errno_, const std::string& filename_, const std::string& origin_)
  : error_number(errno_), filename(filename_), origin(origin_) {}
  
  uint32 error_number;
  std::string filename;
  std::string origin;
};


struct File_Properties_Exception
{
  File_Properties_Exception(int32 i) : id(i) {}
  
  int32 id;
};


struct File_Blocks_Index_Base
{
  virtual ~File_Blocks_Index_Base() {}
};


struct File_Properties
{
  virtual const std::string& get_file_name_trunk() const = 0;
  virtual const std::string& get_index_suffix() const = 0;
  virtual const std::string& get_data_suffix() const = 0;
  virtual const std::string& get_id_suffix() const = 0;
  virtual const std::string& get_shadow_suffix() const = 0;
  virtual uint32 get_block_size() const = 0;
  virtual uint32 get_max_size() const = 0;
  virtual uint32 get_compression_method() const = 0;
  virtual uint32 get_map_block_size() const = 0;
  virtual std::vector< bool > get_data_footprint(const std::string& db_dir) const = 0;
  virtual std::vector< bool > get_map_footprint(const std::string& db_dir) const = 0;
  virtual uint32 id_max_size_of() const = 0;
  
  // The returned object is of type File_Blocks_Index< .. >*
  // and goes into the ownership of the caller.
  virtual File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const = 0;
};


/** Simple RAII class to keep a file descriptor. */
class Raw_File
{
  Raw_File(const Raw_File&);
  Raw_File operator=(const Raw_File&);
  
  public:
    Raw_File(const std::string& name, int oflag, mode_t mode, const std::string& caller_id);
    ~Raw_File() { close(fd_); }
    int fd() const { return fd_; }
    uint64 size(const std::string& caller_id) const;
    void resize(uint64 size, const std::string& caller_id) const;
    void read(uint8* buf, uint64 size, const std::string& caller_id) const;
    void write(uint8* buf, uint64 size, const std::string& caller_id) const;
    void seek(uint64 pos, const std::string& caller_id) const;
    
  private:
    int fd_;
    std::string name;
};


/** Simple RAII class to keep a pointer to some memory on the heap. */
template < class T >
class Void_Pointer
{
  Void_Pointer(const Void_Pointer&);
  Void_Pointer& operator=(const Void_Pointer&);
  
  public:
    Void_Pointer(int block_size) { ptr = (T*)malloc(block_size); }
    ~Void_Pointer() { free(ptr); }
  
    T* ptr;
};


inline bool file_exists(const std::string& filename)
{
  return (access(filename.c_str(), F_OK) == 0);
}


/** Simple RAII class to keep a unix socket. */
class Unix_Socket
{
public:
  Unix_Socket(const std::string& socket_name = "", uint max_num_reading_processes = 0);
  void open(const std::string& socket_name);
  ~Unix_Socket();
  
  int descriptor() const { return socket_descriptor; }
  
private:
  int socket_descriptor;
  uint max_num_reading_processes;
};


//-----------------------------------------------------------------------------

inline Raw_File::Raw_File(const std::string& name_, int oflag, mode_t mode, const std::string& caller_id)
  : fd_(0), name(name_)
{
  fd_ = open64(name.c_str(), oflag, mode);
  if (fd_ < 0)
    throw File_Error(errno, name, caller_id);
  if (mode & O_CREAT)
  {
    int ret = fchmod(fd_, mode);
    if (ret < 0)
      throw File_Error(errno, name, caller_id + "::fchmod");
  }
}

inline uint64 Raw_File::size(const std::string& caller_id) const
{
  uint64 size = lseek64(fd_, 0, SEEK_END);
  uint64 foo = lseek64(fd_, 0, SEEK_SET);
  if (foo != 0)
    throw File_Error(errno, name, caller_id);
  return size;
}

inline void Raw_File::resize(uint64 size, const std::string& caller_id) const
{
  uint64 foo = ftruncate64(fd_, size);
  if (foo != 0)
    throw File_Error(errno, name, caller_id);
}

inline void Raw_File::read(uint8* buf, uint64 size, const std::string& caller_id) const
{
  uint64 foo = ::read(fd_, buf, size);
  if (foo != size)
    throw File_Error(errno, name, caller_id);
}

inline void Raw_File::write(uint8* buf, uint64 size, const std::string& caller_id) const
{
  uint64 foo = ::write(fd_, buf, size);
  if (foo != size)
    throw File_Error(errno, name, caller_id);
}

inline void Raw_File::seek(uint64 pos, const std::string& caller_id) const
{
  uint64 foo = lseek64(fd_, pos, SEEK_SET);
  if (foo != pos)
    throw File_Error(errno, name, caller_id);
}


int& global_read_counter();


void millisleep(uint32 milliseconds);


void copy_file(const std::string& source, const std::string& dest);


#endif
