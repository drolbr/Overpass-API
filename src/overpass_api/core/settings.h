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
#include "datatypes.h"


struct Basic_Settings
{
  std::string DATA_SUFFIX;
  std::string INDEX_SUFFIX;
  std::string ID_SUFFIX;
  std::string SHADOW_SUFFIX;
  
  std::string base_directory;
  std::string logfile_name;
  std::string shared_name_base;
  
  Basic_Settings();
};


template < typename Key, typename Value >
struct Typed_File_Properties : public File_Properties {};


struct Osm_Base_Settings
{
  Typed_File_Properties< Uint31_Index, Node_Skeleton >* NODES;
  Typed_File_Properties< Tag_Index_Local, Node_Skeleton::Id_Type >* NODE_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Tag_Object_Global< Node_Skeleton::Id_Type > >* NODE_TAGS_GLOBAL;
  Typed_File_Properties< Uint32_Index, String_Object >* NODE_KEYS;
  Typed_File_Properties< Uint31_Index, Way_Skeleton >* WAYS;
  Typed_File_Properties< Tag_Index_Local, Way_Skeleton::Id_Type >* WAY_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Tag_Object_Global< Way_Skeleton::Id_Type > >* WAY_TAGS_GLOBAL;
  Typed_File_Properties< Uint32_Index, String_Object >* WAY_KEYS;
  Typed_File_Properties< Uint31_Index, Relation_Skeleton >* RELATIONS;
  Typed_File_Properties< Uint32_Index, String_Object >* RELATION_ROLES;
  Typed_File_Properties< Tag_Index_Local, Relation_Skeleton::Id_Type >* RELATION_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Tag_Object_Global< Relation_Skeleton::Id_Type > >* RELATION_TAGS_GLOBAL;
  Typed_File_Properties< Uint32_Index, String_Object >* RELATION_KEYS;
  
  std::string shared_name;
  uint max_num_processes;
  uint purge_timeout;
  uint64 total_available_space;
  uint64 total_available_time_units;
  
  Osm_Base_Settings();
};


struct Area_Settings
{
  Typed_File_Properties< Uint31_Index, Area_Block >* AREA_BLOCKS;
  Typed_File_Properties< Uint31_Index, Area_Skeleton >* AREAS;
  Typed_File_Properties< Tag_Index_Local, Area_Skeleton::Id_Type >* AREA_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Area_Skeleton::Id_Type >* AREA_TAGS_GLOBAL;
  
  std::string shared_name;
  uint max_num_processes;
  uint purge_timeout;
  uint64 total_available_space;
  uint64 total_available_time_units;
  
  Area_Settings();
};


struct Meta_Settings
{
  Typed_File_Properties< Uint32_Index, User_Data >* USER_DATA;
  Typed_File_Properties< Uint32_Index, Uint31_Index >* USER_INDICES;
  Typed_File_Properties< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >* NODES_META;
  Typed_File_Properties< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >* WAYS_META;
  Typed_File_Properties< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > >* RELATIONS_META;
  
  Meta_Settings();
};


struct Attic_Settings
{
  Typed_File_Properties< Uint31_Index, Attic< Node_Skeleton > >* NODES;
  Typed_File_Properties< Uint31_Index, Attic< Node_Skeleton::Id_Type > >* NODES_UNDELETED;
  Typed_File_Properties< Node::Id_Type, Uint31_Index >* NODE_IDX_LIST;
  Typed_File_Properties< Tag_Index_Local, Attic< Node_Skeleton::Id_Type > >* NODE_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Attic< Tag_Object_Global< Node_Skeleton::Id_Type > > >* NODE_TAGS_GLOBAL;
  Typed_File_Properties< Uint31_Index, OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >* NODES_META;
  Typed_File_Properties< Timestamp, Change_Entry< Node_Skeleton::Id_Type > >* NODE_CHANGELOG;
  
  Typed_File_Properties< Uint31_Index, Attic< Way_Skeleton > >* WAYS;
  Typed_File_Properties< Uint31_Index, Attic< Way_Skeleton::Id_Type > >* WAYS_UNDELETED;
  Typed_File_Properties< Way::Id_Type, Uint31_Index >* WAY_IDX_LIST;
  Typed_File_Properties< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >* WAY_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Attic< Tag_Object_Global< Way_Skeleton::Id_Type > > >* WAY_TAGS_GLOBAL;
  Typed_File_Properties< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >* WAYS_META;
  Typed_File_Properties< Timestamp, Change_Entry< Way_Skeleton::Id_Type > >* WAY_CHANGELOG;
  
  Typed_File_Properties< Uint31_Index, Attic< Relation_Skeleton > >* RELATIONS;
  Typed_File_Properties< Uint31_Index, Attic< Relation_Skeleton::Id_Type > >* RELATIONS_UNDELETED;
  Typed_File_Properties< Relation::Id_Type, Uint31_Index >* RELATION_IDX_LIST;
  Typed_File_Properties< Tag_Index_Local, Attic< Relation_Skeleton::Id_Type > >* RELATION_TAGS_LOCAL;
  Typed_File_Properties< Tag_Index_Global, Attic< Tag_Object_Global< Relation_Skeleton::Id_Type > > >* RELATION_TAGS_GLOBAL;
  Typed_File_Properties< Uint31_Index, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > >* RELATIONS_META;
  Typed_File_Properties< Timestamp, Change_Entry< Relation_Skeleton::Id_Type > >* RELATION_CHANGELOG;

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
