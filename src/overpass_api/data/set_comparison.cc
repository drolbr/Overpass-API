/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#include "../statements/evaluator.h"
#include "set_comparison.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>


Extra_Data_For_Diff::Extra_Data_For_Diff(
    Resource_Manager& rman, const Statement& stmt, const Set& to_print, unsigned int mode_,
    double south, double north, double west, double east)
    : mode(mode_), way_geometry_store(0), attic_way_geometry_store(0),
    relation_geometry_store(0), attic_relation_geometry_store(0), roles(0), users(0)
{
  if (mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
  {
    way_geometry_store = new Way_Bbox_Geometry_Store(to_print.ways, stmt, rman, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_way_geometry_store = new Way_Bbox_Geometry_Store(
          to_print.attic_ways, stmt, rman,
          south, north, west, east);
    }

    relation_geometry_store = new Relation_Geometry_Store(
        to_print.relations, stmt, rman, south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      attic_relation_geometry_store = new Relation_Geometry_Store(
          to_print.attic_relations, stmt, rman,
          south, north, west, east);
    }
  }

  roles = &relation_member_roles(*rman.get_transaction());

  if (mode & Output_Mode::META)
    users = &rman.users();
}


const std::map< uint32, std::string >* Extra_Data_For_Diff::get_users() const
{
  return users;
}


Extra_Data_For_Diff::~Extra_Data_For_Diff()
{
  delete way_geometry_store;
  delete attic_way_geometry_store;
  delete relation_geometry_store;
  delete attic_relation_geometry_store;
}


void Set_Comparison::set_target(bool target)
{
  final_target = target;
  std::sort(nodes.begin(), nodes.end());
  std::sort(ways.begin(), ways.end());
  std::sort(relations.begin(), relations.end());
}


void Set_Comparison::print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Node_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (final_target)
    compare_item(ll_upper, skel, tags, NOW, meta, users);
  else
    store_item(ll_upper, skel, tags, NOW, meta, users);
}


void Set_Comparison::print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Attic< Node_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (final_target)
    compare_item(ll_upper, skel, tags, skel.timestamp, meta, users);
  else
    store_item(ll_upper, skel, tags, skel.timestamp, meta, users);
}


void Set_Comparison::print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Way_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (extra_data.way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = extra_data.way_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    if (final_target)
      compare_item(ll_upper, skel, tags,
          geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
          ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
          NOW, meta, users);
    else
      store_item(ll_upper, skel, tags,
          geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
          ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
          NOW, meta, users);
  }
  else
  {
    if (final_target)
      compare_item(ll_upper, skel, tags, 0, 0, NOW, meta, users);
    else
      store_item(ll_upper, skel, tags, 0, 0, NOW, meta, users);
  }
}


void Set_Comparison::print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (extra_data.attic_way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = extra_data.attic_way_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    if (final_target)
      compare_item(ll_upper, skel, tags,
          geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
          ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
          skel.timestamp, meta, users);
    else
      store_item(ll_upper, skel, tags,
          geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
          ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
          skel.timestamp, meta, users);
  }
  else
  {
    if (final_target)
      compare_item(ll_upper, skel, tags, 0, 0, skel.timestamp, meta, users);
    else
      store_item(ll_upper, skel, tags, 0, 0, skel.timestamp, meta, users);
  }
}


void Set_Comparison::print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (extra_data.relation_geometry_store)
  {
    std::vector< std::vector< Quad_Coord > > geometry = extra_data.relation_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    if (final_target)
      compare_item(ll_upper, skel, tags,
          geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
          ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
          NOW, meta, users);
    else
      store_item(ll_upper, skel, tags,
          geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
          ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
          NOW, meta, users);
  }
  else
  {
    if (final_target)
      compare_item(ll_upper, skel, tags, 0, 0, NOW, meta, users);
    else
      store_item(ll_upper, skel, tags, 0, 0, NOW, meta, users);
  }
}


void Set_Comparison::print_item(Extra_Data_For_Diff& extra_data, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (extra_data.attic_relation_geometry_store)
  {
    std::vector< std::vector< Quad_Coord > > geometry = extra_data.attic_relation_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    if (final_target)
      compare_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        skel.timestamp, meta, users);
    else
      store_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        skel.timestamp, meta, users);
  }
  else
  {
    if (final_target)
      compare_item(ll_upper, skel, tags, 0, 0, skel.timestamp, meta, users);
    else
      store_item(ll_upper, skel, tags, 0, 0, skel.timestamp, meta, users);
  }
}


