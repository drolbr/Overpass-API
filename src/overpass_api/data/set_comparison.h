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

#ifndef DE__OSM3S___OVERPASS_API__DATA__SET_COMPARISON_H
#define DE__OSM3S___OVERPASS_API__DATA__SET_COMPARISON_H


#include "collect_members.h"
#include "diff_set.h"
#include "geometry_from_quad_coords.h"
#include "meta_collector.h"
#include "relation_geometry_store.h"
#include "tag_store.h"
#include "way_geometry_store.h"

#include <map>
#include <string>
#include <utility>
#include <vector>


struct Extra_Data_For_Diff
{
  Extra_Data_For_Diff(
      Resource_Manager& rman, const Statement& stmt, const Set& to_print, unsigned int mode_,
      double south, double north, double west, double east);
  ~Extra_Data_For_Diff();

  const std::map< uint32, std::string >* get_users() const;

  unsigned int mode;
  Way_Bbox_Geometry_Store* way_geometry_store;
  Way_Bbox_Geometry_Store* attic_way_geometry_store;
  Relation_Geometry_Store* relation_geometry_store;
  Relation_Geometry_Store* attic_relation_geometry_store;
  const std::map< uint32, std::string >* roles;
  const std::map< uint32, std::string >* users;
};


class Evaluator;


class Set_Comparison
{
public:
  Set_Comparison(Transaction& transaction, const Set& lhs_set, uint64 lhs_timestamp)
      : final_target(0), lhs_set_(lhs_set), lhs_timestamp_(lhs_timestamp) {}

  Diff_Set compare_to_lhs(Resource_Manager& rman, const Statement& stmt, const Set& input_set,
      double south, double north, double west, double east, bool add_deletion_information);
  Diff_Set compare_to_lhs(Resource_Manager& rman, const Statement& stmt, const Set& input_set,
      Evaluator* evaluator, bool add_deletion_information);

private:
  void print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Node_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users);
  void print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Attic< Node_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users);
  void store_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0);
  void compare_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0);

  void print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Way_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users);
  void print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users);
  void store_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0);
  void compare_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0);

  void print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users);
  void print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users);
  void store_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0);
  void compare_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
                            const Output_Handler::Feature_Action& action = Output_Handler::keep,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0);

  void set_target(bool target);

  void clear_nodes(Resource_Manager& rman, bool add_deletion_information = false);
  void clear_ways(Resource_Manager& rman, bool add_deletion_information = false);
  void clear_relations(Resource_Manager& rman, bool add_deletion_information = false);
    
  const Set& lhs_set() const { return lhs_set_; }
  uint64 lhs_timestamp() const { return lhs_timestamp_; }

  template< class Index, class Object >
  void tags_quadtile
      (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Object > >& items,
      Resource_Manager& rman);
  template< class Index, class Object >
  void tags_quadtile_attic
      (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Attic< Object > > >& items,
      Resource_Manager& rman);
  template< class Index, class Object >
  void tags_quadtile
      (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Object > >& items,
      const std::vector< typename Object::Id_Type >& id_list,
      Resource_Manager& rman);
  template< class Index, class Object >
  void tags_quadtile_attic
      (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Attic< Object > > >& items,
      const std::vector< typename Object::Id_Type >& id_list,
      Resource_Manager& rman);

  bool final_target;
  
  std::vector< Node_With_Context > nodes;
  std::vector< Way_With_Context > ways;
  std::vector< Relation_With_Context > relations;
  
  std::vector< std::pair< Node_Skeleton::Id_Type, std::string > > node_values;
  std::vector< std::pair< Way_Skeleton::Id_Type, std::string > > way_values;
  std::vector< std::pair< Relation_Skeleton::Id_Type, std::string > > relation_values;
  
  Set lhs_set_;
  uint64 lhs_timestamp_;

  Diff_Set result;
};


#endif
