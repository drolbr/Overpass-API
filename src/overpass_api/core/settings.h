#ifndef DE_OSM3S__DISPATCH__SETTINGS
#define DE_OSM3S__DISPATCH__SETTINGS

#include <string>

#include "../../template_db/types.h"

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
  static File_Properties* AREA_BLOCKS;
  static File_Properties* AREAS;
  static File_Properties* AREA_TAGS_LOCAL;
  static File_Properties* AREA_TAGS_GLOBAL;
  
  static uint64 max_allowed_space;
  static uint32 max_allowed_time;  
};

string get_basedir();
void set_basedir(string basedir);
string get_logfile_name();

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

//-----------------------------------------------------------------------------

#endif
