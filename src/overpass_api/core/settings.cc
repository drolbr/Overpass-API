/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "datatypes.h"
#include "settings.h"

#include "../../template_db/file_blocks.h"
#include "../../template_db/file_blocks_index.h"
#include "../../template_db/random_file_index.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>


template < typename TVal >
struct OSM_File_Properties : public File_Properties
{
  OSM_File_Properties(
      const std::string& file_base_name_, uint32 block_size_, uint32 map_block_size_, int32 min_version_ = 0)
    : file_base_name(file_base_name_), block_size(block_size_), map_block_size(map_block_size_),
      min_version(min_version_) {}

  const std::string& get_file_name_trunk() const { return file_base_name; }

  const std::string& get_index_suffix() const { return basic_settings().INDEX_SUFFIX; }
  const std::string& get_data_suffix() const { return basic_settings().DATA_SUFFIX; }
  const std::string& get_id_suffix() const { return basic_settings().ID_SUFFIX; }
  const std::string& get_shadow_suffix() const { return basic_settings().SHADOW_SUFFIX; }

  uint32 get_block_size() const { return block_size/8; }
  uint32 get_compression_factor() const { return 8; }
  uint32 get_compression_method() const { return basic_settings().compression_method; }
  uint32 get_map_block_size() const { return map_block_size/8; }
  uint32 get_map_compression_factor() const { return 8; }
  uint32 get_map_compression_method() const { return basic_settings().map_compression_method; }

  std::vector< bool > get_data_footprint(const std::string& db_dir) const
  {
    std::vector< bool > temp = get_data_index_footprint< TVal >(*this, db_dir, min_version);
    return temp;
  }

  std::vector< bool > get_map_footprint(const std::string& db_dir) const
  {
    return get_map_index_footprint(*this, db_dir);
  }

  uint32 id_max_size_of() const
  {
    return TVal::max_size_of();
  }

  File_Blocks_Index_Base* new_data_index
      (bool writeable, bool use_shadow, const std::string& db_dir, const std::string& file_name_extension)
      const
  {
    if (writeable)
      return new Writeable_File_Blocks_Index< TVal >
          (*this, use_shadow, db_dir, file_name_extension);
    return new Readonly_File_Blocks_Index< TVal >
        (*this, use_shadow, db_dir, file_name_extension);
  }

  std::string file_base_name;
  uint32 block_size;
  uint32 map_block_size;
  int32 min_version;
};


//-----------------------------------------------------------------------------

Basic_Settings::Basic_Settings()
:
  DATA_SUFFIX(".bin"),
  INDEX_SUFFIX(".idx"),
  ID_SUFFIX(".map"),
  SHADOW_SUFFIX(".shadow"),

  base_directory("./"),
  db_logfile_name("database.log"),
  client_logfile_name("transactions.log"),
  shared_name_base("/osm3s"),
  version("0.7.61.7"),
  source_hash("fc8f635e33e5d467b4fbafc65f9fede1220ec384"),
#ifdef HAVE_LZ4
  compression_method(File_Blocks_Index_Base::LZ4_COMPRESSION),
#else
  compression_method(File_Blocks_Index_Base::ZLIB_COMPRESSION),
#endif
  map_compression_method(File_Blocks_Index_Base::NO_COMPRESSION)
{}

Basic_Settings& basic_settings()
{
  static Basic_Settings obj;
  return obj;
}

//-----------------------------------------------------------------------------

