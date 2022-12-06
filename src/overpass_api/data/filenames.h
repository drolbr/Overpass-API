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

#ifndef DE__OSM3S___OVERPASS_API__DATA__FILENAMES_H
#define DE__OSM3S___OVERPASS_API__DATA__FILENAMES_H


#include "../core/settings.h"
#include "../core/datatypes.h"


template< typename Skeleton >
File_Properties* current_skeleton_file_properties()
{
  return 0;
}

template< > inline File_Properties* current_skeleton_file_properties< Node_Skeleton >()
{ return osm_base_settings().NODES; }

template< > inline File_Properties* current_skeleton_file_properties< Way_Skeleton >()
{ return osm_base_settings().WAYS; }

template< > inline File_Properties* current_skeleton_file_properties< Relation_Skeleton >()
{ return osm_base_settings().RELATIONS; }



template< typename Skeleton >
File_Properties* current_meta_file_properties()
{
  return 0;
}

template< > inline File_Properties* current_meta_file_properties< Node_Skeleton >()
{ return meta_settings().NODES_META; }

template< > inline File_Properties* current_meta_file_properties< Way_Skeleton >()
{ return meta_settings().WAYS_META; }

template< > inline File_Properties* current_meta_file_properties< Relation_Skeleton >()
{ return meta_settings().RELATIONS_META; }



template< typename Skeleton >
File_Properties* attic_skeleton_file_properties()
{
  return 0;
}

template< > inline File_Properties* attic_skeleton_file_properties< Node_Skeleton >()
{ return attic_settings().NODES; }

template< > inline File_Properties* attic_skeleton_file_properties< Way_Skeleton >()
{ return attic_settings().WAYS; }

template< > inline File_Properties* attic_skeleton_file_properties< Relation_Skeleton >()
{ return attic_settings().RELATIONS; }



template< typename Skeleton >
File_Properties* current_local_tags_file_properties()
{
  return 0;
}

template< > inline File_Properties* current_local_tags_file_properties< Node_Skeleton >()
{ return osm_base_settings().NODE_TAGS_LOCAL; }

template< > inline File_Properties* current_local_tags_file_properties< Way_Skeleton >()
{ return osm_base_settings().WAY_TAGS_LOCAL; }

template< > inline File_Properties* current_local_tags_file_properties< Relation_Skeleton >()
{ return osm_base_settings().RELATION_TAGS_LOCAL; }

template< > inline File_Properties* current_local_tags_file_properties< Area_Skeleton >()
{ return area_settings().AREA_TAGS_LOCAL; }



template< typename Skeleton >
File_Properties* current_global_tags_file_properties()
{
  return 0;
}

template< > inline File_Properties* current_global_tags_file_properties< Node_Skeleton >()
{ return osm_base_settings().NODE_TAGS_GLOBAL; }

template< > inline File_Properties* current_global_tags_file_properties< Way_Skeleton >()
{ return osm_base_settings().WAY_TAGS_GLOBAL; }

template< > inline File_Properties* current_global_tags_file_properties< Relation_Skeleton >()
{ return osm_base_settings().RELATION_TAGS_GLOBAL; }

template< > inline File_Properties* current_global_tags_file_properties< Area_Skeleton >()
{ return area_settings().AREA_TAGS_GLOBAL; }



template< typename Skeleton >
File_Properties* current_global_tag_frequency_file_properties()
{
  return 0;
}

template< > inline File_Properties* current_global_tag_frequency_file_properties< Node_Skeleton >()
{ return osm_base_settings().NODE_FREQUENT_TAGS; }

template< > inline File_Properties* current_global_tag_frequency_file_properties< Way_Skeleton >()
{ return osm_base_settings().WAY_FREQUENT_TAGS; }

template< > inline File_Properties* current_global_tag_frequency_file_properties< Relation_Skeleton >()
{ return osm_base_settings().RELATION_FREQUENT_TAGS; }



template< typename Skeleton >
File_Properties* key_file_properties()
{
  return 0;
}

template< > inline File_Properties* key_file_properties< Node_Skeleton >()
{ return osm_base_settings().NODE_KEYS; }

