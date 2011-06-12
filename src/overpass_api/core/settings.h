#ifndef DE_OSM3S__DISPATCH__SETTINGS
#define DE_OSM3S__DISPATCH__SETTINGS

#include <string>

#include "../../template_db/types.h"

using namespace std;

struct Basic_Settings
{
  uint64 max_allowed_space;
  uint32 max_allowed_time;
  
  string DATA_SUFFIX;
  string INDEX_SUFFIX;
  string ID_SUFFIX;
  string SHADOW_SUFFIX;
  
  string base_directory;
  string logfile_name;
  string shared_name_base;
  
  Basic_Settings();
};

struct Osm_Base_Settings
{
  File_Properties* NODES;
  File_Properties* NODE_TAGS_LOCAL;
  File_Properties* NODE_TAGS_GLOBAL;
  File_Properties* WAYS;
  File_Properties* WAY_TAGS_LOCAL;
  File_Properties* WAY_TAGS_GLOBAL;
  File_Properties* RELATIONS;
  File_Properties* RELATION_ROLES;
  File_Properties* RELATION_TAGS_LOCAL;
  File_Properties* RELATION_TAGS_GLOBAL;

  string shared_name;
  
  Osm_Base_Settings();
};

struct Area_Settings
{
  File_Properties* AREA_BLOCKS;
  File_Properties* AREAS;
  File_Properties* AREA_TAGS_LOCAL;
  File_Properties* AREA_TAGS_GLOBAL;
  
  string shared_name;
  
  Area_Settings();
};

Basic_Settings& basic_settings();
const Osm_Base_Settings& osm_base_settings();
const Area_Settings& area_settings();

void show_mem_status();

class Logger
{
  public:
    Logger(const string& db_dir);
    void annotated_log(const string& message);
    void raw_log(const string& message);
    
  private:
    string logfile_full_name;
};

#endif