Osm_Base_Settings::Osm_Base_Settings()
:
  NODES(new OSM_File_Properties< Uint32_Index >("nodes", 128*1024, 256*1024)),
  NODE_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("node_tags_local", 128*1024, 0)),
  NODE_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("node_tags_global", 128*1024, 0, 7561)),
  NODE_TAGS_GLOBAL_756(new OSM_File_Properties< Tag_Index_Global_Until756 >
      ("node_tags_global", 128*1024, 0)),
  NODE_KEYS(new OSM_File_Properties< Uint32_Index >
      ("node_keys", 512*1024, 0)),
  NODE_FREQUENT_TAGS(new OSM_File_Properties< String_Index >
      ("node_frequent_tags", 512*1024, 0)),

  WAYS(new OSM_File_Properties< Uint31_Index >("ways", 128*1024, 256*1024)),
  WAY_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("way_tags_local", 128*1024, 0)),
  WAY_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("way_tags_global", 128*1024, 0, 7561)),
  WAY_TAGS_GLOBAL_756(new OSM_File_Properties< Tag_Index_Global_Until756 >
      ("way_tags_global", 128*1024, 0)),
  WAY_KEYS(new OSM_File_Properties< Uint32_Index >
      ("way_keys", 512*1024, 0)),
  WAY_FREQUENT_TAGS(new OSM_File_Properties< String_Index >
      ("way_frequent_tags", 512*1024, 0)),

  RELATIONS(new OSM_File_Properties< Uint31_Index >("relations", 512*1024, 256*1024)),
  RELATION_ROLES(new OSM_File_Properties< Uint32_Index >
      ("relation_roles", 512*1024, 0)),
  RELATION_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("relation_tags_local", 128*1024, 0)),
  RELATION_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("relation_tags_global", 128*1024, 0, 7561)),
  RELATION_TAGS_GLOBAL_756(new OSM_File_Properties< Tag_Index_Global_Until756 >
      ("relation_tags_global", 128*1024, 0)),
  RELATION_KEYS(new OSM_File_Properties< Uint32_Index >
      ("relation_keys", 512*1024, 0)),
  RELATION_FREQUENT_TAGS(new OSM_File_Properties< String_Index >
      ("relation_frequent_tags", 512*1024, 0)),

  shared_name(basic_settings().shared_name_base + "_osm_base"),
  max_num_processes(20),
  purge_timeout(900),
  total_available_space(12ll*1024*1024*1024),
  total_available_time_units(256*1024)
{
  bin_idxs_ = {
      NODES, NODE_TAGS_LOCAL, NODE_TAGS_GLOBAL, NODE_KEYS, NODE_FREQUENT_TAGS,
      WAYS, WAY_TAGS_LOCAL, WAY_TAGS_GLOBAL, WAY_KEYS, WAY_FREQUENT_TAGS,
      RELATIONS, RELATION_ROLES, RELATION_TAGS_LOCAL, RELATION_TAGS_GLOBAL, RELATION_KEYS, RELATION_FREQUENT_TAGS };
  map_idxs_ = { NODES, WAYS, RELATIONS };
}


const std::vector< File_Properties* >& Osm_Base_Settings::bin_idxs() const
{
  return bin_idxs_;
}


const std::vector< File_Properties* >& Osm_Base_Settings::map_idxs() const
{
  return map_idxs_;
}


const Osm_Base_Settings& osm_base_settings()
{
  static Osm_Base_Settings obj;
  return obj;
}

//-----------------------------------------------------------------------------

Area_Settings::Area_Settings()
:
  AREA_BLOCKS(new OSM_File_Properties< Uint31_Index >
      ("area_blocks", 512*1024, 64*1024)),
  AREAS(new OSM_File_Properties< Uint31_Index >("areas", 2*1024*1024, 256*1024)),
  AREA_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("area_tags_local", 256*1024, 0)),
  AREA_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("area_tags_global", 512*1024, 0, 7561)),

  shared_name(basic_settings().shared_name_base + "_areas"),
  max_num_processes(5),
  purge_timeout(900),
  total_available_space(4ll*1024*1024*1024),
  total_available_time_units(256*1024)
{}


const Area_Settings& area_settings()
{
  static Area_Settings obj;
  return obj;
}

//-----------------------------------------------------------------------------

Meta_Settings::Meta_Settings()
:
  USER_DATA(new OSM_File_Properties< Uint32_Index >
      ("user_data", 512*1024, 0)),
  USER_INDICES(new OSM_File_Properties< Uint32_Index >
      ("user_indices", 128*1024, 0)),
  NODES_META(new OSM_File_Properties< Node::Index >
      ("nodes_meta", 128*1024, 0)),
  WAYS_META(new OSM_File_Properties< Way::Index >
      ("ways_meta", 128*1024, 0)),
  RELATIONS_META(new OSM_File_Properties< Relation::Index >
      ("relations_meta", 128*1024, 0))
{
  bin_idxs_ = { USER_DATA, USER_INDICES, NODES_META, WAYS_META, RELATIONS_META };
}


