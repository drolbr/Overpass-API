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
  print_item(ll_upper, skel, tags, meta, users);
}


struct Double_Coords
{
public:
  explicit Double_Coords(const std::vector< Quad_Coord >& geometry)
    : min_lat(100.0), max_lat(-100.0), min_lon(200.0), max_lon(-200.0)
  {
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    {
      if (it->ll_upper != 0 || it->ll_lower != 0)
      {
        double lat = ::lat(it->ll_upper, it->ll_lower);
        double lon = ::lon(it->ll_upper, it->ll_lower);
        min_lat = std::min(min_lat, lat);
        max_lat = std::max(max_lat, lat);
        min_lon = std::min(min_lon, lon);
        max_lon = std::max(max_lon, lon);
      }
    }
  }

  explicit Double_Coords(const std::vector< std::vector< Quad_Coord > >& geometry)
    : min_lat(100.0), max_lat(-100.0), min_lon(200.0), max_lon(-200.0)
  {
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
         it != geometry.end(); ++it)
    {
      for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
      {
        if (it2->ll_upper != 0 || it2->ll_lower != 0)
        {
          double lat = ::lat(it2->ll_upper, it2->ll_lower);
          double lon = ::lon(it2->ll_upper, it2->ll_lower);
          min_lat = std::min(min_lat, lat);
          max_lat = std::max(max_lat, lat);
          min_lon = std::min(min_lon, lon);
          max_lon = std::max(max_lon, lon);
        }
      }
    }
  }

  const std::pair< Quad_Coord, Quad_Coord* >& bounds()
  {
    if (max_lat > -100.0)
    {
      max = Quad_Coord(::ll_upper_(max_lat, max_lon), ::ll_lower(max_lat, max_lon));
      bounds_ = std::make_pair(Quad_Coord(::ll_upper_(min_lat, min_lon), ::ll_lower(min_lat, min_lon)), &max);
    }
    else
    {
      max = Quad_Coord(0u, 0u);
      bounds_ = std::make_pair(Quad_Coord(0u, 0u), &max);
    }
    return bounds_;
  }

  const std::pair< Quad_Coord, Quad_Coord* >& center()
  {
    if (max_lat > -100.0)
      center_ = std::make_pair(Quad_Coord(
          ::ll_upper_((min_lat + max_lat) / 2, (min_lon + max_lon) / 2),
          ::ll_lower((min_lat + max_lat) / 2, (min_lon + max_lon) / 2)
          ), (Quad_Coord*)0);
    else
      center_ = std::make_pair(Quad_Coord(0u, 0u), (Quad_Coord*)0);
    return center_;
  }

private:
  double min_lat;
  double max_lat;
  double min_lon;
  double max_lon;
  std::pair< Quad_Coord, Quad_Coord* > bounds_;
  std::pair< Quad_Coord, Quad_Coord* > center_;
  Quad_Coord max;
};


const std::pair< Quad_Coord, Quad_Coord* >* bound_variant(Double_Coords& double_coords, unsigned int mode)
{
  if (mode & Output_Mode::BOUNDS)
    return &double_coords.bounds();
  else if (mode & Output_Mode::CENTER)
    return &double_coords.center();

  return 0;
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
    print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
        meta, users);
  }
  else
    print_item(ll_upper, skel, tags, 0, 0, meta, users);
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
    print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
        meta, users);
  }
  else
    print_item(ll_upper, skel, tags, 0, 0, meta, users);
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
    print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        meta, users);
  }
  else
    print_item(ll_upper, skel, tags, 0, 0, meta, users);
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
    print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, extra_data.mode),
        ((extra_data.mode & Output_Mode::GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        meta, users);
  }
  else
    print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


