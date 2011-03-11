#ifndef DE_OSM3S__BACKEND__TYPES
#define DE_OSM3S__BACKEND__TYPES

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
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

struct File_Properties
{
  virtual string get_basedir() const = 0;
  virtual string get_file_base_name() const = 0;
  virtual string get_index_suffix() const = 0;
  virtual string get_data_suffix() const = 0;
  virtual string get_id_suffix() const = 0;
  virtual uint32 get_block_size() const = 0;
};

#endif