const std::vector< File_Properties* >& Meta_Settings::bin_idxs() const
{
  return bin_idxs_;
}


const Meta_Settings& meta_settings()
{
  static Meta_Settings obj;
  return obj;
}

//-----------------------------------------------------------------------------

Attic_Settings::Attic_Settings()
:
  NODES(new OSM_File_Properties< Node::Index >("nodes_attic", 128*1024, 256*1024)),
  NODES_UNDELETED(new OSM_File_Properties< Node::Index >("nodes_attic_undeleted", 128*1024, 64*1024)),
  NODE_IDX_LIST(new OSM_File_Properties< Node::Id_Type >
      ("node_attic_indexes", 128*1024, 0)),
  NODE_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("node_tags_local_attic", 128*1024, 0)),
  NODE_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("node_tags_global_attic", 128*1024, 0, 7561)),
  NODE_TAGS_GLOBAL_756(new OSM_File_Properties< Tag_Index_Global_Until756 >
      ("node_tags_global_attic", 128*1024, 0)),
  NODE_FREQUENT_TAGS(new OSM_File_Properties< String_Index >
      ("node_frequent_tags_attic", 512*1024, 0)),
  NODES_META(new OSM_File_Properties< Node::Index >
      ("nodes_meta_attic", 128*1024, 0)),
  NODE_CHANGELOG(new OSM_File_Properties< Timestamp >
      ("node_changelog", 128*1024, 0)),

  WAYS(new OSM_File_Properties< Way::Index >("ways_attic", 128*1024, 256*1024)),
  WAYS_UNDELETED(new OSM_File_Properties< Way::Index >("ways_attic_undeleted", 128*1024, 64*1024)),
  WAY_IDX_LIST(new OSM_File_Properties< Way::Id_Type >
      ("way_attic_indexes", 128*1024, 0)),
  WAY_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("way_tags_local_attic", 128*1024, 0)),
  WAY_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("way_tags_global_attic", 128*1024, 0, 7561)),
  WAY_TAGS_GLOBAL_756(new OSM_File_Properties< Tag_Index_Global_Until756 >
      ("way_tags_global_attic", 128*1024, 0)),
  WAY_FREQUENT_TAGS(new OSM_File_Properties< String_Index >
      ("way_frequent_tags_attic", 512*1024, 0)),
  WAYS_META(new OSM_File_Properties< Uint31_Index >
      ("ways_meta_attic", 128*1024, 0)),
  WAY_CHANGELOG(new OSM_File_Properties< Timestamp >
      ("way_changelog", 128*1024, 0)),

  RELATIONS(new OSM_File_Properties< Relation::Index >("relations_attic", 512*1024, 256*1024)),
  RELATIONS_UNDELETED(new OSM_File_Properties< Relation::Index >("relations_attic_undeleted", 128*1024, 64*1024)),
  RELATION_IDX_LIST(new OSM_File_Properties< Relation::Id_Type >
      ("relation_attic_indexes", 128*1024, 0)),
  RELATION_TAGS_LOCAL(new OSM_File_Properties< Tag_Index_Local >
      ("relation_tags_local_attic", 128*1024, 0)),
  RELATION_TAGS_GLOBAL(new OSM_File_Properties< Tag_Index_Global >
      ("relation_tags_global_attic", 128*1024, 0, 7561)),
  RELATION_TAGS_GLOBAL_756(new OSM_File_Properties< Tag_Index_Global_Until756 >
      ("relation_tags_global_attic", 128*1024, 0)),
  RELATION_FREQUENT_TAGS(new OSM_File_Properties< String_Index >
      ("relation_frequent_tags_attic", 512*1024, 0)),
  RELATIONS_META(new OSM_File_Properties< Uint31_Index >
      ("relations_meta_attic", 128*1024, 0)),
  RELATION_CHANGELOG(new OSM_File_Properties< Timestamp >
      ("relation_changelog", 128*1024, 0))
{
  bin_idxs_ = {
      NODES, NODES_UNDELETED, NODE_IDX_LIST, NODE_TAGS_LOCAL, NODE_TAGS_GLOBAL, NODE_FREQUENT_TAGS,
      NODES_META, NODE_CHANGELOG,
      WAYS, WAYS_UNDELETED, WAY_IDX_LIST, WAY_TAGS_LOCAL, WAY_TAGS_GLOBAL, WAY_FREQUENT_TAGS,
      WAYS_META, WAY_CHANGELOG,
      RELATIONS, RELATIONS_UNDELETED, RELATION_IDX_LIST,
      RELATION_TAGS_LOCAL, RELATION_TAGS_GLOBAL, RELATION_FREQUENT_TAGS,
      RELATIONS_META, RELATION_CHANGELOG };
  map_idxs_ = {
      NODES, NODES_UNDELETED,
      WAYS, WAYS_UNDELETED,
      RELATIONS, RELATIONS_UNDELETED };
}


