/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#ifndef DE__OSM3S___OVERPASS_API__CORE__SETTINGS_H
#define DE__OSM3S___OVERPASS_API__CORE__SETTINGS_H

#include <limits>
#include <string>

#include "../../template_db/types.h"


struct Basic_Settings
{
  std::string DATA_SUFFIX;
  std::string INDEX_SUFFIX;
  std::string ID_SUFFIX;
  std::string SHADOW_SUFFIX;
  
  std::string base_directory;
  std::string logfile_name;
  std::string shared_name_base;
  
  uint32 compression_method;
  
  Basic_Settings();
};


struct Osm_Base_Settings
{
  File_Properties* NODES;
  File_Properties* NODE_TAGS_LOCAL;
  File_Properties* NODE_TAGS_GLOBAL;
  File_Properties* NODE_KEYS;
  File_Properties* WAYS;
  File_Properties* WAY_TAGS_LOCAL;
  File_Properties* WAY_TAGS_GLOBAL;
  File_Properties* WAY_KEYS;
  File_Properties* RELATIONS;
  File_Properties* RELATION_ROLES;
  File_Properties* RELATION_TAGS_LOCAL;
  File_Properties* RELATION_TAGS_GLOBAL;
  File_Properties* RELATION_KEYS;
  
  std::string shared_name;
  uint max_num_processes;
  uint purge_timeout;
  uint64 total_available_space;
  uint64 total_available_time_units;
  
  Osm_Base_Settings();
};


struct Area_Settings
{
  File_Properties* AREA_BLOCKS;
  File_Properties* AREAS;
  File_Properties* AREA_TAGS_LOCAL;
  File_Properties* AREA_TAGS_GLOBAL;
  
  std::string shared_name;
  uint max_num_processes;
  uint purge_timeout;
  uint64 total_available_space;
  uint64 total_available_time_units;
  
  Area_Settings();
};


struct Meta_Settings
{
  File_Properties* USER_DATA;
  File_Properties* USER_INDICES;
  File_Properties* NODES_META;
  File_Properties* WAYS_META;
  File_Properties* RELATIONS_META;
  
  Meta_Settings();
};


struct Attic_Settings
{
  File_Properties* NODES;
  File_Properties* NODES_UNDELETED;
  File_Properties* NODE_IDX_LIST;
  File_Properties* NODE_TAGS_LOCAL;
  File_Properties* NODE_TAGS_GLOBAL;
  File_Properties* NODES_META;
  File_Properties* NODE_CHANGELOG;
  File_Properties* WAYS;
  File_Properties* WAYS_UNDELETED;
  File_Properties* WAY_IDX_LIST;
  File_Properties* WAY_TAGS_LOCAL;
  File_Properties* WAY_TAGS_GLOBAL;
  File_Properties* WAYS_META;
  File_Properties* WAY_CHANGELOG;
  File_Properties* RELATIONS;
  File_Properties* RELATIONS_UNDELETED;
  File_Properties* RELATION_IDX_LIST;
  File_Properties* RELATION_TAGS_LOCAL;
  File_Properties* RELATION_TAGS_GLOBAL;
  File_Properties* RELATIONS_META;
  File_Properties* RELATION_CHANGELOG;
  
  Attic_Settings();
};


Basic_Settings& basic_settings();
const Osm_Base_Settings& osm_base_settings();
const Area_Settings& area_settings();
const Meta_Settings& meta_settings();
const Attic_Settings& attic_settings();

void show_mem_status();


class Logger
{
  public:
    Logger(const std::string& db_dir);
    void annotated_log(const std::string& message);
    void raw_log(const std::string& message);
    
  private:
    std::string logfile_full_name;
};


const uint64 NOW = std::numeric_limits< unsigned long long >::max();

#endif
