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

#include "../core/index_computations.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "../data/tag_store.h"
#include "../data/utils.h"
#include "../data/way_geometry_store.h"
#include "meta_collector.h"
#include "print.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>


const unsigned int NODE_FLUSH_SIZE = 1024*1024;
const unsigned int WAY_FLUSH_SIZE = 512*1024;
const unsigned int RELATION_FLUSH_SIZE = 512*1024;
const unsigned int AREA_FLUSH_SIZE = 64*1024;


class Print_Target
{
  public:
    typedef enum { visible_void, visible_false, visible_true } Show_New_Elem;
    
    Print_Target(uint32 mode_, Transaction& transaction);
    virtual ~Print_Target() {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void) = 0;
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void) = 0;
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void) = 0;
                            
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
			    const map< uint32, string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep) = 0;

    //TODO: remove. Replaced by Output_Mode
    static const unsigned int PRINT_IDS = 1;
    static const unsigned int PRINT_COORDS = 2;
    static const unsigned int PRINT_NDS = 4;
    static const unsigned int PRINT_MEMBERS = 8;
    static const unsigned int PRINT_TAGS = 0x10;
    static const unsigned int PRINT_VERSION = 0x20;
    static const unsigned int PRINT_META = 0x40;
    static const unsigned int PRINT_GEOMETRY = 0x80;
    static const unsigned int PRINT_BOUNDS = 0x100;
    static const unsigned int PRINT_CENTER = 0x200;
    static const unsigned int PRINT_COUNT = 0x400;

  protected:
    uint32 mode;
    map< uint32, string > roles;
};


Print_Target::Print_Target(uint32 mode_, Transaction& transaction) : mode(mode_)
{
  // prepare check update_members - load roles
  roles = relation_member_roles(transaction);
}


class Relation_Geometry_Store
{
public:
  Relation_Geometry_Store
      (const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
      const Statement& query, Resource_Manager& rman,
      double south_, double north_, double west_, double east_);
  Relation_Geometry_Store
      (const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& relations, uint64 timestamp,
      const Statement& query, Resource_Manager& rman,
      double south_, double north_, double west_, double east_);
      
  ~Relation_Geometry_Store();
  
  // return the empty vector if the relation is not found
  std::vector< std::vector< Quad_Coord > > get_geometry(const Relation_Skeleton& relation) const;
  
private:
  std::vector< Node > nodes;
  std::vector< Way_Skeleton > ways;
  Way_Geometry_Store* way_geometry_store;
  
  uint32 south;
  uint32 north;
  int32 west;
  int32 east;
  
  bool matches_bbox(uint32 ll_upper, uint32 ll_lower) const;
};


Generic_Statement_Maker< Print_Statement > Print_Statement::statement_maker("print");


