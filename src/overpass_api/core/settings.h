/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__CORE__SETTINGS_H
#define DE__OSM3S___OVERPASS_API__CORE__SETTINGS_H

#include <string>

#include "../../template_db/types.h"

using namespace std;

struct Basic_Settings
{
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
  uint max_num_processes;
  uint purge_timeout;
  
  Osm_Base_Settings();
};

struct Area_Settings
{
  File_Properties* AREA_BLOCKS;
  File_Properties* AREAS;
  File_Properties* AREA_TAGS_LOCAL;
  File_Properties* AREA_TAGS_GLOBAL;
  
  string shared_name;
  uint max_num_processes;
  uint purge_timeout;
  
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

Basic_Settings& basic_settings();
const Osm_Base_Settings& osm_base_settings();
const Area_Settings& area_settings();
const Meta_Settings& meta_settings();

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
