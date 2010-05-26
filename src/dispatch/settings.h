#ifndef DE_OSM3S__DISPATCH__SETTINGS
#define DE_OSM3S__DISPATCH__SETTINGS

#include <string>

using namespace std;

struct de_osm3s_file_ids
{
  const static int32 NODES = 1001;
  const static int32 NODE_TAGS_LOCAL = 1011;
  const static int32 NODE_TAGS_GLOBAL = 1012;
  const static int32 WAYS = 2001;
  const static int32 WAY_TAGS_LOCAL = 2011;
  const static int32 WAY_TAGS_GLOBAL = 2012;
  const static int32 RELATIONS = 3001;
  const static int32 RELATION_ROLES = 3002;
  const static int32 RELATION_TAGS_LOCAL = 3011;
  const static int32 RELATION_TAGS_GLOBAL = 3012;
};

//-----------------------------------------------------------------------------

string get_basedir();
string get_file_base_name(int32 FILE_PROPERTIES);
string get_index_suffix(int32 FILE_PROPERTIES);
string get_data_suffix(int32 FILE_PROPERTIES);
string get_id_suffix(int32 FILE_PROPERTIES);
uint32 get_block_size(int32 FILE_PROPERTIES);

#endif