Print_Statement::Print_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_),
      mode(0), order(order_by_id), limit(numeric_limits< unsigned int >::max()),
      way_geometry_store(0), attic_way_geometry_store(0), relation_geometry_store(0), attic_relation_geometry_store(0), collection_print_target(0),
      collection_mode(dont_collect), add_deletion_information(false),
      south(1.0), north(0.0), west(0.0), east(0.0)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["mode"] = "body";
  attributes["order"] = "id";
  attributes["limit"] = "";
  attributes["geometry"] = "skeleton";
  attributes["s"] = "";
  attributes["n"] = "";
  attributes["w"] = "";
  attributes["e"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  
  if (attributes["mode"] == "ids_only")
    mode = Print_Target::PRINT_IDS;
  else if (attributes["mode"] == "skeleton")
    mode = Print_Target::PRINT_IDS
        | Print_Target::PRINT_COORDS | Print_Target::PRINT_NDS | Print_Target::PRINT_MEMBERS;
  else if (attributes["mode"] == "tags")
    mode = Print_Target::PRINT_IDS | Print_Target::PRINT_TAGS;
  else if (attributes["mode"] == "body")
    mode = Print_Target::PRINT_IDS
        | Print_Target::PRINT_COORDS | Print_Target::PRINT_NDS | Print_Target::PRINT_MEMBERS
	| Print_Target::PRINT_TAGS;
  else if (attributes["mode"] == "meta")
    mode = Print_Target::PRINT_IDS
        | Print_Target::PRINT_COORDS | Print_Target::PRINT_NDS | Print_Target::PRINT_MEMBERS
	| Print_Target::PRINT_TAGS | Print_Target::PRINT_VERSION | Print_Target::PRINT_META;
  else if (attributes["mode"] == "count")
    mode = Print_Target::PRINT_COUNT;
  else
  {
    mode = 0;
    std::ostringstream temp;
    temp<<"For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\", \"body\", \"tags\",  \"count\", or \"meta\".";
    add_static_error(temp.str());
  }
  
  if (attributes["order"] == "id")
    order = order_by_id;
  else if (attributes["order"] == "quadtile")
    order = order_by_quadtile;
  else
  {
    std::ostringstream temp;
    temp<<"For the attribute \"order\" of the element \"print\""
        <<" the only allowed values are \"id\" or \"quadtile\".";
    add_static_error(temp.str());
  }
  
  if (attributes["limit"] != "")
    limit = atoll(attributes["limit"].c_str());
    
  if (attributes["geometry"] == "skeleton")
    ;
  else if (attributes["geometry"] == "full")
    mode |= Print_Target::PRINT_GEOMETRY | Print_Target::PRINT_BOUNDS;
  else if (attributes["geometry"] == "bounds")
    mode |= Print_Target::PRINT_BOUNDS;
  else if (attributes["geometry"] == "center")
    mode |= Print_Target::PRINT_CENTER;
  else
  {
    std::ostringstream temp;
    temp<<"For the attribute \"geometry\" of the element \"print\""
        <<" the only allowed values are \"skeleton\", \"full\", \"bounds\", or \"center\".";
    add_static_error(temp.str());
  }
  
  south = atof(attributes["s"].c_str());
  if ((south < -90.0) || (south > 90.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"print\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"print\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  if (north < south)
  {
    std::ostringstream temp;
    temp<<"The value of attribute \"n\" of the element \"print\""
    <<" must always be greater or equal than the value of attribute \"s\".";
    add_static_error(temp.str());
  }
  
  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"print\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0))
  {
    std::ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"print\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  if ((attributes["n"] == "") && (attributes["s"] == "") &&
      (attributes["w"] == "") && (attributes["e"] == ""))
  {
    south = 1.0;
    north = 0.0;
  }
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Node_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  target.print_item(ll_upper, skel, tags, meta, users);
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
  if (mode & Print_Target::PRINT_BOUNDS)
    return &double_coords.bounds();
  else if (mode & Print_Target::PRINT_CENTER)
    return &double_coords.center();
  
  return 0;
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Way_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = way_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, mode),
        ((mode & Print_Target::PRINT_GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Attic< Way_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (attic_way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = attic_way_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, mode),
        ((mode & Print_Target::PRINT_GEOMETRY) && geometry.size() == skel.nds.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Relation_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (relation_geometry_store)
  {
    std::vector< std::vector< Quad_Coord > > geometry = relation_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, mode),
        ((mode & Print_Target::PRINT_GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Attic< Relation_Skeleton >& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  if (attic_relation_geometry_store)
  {
    std::vector< std::vector< Quad_Coord > > geometry = attic_relation_geometry_store->get_geometry(skel);
    Double_Coords double_coords(geometry);
    target.print_item(ll_upper, skel, tags,
        geometry.empty() ? 0 : bound_variant(double_coords, mode),
        ((mode & Print_Target::PRINT_GEOMETRY) && geometry.size() == skel.members.size()) ? &geometry : 0,
        meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Area_Skeleton& skel,
                    const std::vector< std::pair< std::string, std::string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Area_Skeleton::Id_Type >* meta,
                    const std::map< uint32, std::string >* users)
{
  target.print_item(ll_upper, skel, tags, meta, users);
}


template< class Id_Type >
void collect_tags
  (std::map< Id_Type, std::vector< std::pair< std::string, std::string > > >& tags_by_id,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   std::map< uint32, std::vector< Id_Type > >& ids_by_coarse,
   uint32 coarse_index)
{
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if (binary_search(ids_by_coarse[coarse_index].begin(),
        ids_by_coarse[coarse_index].end(), tag_it.object()))
      tags_by_id[tag_it.object()].push_back
          (std::make_pair(tag_it.index().key, tag_it.index().value));
    ++tag_it;
  }
}


template< class TIndex, class TObject >
void quadtile
    (const std::map< TIndex, std::vector< TObject > >& items, Print_Target& target,
     Transaction& transaction, Print_Statement& stmt, uint32 limit, uint32& element_count)
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
      stmt.print_item(target, item_it->first.val(), *it2);
    }
    ++item_it;
  }
}


template< class Index, class Object >
void Print_Statement::tags_quadtile
    (const std::map< Index, std::vector< Object > >& items,
     const File_Properties& file_prop, Print_Target& target,
     Resource_Manager& rman, Transaction& transaction, uint32& element_count)
{
  //generate set of relevant coarse indices
  set< Index > coarse_indices;
  std::map< uint32, std::vector< typename Object::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< std::pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, transaction,
      (mode & Print_Target::PRINT_META) ? current_meta_file_properties< Object >() : 0);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename Object::Id_Type > items_db
      (transaction.data_index(&file_prop));
  typename Block_Backend< Tag_Index_Local, typename Object::Id_Type >::Range_Iterator
    tag_it(items_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  typename std::map< Index, std::vector< Object > >::const_iterator
      item_it(items.begin());
      
  //uint coarse_count = 0;
  for (typename set< Index >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    // Disable health_check: This ensures that a result will be always printed completely
//     if (++coarse_count >= 1024)
//     {
//       coarse_count = 0;
//       rman.health_check(*this);
//     }
    
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
    std::map< typename Object::Id_Type, std::vector< std::pair< std::string, std::string > > > tags_by_id;
    collect_tags(tags_by_id, items_db, tag_it, ids_by_coarse, it->val());
    
    // print the result
    while ((item_it != items.end()) &&
        ((item_it->first.val() & 0x7fffff00) == it->val()))
    {
      for (typename std::vector< Object >::const_iterator it2(item_it->second.begin());
          it2 != item_it->second.end(); ++it2)
      {
	if (++element_count > limit)
	  return;
	print_item(target, item_it->first.val(), *it2, &(tags_by_id[it2->id]),
		   meta_printer.get(item_it->first, it2->id), &(meta_printer.users()));
      }
      ++item_it;
    }
  }
}


template< class Index, class Object >
void Print_Statement::tags_quadtile_attic
    (const std::map< Index, std::vector< Attic< Object > > >& items,
     Print_Target& target,
     Resource_Manager& rman, Transaction& transaction, uint32& element_count)
{
  //generate set of relevant coarse indices
  set< Index > coarse_indices;
  std::map< uint32, std::vector< Attic< typename Object::Id_Type > > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< std::pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (items, transaction, 
      (mode & Print_Target::PRINT_META) ? current_meta_file_properties< Object >() : 0);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (items, transaction, 
      (mode & Print_Target::PRINT_META) ? attic_meta_file_properties< Object >() : 0);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename Object::Id_Type > current_tags_db
      (transaction.data_index(current_local_tags_file_properties< Object >()));
  typename Block_Backend< Tag_Index_Local, typename Object::Id_Type >::Range_Iterator
    current_tag_it(current_tags_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
      
  // iterate over the result
  Block_Backend< Tag_Index_Local, Attic< typename Object::Id_Type > > attic_tags_db
      (transaction.data_index(attic_local_tags_file_properties< Object >()));
  typename Block_Backend< Tag_Index_Local, Attic< typename Object::Id_Type > >::Range_Iterator
    attic_tag_it(attic_tags_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    
  typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      item_it(items.begin());
      
  //uint coarse_count = 0;
  for (typename set< Index >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    // Disable health_check: This ensures that a result will be always printed completely
//     if (++coarse_count >= 1024)
//     {
//       coarse_count = 0;
//       rman.health_check(*this);
//     }
    
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
    std::map< typename Object::Id_Type, std::vector< std::pair< std::string, std::string > > > tags_by_id;
    collect_attic_tags(tags_by_id, current_tags_db, current_tag_it, attic_tags_db, attic_tag_it,
                 ids_by_coarse, it->val(), typename Object::Id_Type(), typename Object::Id_Type());
    
    // print the result
    while ((item_it != items.end()) &&
        ((item_it->first.val() & 0x7fffff00) == it->val()))
    {
      for (typename std::vector< Attic< Object > >::const_iterator it2(item_it->second.begin());
          it2 != item_it->second.end(); ++it2)
      {
        if (++element_count > limit)
          return;
        const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
            = attic_meta_printer.get(item_it->first, it2->id, it2->timestamp);
        if (!meta)
          meta = current_meta_printer.get(item_it->first, it2->id, it2->timestamp);
        print_item(target, item_it->first.val(), *it2,
                          &(tags_by_id[it2->id]),
                   meta, &(current_meta_printer.users()));
      }
      ++item_it;
    }
  }
}


template< class TComp >
struct Skeleton_Comparator_By_Id {
  bool operator() (const std::pair< const TComp*, uint32 >& a, 
		   const std::pair< const TComp*, uint32 >& b)
  {
    return (*a.first < *b.first);
  }
};


template< typename Index, typename Object >
struct Maybe_Attic_Ref
{
public:
  Maybe_Attic_Ref(Index idx_, const Object* obj_, uint64 timestamp_)
  : idx(idx_), obj(obj_), timestamp(timestamp_) {}
  
  Index idx;
  const Object* obj;
  uint64 timestamp;
  
  bool operator<(const Maybe_Attic_Ref& rhs) const { return obj->id < rhs.obj->id; }
};


template< class Index, class Object >
std::vector< std::pair< const Object*, uint32 > > collect_items_by_id(
    const std::map< Index, std::vector< Object > >& items)
{
  std::vector< std::pair< const Object*, uint32 > > items_by_id;
  
  for (typename std::map< Index, std::vector< Object > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(std::make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< Object >());
  
  return items_by_id;
}


template< class Index, class Object >
std::vector< Maybe_Attic_Ref< Index, Object > > collect_items_by_id(
    const std::map< Index, std::vector< Object > >& items,
    const std::map< Index, std::vector< Attic< Object > > >& attic_items)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id;
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(Maybe_Attic_Ref< Index, Object >(it->first, &(*it2), NOW));
  }
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      it(attic_items.begin()); it != attic_items.end(); ++it)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(Maybe_Attic_Ref< Index, Object >(it->first, &(*it2), it2->timestamp));
  }
  sort(items_by_id.begin(), items_by_id.end());
  
  return items_by_id;
}


template< class Index, class Object >
void by_id
  (const std::map< Index, std::vector< Object > >& items, Print_Target& target,
   Transaction& transaction, Print_Statement& stmt, uint32 limit, uint32& element_count)
{
  std::vector< std::pair< const Object*, uint32 > > items_by_id = collect_items_by_id(items);
  
  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    stmt.print_item(target, items_by_id[i].second, *(items_by_id[i].first));
  }
}


template< class Index, class Object >
void by_id
  (const std::map< Index, std::vector< Object > >& items,
   const std::map< Index, std::vector< Attic< Object > > >& attic_items,
   Print_Target& target,
   Transaction& transaction, Print_Statement& stmt, uint32 limit, uint32& element_count)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id = collect_items_by_id(items, attic_items);
  
  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    if (items_by_id[i].timestamp == NOW)
      stmt.print_item(target, items_by_id[i].idx.val(), *items_by_id[i].obj);
    else
      stmt.print_item(target, items_by_id[i].idx.val(),
		      Attic< Object >(*items_by_id[i].obj, items_by_id[i].timestamp));
  }
}


