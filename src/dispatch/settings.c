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

map< uint32, Properties > init_settings()
{
  map< uint32, Properties > settings;
  Properties p;
  
  p.file_base_name = BASE_DIRECTORY + "nodes";
  p.block_size = 4*1024;
  settings[de_osm3s_file_ids::NODES] = p;
  
  p.file_base_name = BASE_DIRECTORY + "ways";
  p.block_size = 4*1024;
  settings[de_osm3s_file_ids::WAYS] = p;
  
  p.file_base_name = BASE_DIRECTORY + "relations";
  p.block_size = 2*1024;
  settings[de_osm3s_file_ids::RELATIONS] = p;
  
  return settings;
}

//-----------------------------------------------------------------------------

const map< uint32, Properties > settings(init_settings());

string get_file_base_name(int32 FILE_PROPERTIES)
{
  map< uint32, Properties >::const_iterator it(settings.find(FILE_PROPERTIES));
  if (it == settings.end())
    throw File_Properties_Exception(FILE_PROPERTIES);
  return it->second.file_base_name;
}

string get_index_suffix(int32 FILE_PROPERTIES)
{
  return INDEX_SUFFIX;
}

string get_data_suffix(int32 FILE_PROPERTIES)
{
  return DATA_SUFFIX;
}

uint32 get_block_size(int32 FILE_PROPERTIES)
{
  map< uint32, Properties >::const_iterator it(settings.find(FILE_PROPERTIES));
  if (it == settings.end())
    throw File_Properties_Exception(FILE_PROPERTIES);
  return it->second.block_size;
}
