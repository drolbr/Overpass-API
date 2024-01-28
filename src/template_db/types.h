/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
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
  virtual bool empty() const = 0;
  virtual ~File_Blocks_Index_Base() {}

  virtual std::string get_data_file_name() const = 0;
  virtual uint64 get_block_size() const = 0;
  virtual uint32 get_compression_factor() const = 0;
  virtual uint32 get_compression_method() const = 0;
  virtual uint32 get_block_count() const = 0;
  virtual int32 get_file_format_version() const = 0;

  static const int USE_DEFAULT = -1;
  static const int NO_COMPRESSION = 0;
  static const int ZLIB_COMPRESSION = 1;
  static const int LZ4_COMPRESSION = 2;
  static const unsigned int IDX_HEADER_LENGTH = 8;
};


enum class Access_Mode
{ readonly, writeable, truncate };


struct File_Properties
{
  virtual const std::string& get_file_name_trunk() const = 0;
  virtual const std::string& get_index_suffix() const = 0;
  virtual const std::string& get_data_suffix() const = 0;
  virtual const std::string& get_id_suffix() const = 0;
  virtual const std::string& get_shadow_suffix() const = 0;
  virtual uint32 get_block_size() const = 0;
  virtual uint32 get_compression_factor() const = 0;
  virtual uint32 get_compression_method() const = 0;
  virtual uint32 get_map_block_size() const = 0;
  virtual uint32 get_map_compression_factor() const = 0;
  virtual uint32 get_map_compression_method() const = 0;
  virtual std::vector< bool > get_data_footprint(const std::string& db_dir) const = 0;
  virtual std::vector< bool > get_map_footprint(const std::string& db_dir) const = 0;
  virtual uint32 id_max_size_of() const = 0;

  // The returned object is of type File_Blocks_Index< .. >*
  // and goes into the ownership of the caller.
  virtual File_Blocks_Index_Base* new_data_index(
      Access_Mode access_mode, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
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
    void read(void* buf, uint64 size, const std::string& caller_id) const;
    void write(void* buf, uint64 size, const std::string& caller_id) const;
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
  explicit Void_Pointer(int block_size) { ptr = block_size > 0 ? (T*)malloc(block_size) : 0; }
  ~Void_Pointer() { clear(); }

  Void_Pointer(Void_Pointer&& rhs) : ptr(rhs.ptr)
  {
    rhs.ptr = nullptr;
  }
  Void_Pointer& operator=(Void_Pointer&& rhs)
  {
    if (ptr != rhs.ptr)
    {
      clear();
      ptr = rhs.ptr;
      rhs.ptr = nullptr;
    }
    return *this;
  }

  void clear()
  {
    if (ptr)
      free(ptr);
    ptr = 0;
  }
  void resize(int block_size)
  {
    clear();
    ptr = 0;
    if (block_size > 0)
      ptr = (T*)malloc(block_size);
  }

  T* ptr;
};



/** Simple RAII class to keep a pointer to some memory on the heap. Enforces 64 bit alignment. */
template < class T >
class Void64_Pointer
{
  Void64_Pointer(const Void64_Pointer&);
  Void64_Pointer& operator=(const Void64_Pointer&);

public:
  explicit Void64_Pointer(int block_size)
  { 
    ptr = block_size > 0 ? (T*)aligned_alloc(8, block_size) : 0;
  }
  ~Void64_Pointer() { clear(); }

  Void64_Pointer(Void64_Pointer&& rhs) : ptr(rhs.ptr)
  {
    rhs.ptr = nullptr;
  }
  Void64_Pointer& operator=(Void64_Pointer&& rhs)
  {
    if (ptr != rhs.ptr)
    {
      clear();
      ptr = rhs.ptr;
      rhs.ptr = nullptr;
    }
    return *this;
  }

  void clear()
  {
    if (ptr)
      free(ptr);
    ptr = 0;
  }
  void resize(int block_size)
  {
    clear();
    ptr = 0;
    if (block_size > 0)
      ptr = (T*)aligned_alloc(8, block_size);
  }
  void swap(Void64_Pointer& rhs)
  {
    T* temp = ptr;
    ptr = rhs.ptr;
    rhs.ptr = temp;
  }

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
  off64_t size = lseek64(fd_, 0, SEEK_END);
  if (size < 0)
    throw File_Error(errno, name, caller_id);
  off64_t foo = lseek64(fd_, 0, SEEK_SET);
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

inline void Raw_File::read(void* buf, uint64 size, const std::string& caller_id) const
{
  uint64 foo = ::read(fd_, buf, size);
  if (foo != size)
    throw File_Error(errno, name, caller_id);
}

inline void Raw_File::write(void* buf, uint64 size, const std::string& caller_id) const
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

//-----------------------------------------------------------------------------


template< typename Int >
int shift_log(Int val)
{
  int count = 0;
  while (val > 1)
  {
    val = val>>1;
    ++count;
  }
  return count;
}


template< typename Iterator, typename Object >
void rearrange_block(const Iterator& begin, Iterator& it, Object to_move)
{
  Iterator predecessor = it;
  if (it != begin)
    --predecessor;
  while (to_move < *predecessor)
  {
    *it = *predecessor;
    --it;
    if (it == begin)
      break;
    --predecessor;
  }
  *it = to_move;
}


inline void zero_padding(uint8* from, uint32 bytes)
{
  for (uint32 i = 0; i < bytes; ++i)
    *(from + i) = 0;
}


//-----------------------------------------------------------------------------


int& global_read_counter();

enum Signal_Status { absent = 0, received, processed };
Signal_Status& sigterm_status();
void sigterm(int);


void millisleep(uint32 milliseconds);


void copy_file(const std::string& source, const std::string& dest);
void force_link_file(const std::string& source, const std::string& dest);


#endif