template< class Index, class Object >
void collect_metadata(set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >& metadata,
		      const std::map< Index, std::vector< Object > >& items,
		      typename Object::Id_Type lower_id_bound, typename Object::Id_Type upper_id_bound,
		      Meta_Collector< Index, typename Object::Id_Type >& meta_printer)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (!(it2->id < lower_id_bound) && (it2->id < upper_id_bound))
      {
	const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
	    = meta_printer.get(it->first, it2->id);
	if (meta)
	  metadata.insert(*meta);
      }
    }
  }
}


template< class Index, class Object >
void collect_metadata(set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >& metadata,
                      const std::map< Index, std::vector< Attic< Object > > >& items,
                      typename Object::Id_Type lower_id_bound, typename Object::Id_Type upper_id_bound,
                      Meta_Collector< Index, typename Object::Id_Type >& current_meta_printer,
                      Meta_Collector< Index, typename Object::Id_Type >& attic_meta_printer)
{
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (!(it2->id < lower_id_bound) && (it2->id < upper_id_bound))
      {
        const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
            = current_meta_printer.get(it->first, it2->id, it2->timestamp);
        if (!meta || !(meta->timestamp < it2->timestamp))
          meta = attic_meta_printer.get(it->first, it2->id, it2->timestamp);
        if (meta)          
          metadata.insert(*meta);
      }
    }
  }
}


template< typename Id_Type >
typename set< OSM_Element_Metadata_Skeleton< Id_Type > >::const_iterator
    find_matching_metadata
    (const set< OSM_Element_Metadata_Skeleton< Id_Type > >& metadata,
     Id_Type ref, uint64 timestamp)
{
  typename set< OSM_Element_Metadata_Skeleton< Id_Type > >::iterator it
      = metadata.lower_bound(OSM_Element_Metadata_Skeleton< Id_Type >(ref, timestamp));
  if (it == metadata.begin())
    return metadata.end();
  --it;
  if (it->ref == ref)
    return it;
  else
    return metadata.end();
}


template< class Index, class Object >
void Print_Statement::tags_by_id
  (const std::map< Index, std::vector< Object > >& items,
   uint32 FLUSH_SIZE, Print_Target& target,
   Resource_Manager& rman, Transaction& transaction, uint32& element_count)
{
  std::vector< std::pair< const Object*, uint32 > > items_by_id = collect_items_by_id(items);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, transaction,
      (mode & Print_Target::PRINT_META) ? current_meta_file_properties< Object >() : 0);
  
  // iterate over the result
  for (typename Object::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    // Disable health_check: This ensures that a result will be always printed completely
    //rman.health_check(*this);
    
    typename Object::Id_Type lower_id_bound(items_by_id[id_pos.val()].first->id);
    typename Object::Id_Type upper_id_bound;
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[(id_pos + FLUSH_SIZE).val()].first->id;
    else
    {
      upper_id_bound = items_by_id[items_by_id.size()-1].first->id;
      ++upper_id_bound;
    }
    
    Tag_Store< Index, Object > tag_store(items, transaction, lower_id_bound, upper_id_bound);
    
    // collect metadata if required
    set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > metadata;
    collect_metadata(metadata, items, lower_id_bound, upper_id_bound, meta_printer);
    meta_printer.reset();

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      if (++element_count > limit)
	return;
      typename set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
          = metadata.lower_bound(OSM_Element_Metadata_Skeleton< typename Object::Id_Type >
              (items_by_id[i.val()].first->id));
      print_item(target, items_by_id[i.val()].second, *(items_by_id[i.val()].first),
		 tag_store.get_tags(*items_by_id[i.val()].first),
		 (meta_it != metadata.end() && meta_it->ref == items_by_id[i.val()].first->id) ?
		     &*meta_it : 0, &(meta_printer.users()));
    }
  }
}


template< class Index, class Object >
void Print_Statement::tags_by_id_attic
  (const std::map< Index, std::vector< Object > >& current_items,
   const std::map< Index, std::vector< Attic< Object > > >& attic_items,
   uint32 FLUSH_SIZE, Print_Target& target,
   Resource_Manager& rman, Transaction& transaction, uint32& element_count)
{
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id = collect_items_by_id(current_items, attic_items);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > only_current_meta_printer
      (current_items, transaction, 
      (mode & Print_Target::PRINT_META) ? current_meta_file_properties< Object >() : 0);
  
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (attic_items, transaction, 
      (mode & Print_Target::PRINT_META) ? current_meta_file_properties< Object >() : 0);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (attic_items, transaction, 
      (mode & Print_Target::PRINT_META) ? attic_meta_file_properties< Object >() : 0);
      
  for (typename Object::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    // Disable health_check: This ensures that a result will be always printed completely
    //rman.health_check(*this);
    
    typename Object::Id_Type lower_id_bound(items_by_id[id_pos.val()].obj->id);
    typename Object::Id_Type upper_id_bound;
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[(id_pos + FLUSH_SIZE).val()].obj->id;
    else
    {
      upper_id_bound = items_by_id[items_by_id.size()-1].obj->id;
      ++upper_id_bound;
    }
    
    Tag_Store< Index, Object > current_tag_store(current_items, transaction, lower_id_bound, upper_id_bound);
    Tag_Store< Index, Object > attic_tag_store(attic_items, transaction, lower_id_bound, upper_id_bound);
    
    // collect metadata if required
    set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > only_current_metadata;
    collect_metadata(only_current_metadata, current_items, lower_id_bound, upper_id_bound,
		     only_current_meta_printer);
    only_current_meta_printer.reset();

    set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > attic_metadata;
    collect_metadata(attic_metadata, attic_items, lower_id_bound, upper_id_bound,
                     current_meta_printer, attic_meta_printer);
    attic_meta_printer.reset();
    current_meta_printer.reset();

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      if (++element_count > limit)
	return;
      if (items_by_id[i.val()].timestamp == NOW)
      {
        typename set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
            = only_current_metadata.lower_bound(OSM_Element_Metadata_Skeleton< typename Object::Id_Type >
                (items_by_id[i.val()].obj->id));
        print_item(target, items_by_id[i.val()].idx.val(), *items_by_id[i.val()].obj,
		 current_tag_store.get_tags(*items_by_id[i.val()].obj),
		 (meta_it != only_current_metadata.end() && meta_it->ref == items_by_id[i.val()].obj->id) ?
		     &*meta_it : 0, &(only_current_meta_printer.users()));
      }
      else
      {
        typename set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
            = find_matching_metadata(attic_metadata,
                  items_by_id[i.val()].obj->id, items_by_id[i.val()].timestamp);
        print_item(target, items_by_id[i.val()].idx.val(),
		   Attic< Object >(*items_by_id[i.val()].obj, items_by_id[i.val()].timestamp),
		 attic_tag_store.get_tags(*items_by_id[i.val()].obj),
                 meta_it != attic_metadata.end() ? &*meta_it : 0, &current_meta_printer.users());
      }
    }
  }
}


Relation_Geometry_Store::~Relation_Geometry_Store()
{
  delete way_geometry_store;
}


Relation_Geometry_Store::Relation_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const Statement& query, Resource_Manager& rman,
     double south_, double north_, double west_, double east_)
    : way_geometry_store(0), south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{
  if (relations.empty())
  {
    // Turn off bounding bix, because it isn't needed anyway
    north = 0;
    south = 1;
  }
  
  std::set< std::pair< Uint32_Index, Uint32_Index > > node_ranges;
  if (south <= north)
    get_ranges_32(south_, north_, west_, east_).swap(node_ranges);
  
  // Retrieve all nodes referred by the relations.
  std::map< Uint32_Index, std::vector< Node_Skeleton > > node_members
      = relation_node_members(&query, rman, relations, north < south ? 0 : &node_ranges);
  
  // Order node ids by id.
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it = node_members.begin();
      it != node_members.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());
  
  std::set< std::pair< Uint31_Index, Uint31_Index > > way_ranges;
  if (south <= north)
    calc_parents(node_ranges).swap(way_ranges);
  
  // Retrieve all ways referred by the relations.
  std::map< Uint31_Index, std::vector< Way_Skeleton > > way_members
      = relation_way_members(&query, rman, relations, north < south ? 0 : &way_ranges);
      
  way_geometry_store = new Way_Geometry_Store(way_members, query, rman);
  
  // Order way ids by id.
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = way_members.begin();
      it != way_members.end(); ++it)
  {
    for (std::vector< Way_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      ways.push_back(*iit);
  }
  sort(ways.begin(), ways.end());
}