template< class Index, class Object >
void Set_Comparison::tags_quadtile
    (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Object > >& items, Resource_Manager& rman)
{
  Tag_Store< Index, Object > tag_store(*rman.get_transaction());
  tag_store.prefetch_all(items);

  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, *rman.get_transaction(),
      (extra_data.mode & Output_Mode::META) ? current_meta_file_properties< Object >() : 0);

  typename std::map< Index, std::vector< Object > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename std::vector< Object >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      print_item(extra_data, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
          meta_printer.get(item_it->first, it2->id), extra_data.users);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void Set_Comparison::tags_quadtile_attic
    (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Attic< Object > > >& items,
     Resource_Manager& rman)
{
  Tag_Store< Index, Object > tag_store(*rman.get_transaction());
  tag_store.prefetch_all(items);
  // formulate meta query if meta data shall be printed
  Attic_Meta_Collector< Index, Object > meta_printer(
      items, *rman.get_transaction(), extra_data.mode & Output_Mode::META);

  typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      item_it(items.begin());
  while (item_it != items.end())
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      print_item(extra_data, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
                 meta_printer.get(item_it->first, it2->id, it2->timestamp), extra_data.users);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void Set_Comparison::tags_quadtile
    (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Object > >& items,
    const std::vector< typename Object::Id_Type >& id_list, Resource_Manager& rman)
{
  Tag_Store< Index, Object > tag_store(*rman.get_transaction());
  tag_store.prefetch_all(items);

  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, *rman.get_transaction(),
      (extra_data.mode & Output_Mode::META) ? current_meta_file_properties< Object >() : 0);

  typename std::map< Index, std::vector< Object > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename std::vector< Object >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (std::binary_search(id_list.begin(), id_list.end(), it2->id))
        print_item(extra_data, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
            meta_printer.get(item_it->first, it2->id), extra_data.users);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void Set_Comparison::tags_quadtile_attic
    (Extra_Data_For_Diff& extra_data, const std::map< Index, std::vector< Attic< Object > > >& items,
    const std::vector< typename Object::Id_Type >& id_list, Resource_Manager& rman)
{
  Tag_Store< Index, Object > tag_store(*rman.get_transaction());
  tag_store.prefetch_all(items);
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (items, *rman.get_transaction(),
      (extra_data.mode & Output_Mode::META) ? current_meta_file_properties< Object >() : 0);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (items, *rman.get_transaction(),
      (extra_data.mode & Output_Mode::META) ? attic_meta_file_properties< Object >() : 0);

  typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      item_it(items.begin());
  while (item_it != items.end())
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (std::binary_search(id_list.begin(), id_list.end(), it2->id))
      {
        const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
            = attic_meta_printer.get(item_it->first, it2->id, it2->timestamp);
        if (!meta)
          meta = current_meta_printer.get(item_it->first, it2->id, it2->timestamp);
        print_item(extra_data, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
                 meta, extra_data.users);
      }
    }
    ++item_it;
  }
}


template< typename Index, typename Skeleton >
std::vector< typename Skeleton::Id_Type > find_still_existing_skeletons
    (Resource_Manager& rman, uint64 timestamp, const std::vector< Index >& req,
     const std::vector< typename Skeleton::Id_Type >& searched_ids)
{
  std::vector< typename Skeleton::Id_Type > found_ids;
  std::map< Index, std::vector< Skeleton > > current_result;
  std::map< Index, std::vector< Attic< Skeleton > > > attic_result;
  if (timestamp == NOW)
    collect_items_discrete(0, rman, *current_skeleton_file_properties< Skeleton >(), req,
        Id_Predicate< Skeleton >(searched_ids), current_result);
  else
  {
    collect_items_discrete_by_timestamp(0, rman, req,
        Id_Predicate< Skeleton >(searched_ids), timestamp, current_result, attic_result);
    filter_attic_elements(rman, timestamp, current_result, attic_result);
  }
  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current_result.begin();
       it != current_result.end(); ++it)
  {
    for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      found_ids.push_back(it2->id);
  }
  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic_result.begin();
       it != attic_result.end(); ++it)
  {
    for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
	 it2 != it->second.end(); ++it2)
      found_ids.push_back(it2->id);
  }
  std::sort(found_ids.begin(), found_ids.end());
  found_ids.erase(std::unique(found_ids.begin(), found_ids.end()), found_ids.end());

  return found_ids;
}


