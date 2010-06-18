#ifndef DE_OSM3S__DISPATCH__SETTINGS
#define DE_OSM3S__DISPATCH__SETTINGS

#include <string>

#include "../backend/types.h"

using namespace std;

struct de_osm3s_file_ids
{
  static File_Properties* NODES;
  static File_Properties* NODE_TAGS_LOCAL;
  static File_Properties* NODE_TAGS_GLOBAL;
  static File_Properties* WAYS;
  static File_Properties* WAY_TAGS_LOCAL;
  static File_Properties* WAY_TAGS_GLOBAL;
  static File_Properties* RELATIONS;
  static File_Properties* RELATION_ROLES;
  static File_Properties* RELATION_TAGS_LOCAL;
  static File_Properties* RELATION_TAGS_GLOBAL;
};

string get_basedir();
void set_basedir(string basedir);

void show_mem_status();

//-----------------------------------------------------------------------------

#endif
