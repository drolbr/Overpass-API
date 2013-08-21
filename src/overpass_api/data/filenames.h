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

#ifndef DE__OSM3S___OVERPASS_API__DATA__FILENAMES_H
#define DE__OSM3S___OVERPASS_API__DATA__FILENAMES_H


#include "../core/settings.h"
#include "../core/datatypes.h"


template< typename Skeleton >
File_Properties* current_skeleton_file_properties()
{
  return 0;
}


template< > inline
File_Properties* current_skeleton_file_properties< Node_Skeleton >()
{
  return osm_base_settings().NODES;
}


template< typename Skeleton >
File_Properties* current_meta_file_properties()
{
  return 0;
}


template< > inline
File_Properties* current_meta_file_properties< Node_Skeleton >()
{
  return meta_settings().NODES_META;
}


template< typename Skeleton >
File_Properties* attic_skeleton_file_properties()
{
  return 0;
}


template< typename Skeleton >
File_Properties* current_local_tags_file_properties()
{
  return 0;
}


template< > inline
File_Properties* current_local_tags_file_properties< Node_Skeleton >()
{
  return osm_base_settings().NODE_TAGS_LOCAL;
}


template< > inline
File_Properties* attic_skeleton_file_properties< Node_Skeleton >()
{
  return attic_settings().NODES;
}


template< typename Skeleton >
File_Properties* attic_idx_list_properties()
{
  return 0;
}


template< > inline
File_Properties* attic_idx_list_properties< Node_Skeleton >()
{
  return attic_settings().NODE_IDX_LIST;
}


template< typename Skeleton >
File_Properties* attic_undeleted_file_properties()
{
  return 0;
}


template< > inline
File_Properties* attic_undeleted_file_properties< Node_Skeleton >()
{
  return attic_settings().NODES_UNDELETED;
}


template< typename Skeleton >
File_Properties* attic_meta_file_properties()
{
  return 0;
}


template< > inline
File_Properties* attic_meta_file_properties< Node_Skeleton >()
{
  return attic_settings().NODES_META;
}


template< typename Skeleton >
File_Properties* attic_local_tags_file_properties()
{
  return 0;
}


template< > inline
File_Properties* attic_local_tags_file_properties< Node_Skeleton >()
{
  return attic_settings().NODE_TAGS_LOCAL;
}


#endif
