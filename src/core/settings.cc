#include <iostream>
#include <map>
#include <string>
#include "settings.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

struct Properties
{
  string file_base_name;
  uint32 block_size;
};

//-----------------------------------------------------------------------------

string base_directory("./");
string DATA_SUFFIX(".bin");
string INDEX_SUFFIX(".idx");
string ID_SUFFIX(".map");

struct OSM_File_Properties : File_Properties
{
  OSM_File_Properties(string file_base_name_, uint32 block_size_)
    : file_base_name(file_base_name_), block_size(block_size_) {}
  
  string get_basedir() const
  {
    return base_directory;
  }
  
  string get_file_base_name() const
  {
    return base_directory + file_base_name;
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
File_Properties* de_osm3s_file_ids::AREA_BLOCKS
  = new OSM_File_Properties("area_blocks", 512*1024);
File_Properties* de_osm3s_file_ids::AREAS
  = new OSM_File_Properties("areas", 512*1024);
File_Properties* de_osm3s_file_ids::AREA_TAGS_LOCAL
  = new OSM_File_Properties("area_tags_local", 256*1024);
File_Properties* de_osm3s_file_ids::AREA_TAGS_GLOBAL
  = new OSM_File_Properties("area_tags_global", 512*1024);

string get_basedir()
{
  return base_directory;
}

void set_basedir(string basedir)
{
  base_directory = basedir;
}

void show_mem_status()
{
  ostringstream proc_file_name_("");
  proc_file_name_<<"/proc/"<<getpid()<<"/stat";
  ifstream stat(proc_file_name_.str().c_str());
  while (stat.good())
  {
    string line;
    getline(stat, line);
    cerr<<line;
  }
  cerr<<'\n';
}
