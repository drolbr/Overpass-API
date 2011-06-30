#ifndef DE__OSM3S___TEMPLATE_DB__TYPES_H
#define DE__OSM3S___TEMPLATE_DB__TYPES_H

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
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

const int S_666 = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

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
  Raw_File(const Raw_File&);
  Raw_File operator=(const Raw_File&);
  
  public:
    Raw_File(string name, int oflag, mode_t mode, string caller_id);
    ~Raw_File() { close(fd_); }
    int fd() const { return fd_; }
  
  private:
    int fd_;
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

//-----------------------------------------------------------------------------

inline Raw_File::Raw_File(string name, int oflag, mode_t mode, string caller_id)
  : fd_(0)
{
  fd_ = open64(name.c_str(), oflag, mode);
  if (fd_ < 0)
    throw File_Error(errno, name, caller_id);
  fchmod(fd_, mode);
}

#endif