Relation_Geometry_Store::Relation_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations, uint64 timestamp,
     const Statement& query, Resource_Manager& rman,
     double south_, double north_, double west_, double east_)
    : way_geometry_store(0), south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{
  if (relations.empty())
  {
    // Turn off bounding bix, because it isn't needed anyway
    north = 0;
    south = 1;
  }
  
  std::set< std::pair< Uint32_Index, Uint32_Index > > node_ranges;
  if (south <= north)
    get_ranges_32(south_, north_, west_, east_).swap(node_ranges);
  
  // Retrieve all nodes referred by the relations.
  std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
      std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > nodes_by_idx
      = relation_node_members(&query, rman,
          std::map< Uint31_Index, std::vector< Relation_Skeleton > >(), relations, timestamp,
          north < south ? 0 : &node_ranges);
  
  // Order node ids by id.
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it = nodes_by_idx.first.begin();
      it != nodes_by_idx.first.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::iterator it = nodes_by_idx.second.begin();
      it != nodes_by_idx.second.end(); ++it)
  {
    for (std::vector< Attic< Node_Skeleton > >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());
      
  std::set< std::pair< Uint31_Index, Uint31_Index > > way_ranges;
  if (south <= north)
    calc_parents(node_ranges).swap(way_ranges);
  
  // Retrieve all ways referred by the relations.
  std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > ways_by_idx
      = relation_way_members(&query, rman,
          std::map< Uint31_Index, std::vector< Relation_Skeleton > >(), relations, timestamp,
          north < south ? 0 : &way_ranges);
  
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways_by_idx.first.begin();
      it != ways_by_idx.first.end(); ++it)
  {
    std::vector< Attic< Way_Skeleton > >& target = ways_by_idx.second[it->first];
    for (std::vector< Way_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      target.push_back(Attic< Way_Skeleton >(*iit, NOW));
  }
  
  way_geometry_store = new Way_Geometry_Store(ways_by_idx.second, timestamp, query, rman);
  
  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::iterator it = ways_by_idx.second.begin();
      it != ways_by_idx.second.end(); ++it)
  {
    for (std::vector< Attic< Way_Skeleton > >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      ways.push_back(*iit);
  }
  sort(ways.begin(), ways.end());
}


bool Relation_Geometry_Store::matches_bbox(uint32 ll_upper, uint32 ll_lower) const
{
  if (north < south)
    return true;
  uint32 lat(::ilat(ll_upper, ll_lower));
  int32 lon(::ilon(ll_upper, ll_lower));
  return (lat >= south && lat <= north &&
     ((lon >= west && lon <= east)
            || (east < west && (lon >= west || lon <= east))));
}


std::vector< std::vector< Quad_Coord > > Relation_Geometry_Store::get_geometry
    (const Relation_Skeleton& relation) const
{
  std::vector< std::vector< Quad_Coord > > result;
  for (std::vector< Relation_Entry >::const_iterator it = relation.members.begin();
       it != relation.members.end(); ++it)
  {
    if (it->type == Relation_Entry::NODE)
    {
      const Node* node = binary_search_for_id(nodes, it->ref);
      if (node == 0 || !matches_bbox(node->index, node->ll_lower_))
        result.push_back(std::vector< Quad_Coord >(1, Quad_Coord(0u, 0u)));
      else
        result.push_back(std::vector< Quad_Coord >(1, Quad_Coord(node->index, node->ll_lower_)));
    }
    else if (it->type == Relation_Entry::WAY)
    {
      const Way_Skeleton* way = binary_search_for_id(ways, Way_Skeleton::Id_Type(it->ref.val()));
      if (way == 0)
        result.push_back(std::vector< Quad_Coord >());
      else
      {
        result.push_back(way_geometry_store->get_geometry(*way));
        if (result.back().empty())
          ;
        else if (result.back().size() == 1)
        {
          if (!matches_bbox(result.back().begin()->ll_upper, result.back().begin()->ll_lower))
            *result.back().begin() = Quad_Coord(0u, 0u);
        }
        else
        {
          bool this_matches = matches_bbox(result.back()[0].ll_upper, result.back()[0].ll_lower);
          bool next_matches = matches_bbox(result.back()[1].ll_upper, result.back()[1].ll_lower);
          if (!this_matches && !next_matches)
            result.back()[0] = Quad_Coord(0u, 0u);
          for (uint i = 1; i < result.back().size() - 1; ++i)
          {
            bool last_matches = this_matches;
            this_matches = next_matches;
            next_matches = matches_bbox(result.back()[i+1].ll_upper, result.back()[i+1].ll_lower);
            if (!last_matches && !this_matches && !next_matches)
              result.back()[i] = Quad_Coord(0u, 0u);
          }
          if (!this_matches && !next_matches)
            result.back()[result.back().size()-1] = Quad_Coord(0u, 0u);
        }
      }
    }
    else if (it->type == Relation_Entry::RELATION)
      result.push_back(std::vector< Quad_Coord >());
  }
  
  return result;
}


struct Plain_Print_Target : public Print_Target
{
    Plain_Print_Target(uint32 mode_, bool add_deletion_information_, Transaction& transaction, Output_Handler* output_)
        : Print_Target(mode_, transaction), add_deletion_information(add_deletion_information_), output(output_) {}
    virtual ~Plain_Print_Target() {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
                            
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep);

    // helper functions for attic diffs
    void print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem,
			    uint32 new_ll_upper, const Node_Skeleton& new_skel,
                            const std::vector< std::pair< std::string, std::string > >* new_tags);
    void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem,
			    uint32 new_ll_upper, const Way_Skeleton& new_skel,
                            const std::vector< std::pair< std::string, std::string > >* new_tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* new_bounds = 0,
                            const std::vector< Quad_Coord >* new_geometry = 0);
    void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem,
			    uint32 new_ll_upper, const Relation_Skeleton& new_skel,
                            const std::vector< std::pair< std::string, std::string > >* new_tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* new_bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* new_geometry = 0);
    
private:
    bool add_deletion_information;
    Output_Handler* output;
};


struct Geometry_Broker
{
  Geometry_Broker() : geom(0) {}
  ~Geometry_Broker() { delete geom; }
  
  const Opaque_Geometry& make_way_geom(
      const std::vector< Quad_Coord >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds);
  const Opaque_Geometry& make_relation_geom(
      const std::vector< std::vector< Quad_Coord > >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds);
  
private:
  Opaque_Geometry* geom;
};


const Opaque_Geometry& Geometry_Broker::make_way_geom(
    const std::vector< Quad_Coord >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds)
{
  delete geom;
  geom = 0;
  
  if (geometry)
  {
    bool is_complete = true;
    for (std::vector< Quad_Coord >::const_iterator it = geometry->begin(); it != geometry->end(); ++it)
      is_complete &= (it->ll_upper != 0 || it->ll_lower != 0);
    
    if (is_complete)
    {
      std::vector< Point_Double > coords;
      for (std::vector< Quad_Coord >::const_iterator it = geometry->begin(); it != geometry->end(); ++it)
	coords.push_back(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
      geom = new Linestring_Geometry(coords);
    }
    else
    {
      Partial_Way_Geometry* pw_geom = new Partial_Way_Geometry();
      geom = pw_geom;
      for (std::vector< Quad_Coord >::const_iterator it = geometry->begin(); it != geometry->end(); ++it)
      {
	if (it->ll_upper != 0 || it->ll_lower != 0)
	  pw_geom->add_point(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
	else
	  pw_geom->add_point(Point_Double(100., 200.));
      }
    }
  }
  else if (bounds) 
  {
    if (bounds->second)
      geom = new Bbox_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
			   ::lon(bounds->first.ll_upper, bounds->first.ll_lower),
			   ::lat(bounds->second->ll_upper, bounds->second->ll_lower),
			   ::lon(bounds->second->ll_upper, bounds->second->ll_lower));
    else
      geom = new Point_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
			    ::lon(bounds->first.ll_upper, bounds->first.ll_lower));
  }
  else
    geom = new Null_Geometry();
  
  return *geom;
}


