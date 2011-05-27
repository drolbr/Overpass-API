#include "datatypes.h"
#include "settings.h"

#include "../../template_db/file_blocks.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
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
string SHADOW_SUFFIX(".shadow");
string logfile_name("transactions.log");

template < typename TVal >
struct OSM_File_Properties : public File_Properties
{
  OSM_File_Properties(string file_base_name_, uint32 block_size_,
		      uint32 map_block_size_)
    : file_base_name(file_base_name_), block_size(block_size_),
      map_block_size(map_block_size_ > 0 ? map_block_size_*TVal::max_size_of() : 0) {}
  
  string get_basedir() const
  {
    return base_directory;
  }
  
  string get_file_base_name() const
  {
    return base_directory + file_base_name;
  }
  
  string get_file_name_trunk() const
  {
    return file_base_name;
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
  
  string get_shadow_suffix() const
  {
    return SHADOW_SUFFIX;
  }
  
  uint32 get_block_size() const
  {
    return block_size;
  }

  uint32 get_map_block_size() const
  {
    return map_block_size;
  }
  
  vector< bool > get_data_footprint() const
  {
    return vector< bool >();
  }
  
  vector< bool > get_map_footprint() const
  {
    return vector< bool >();
  }
  
  uint32 id_max_size_of() const
  {
    return TVal::max_size_of();
  }
  
  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, string db_dir, string file_name_extension)
      const
  {
    return new File_Blocks_Index< TVal >
        (*this, writeable, use_shadow, db_dir, file_name_extension);
  }
		  
  string file_base_name;
  uint32 block_size;
  uint32 map_block_size;
};

File_Properties* de_osm3s_file_ids::NODES
  = new OSM_File_Properties< Uint32_Index >("nodes", 512*1024, 64*1024);
File_Properties* de_osm3s_file_ids::NODE_TAGS_LOCAL
  = new OSM_File_Properties< Tag_Index_Local >
      ("node_tags_local", 512*1024, 0);
File_Properties* de_osm3s_file_ids::NODE_TAGS_GLOBAL
  = new OSM_File_Properties< Tag_Index_Global >
      ("node_tags_global", 2*1024*1024, 0);
File_Properties* de_osm3s_file_ids::WAYS
  = new OSM_File_Properties< Uint31_Index >("ways", 512*1024, 64*1024);
File_Properties* de_osm3s_file_ids::WAY_TAGS_LOCAL
  = new OSM_File_Properties< Tag_Index_Local >
      ("way_tags_local", 512*1024, 0);
File_Properties* de_osm3s_file_ids::WAY_TAGS_GLOBAL
  = new OSM_File_Properties< Tag_Index_Global >
      ("way_tags_global", 2*1024*1024, 0);
File_Properties* de_osm3s_file_ids::RELATIONS
  = new OSM_File_Properties< Uint31_Index >("relations", 1024*1024, 64*1024);
File_Properties* de_osm3s_file_ids::RELATION_ROLES
  = new OSM_File_Properties< Uint32_Index >("relation_roles", 512*1024, 0);
File_Properties* de_osm3s_file_ids::RELATION_TAGS_LOCAL
  = new OSM_File_Properties< Tag_Index_Local >
      ("relation_tags_local", 512*1024, 0);
File_Properties* de_osm3s_file_ids::RELATION_TAGS_GLOBAL
  = new OSM_File_Properties< Tag_Index_Global >
      ("relation_tags_global", 2*1024*1024, 0);
File_Properties* de_osm3s_file_ids::AREA_BLOCKS
  = new OSM_File_Properties< Uint31_Index >("area_blocks", 512*1024, 64*1024);
File_Properties* de_osm3s_file_ids::AREAS
  = new OSM_File_Properties< Uint31_Index >("areas", 512*1024, 64*1024);
File_Properties* de_osm3s_file_ids::AREA_TAGS_LOCAL
  = new OSM_File_Properties< Tag_Index_Local >
      ("area_tags_local", 256*1024, 0);
File_Properties* de_osm3s_file_ids::AREA_TAGS_GLOBAL
  = new OSM_File_Properties< Tag_Index_Global >
      ("area_tags_global", 512*1024, 0);
  
uint64 de_osm3s_file_ids::max_allowed_space(512*1024*1024);
uint32 de_osm3s_file_ids::max_allowed_time(3600*24);

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

//-----------------------------------------------------------------------------

Logger::Logger(const string& db_dir)
  : logfile_full_name(db_dir + get_logfile_name()) {}

void Logger::annotated_log(const string& message)
{
  // Collect current time in a user-readable form.
  time_t time_t_ = time(0);
  struct tm* tm_ = gmtime(&time_t_);
  char strftime_buf[21];
  strftime_buf[0] = 0;
  if (tm_)
    strftime(strftime_buf, 21, "%F %H:%M:%S ", tm_);
  
  ofstream out(logfile_full_name.c_str(), ios_base::app);
  out<<strftime_buf<<'['<<getpid()<<"] "<<message<<'\n';
}

void Logger::raw_log(const string& message)
{
  ofstream out(logfile_full_name.c_str(), ios_base::app);
  out<<message<<'\n';
}

string get_logfile_name()
{
  return logfile_name;
}