template< > inline File_Properties* key_file_properties< Way_Skeleton >()
{ return osm_base_settings().WAY_KEYS; }

template< > inline File_Properties* key_file_properties< Relation_Skeleton >()
{ return osm_base_settings().RELATION_KEYS; }



template< typename Skeleton >
File_Properties* attic_idx_list_properties()
{
  return 0;
}

template< > inline File_Properties* attic_idx_list_properties< Node_Skeleton >()
{ return attic_settings().NODE_IDX_LIST; }

template< > inline File_Properties* attic_idx_list_properties< Way_Skeleton >()
{ return attic_settings().WAY_IDX_LIST; }

template< > inline File_Properties* attic_idx_list_properties< Relation_Skeleton >()
{ return attic_settings().RELATION_IDX_LIST; }



template< typename Skeleton >
File_Properties* attic_undeleted_file_properties()
{
  return 0;
}

template< > inline File_Properties* attic_undeleted_file_properties< Node_Skeleton >()
{ return attic_settings().NODES_UNDELETED; }

template< > inline File_Properties* attic_undeleted_file_properties< Way_Skeleton >()
{ return attic_settings().WAYS_UNDELETED; }

template< > inline File_Properties* attic_undeleted_file_properties< Relation_Skeleton >()
{ return attic_settings().RELATIONS_UNDELETED; }



template< typename Skeleton >
File_Properties* attic_meta_file_properties()
{
  return 0;
}

template< > inline File_Properties* attic_meta_file_properties< Node_Skeleton >()
{ return attic_settings().NODES_META; }

template< > inline File_Properties* attic_meta_file_properties< Way_Skeleton >()
{ return attic_settings().WAYS_META; }

template< > inline File_Properties* attic_meta_file_properties< Relation_Skeleton >()
{ return attic_settings().RELATIONS_META; }



template< typename Skeleton >
File_Properties* attic_local_tags_file_properties()
{
  return 0;
}

template< > inline File_Properties* attic_local_tags_file_properties< Node_Skeleton >()
{ return attic_settings().NODE_TAGS_LOCAL; }

template< > inline File_Properties* attic_local_tags_file_properties< Way_Skeleton >()
{ return attic_settings().WAY_TAGS_LOCAL; }

template< > inline File_Properties* attic_local_tags_file_properties< Relation_Skeleton >()
{ return attic_settings().RELATION_TAGS_LOCAL; }



template< typename Skeleton >
File_Properties* attic_global_tags_file_properties()
{
  return 0;
}

template< > inline File_Properties* attic_global_tags_file_properties< Node_Skeleton >()
{ return attic_settings().NODE_TAGS_GLOBAL; }

template< > inline File_Properties* attic_global_tags_file_properties< Way_Skeleton >()
{ return attic_settings().WAY_TAGS_GLOBAL; }

template< > inline File_Properties* attic_global_tags_file_properties< Relation_Skeleton >()
{ return attic_settings().RELATION_TAGS_GLOBAL; }



template< typename Skeleton >
File_Properties* attic_global_tag_frequency_file_properties()
{
  return 0;
}

template< > inline File_Properties* attic_global_tag_frequency_file_properties< Node_Skeleton >()
{ return osm_base_settings().NODE_FREQUENT_TAGS; }

template< > inline File_Properties* attic_global_tag_frequency_file_properties< Way_Skeleton >()
{ return osm_base_settings().WAY_FREQUENT_TAGS; }

template< > inline File_Properties* attic_global_tag_frequency_file_properties< Relation_Skeleton >()
{ return osm_base_settings().RELATION_FREQUENT_TAGS; }



template< typename Skeleton >
File_Properties* changelog_file_properties()
{
  return 0;
}

template< > inline File_Properties* changelog_file_properties< Node_Skeleton >()
{ return attic_settings().NODE_CHANGELOG; }

template< > inline File_Properties* changelog_file_properties< Way_Skeleton >()
{ return attic_settings().WAY_CHANGELOG; }

template< > inline File_Properties* changelog_file_properties< Relation_Skeleton >()
{ return attic_settings().RELATION_CHANGELOG; }


#endif
