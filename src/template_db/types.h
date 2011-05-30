#ifndef DE_OSM3S__BACKEND__TYPES
#define DE_OSM3S__BACKEND__TYPES

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <vector>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

struct File_Error
{
  File_Error(uint32 errno_, string filename_, string origin_)
  : error_number(errno_), filename(filename_), origin(origin_) {}
  
  uint32 error_number;
  string filename;
  string origin;
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
  virtual string get_file_name_trunk() const = 0;
  virtual string get_index_suffix() const = 0;
  virtual string get_data_suffix() const = 0;
  virtual string get_id_suffix() const = 0;
  virtual string get_shadow_suffix() const = 0;
  virtual uint32 get_block_size() const = 0;
  virtual uint32 get_map_block_size() const = 0;
  virtual vector< bool > get_data_footprint(const string& db_dir) const = 0;
  virtual vector< bool > get_map_footprint(const string& db_dir) const = 0;
  virtual uint32 id_max_size_of() const = 0;
  
  // The returned object is of type File_Blocks_Index< .. >*
  // and goes into the ownership of the caller.
  virtual File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, string db_dir, string file_name_extension)
      const = 0;
};

/** Simple RAII class to keep a file descriptor. */
class Raw_File
{
  public:
    Raw_File(string name, int oflag, mode_t mode, string caller_id);
    ~Raw_File() { close(fd_); }
    int fd() const { return fd_; }
  
  private:
    int fd_;
};

/** Simple RAII class to keep a pointer to some memory on the heap.
 *  WARNING: Doesn't work properly with copy constructors. */
template < class T >
struct Void_Pointer
{
  Void_Pointer(int block_size) { ptr = (T*)malloc(block_size); }
  ~Void_Pointer() { free(ptr); }
  
  public:
    T* ptr;
};

template < class T >
class Index_Smart_Pimpl
{
  public:
    Index_Smart_Pimpl(string source_index_file_name,
		      string dest_index_file_name,
		      uint32 block_count);
    Index_Smart_Pimpl(const Index_Smart_Pimpl&);
    ~Index_Smart_Pimpl();
    Index_Smart_Pimpl& operator=(const Index_Smart_Pimpl&);
    
    T* operator->() { return pimpl; }
    
  private:
    T* pimpl;
};

inline Raw_File::Raw_File(string name, int oflag, mode_t mode, string caller_id)
  : fd_(0)
{
  fd_ = open64(name.c_str(), oflag, mode);
  if (fd_ < 0)
    throw File_Error(errno, name, caller_id);
}

template < class T >
Index_Smart_Pimpl< T >::Index_Smart_Pimpl
    (string source_index_file_name, string dest_index_file_name, uint32 block_count)
    : pimpl(new T(source_index_file_name, dest_index_file_name, block_count))
{
  ++(pimpl->count);
}

template < class T >
Index_Smart_Pimpl< T >::Index_Smart_Pimpl(const Index_Smart_Pimpl& isp)
  : pimpl(isp.pimpl)
{
  ++(pimpl->count);
}

template < class T >
Index_Smart_Pimpl< T >::~Index_Smart_Pimpl()
{
  if (--(pimpl->count) == 0)
    delete pimpl;
}

template < class T >
Index_Smart_Pimpl< T >& Index_Smart_Pimpl< T >::operator=(const Index_Smart_Pimpl& isp)
{
  if (pimpl == isp.pimpl)
    return *this;
  
  if (--(pimpl->count) == 0)
    delete pimpl;
  pimpl = isp.pimpl;
  ++(pimpl->count);
  
  return *this;
}

#endif