template< typename Index, typename Skeleton >
std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
    find_meta_elements
    (Resource_Manager& rman, uint64 timestamp, const std::vector< Index >& idx_set,
     const std::vector< typename Skeleton::Id_Type >& searched_ids)
{
  std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > result;

  Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
        typename std::vector< Index >::const_iterator >
      attic_meta_db(rman.get_transaction()->data_index(attic_meta_file_properties< Skeleton >()));
  for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
          typename std::vector< Index >::const_iterator >::Discrete_Iterator
      it = attic_meta_db.discrete_begin(idx_set.begin(), idx_set.end());
      !(it == attic_meta_db.discrete_end()); ++it)
  {
    if (!(timestamp < it.object().timestamp)
        && std::binary_search(searched_ids.begin(), searched_ids.end(), it.object().ref))
    {
      typename std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
          ::iterator meta_it = result.find(it.object().ref);
      if (meta_it == result.end())
	result.insert(std::make_pair(it.object().ref, it.object()));
      else if (meta_it->second.timestamp < it.object().timestamp)
	meta_it->second = it.object();
    }
  }

  // Same thing with current meta data
  Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
          typename std::vector< Index >::const_iterator >
      meta_db(rman.get_transaction()->data_index(current_meta_file_properties< Skeleton >()));

  for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
          typename std::vector< Index >::const_iterator >::Discrete_Iterator
      it = meta_db.discrete_begin(idx_set.begin(), idx_set.end());
      !(it == meta_db.discrete_end()); ++it)
  {
    if (!(timestamp < it.object().timestamp)
        && std::binary_search(searched_ids.begin(), searched_ids.end(), it.object().ref))
    {
      typename std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > >
          ::iterator meta_it = result.find(it.object().ref);
      if (meta_it == result.end())
	result.insert(std::make_pair(it.object().ref, it.object()));
      else if (meta_it->second.timestamp < it.object().timestamp)
	meta_it->second = it.object();
    }
  }

  return result;
}