const Opaque_Geometry& Geometry_Broker::make_relation_geom(
    const std::vector< std::vector< Quad_Coord > >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds)
{
  delete geom;
  geom = 0;
  
  if (geometry)
  {
    bool is_complete = true;
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry->begin();
	it != geometry->end(); ++it)
    {
      if (it->empty())
	is_complete = false;
      else if (it->size() == 1)
	is_complete &= ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0);
      else
      {
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
	  is_complete &= (it2->ll_upper != 0 || it2->ll_lower != 0);
      }
    }
      
    if (is_complete)
    {
      Compound_Geometry* cp_geom = new Compound_Geometry();
      geom = cp_geom;
      for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry->begin();
	  it != geometry->end(); ++it)
      {
        if (it->empty())
	  cp_geom->add_component(new Null_Geometry());
	else if (it->size() == 1)
	  cp_geom->add_component(new Point_Geometry(
	      ::lat(it->front().ll_upper, it->front().ll_lower),
	      ::lon(it->front().ll_upper, it->front().ll_lower)));
	else
	{
          std::vector< Point_Double > coords;
          for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
	    coords.push_back(Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
	  cp_geom->add_component(new Linestring_Geometry(coords));
	}
      }
    }
    else if (geometry->empty())
      geom = new Null_Geometry();
    else
    {
      Partial_Relation_Geometry* pr_geom = new Partial_Relation_Geometry();
      geom = pr_geom;
      for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry->begin();
	  it != geometry->end(); ++it)
      {
        if (it->empty())
	  pr_geom->add_placeholder();
	else if (it->size() == 1 && ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0))
	  pr_geom->add_point(Point_Double(
	        ::lat(it->front().ll_upper, it->front().ll_lower),
	        ::lon(it->front().ll_upper, it->front().ll_lower)));
	else
	{
	  pr_geom->start_way();
          for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
	  {
	    if (it2->ll_upper != 0 || it2->ll_lower != 0)
	      pr_geom->add_way_point(
		  Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
	    else
	      pr_geom->add_way_placeholder();
	  }
	}
      }
    }
  }
  else if (bounds) 
  {
    if (bounds->second)
      geom = new Bbox_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
	  ::lon(bounds->first.ll_upper, bounds->first.ll_lower),
	  ::lat(bounds->second->ll_upper, bounds->second->ll_lower),
	  ::lon(bounds->second->ll_upper, bounds->second->ll_lower));
    else
      geom = new Point_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
	  ::lon(bounds->first.ll_upper, bounds->first.ll_lower));
  }
  else
    geom = new Null_Geometry();
  
  return *geom;
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
{
  if (output)
  {
    Node_Skeleton new_skel(skel.id);
    output->print_item(skel, Point_Geometry(::lat(ll_upper, skel.ll_lower), ::lon(ll_upper, skel.ll_lower)),
      mode & Print_Target::PRINT_TAGS ? tags : 0,
      mode & Print_Target::PRINT_META ? meta : 0,
      mode & Print_Target::PRINT_META ? users : 0,
      Output_Mode(mode),
      action == Output_Handler::erase && show_new_elem == visible_true ? Output_Handler::push_away : action,
      action == Output_Handler::erase && add_deletion_information ? &new_skel : 0, 0, 0, new_meta);
  }
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem,
			    uint32 new_ll_upper, const Node_Skeleton& new_skel,
                            const std::vector< std::pair< std::string, std::string > >* new_tags)
{
  if (output)
  {
    Point_Geometry new_geom(::lat(new_ll_upper, new_skel.ll_lower), ::lon(new_ll_upper, new_skel.ll_lower));
    output->print_item(skel, Point_Geometry(::lat(ll_upper, skel.ll_lower), ::lon(ll_upper, skel.ll_lower)),
      mode & Print_Target::PRINT_TAGS ? tags : 0,
      mode & Print_Target::PRINT_META ? meta : 0,
      mode & Print_Target::PRINT_META ? users : 0,
      Output_Mode(mode),
      action,
      &new_skel, &new_geom, new_tags, new_meta);
  }
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
{
  if (output)
  {
    Geometry_Broker broker;
    Way_Skeleton new_skel(skel.id);
    output->print_item(skel, broker.make_way_geom(geometry, bounds),
        mode & Print_Target::PRINT_TAGS ? tags : 0,
        mode & Print_Target::PRINT_META ? meta : 0,
        mode & Print_Target::PRINT_META ? users : 0,
        Output_Mode(mode),
        action == Output_Handler::erase && show_new_elem == visible_true ? Output_Handler::push_away : action,
        action == Output_Handler::erase && add_deletion_information ? &new_skel : 0, 0, 0, new_meta);
  }
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem,
			    uint32 new_ll_upper, const Way_Skeleton& new_skel,
                            const std::vector< std::pair< std::string, std::string > >* new_tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* new_bounds,
                            const std::vector< Quad_Coord >* new_geometry)
{
  if (output)
  {
    Geometry_Broker broker;
    Geometry_Broker new_broker;
    output->print_item(skel, broker.make_way_geom(geometry, bounds),
        mode & Print_Target::PRINT_TAGS ? tags : 0,
        mode & Print_Target::PRINT_META ? meta : 0,
        mode & Print_Target::PRINT_META ? users : 0,
        Output_Mode(mode),
        action,
        &new_skel, &new_broker.make_way_geom(new_geometry, new_bounds), new_tags, new_meta);
  }
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
{
  if (output)
  {
    Geometry_Broker broker;
    Relation_Skeleton new_skel(skel.id);
    output->print_item(skel, broker.make_relation_geom(geometry, bounds),
        mode & Print_Target::PRINT_TAGS ? tags : 0,
        mode & Print_Target::PRINT_META ? meta : 0,
	&roles,
        mode & Print_Target::PRINT_META ? users : 0,
	Output_Mode(mode),
        action == Output_Handler::erase && show_new_elem == visible_true ? Output_Handler::push_away : action,
        action == Output_Handler::erase && add_deletion_information ? &new_skel : 0, 0, 0, new_meta);
  }
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem,
			    uint32 new_ll_upper, const Relation_Skeleton& new_skel,
                            const std::vector< std::pair< std::string, std::string > >* new_tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* new_bounds,
                            const std::vector< std::vector< Quad_Coord > >* new_geometry)
{
  if (output)
  {
    Geometry_Broker broker;
    Geometry_Broker new_broker;
    output->print_item(skel, broker.make_relation_geom(geometry, bounds),
        mode & Print_Target::PRINT_TAGS ? tags : 0,
        mode & Print_Target::PRINT_META ? meta : 0,
	&roles,
        mode & Print_Target::PRINT_META ? users : 0,
        Output_Mode(mode),
        action,
        &new_skel, &new_broker.make_relation_geom(new_geometry, new_bounds), new_tags, new_meta);
  }
}


void Plain_Print_Target::print_item(uint32 ll_upper, const Area_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action)
{
  if (output)
  {
    Null_Geometry geom;
    Derived_Skeleton derived("area", Uint64(skel.id.val()));
    output->print_item(derived, geom,
        mode & Print_Target::PRINT_TAGS ? tags : 0,
	Output_Mode(mode));
  }
}
                            
                            
template < typename T >
struct Optional
{
  Optional(T* obj_) : obj(obj_) {}
  ~Optional() { delete obj; }
  
  T* obj;
};


template < typename T >
std::string to_string(T t)
{
  std::ostringstream out;
  out<<t;
  return out.str();
}


std::vector< std::pair< std::string, std::string > > make_count_tags(const Set& set, bool include_areas)
{    
  unsigned int num_nodes = count(set.nodes) + count(set.attic_nodes);
  unsigned int num_ways = count(set.ways) + count(set.attic_ways);
  unsigned int num_relations = count(set.relations) + count(set.attic_relations);
  unsigned int num_areas = include_areas ? count(set.areas) : 0;   
    
  std::vector< std::pair< std::string, std::string > > count_tags;
  count_tags.push_back(std::make_pair("nodes", to_string(num_nodes)));
  count_tags.push_back(std::make_pair("ways", to_string(num_ways)));
  count_tags.push_back(std::make_pair("relations", to_string(num_relations)));    
  if (include_areas)
    count_tags.push_back(std::make_pair("areas", to_string(num_areas)));
  count_tags.push_back(std::make_pair("total", to_string(num_nodes + num_ways + num_relations + num_areas)));
    
  return count_tags;
}


void Print_Statement::execute(Resource_Manager& rman)
{
  if (collection_mode != dont_collect)
  {
    execute_comparison(rman);
    return;
  }
  
  if (rman.area_updater())
    rman.area_updater()->flush();

  std::map< std::string, Set >::const_iterator mit(rman.sets().find(input));
  uint32 element_count = 0;
  if (mit == rman.sets().end())
    return;
    
  Plain_Print_Target output_target(mode, add_deletion_information,
			       *rman.get_transaction(), rman.get_global_settings().get_output_handler());
  
  if (mode & (Print_Target::PRINT_GEOMETRY | Print_Target::PRINT_BOUNDS | Print_Target::PRINT_CENTER))
  {
    delete way_geometry_store;
    way_geometry_store = new Way_Bbox_Geometry_Store(mit->second.ways, *this, rman,
        south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      delete attic_way_geometry_store;
      attic_way_geometry_store = new Way_Bbox_Geometry_Store
          (mit->second.attic_ways, rman.get_desired_timestamp(), *this, rman,
          south, north, west, east);
    }
        
    delete relation_geometry_store;
    relation_geometry_store = new Relation_Geometry_Store(
        mit->second.relations, *this, rman,
        south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      delete attic_relation_geometry_store;
      attic_relation_geometry_store = new Relation_Geometry_Store(
          mit->second.attic_relations, rman.get_desired_timestamp(), *this, rman,
          south, north, west, east);
    }
  }

  if (mode & Print_Target::PRINT_TAGS)
  {
    if (order == order_by_id)
    {
      if (rman.get_desired_timestamp() == NOW)
        tags_by_id(mit->second.nodes, NODE_FLUSH_SIZE, output_target, rman,
		  *rman.get_transaction(), element_count);
      else
        tags_by_id_attic(mit->second.nodes, mit->second.attic_nodes,
                   NODE_FLUSH_SIZE, output_target, rman,
                   *rman.get_transaction(), element_count);
      
      if (rman.get_desired_timestamp() == NOW)
        tags_by_id(mit->second.ways, WAY_FLUSH_SIZE, output_target, rman,
		  *rman.get_transaction(), element_count);
      else
        tags_by_id_attic(mit->second.ways, mit->second.attic_ways,
                   WAY_FLUSH_SIZE, output_target, rman,
                   *rman.get_transaction(), element_count);
      
      if (rman.get_desired_timestamp() == NOW)
        tags_by_id(mit->second.relations, RELATION_FLUSH_SIZE, output_target, rman,
		  *rman.get_transaction(), element_count);
      else
        tags_by_id_attic(mit->second.relations, mit->second.attic_relations,
                   RELATION_FLUSH_SIZE, output_target, rman,
                   *rman.get_transaction(), element_count);
      
      if (rman.get_area_transaction())
	tags_by_id(mit->second.areas, AREA_FLUSH_SIZE, output_target, rman,
		   *rman.get_area_transaction(), element_count);
    }
    else
    {
      tags_quadtile(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		    output_target, rman, *rman.get_transaction(), element_count);
      
      if (rman.get_desired_timestamp() != NOW)
        tags_quadtile_attic(mit->second.attic_nodes,
                      output_target, rman, *rman.get_transaction(), element_count);
      
      tags_quadtile(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		    output_target, rman, *rman.get_transaction(), element_count);
      
      if (rman.get_desired_timestamp() != NOW)
        tags_quadtile_attic(mit->second.attic_ways,
                      output_target, rman, *rman.get_transaction(), element_count);
      
      tags_quadtile(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		    output_target, rman, *rman.get_transaction(), element_count);
      
      if (rman.get_desired_timestamp() != NOW)
        tags_quadtile_attic(mit->second.attic_relations,
                      output_target, rman, *rman.get_transaction(), element_count);
      
      if (rman.get_area_transaction())
        tags_quadtile(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		      output_target, rman, *rman.get_area_transaction(), element_count);
    }
  }
  else if (mode & Print_Target::PRINT_COUNT)
  {
    std::vector< std::pair< std::string, std::string > > count_tags
        = make_count_tags(mit->second, rman.get_area_transaction());
    Derived_Skeleton derived("count", Uint64(0ull));
    rman.get_global_settings().get_output_handler()->print_item(
        derived, Null_Geometry(), &count_tags, Output_Mode(mode));
  }
  else
  {
    if (order == order_by_id)
    {
      by_id(mit->second.nodes, mit->second.attic_nodes,
	    output_target, *rman.get_transaction(), *this, limit, element_count);
      by_id(mit->second.ways, mit->second.attic_ways,
	    output_target, *rman.get_transaction(), *this, limit, element_count);
      by_id(mit->second.relations, mit->second.attic_relations,
	    output_target, *rman.get_transaction(), *this, limit, element_count);      
      if (rman.get_area_transaction())
	by_id(mit->second.areas, output_target, *rman.get_area_transaction(), *this, limit, element_count);
    }
    else
    {
      quadtile(mit->second.nodes, output_target, *rman.get_transaction(), *this, limit, element_count);
      quadtile(mit->second.attic_nodes, output_target, *rman.get_transaction(), *this, limit, element_count);
      
      quadtile(mit->second.ways, output_target, *rman.get_transaction(), *this, limit, element_count);
      quadtile(mit->second.attic_ways, output_target, *rman.get_transaction(), *this, limit, element_count);
      
      quadtile(mit->second.relations, output_target, *rman.get_transaction(), *this, limit, element_count);
      quadtile(mit->second.attic_relations, output_target, *rman.get_transaction(), *this, limit, element_count);
      
      if (rman.get_area_transaction())
	quadtile(mit->second.areas, output_target, *rman.get_area_transaction(), *this, limit, element_count);
    }
  }
  
  rman.health_check(*this);
}


class Collection_Print_Target : public Print_Target
{
  public:
    Collection_Print_Target(uint32 mode_, Transaction& transaction)
        : Print_Target(mode_, transaction), final_target(0) {}
    virtual ~Collection_Print_Target() {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
                            
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
                            const std::map< uint32, std::string >* users = 0,
			    const Output_Handler::Feature_Action& action = Output_Handler::keep);

    void set_target(Plain_Print_Target* target);
    
    void clear_nodes
        (Resource_Manager& rman, const std::map< uint32, std::string >* users = 0, bool add_deletion_information = false);
    void clear_ways
        (Resource_Manager& rman, const std::map< uint32, std::string >* users = 0, bool add_deletion_information = false);
    void clear_relations
        (Resource_Manager& rman, const std::map< uint32, std::string >* users = 0, bool add_deletion_information = false);
    
  private:
    
    struct Node_Entry
    {
      Uint31_Index idx;
      Node_Skeleton elem;
      OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta;
      std::vector< std::pair< std::string, std::string > > tags;
    
      Node_Entry(Uint31_Index idx_, Node_Skeleton elem_,
            OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > meta_
                = OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
            std::vector< std::pair< std::string, std::string > > tags_
                = std::vector< std::pair< std::string, std::string > >())
          : idx(idx_), elem(elem_), meta(meta_), tags(tags_) {}
    
      bool operator<(const Node_Entry& e) const
      {
        if (this->elem.id < e.elem.id)
          return true;
        if (e.elem.id < this->elem.id)
          return false;
        return (this->meta.version < e.meta.version);
      }
    };
    
    struct Way_Entry
    {
      Uint31_Index idx;
      Way_Skeleton elem;
      OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
      std::vector< std::pair< std::string, std::string > > tags;
      std::vector< Quad_Coord > geometry;
    
      Way_Entry(Uint31_Index idx_, Way_Skeleton elem_,
            const std::vector< Quad_Coord >& geometry_,
            OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta_
                = OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
            std::vector< std::pair< std::string, std::string > > tags_
                = std::vector< std::pair< std::string, std::string > >())
          : idx(idx_), elem(elem_), meta(meta_), tags(tags_), geometry(geometry_) {}
    
      bool operator<(const Way_Entry& e) const
      {
        if (this->elem.id < e.elem.id)
          return true;
        if (e.elem.id < this->elem.id)
          return false;
        return (this->meta.version < e.meta.version);
      }
    };
    
    struct Relation_Entry
    {
      Uint31_Index idx;
      Relation_Skeleton elem;
      OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > meta;
      std::vector< std::pair< std::string, std::string > > tags;
      std::vector< std::vector< Quad_Coord > > geometry;
    
      Relation_Entry(Uint31_Index idx_, Relation_Skeleton elem_,
            const std::vector< std::vector< Quad_Coord > >& geometry_,
            OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type > meta_
                = OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
            std::vector< std::pair< std::string, std::string > > tags_
                = std::vector< std::pair< std::string, std::string > >())
          : idx(idx_), elem(elem_), meta(meta_), tags(tags_), geometry(geometry_) {}
    
      bool operator<(const Relation_Entry& e) const
      {
        if (this->elem.id < e.elem.id)
          return true;
        if (e.elem.id < this->elem.id)
          return false;
        return (this->meta.version < e.meta.version);
      }
    };
  
    Plain_Print_Target* final_target;
    std::vector< Node_Entry > nodes;
    std::vector< Way_Entry > ways;
    std::vector< Relation_Entry > relations;
    std::vector< std::pair< Node_Entry, Node_Entry > > different_nodes;
    std::vector< std::pair< Way_Entry, Way_Entry > > different_ways;
    std::vector< std::pair< Relation_Entry, Relation_Entry > > different_relations;
};

    
void Collection_Print_Target::set_target(Plain_Print_Target* target)
{ 
  final_target = target;
  std::sort(nodes.begin(), nodes.end());
  std::sort(ways.begin(), ways.end());
  std::sort(relations.begin(), relations.end());
}


template< typename Index, typename Skeleton >
std::vector< typename Skeleton::Id_Type > find_still_existing_skeletons
    (Resource_Manager& rman, const std::vector< Index >& req,
     const std::vector< typename Skeleton::Id_Type >& searched_ids)
{
  std::vector< typename Skeleton::Id_Type > found_ids;
  std::map< Index, std::vector< Skeleton > > current_result;
  std::map< Index, std::vector< Attic< Skeleton > > > attic_result;
  if (rman.get_desired_timestamp() == NOW)
    collect_items_discrete(0, rman, *current_skeleton_file_properties< Skeleton >(), req,
        Id_Predicate< Skeleton >(searched_ids), current_result);
  else
  {
    collect_items_discrete_by_timestamp(0, rman, req,
        Id_Predicate< Skeleton >(searched_ids), current_result, attic_result);
    filter_attic_elements(rman, rman.get_desired_timestamp(), current_result, attic_result);
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
    (Resource_Manager& rman, const std::vector< Index >& idx_set,
     const std::vector< typename Skeleton::Id_Type >& searched_ids)
{
  std::map< typename Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > result;
  uint64 timestamp = rman.get_desired_timestamp();
  
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


void Collection_Print_Target::print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
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


void Collection_Print_Target::clear_nodes
    (Resource_Manager& rman, const std::map< uint32, std::string >* users, bool add_deletion_information)
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
        = find_still_existing_skeletons< Uint32_Index, Node_Skeleton >(rman, req, searched_ids);
    std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > > found_meta
        = find_meta_elements< Uint32_Index, Node_Skeleton >(rman, req, searched_ids);
      
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_nodes.push_back(std::make_pair(*it,
	    Node_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfeu : 0xffu,
		Node_Skeleton(it->elem.id),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
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
    
  std::sort(different_nodes.begin(), different_nodes.end());
  for (std::vector< std::pair< Node_Entry, Node_Entry > >::const_iterator it = different_nodes.begin();
      it != different_nodes.end(); ++it)
  {
    if ((it->second.idx.val() | 1) == 0xffu)
    {
      if (add_deletion_information)
        final_target->print_item(it->first.idx.val(), it->first.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
	    (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::erase,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0,
	    it->second.idx.val() == 0xfeu ? visible_true : visible_false);
      else
        final_target->print_item(it->first.idx.val(), it->first.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
	    (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
      // The elements differ
      final_target->print_item(it->first.idx.val(), it->first.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
	    (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::modify,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0,
	    visible_void, it->second.idx.val(), it->second.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->second.tags : 0);
    else
      // No old element exists
      final_target->print_item(it->second.idx.val(), it->second.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->second.tags : 0,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0, users, Output_Handler::create);
  }
}
 
                            
void Collection_Print_Target::print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
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


void Collection_Print_Target::clear_ways
    (Resource_Manager& rman, const std::map< uint32, std::string >* users, bool add_deletion_information)
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
        = find_still_existing_skeletons< Uint31_Index, Way_Skeleton >(rman, req, searched_ids);
    std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > > found_meta
        = find_meta_elements< Uint31_Index, Way_Skeleton >(rman, req, searched_ids);
      
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_ways.push_back(std::make_pair(*it,
	    Way_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfeu : 0xffu,
		Way_Skeleton(it->elem.id),
		std::vector< Quad_Coord >(),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
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
    
  std::sort(different_ways.begin(), different_ways.end());
  for (std::vector< std::pair< Way_Entry, Way_Entry > >::const_iterator it = different_ways.begin();
      it != different_ways.end(); ++it)
  {
    if ((it->second.idx.val() | 1) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      if (add_deletion_information)
        final_target->print_item(it->first.idx.val(), it->first.elem,
            (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->first.geometry : 0,
            (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::erase,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0,
	    it->second.idx.val() == 0xfeu ? visible_true : visible_false);
      else
        final_target->print_item(it->first.idx.val(), it->first.elem,
            (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->first.geometry : 0,
            (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      final_target->print_item(it->first.idx.val(), it->first.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->first.geometry : 0,
	    (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::modify,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0,
	    visible_void, it->second.idx.val(), it->second.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->second.tags : 0,
            bound_variant(double_coords_new, mode),
	    (mode & Print_Target::PRINT_GEOMETRY) ? &it->second.geometry : 0);
    }
    else
    {
      // No old element exists
      Double_Coords double_coords(it->second.geometry);
      final_target->print_item(it->second.idx.val(), it->second.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->second.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->second.geometry : 0,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0, users, Output_Handler::create);
    }
  }
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
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


void Collection_Print_Target::clear_relations
    (Resource_Manager& rman, const std::map< uint32, std::string >* users, bool add_deletion_information)
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
        = find_still_existing_skeletons< Uint31_Index, Relation_Skeleton >(rman, req, searched_ids);
    std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > > found_meta
        = find_meta_elements< Uint31_Index, Relation_Skeleton >(rman, req, searched_ids);
      
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
	std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
	    meta_it = found_meta.find(it->elem.id);
	different_relations.push_back(std::make_pair(*it,
	    Relation_Entry(std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ? 0xfeu : 0xffu,
		Relation_Skeleton(it->elem.id),
		std::vector< std::vector< Quad_Coord > >(),
	        meta_it != found_meta.end() ? meta_it->second
		    : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
		std::vector< std::pair< std::string, std::string > >())));
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
    
  std::sort(different_relations.begin(), different_relations.end());
  for (std::vector< std::pair< Relation_Entry, Relation_Entry > >::const_iterator it = different_relations.begin();
      it != different_relations.end(); ++it)
  {
    if ((it->second.idx.val() | 1) == 0xffu)
    {
      Double_Coords double_coords(it->first.geometry);
      if (add_deletion_information)
        final_target->print_item(it->first.idx.val(), it->first.elem,
            (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->first.geometry : 0,
            (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::erase,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0,
	    it->second.idx.val() == 0xfeu ? visible_true : visible_false);
      else
        final_target->print_item(it->first.idx.val(), it->first.elem,
            (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->first.geometry : 0,
            (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::erase);
    }
    else if (it->first.idx.val() != 0xffu)
    {
      // The elements differ
      Double_Coords double_coords(it->first.geometry);
      Double_Coords double_coords_new(it->second.geometry);
      final_target->print_item(it->first.idx.val(), it->first.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->first.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->first.geometry : 0,
	    (mode & Print_Target::PRINT_META) ? &it->first.meta : 0, users, Output_Handler::modify,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0,
	    visible_void, it->second.idx.val(), it->second.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->second.tags : 0,
            bound_variant(double_coords_new, mode),
	    (mode & Print_Target::PRINT_GEOMETRY) ? &it->second.geometry : 0);
    }
    else
    {
      // No old element exists
      Double_Coords double_coords(it->second.geometry);
      final_target->print_item(it->second.idx.val(), it->second.elem,
	    (mode & Print_Target::PRINT_TAGS) ? &it->second.tags : 0,
            bound_variant(double_coords, mode),
            (mode & Print_Target::PRINT_GEOMETRY) ? &it->second.geometry : 0,
	    (mode & Print_Target::PRINT_META) ? &it->second.meta : 0, users, Output_Handler::create);
    }
  }
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Area_Skeleton& skel,
                            const std::vector< std::pair< std::string, std::string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
                            const std::map< uint32, std::string >* users, const Output_Handler::Feature_Action& action) {}
                            

void Print_Statement::execute_comparison(Resource_Manager& rman)
{
  std::map< std::string, Set >::const_iterator mit(rman.sets().find(input));
  uint32 element_count = 0;
  if (mit == rman.sets().end())
    return;
    
  Print_Target* target = 0;
  Optional< Plain_Print_Target > output_target(0);
  uint32 outer_mode = mode;
  if (collection_mode == collect_lhs)
  {
    collection_print_target = new Collection_Print_Target(mode, *rman.get_transaction());
    target = collection_print_target;
    mode = Print_Target::PRINT_IDS
        | Print_Target::PRINT_COORDS | Print_Target::PRINT_NDS | Print_Target::PRINT_MEMBERS
        | Print_Target::PRINT_TAGS | Print_Target::PRINT_VERSION | Print_Target::PRINT_META
        | Print_Target::PRINT_GEOMETRY;
  }
  else
  {    
    output_target.obj =
        new Plain_Print_Target(mode, add_deletion_information,
			       *rman.get_transaction(), rman.get_global_settings().get_output_handler());
    collection_print_target->set_target(dynamic_cast< Plain_Print_Target* >(output_target.obj));
    target = collection_print_target;
    mode = Print_Target::PRINT_IDS
        | Print_Target::PRINT_COORDS | Print_Target::PRINT_NDS | Print_Target::PRINT_MEMBERS
        | Print_Target::PRINT_TAGS | Print_Target::PRINT_VERSION | Print_Target::PRINT_META
        | Print_Target::PRINT_GEOMETRY;
  }

  if (mode & (Print_Target::PRINT_GEOMETRY | Print_Target::PRINT_BOUNDS | Print_Target::PRINT_CENTER))
  {
    delete way_geometry_store;
    way_geometry_store = new Way_Bbox_Geometry_Store(mit->second.ways, *this, rman,
        south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      delete attic_way_geometry_store;
      attic_way_geometry_store = new Way_Bbox_Geometry_Store
          (mit->second.attic_ways, rman.get_desired_timestamp(), *this, rman,
          south, north, west, east);
    }
        
    delete relation_geometry_store;
    relation_geometry_store = new Relation_Geometry_Store(
        mit->second.relations, *this, rman,
        south, north, west, east);
    if (rman.get_desired_timestamp() < NOW)
    {
      delete attic_relation_geometry_store;
      attic_relation_geometry_store = new Relation_Geometry_Store(
          mit->second.attic_relations, rman.get_desired_timestamp(), *this, rman,
          south, north, west, east);
    }
  }

  if (mode & Print_Target::PRINT_TAGS)
  {
    User_Data_Cache* user_data_cache =
        (collection_mode == collect_rhs ? new User_Data_Cache(*rman.get_transaction()) : 0);
  
    tags_quadtile(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(), element_count);
      
    if (rman.get_desired_timestamp() != NOW)
      tags_quadtile_attic(mit->second.attic_nodes,
                      *target, rman, *rman.get_transaction(), element_count);
      
    if (collection_mode == collect_rhs)
      collection_print_target->clear_nodes(rman, &user_data_cache->users(), add_deletion_information);
      
    tags_quadtile(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(), element_count);
      
    if (rman.get_desired_timestamp() != NOW)
      tags_quadtile_attic(mit->second.attic_ways,
                      *target, rman, *rman.get_transaction(), element_count);
      
    if (collection_mode == collect_rhs)
      collection_print_target->clear_ways(rman, &user_data_cache->users(), add_deletion_information);
      
    tags_quadtile(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(), element_count);
      
    if (rman.get_desired_timestamp() != NOW)
      tags_quadtile_attic(mit->second.attic_relations,
                      *target, rman, *rman.get_transaction(), element_count);
      
    if (collection_mode == collect_rhs)
      collection_print_target->clear_relations(rman, &user_data_cache->users(), add_deletion_information);
    
    delete user_data_cache;
  }
  else
  {
    quadtile(mit->second.nodes, *target, *rman.get_transaction(), *this, limit, element_count);
    quadtile(mit->second.attic_nodes, *target, *rman.get_transaction(), *this, limit, element_count);
    if (collection_mode == collect_rhs)
      collection_print_target->clear_nodes(rman, 0, add_deletion_information);
      
    quadtile(mit->second.ways, *target, *rman.get_transaction(), *this, limit, element_count);
    quadtile(mit->second.attic_ways, *target, *rman.get_transaction(), *this, limit, element_count);
    if (collection_mode == collect_rhs)
      collection_print_target->clear_ways(rman, 0, add_deletion_information);
      
    quadtile(mit->second.relations, *target, *rman.get_transaction(), *this, limit, element_count);
    quadtile(mit->second.attic_relations, *target, *rman.get_transaction(), *this, limit, element_count);
    if (collection_mode == collect_rhs)
      collection_print_target->clear_relations(rman, 0, add_deletion_information);
  }

  if (collection_mode == collect_lhs)
    mode = outer_mode;
  
  rman.health_check(*this);
}


Print_Statement::~Print_Statement()
{
  delete way_geometry_store;
  delete attic_way_geometry_store;
  delete relation_geometry_store;
  delete attic_relation_geometry_store;
  delete collection_print_target;
}


void Print_Statement::set_collect_lhs()
{
  collection_mode = collect_lhs;
}


void Print_Statement::set_collect_rhs(bool add_deletion_information_)
{  
  collection_mode = collect_rhs;
  add_deletion_information = add_deletion_information_;
}
