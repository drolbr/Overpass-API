#include <map>
#include <string>
#include "settings.h"

struct Properties
{
  string file_base_name;
  uint32 block_size;
};

//-----------------------------------------------------------------------------

string BASE_DIRECTORY("./");
string DATA_SUFFIX(".bin");
string INDEX_SUFFIX(".idx");
string ID_SUFFIX(".map");

struct OSM_File_Properties : File_Properties
{
  OSM_File_Properties(string file_base_name_, uint32 block_size_)
    : file_base_name(file_base_name_), block_size(block_size_) {}
  
  string get_basedir() const
  {
    return BASE_DIRECTORY;
  }
  
  string get_file_base_name() const
  {
    return BASE_DIRECTORY + file_base_name;
  }
  
  string get_index_suffix() const
  {
    return INDEX_SUFFIX;
  }
  
  string get_data_suffix() const
  {
    return DATA_SUFFIX;
  }
  
  string get_id_suffix() const
  {
    return ID_SUFFIX;
  }
  
  uint32 get_block_size() const
  {
    return block_size;
  }

  string file_base_name;
  uint32 block_size;
};

File_Properties* de_osm3s_file_ids::NODES
  = new OSM_File_Properties("nodes", 512*1024);
File_Properties* de_osm3s_file_ids::NODE_TAGS_LOCAL
  = new OSM_File_Properties("node_tags_local", 512*1024);
File_Properties* de_osm3s_file_ids::NODE_TAGS_GLOBAL
  = new OSM_File_Properties("node_tags_global", 2*1024*1024);
File_Properties* de_osm3s_file_ids::WAYS
  = new OSM_File_Properties("ways", 512*1024);
File_Properties* de_osm3s_file_ids::WAY_TAGS_LOCAL
  = new OSM_File_Properties("way_tags_local", 512*1024);
File_Properties* de_osm3s_file_ids::WAY_TAGS_GLOBAL
= new OSM_File_Properties("way_tags_global", 2*1024*1024);
File_Properties* de_osm3s_file_ids::RELATIONS
  = new OSM_File_Properties("relations", 512*1024);
File_Properties* de_osm3s_file_ids::RELATION_ROLES
  = new OSM_File_Properties("relation_roles", 512*1024);
File_Properties* de_osm3s_file_ids::RELATION_TAGS_LOCAL
  = new OSM_File_Properties("relation_tags_local", 512*1024);
File_Properties* de_osm3s_file_ids::RELATION_TAGS_GLOBAL
  = new OSM_File_Properties("relation_tags_global", 2*1024*1024);

string get_basedir()
{
  return BASE_DIRECTORY;
}