template< class TIndex, class TObject >
void quadtile
    (const std::map< TIndex, std::vector< TObject > >& items, Set_Comparison& target,
     Transaction& transaction, Extra_Data_For_Diff& extra_data, uint32 limit, uint32& element_count)
{
  typename std::map< TIndex, std::vector< TObject > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename std::vector< TObject >::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
    {
      if (++element_count > limit)
        return;
      print_item(extra_data, target, item_it->first.val(), *it2);
    }
    ++item_it;
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
      const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
          = attic_meta_printer.get(item_it->first, it2->id, it2->timestamp);
      if (!meta)
        meta = current_meta_printer.get(item_it->first, it2->id, it2->timestamp);
      print_item(extra_data, item_it->first.val(), *it2, tag_store.get(item_it->first, *it2),
                 meta, extra_data.users);
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


void Set_Comparison::print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta)
{
  if (final_target)
  {
    std::vector< Node_Entry >::iterator nodes_it
        = std::lower_bound(nodes.begin(), nodes.end(), Node_Entry(ll_upper, skel));

    if (nodes_it == nodes.end() || skel.id < nodes_it->elem.id)
      different_nodes.push_back(std::make_pair(
	  Node_Entry(0xffu, Node_Skeleton(skel.id),
	      OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Node_Entry(ll_upper, skel,
              meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
    else
    {
      if (!(nodes_it->idx.val() == ll_upper) || !(nodes_it->elem.ll_lower == skel.ll_lower) ||
          (tags && !(nodes_it->tags == *tags)) || (meta && !(nodes_it->meta.timestamp == meta->timestamp)))
	different_nodes.push_back(std::make_pair(*nodes_it, Node_Entry(ll_upper, skel,
                  meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
                  tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
      nodes_it->idx = 0xffu;
    }
  }
  else
    nodes.push_back(Node_Entry(ll_upper, skel,
        meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Set_Comparison::clear_nodes(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Node_Skeleton::Id_Type > searched_ids;
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
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

    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_nodes.push_back(std::make_pair(*it,
	    Node_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfdu : 0xffu,
		Node_Skeleton(it->elem.id),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
    
    searched_ids.clear();
    for (std::vector< std::pair< Node_Entry, Node_Entry > >::const_iterator
        it = different_nodes.begin(); it != different_nodes.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
        searched_ids.push_back(it->second.elem.id);
    }

    get_indexes_< Uint32_Index, Node_Skeleton >(searched_ids, rman, true).swap(req);
    find_still_existing_skeletons< Uint32_Index, Node_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_ids);
    find_meta_elements< Uint32_Index, Node_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_meta);

    for (std::vector< std::pair< Node_Entry, Node_Entry > >::iterator
        it = different_nodes.begin(); it != different_nodes.end(); ++it)
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
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	different_nodes.push_back(std::make_pair(*it,
	    Node_Entry(0xffu, Node_Skeleton(it->elem.id),
	        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }

}


void Set_Comparison::print_nodes(
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, bool add_deletion_information)
{
  std::sort(different_nodes.begin(), different_nodes.end());
  for (std::vector< std::pair< Node_Entry, Node_Entry > >::const_iterator it = different_nodes.begin();
      it != different_nodes.end(); ++it)
  {
    if ((it->second.idx.val() | 2) == 0xffu)
    {
      if (add_deletion_information)
      {
        Node_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
                ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode,
            it->second.idx.val() == 0xfdu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            Point_Geometry(::lat(it->first.idx.val(), it->first.elem.ll_lower),
                ::lon(it->first.idx.val(), it->first.elem.ll_lower)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Null_Geometry null_geom;
      Point_Geometry old_geom(::lat(it->first.idx.val(), it->first.elem.ll_lower),
          ::lon(it->first.idx.val(), it->first.elem.ll_lower));
      Point_Geometry new_geom(::lat(it->second.idx.val(), it->second.elem.ll_lower),
          ::lon(it->second.idx.val(), it->second.elem.ll_lower));
      Opaque_Geometry* old_opaque = &null_geom;
      if (it->first.idx.val() != 0xfdu)
        old_opaque = &old_geom;
      output->print_item(it->first.elem, *old_opaque,
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          &users, output_mode, Output_Handler::modify,
          &it->second.elem, &new_geom,
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0);
    }
    else
      // No old element exists
      output->print_item(it->second.elem,
          Point_Geometry(::lat(it->second.idx.val(), it->second.elem.ll_lower),
              ::lon(it->second.idx.val(), it->second.elem.ll_lower)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          &users, output_mode, Output_Handler::create);
  }
}


void Set_Comparison::print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta)
{
  if (final_target)
  {
    std::vector< Way_Entry >::iterator ways_it
        = std::lower_bound(ways.begin(), ways.end(), Way_Entry(ll_upper, skel, std::vector< Quad_Coord >()));

    if (ways_it == ways.end() || skel.id < ways_it->elem.id)
      different_ways.push_back(std::make_pair(
	  Way_Entry(0xffu, Way_Skeleton(skel.id),
	      std::vector< Quad_Coord >(),
	      OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Way_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< Quad_Coord >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
    else
    {
      if (!(ways_it->idx.val() == ll_upper) || !(ways_it->elem.nds == skel.nds) ||
          (geometry && !(ways_it->geometry == *geometry)) ||
          (tags && !(ways_it->tags == *tags)) || (meta && !(ways_it->meta.timestamp == meta->timestamp)))
	different_ways.push_back(std::make_pair(*ways_it, Way_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< Quad_Coord >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
      ways_it->idx = 0xffu;
    }
  }
  else
    ways.push_back(Way_Entry(ll_upper, skel,
        geometry ? *geometry : std::vector< Quad_Coord >(),
        meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Set_Comparison::clear_ways(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Way_Skeleton::Id_Type > searched_ids;
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
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

    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_ways.push_back(std::make_pair(*it,
	    Way_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfdu : 0xffu,
		Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }
    }
    
    searched_ids.clear();
    for (std::vector< std::pair< Way_Entry, Way_Entry > >::const_iterator
        it = different_ways.begin(); it != different_ways.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
        searched_ids.push_back(it->second.elem.id);
    }

    get_indexes_< Uint31_Index, Way_Skeleton >(searched_ids, rman, true).swap(req);
    find_still_existing_skeletons< Uint31_Index, Way_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_ids);
    find_meta_elements< Uint31_Index, Way_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_meta);

    for (std::vector< std::pair< Way_Entry, Way_Entry > >::iterator
        it = different_ways.begin(); it != different_ways.end(); ++it)
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
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	different_ways.push_back(std::make_pair(*it,
	    Way_Entry(0xffu, Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }
}


void Set_Comparison::print_ways(
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, bool add_deletion_information)
{
  std::sort(different_ways.begin(), different_ways.end());
  for (std::vector< std::pair< Way_Entry, Way_Entry > >::const_iterator it = different_ways.begin();
      it != different_ways.end(); ++it)
  {
    if ((it->second.idx.val() | 2) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      Geometry_From_Quad_Coords broker;
      if (add_deletion_information)
      {
        Way_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode,
            it->second.idx.val() == 0xfdu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &users, output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      Geometry_From_Quad_Coords broker;
      Geometry_From_Quad_Coords new_broker;
      output->print_item(it->first.elem,
          broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          &users, output_mode, Output_Handler::modify,
          &it->second.elem,
          &new_broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords_new, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0);
    }
    else
    {
      // No old element exists
      Double_Coords double_coords(it->second.geometry);
      Geometry_From_Quad_Coords broker;
      output->print_item(it->second.elem,
          broker.make_way_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          &users, output_mode, Output_Handler::create);
    }
  }
}


void Set_Comparison::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta)
{
  if (final_target)
  {
    std::vector< Relation_Entry >::iterator relations_it
        = std::lower_bound(relations.begin(), relations.end(),
	    Relation_Entry(ll_upper, skel, std::vector< std::vector< Quad_Coord > >()));

    if (relations_it == relations.end() || skel.id < relations_it->elem.id)
      different_relations.push_back(std::make_pair(
	  Relation_Entry(0xffu, Relation_Skeleton(skel.id),
	      std::vector< std::vector< Quad_Coord > >(),
	      OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
	      std::vector< std::pair< std::string, std::string > >()),
	  Relation_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
    else
    {
      if (!(relations_it->idx.val() == ll_upper) || !(relations_it->elem.members == skel.members) ||
	  (geometry && !(relations_it->geometry == *geometry)) ||
	  (tags && !(relations_it->tags == *tags)) || (meta && !(relations_it->meta.timestamp == meta->timestamp)))
	different_relations.push_back(std::make_pair(*relations_it, Relation_Entry(ll_upper, skel,
              geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
              meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
              tags ? *tags : std::vector< std::pair< std::string, std::string > >())));
	
      relations_it->idx = 0xffu;
    }
  }
  else
    relations.push_back(Relation_Entry(ll_upper, skel,
        geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
        meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Set_Comparison::clear_relations(Resource_Manager& rman, bool add_deletion_information)
{
  if (add_deletion_information)
  {
    std::vector< Relation_Skeleton::Id_Type > searched_ids;
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
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

    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_relations.push_back(std::make_pair(*it,
	    Relation_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfdu : 0xffu,
		Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
      }

      req = get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true);
    }
    
    searched_ids.clear();
    for (std::vector< std::pair< Relation_Entry, Relation_Entry > >::const_iterator
        it = different_relations.begin(); it != different_relations.end(); ++it)
    {
      if (it->first.idx.val() == 0xffu)
        searched_ids.push_back(it->second.elem.id);
    }

    get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true).swap(req);
    find_still_existing_skeletons< Uint31_Index, Relation_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_ids);
    find_meta_elements< Uint31_Index, Relation_Skeleton >(
        rman, rman.get_diff_from_timestamp(), req, searched_ids).swap(found_meta);

    for (std::vector< std::pair< Relation_Entry, Relation_Entry > >::iterator
        it = different_relations.begin(); it != different_relations.end(); ++it)
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
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
	different_relations.push_back(std::make_pair(*it,
	    Relation_Entry(0xffu, Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
    }
  }
}


void Set_Comparison::print_relations(
    uint32 output_mode, Output_Handler* output,
    const std::map< uint32, std::string >& users, const std::map< uint32, std::string >& roles,
    bool add_deletion_information)
{
  std::sort(different_relations.begin(), different_relations.end());
  for (std::vector< std::pair< Relation_Entry, Relation_Entry > >::const_iterator it = different_relations.begin();
      it != different_relations.end(); ++it)
  {
    if ((it->second.idx.val() | 2) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      Geometry_From_Quad_Coords broker;
      if (add_deletion_information)
      {
        Relation_Skeleton new_skel(it->first.elem.id);
        output->print_item(it->first.elem,
            broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &roles, &users, output_mode,
            it->second.idx.val() == 0xfdu ? Output_Handler::push_away : Output_Handler::erase,
            &new_skel, 0, 0, &it->second.meta);
      }
      else
        output->print_item(it->first.elem,
            broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
                bound_variant(double_coords, output_mode)),
            (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
            (output_mode & Output_Mode::META) ? &it->first.meta : 0,
            &roles, &users, output_mode, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      Geometry_From_Quad_Coords broker;
      Geometry_From_Quad_Coords new_broker;
      output->print_item(it->first.elem,
          broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->first.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->first.tags : 0,
          (output_mode & Output_Mode::META) ? &it->first.meta : 0,
          &roles, &users, output_mode, Output_Handler::modify,
          &it->second.elem,
          &new_broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords_new, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0);
    }
    else
    {
      // No old element exists
      Double_Coords double_coords(it->second.geometry);
      Geometry_From_Quad_Coords broker;
      output->print_item(it->second.elem,
          broker.make_relation_geom((output_mode & Output_Mode::GEOMETRY) ? &it->second.geometry : 0,
              bound_variant(double_coords, output_mode)),
          (output_mode & Output_Mode::TAGS) ? &it->second.tags : 0,
          (output_mode & Output_Mode::META) ? &it->second.meta : 0,
          &roles, &users, output_mode, Output_Handler::create);
    }
  }
}


void Set_Comparison::compare_to_lhs(Resource_Manager& rman, const Statement& stmt,
    const Set& input_set, double south, double north, double west, double east, bool add_deletion_information)
{
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
}