void Set_Comparison::store_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
{
  nodes.push_back(Node_With_Context(ll_upper, skel, timestamp,
      meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
      tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Set_Comparison::compare_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
{
  std::vector< Node_With_Context >::iterator nodes_it
      = std::lower_bound(nodes.begin(), nodes.end(), Node_With_Context(ll_upper, skel, 0));

  if (nodes_it == nodes.end() || skel.id < nodes_it->elem.id)
    result.different_nodes.push_back(std::make_pair(
	  Node_With_Context(0xffu, Node_Skeleton(skel.id), 0,
	      OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Node_With_Context(ll_upper, skel, timestamp,
              meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
  else
  {
    if (!(nodes_it->idx.val() == ll_upper) || !(nodes_it->elem.ll_lower == skel.ll_lower) ||
          (tags && !(nodes_it->tags == *tags)) || (meta && !(nodes_it->meta.timestamp == meta->timestamp)))
      result.different_nodes.push_back(std::make_pair(*nodes_it, Node_With_Context(ll_upper, skel, timestamp,
                  meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
                  tags ? *tags : std::vector< std::pair< std::string, std::string > >())));

    nodes_it->idx = 0xffu;
  }
}


void Set_Comparison::clear_nodes(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Node_Skeleton::Id_Type > searched_ids;
    for (std::vector< Node_With_Context >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	searched_ids.push_back(it->elem.id);
    }

    std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(searched_ids, rman, true);
    std::vector< Node_Skeleton::Id_Type > found_ids
        = find_still_existing_skeletons< Uint32_Index, Node_Skeleton >(
            rman, rman.get_desired_timestamp(), req, searched_ids);
    std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > > found_meta
        = find_meta_elements< Uint32_Index, Node_Skeleton >(rman, rman.get_diff_to_timestamp(), req, searched_ids);

    for (std::vector< Node_With_Context >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	result.different_nodes.push_back(std::make_pair(*it,
	    Node_With_Context(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfdu : 0xffu,
		Node_Skeleton(it->elem.id), 0,
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
    
    searched_ids.clear();
    for (std::vector< std::pair< Node_With_Context, Node_With_Context > >::const_iterator
        it = result.different_nodes.begin(); it != result.different_nodes.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
        searched_ids.push_back(it->second.elem.id);
    }

    get_indexes_< Uint32_Index, Node_Skeleton >(searched_ids, rman, true).swap(req);
    find_still_existing_skeletons< Uint32_Index, Node_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_ids);
    find_meta_elements< Uint32_Index, Node_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_meta);

    for (std::vector< std::pair< Node_With_Context, Node_With_Context > >::iterator
        it = result.different_nodes.begin(); it != result.different_nodes.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
      {
        it->first.idx = std::binary_search(found_ids.begin(), found_ids.end(), it->second.elem.id) ? 0xfdu : 0xffu;
        it->first.elem = Node_Skeleton(it->second.elem.id);
        
	std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->second.elem.id);
        if (meta_it != found_meta.end())
          it->first.meta = meta_it->second;
      }
    }
  }
  else
  {
    for (std::vector< Node_With_Context >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	result.different_nodes.push_back(std::make_pair(*it,
	    Node_With_Context(0xffu, Node_Skeleton(it->elem.id), 0,
	        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }

  std::sort(result.different_nodes.begin(), result.different_nodes.end());
}


void Set_Comparison::store_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta)
{
  ways.push_back(Way_With_Context(ll_upper, skel,
      geometry ? *geometry : std::vector< Quad_Coord >(),
      timestamp, meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
      tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Set_Comparison::compare_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta)
{
  std::vector< Way_With_Context >::iterator ways_it
      = std::lower_bound(ways.begin(), ways.end(),
          Way_With_Context(ll_upper, skel, std::vector< Quad_Coord >(), timestamp));

  if (ways_it == ways.end() || skel.id < ways_it->elem.id)
    result.different_ways.push_back(std::make_pair(
	  Way_With_Context(0xffu, Way_Skeleton(skel.id),
	      std::vector< Quad_Coord >(),
	      0, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Way_With_Context(ll_upper, skel,
              geometry ? *geometry : std::vector< Quad_Coord >(),
              timestamp, meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
  else
  {
    if (!(ways_it->idx.val() == ll_upper) || !(ways_it->elem.nds == skel.nds) ||
          (geometry && !(ways_it->geometry == *geometry)) ||
          (tags && !(ways_it->tags == *tags)) || (meta && !(ways_it->meta.timestamp == meta->timestamp)))
      result.different_ways.push_back(std::make_pair(*ways_it, Way_With_Context(ll_upper, skel,
              geometry ? *geometry : std::vector< Quad_Coord >(),
              timestamp, meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
    ways_it->idx = 0xffu;
  }
}


void Set_Comparison::clear_ways(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Way_Skeleton::Id_Type > searched_ids;
    for (std::vector< Way_With_Context >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	searched_ids.push_back(it->elem.id);
    }

    std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(searched_ids, rman, true);
    std::vector< Way_Skeleton::Id_Type > found_ids
        = find_still_existing_skeletons< Uint31_Index, Way_Skeleton >(
            rman, rman.get_desired_timestamp(), req, searched_ids);
    std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > > found_meta
        = find_meta_elements< Uint31_Index, Way_Skeleton >(rman, rman.get_diff_to_timestamp(), req, searched_ids);

    for (std::vector< Way_With_Context >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	result.different_ways.push_back(std::make_pair(*it,
	    Way_With_Context(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfdu : 0xffu,
		Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        0, meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
    
    searched_ids.clear();
    for (std::vector< std::pair< Way_With_Context, Way_With_Context > >::const_iterator
        it = result.different_ways.begin(); it != result.different_ways.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
        searched_ids.push_back(it->second.elem.id);
    }

    get_indexes_< Uint31_Index, Way_Skeleton >(searched_ids, rman, true).swap(req);
    find_still_existing_skeletons< Uint31_Index, Way_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_ids);
    find_meta_elements< Uint31_Index, Way_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_meta);

    for (std::vector< std::pair< Way_With_Context, Way_With_Context > >::iterator
        it = result.different_ways.begin(); it != result.different_ways.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
      {
        it->first.idx = std::binary_search(found_ids.begin(), found_ids.end(), it->second.elem.id) ? 0xfdu : 0xffu;
        it->first.elem = Way_Skeleton(it->second.elem.id);
        
	std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->second.elem.id);
        if (meta_it != found_meta.end())
          it->first.meta = meta_it->second;
      }
    }
  }
  else
  {
    for (std::vector< Way_With_Context >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	result.different_ways.push_back(std::make_pair(*it,
	    Way_With_Context(0xffu, Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        0, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }
  
  std::sort(result.different_ways.begin(), result.different_ways.end());
}


void Set_Comparison::store_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  relations.push_back(Relation_With_Context(ll_upper, skel,
      geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
      timestamp, meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
      tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Set_Comparison::compare_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            uint64 timestamp, const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  std::vector< Relation_With_Context >::iterator relations_it
      = std::lower_bound(relations.begin(), relations.end(),
          Relation_With_Context(ll_upper, skel, std::vector< std::vector< Quad_Coord > >(), 0));

  if (relations_it == relations.end() || skel.id < relations_it->elem.id)
      result.different_relations.push_back(std::make_pair(
	  Relation_With_Context(0xffu, Relation_Skeleton(skel.id),
	      std::vector< std::vector< Quad_Coord > >(),
	      0, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Relation_With_Context(ll_upper, skel,
              geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
              timestamp, meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
  else
  {
    if (!(relations_it->idx.val() == ll_upper) || !(relations_it->elem.members == skel.members) ||
	  (geometry && !(relations_it->geometry == *geometry)) ||
	  (tags && !(relations_it->tags == *tags)) || (meta && !(relations_it->meta.timestamp == meta->timestamp)))
      result.different_relations.push_back(std::make_pair(*relations_it, Relation_With_Context(ll_upper, skel,
              geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
              timestamp, meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
    relations_it->idx = 0xffu;
  }
}


void Set_Comparison::clear_relations(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Relation_Skeleton::Id_Type > searched_ids;
    for (std::vector< Relation_With_Context >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	searched_ids.push_back(it->elem.id);
    }

    std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true);
    std::vector< Relation_Skeleton::Id_Type > found_ids
        = find_still_existing_skeletons< Uint31_Index, Relation_Skeleton >(
            rman, rman.get_diff_to_timestamp(), req, searched_ids);
    std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > > found_meta
        = find_meta_elements< Uint31_Index, Relation_Skeleton >(
            rman, rman.get_diff_to_timestamp(), req, searched_ids);

    for (std::vector< Relation_With_Context >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	result.different_relations.push_back(std::make_pair(*it,
	    Relation_With_Context(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfdu : 0xffu,
		Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        0, meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }

      req = get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true);
    }
    
    searched_ids.clear();
    for (std::vector< std::pair< Relation_With_Context, Relation_With_Context > >::const_iterator
        it = result.different_relations.begin(); it != result.different_relations.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
        searched_ids.push_back(it->second.elem.id);
    }

    get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true).swap(req);
    find_still_existing_skeletons< Uint31_Index, Relation_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_ids);
    find_meta_elements< Uint31_Index, Relation_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_meta);

    for (std::vector< std::pair< Relation_With_Context, Relation_With_Context > >::iterator
        it = result.different_relations.begin(); it != result.different_relations.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
      {
        it->first.idx = std::binary_search(found_ids.begin(), found_ids.end(), it->second.elem.id) ? 0xfdu : 0xffu;
        it->first.elem = Relation_Skeleton(it->second.elem.id);
        
	std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->second.elem.id);
        if (meta_it != found_meta.end())
          it->first.meta = meta_it->second;
      }
    }
  }
  else
  {
    for (std::vector< Relation_With_Context >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	result.different_relations.push_back(std::make_pair(*it,
	    Relation_With_Context(0xffu, Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        0, OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }
  
  std::sort(result.different_relations.begin(), result.different_relations.end());
}


Diff_Set Set_Comparison::compare_to_lhs(Resource_Manager& rman, const Statement& stmt,
    const Set& input_set, double south, double north, double west, double east, bool add_deletion_information)
{
  result.clear();
  
  uint64 rhs_timestamp = rman.get_desired_timestamp();
  rman.set_desired_timestamp(lhs_timestamp_);
    
  Extra_Data_For_Diff extra_data_lhs(rman, stmt, lhs_set_, Output_Mode::ID
      | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
      | Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META
      | Output_Mode::GEOMETRY, south, north, west, east);

  tags_quadtile(extra_data_lhs, lhs_set_.nodes, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_lhs, lhs_set_.attic_nodes, rman);

  tags_quadtile(extra_data_lhs, lhs_set_.ways, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_lhs, lhs_set_.attic_ways, rman);

  tags_quadtile(extra_data_lhs, lhs_set_.relations, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_lhs, lhs_set_.attic_relations, rman);
    
  rman.set_desired_timestamp(rhs_timestamp);
      
  set_target(true);
    
  Extra_Data_For_Diff extra_data_rhs(rman, stmt, input_set, Output_Mode::ID
      | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
      | Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META
      | Output_Mode::GEOMETRY, south, north, west, east);

  tags_quadtile(extra_data_rhs, input_set.nodes, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_rhs, input_set.attic_nodes, rman);
  clear_nodes(rman, add_deletion_information);

  tags_quadtile(extra_data_rhs, input_set.ways, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_rhs, input_set.attic_ways, rman);
  clear_ways(rman, add_deletion_information);

  tags_quadtile(extra_data_rhs, input_set.relations, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_rhs, input_set.attic_relations, rman);
  clear_relations(rman, add_deletion_information);
  
  Diff_Set local_result;
  local_result.swap(result);
  return local_result;
}


template< typename A, typename B >
struct First_Comparator
{
  bool operator()(const std::pair< A, B >& lhs, const std::pair< A, B >& rhs)
  {
    return lhs.first < rhs.first;
  }
};


template< typename Index, typename Id_Type, typename Maybe_Attic >
void eval_lhs_elems(const std::map< Index, std::vector< Maybe_Attic > >& items,
    std::vector< std::pair< Id_Type, std::string > >& result, Set_With_Context& into_context, Eval_Task& task)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
      result.push_back(std::make_pair(
          it_elem->id, task.eval(into_context.get_context(it_idx->first, *it_elem), 0)));
  }
  
  std::sort(result.begin(), result.end(), First_Comparator< Id_Type, std::string >());
}


template< typename Index, typename Id_Type, typename Maybe_Attic >
void eval_rhs_elems(const std::map< Index, std::vector< Maybe_Attic > >& items,
    std::vector< std::pair< Id_Type, std::string > >& lhs_set, std::vector< Id_Type >& result,
    Set_With_Context& into_context, Eval_Task& task)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      std::string rhs_val = task.eval(into_context.get_context(it_idx->first, *it_elem), 0);
      typename std::vector< std::pair< Id_Type, std::string > >::iterator it_lhs = 
          std::lower_bound(lhs_set.begin(), lhs_set.end(), std::make_pair(it_elem->id, ""),
              First_Comparator< Id_Type, std::string >());

      if (it_lhs == lhs_set.end() || !(it_lhs->first == it_elem->id))
      {
        if (!rhs_val.empty())
          result.push_back(it_elem->id);
      }
      else
      {
        if (rhs_val != it_lhs->second)
          result.push_back(it_elem->id);
        
        it_lhs->second.clear();
      }
    }
  }
}


template< typename Id_Type >
void clear_elems(std::vector< std::pair< Id_Type, std::string > >& lhs_set, std::vector< Id_Type >& result)
{
  for (typename std::vector< std::pair< Id_Type, std::string > >::const_iterator it_lhs = lhs_set.begin();
      it_lhs != lhs_set.end(); ++it_lhs)
  {
    if (!it_lhs->second.empty())
      result.push_back(it_lhs->first);
  }
  
  lhs_set.clear();
  std::sort(result.begin(), result.end());
}


Diff_Set Set_Comparison::compare_to_lhs(Resource_Manager& rman, const Statement& stmt,
    const Set& input_set, Evaluator* evaluator, bool add_deletion_information)
{
  result.clear();
  
  uint64 rhs_timestamp = rman.get_desired_timestamp();
  rman.set_desired_timestamp(lhs_timestamp_);
  
  {
    Requested_Context requested_context = evaluator->request_context();
    Prepare_Task_Context context(requested_context, stmt, rman);

    Owner< Eval_Task > task(evaluator->get_string_task(context, 0));

    Set_With_Context into_context;
    into_context.name = "";
    into_context.parent = &context;
    into_context.prefetch(requested_context.object_usage, lhs_set_, stmt, rman);

    if (task)
    {
      node_values.clear();
      eval_lhs_elems(lhs_set_.nodes, node_values, into_context, *task);
      eval_lhs_elems(lhs_set_.attic_nodes, node_values, into_context, *task);
      
      way_values.clear();
      eval_lhs_elems(lhs_set_.ways, way_values, into_context, *task);
      eval_lhs_elems(lhs_set_.attic_ways, way_values, into_context, *task);
      
      relation_values.clear();
      eval_lhs_elems(lhs_set_.relations, relation_values, into_context, *task);
      eval_lhs_elems(lhs_set_.attic_relations, relation_values, into_context, *task);
    }
  }
  
  rman.set_desired_timestamp(rhs_timestamp);
  
  std::vector< Node_Skeleton::Id_Type > changed_nodes;
  std::vector< Way_Skeleton::Id_Type > changed_ways;
  std::vector< Relation_Skeleton::Id_Type > changed_relations;
  {
    Requested_Context requested_context = evaluator->request_context();
    Prepare_Task_Context context(requested_context, stmt, rman);

    Owner< Eval_Task > task(evaluator->get_string_task(context, 0));

    Set_With_Context into_context;
    into_context.name = "";
    into_context.parent = &context;
    into_context.prefetch(requested_context.object_usage, input_set, stmt, rman);

    if (task)
    {
      eval_rhs_elems(input_set.nodes, node_values, changed_nodes, into_context, *task);
      eval_rhs_elems(input_set.attic_nodes, node_values, changed_nodes, into_context, *task);
      clear_elems(node_values, changed_nodes);
      
      eval_rhs_elems(input_set.ways, way_values, changed_ways, into_context, *task);
      eval_rhs_elems(input_set.attic_ways, way_values, changed_ways, into_context, *task);
      clear_elems(way_values, changed_ways);
      
      eval_rhs_elems(input_set.relations, relation_values, changed_relations, into_context, *task);
      eval_rhs_elems(input_set.attic_relations, relation_values, changed_relations, into_context, *task);
      clear_elems(relation_values, changed_relations);
    }
  }
  
  rman.set_desired_timestamp(lhs_timestamp_);
    
  Extra_Data_For_Diff extra_data_lhs(rman, stmt, lhs_set_, Output_Mode::ID
      | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
      | Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META
      | Output_Mode::GEOMETRY, 1., 0., 0., 0.);

  tags_quadtile(extra_data_lhs, lhs_set_.nodes, changed_nodes, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_lhs, lhs_set_.attic_nodes, changed_nodes, rman);

  tags_quadtile(extra_data_lhs, lhs_set_.ways, changed_ways, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_lhs, lhs_set_.attic_ways, changed_ways, rman);

  tags_quadtile(extra_data_lhs, lhs_set_.relations, changed_relations, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_lhs, lhs_set_.attic_relations, changed_relations, rman);
    
  rman.set_desired_timestamp(rhs_timestamp);
      
  set_target(true);
    
  Extra_Data_For_Diff extra_data_rhs(rman, stmt, input_set, Output_Mode::ID
      | Output_Mode::COORDS | Output_Mode::NDS | Output_Mode::MEMBERS
      | Output_Mode::TAGS | Output_Mode::VERSION | Output_Mode::META
      | Output_Mode::GEOMETRY, 1., 0., 0., 0.);

  tags_quadtile(extra_data_rhs, input_set.nodes, changed_nodes, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_rhs, input_set.attic_nodes, changed_nodes, rman);
  clear_nodes(rman, add_deletion_information);

  tags_quadtile(extra_data_rhs, input_set.ways, changed_ways, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_rhs, input_set.attic_ways, changed_ways, rman);
  clear_ways(rman, add_deletion_information);

  tags_quadtile(extra_data_rhs, input_set.relations, changed_relations, rman);
  if (rman.get_desired_timestamp() != NOW)
    tags_quadtile_attic(extra_data_rhs, input_set.attic_relations, changed_relations, rman);
  clear_relations(rman, add_deletion_information);
  
  Diff_Set local_result;
  local_result.swap(result);
  return local_result;
}
