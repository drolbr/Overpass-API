#ifndef DE_OSM3S__DISPATCH__SETTINGS
#define DE_OSM3S__DISPATCH__SETTINGS

#include <string>

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

struct de_osm3s_file_ids
{
  const static int32 NODES = 1001;
  const static int32 WAYS = 2001;
  const static int32 RELATIONS = 3001;
};

//-----------------------------------------------------------------------------

string get_file_base_name(int32 FILE_PROPERTIES);
string get_index_suffix(int32 FILE_PROPERTIES);
string get_data_suffix(int32 FILE_PROPERTIES);
uint32 get_block_size(int32 FILE_PROPERTIES);

struct File_Properties_Exception
{
  File_Properties_Exception(int32 i) : id(i) {}
  
  int32 id;
};

#endif