const std::vector< File_Properties* >& Attic_Settings::bin_idxs() const
{
  return bin_idxs_;
}


const std::vector< File_Properties* >& Attic_Settings::map_idxs() const
{
  return map_idxs_;
}


const Attic_Settings& attic_settings()
{
  static Attic_Settings obj;
  return obj;
}

//-----------------------------------------------------------------------------

void show_mem_status()
{
  std::ostringstream proc_file_name_("");
  proc_file_name_<<"/proc/"<<getpid()<<"/stat";
  std::ifstream stat(proc_file_name_.str().c_str());
  while (stat.good())
  {
    std::string line;
    getline(stat, line);
    std::cerr<<line;
  }
  std::cerr<<'\n';
}

//-----------------------------------------------------------------------------

Logger::Logger(const std::string& db_dir, const std::string& filename)
  : logfile_full_name(db_dir + (filename.empty() ? basic_settings().db_logfile_name : filename)) {}

void Logger::annotated_log(const std::string& message)
{
  // Collect current time in a user-readable form.
  time_t time_t_ = time(0);
  struct tm* tm_ = gmtime(&time_t_);
  char strftime_buf[21];
  strftime_buf[0] = 0;
  if (tm_)
    strftime(strftime_buf, 21, "%F %H:%M:%S ", tm_);

  std::ofstream out(logfile_full_name.c_str(), std::ios_base::app);
  out<<strftime_buf<<'['<<getpid()<<"] "<<message<<'\n';
}

void Logger::raw_log(const std::string& message)
{
  std::ofstream out(logfile_full_name.c_str(), std::ios_base::app);
  out<<message<<'\n';
}

const std::string& get_logfile_name()
{
  return basic_settings().db_logfile_name;
}


const uint64 NOW = std::numeric_limits< unsigned long long >::max();

//-----------------------------------------------------------------------------

Database_Meta_State::Mode Database_Meta_State::from_db_files(const std::string& db_dir)
{
  for (auto i : attic_settings().bin_idxs())
  {
    if (file_exists(db_dir + i->get_file_name_trunk() + i->get_data_suffix()))
      return Database_Meta_State::keep_attic;
  }

  for (auto i : meta_settings().bin_idxs())
  {
    if (file_exists(db_dir + i->get_file_name_trunk() + i->get_data_suffix()))
      return Database_Meta_State::keep_meta;
  }
  
  return Database_Meta_State::only_data;
}


std::string get_server_name(const std::string& db_dir)
{
  std::string server_name("/api");
  
  try
  {
    std::ifstream server_name_f((db_dir + "server_name").c_str());
    getline(server_name_f, server_name);
  }
  catch(...) {}

  if (!server_name.empty() && server_name.back() != '/')
    return server_name + '/';
  return server_name;
}


void set_server_name(const std::string& db_dir, const std::string& server_name)
{
  std::ofstream out((db_dir + "/server_name").c_str());
  out<<server_name<<'\n';
}
