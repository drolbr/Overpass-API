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
#include "../frontend/print_target.h"
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


Print_Target::Print_Target(uint32 mode_, Transaction& transaction) : mode(mode_)
{
  // prepare check update_members - load roles
  roles = relation_member_roles(transaction);
}


Generic_Statement_Maker< Print_Statement > Print_Statement::statement_maker("print");


Print_Statement::Print_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Statement(line_number_),
      mode(0), order(order_by_id), limit(numeric_limits< unsigned int >::max()),
      output_handle(0), way_geometry_store(0), attic_way_geometry_store(0), relation_geometry_store(0), attic_relation_geometry_store(0), collection_print_target(0),
      collection_mode(dont_collect), add_deletion_information(false),
      south(1.0), north(0.0), west(0.0), east(0.0)
{
  map< string, string > attributes;
  
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
    ostringstream temp;
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
    ostringstream temp;
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
    ostringstream temp;
    temp<<"For the attribute \"geometry\" of the element \"print\""
        <<" the only allowed values are \"skeleton\", \"full\", \"bounds\", or \"center\".";
    add_static_error(temp.str());
  }
  
  south = atof(attributes["s"].c_str());
  if ((south < -90.0) || (south > 90.0))
  {
    ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"print\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  north = atof(attributes["n"].c_str());
  if ((north < -90.0) || (north > 90.0))
  {
    ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"print\""
    <<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  if (north < south)
  {
    ostringstream temp;
    temp<<"The value of attribute \"n\" of the element \"print\""
    <<" must always be greater or equal than the value of attribute \"s\".";
    add_static_error(temp.str());
  }
  
  west = atof(attributes["w"].c_str());
  if ((west < -180.0) || (west > 180.0))
  {
    ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"print\""
    <<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  east = atof(attributes["e"].c_str());
  if ((east < -180.0) || (east > 180.0))
  {
    ostringstream temp;
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
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
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
      bounds_ = make_pair(Quad_Coord(::ll_upper_(min_lat, min_lon), ::ll_lower(min_lat, min_lon)), &max);
    }
    else
    {
      max = Quad_Coord(0u, 0u);
      bounds_ = make_pair(Quad_Coord(0u, 0u), &max);
    }
    return bounds_;
  }
  
  const std::pair< Quad_Coord, Quad_Coord* >& center()
  {
    if (max_lat > -100.0)
      center_ = make_pair(Quad_Coord(
          ::ll_upper_((min_lat + max_lat) / 2, (min_lon + max_lon) / 2),
          ::ll_lower((min_lat + max_lat) / 2, (min_lon + max_lon) / 2)
          ), (Quad_Coord*)0);
    else
      center_ = make_pair(Quad_Coord(0u, 0u), (Quad_Coord*)0);
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
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
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
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
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
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
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
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
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
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Area_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
{
  target.print_item(ll_upper, skel, tags, meta, users);
}

void Print_Statement::print_item_count(Print_Target& target, const Output_Item_Count & item_count)
{
  target.print_item_count(item_count);
}

template< class TIndex, class TObject >
uint32 count_items
    (const map< TIndex, vector< TObject > >& items, Print_Target& target,
     Transaction& transaction, Print_Statement& stmt)
{
  uint32 item_count = 0;
  typename map<TIndex, vector<TObject> >::const_iterator item_it(items.begin());

  while (item_it != items.end())
  {
    for (typename vector<TObject>::const_iterator it2(item_it->second.begin());
        it2 != item_it->second.end(); ++it2)
      item_count++;
    ++item_it;
  }
  return item_count;
}

template< class Id_Type >
void collect_tags
  (map< Id_Type, vector< pair< string, string > > >& tags_by_id,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   map< uint32, vector< Id_Type > >& ids_by_coarse,
   uint32 coarse_index)
{
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if (binary_search(ids_by_coarse[coarse_index].begin(),
        ids_by_coarse[coarse_index].end(), tag_it.object()))
      tags_by_id[tag_it.object()].push_back
          (make_pair(tag_it.index().key, tag_it.index().value));
    ++tag_it;
  }
}


template< class Id_Type >
void collect_tags
  (std::map< Attic< Id_Type >, std::vector< std::pair< std::string, std::string > > >& tags_by_id,
   const Block_Backend< Tag_Index_Local, Id_Type >& current_items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& current_tag_it,
   const Block_Backend< Tag_Index_Local, Attic< Id_Type > >& attic_items_db,
   typename Block_Backend< Tag_Index_Local, Attic< Id_Type > >::Range_Iterator& attic_tag_it,
   std::map< uint32, std::vector< Attic< Id_Type > > >& ids_by_coarse,
   uint32 coarse_index,
   Id_Type lower_id_bound, Id_Type upper_id_bound)
{
  std::vector< Attic< Id_Type > > id_vec = ids_by_coarse[coarse_index];
  std::sort(id_vec.begin(), id_vec.end());

  if (!(upper_id_bound == Id_Type()))
  {
    typename std::vector< Attic< Id_Type > >::iterator it_id
        = std::lower_bound(id_vec.begin(), id_vec.end(),
                           Attic< Id_Type >(upper_id_bound, 0ull));
    id_vec.erase(it_id, id_vec.end());
  }
  if (!(lower_id_bound == Id_Type()))
  {
    typename std::vector< Attic< Id_Type > >::iterator it_id
        = std::lower_bound(id_vec.begin(), id_vec.end(),
                           Attic< Id_Type >(lower_id_bound, 0ull));
    id_vec.erase(id_vec.begin(), it_id);
  }
  
  std::map< Attic< Id_Type >, std::vector< std::pair< std::string, std::string > > > found_tags;
  
  // Collect all id-matched tag information from the current tags
  while ((!(current_tag_it == current_items_db.range_end())) &&
      (((current_tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    typename std::vector< Attic< Id_Type > >::const_iterator it_id
        = std::lower_bound(id_vec.begin(), id_vec.end(), Attic< Id_Type >(current_tag_it.object(), 0ull));
    typename std::vector< Attic< Id_Type > >::const_iterator it_id_end
        = std::upper_bound(id_vec.begin(), id_vec.end(), Attic< Id_Type >
            (current_tag_it.object(), 0xffffffffffffffffull));
    if (it_id != it_id_end)
      found_tags[Attic< Id_Type >(current_tag_it.object(), 0xffffffffffffffffull)].push_back
          (make_pair(current_tag_it.index().key, current_tag_it.index().value));
    ++current_tag_it;
  }
  
  // Collect all id-matched tag information that is younger than the respective timestamp from the attic tags
  while ((!(attic_tag_it == attic_items_db.range_end())) &&
      (((attic_tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    typename std::vector< Attic< Id_Type > >::const_iterator it_id
        = std::lower_bound(id_vec.begin(), id_vec.end(), Attic< Id_Type >(attic_tag_it.object(), 0ull));
    typename std::vector< Attic< Id_Type > >::const_iterator it_id_end
        = std::upper_bound(id_vec.begin(), id_vec.end(), attic_tag_it.object());
    if (it_id != it_id_end)
      found_tags[attic_tag_it.object()].push_back
          (make_pair(attic_tag_it.index().key, attic_tag_it.index().value));
    ++attic_tag_it;
  }
  
  // Actually take for each object and key of the multiple versions only the oldest valid version
  for (typename std::map< Attic< Id_Type >, std::vector< std::pair< std::string, std::string > > >
          ::const_iterator
      it = found_tags.begin(); it != found_tags.end(); ++it)
  {
    typename std::vector< Attic< Id_Type > >::const_iterator it_id
        = std::lower_bound(id_vec.begin(), id_vec.end(), Attic< Id_Type >(it->first, 0ull));
    typename std::vector< Attic< Id_Type > >::const_iterator it_id_end
        = std::upper_bound(id_vec.begin(), id_vec.end(), it->first);
    while (it_id != it_id_end)
    {
      std::vector< std::pair< std::string, std::string > >& obj_vec = tags_by_id[*it_id];
      std::vector< std::pair< std::string, std::string > >::const_iterator last_added_it = it->second.end();
      for (std::vector< std::pair< std::string, std::string > >::const_iterator
          it_source = it->second.begin(); it_source != it->second.end(); ++it_source)
      {
        if (last_added_it != it->second.end())
        {
          if (last_added_it->first == it_source->first)
          {
            obj_vec.push_back(*it_source);
            continue;
          }
          else
            last_added_it = it->second.end();
        }
        
        std::vector< std::pair< std::string, std::string > >::const_iterator it_obj = obj_vec.begin();
        for (; it_obj != obj_vec.end(); ++it_obj)
        {
          if (it_obj->first == it_source->first)
            break;
        }
        if (it_obj == obj_vec.end())
        {
          obj_vec.push_back(*it_source);
          last_added_it = it_source;
        }
      }
      ++it_id;
    }
  }
  
  // Remove empty tags. They are placeholders for tags added later than each timestamp in question.
  for (typename std::map< Attic< Id_Type >, std::vector< std::pair< std::string, std::string > > >
          ::iterator
      it_obj = tags_by_id.begin(); it_obj != tags_by_id.end(); ++it_obj)
  {
    for (typename std::vector< std::pair< std::string, std::string > >::size_type
        i = 0; i < it_obj->second.size(); )
    {
      if (it_obj->second[i].second == void_tag_value())
      {
        it_obj->second[i] = it_obj->second.back();
        it_obj->second.pop_back();
      }
      else
        ++i;
    }
  }
}


template< class Id_Type >
void collect_tags_framed
  (map< Id_Type, vector< pair< string, string > > >& tags_by_id,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   map< uint32, vector< Id_Type > >& ids_by_coarse,
   uint32 coarse_index,
   Id_Type lower_id_bound, Id_Type upper_id_bound)
{
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if (!(tag_it.object() < lower_id_bound) &&
      (tag_it.object() < upper_id_bound) &&
      (binary_search(ids_by_coarse[coarse_index].begin(),
	ids_by_coarse[coarse_index].end(), tag_it.object())))
      tags_by_id[tag_it.object()].push_back
          (make_pair(tag_it.index().key, tag_it.index().value));
    ++tag_it;
  }
}


template< class TIndex, class TObject >
void quadtile
    (const map< TIndex, vector< TObject > >& items, Print_Target& target,
     Transaction& transaction, Print_Statement& stmt, uint32 limit, uint32& element_count)
{
  typename map< TIndex, vector< TObject > >::const_iterator
      item_it(items.begin());
  // print the result
  while (item_it != items.end())
  {
    for (typename vector< TObject >::const_iterator it2(item_it->second.begin());
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
    (const map< Index, vector< Object > >& items,
     const File_Properties& file_prop, Print_Target& target,
     Resource_Manager& rman, Transaction& transaction,
     const File_Properties* meta_file_prop, uint32& element_count)
{
  //generate set of relevant coarse indices
  set< Index > coarse_indices;
  map< uint32, vector< typename Object::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, rman, meta_file_prop);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename Object::Id_Type > items_db
      (transaction.data_index(&file_prop));
  typename Block_Backend< Tag_Index_Local, typename Object::Id_Type >::Range_Iterator
    tag_it(items_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  typename map< Index, vector< Object > >::const_iterator
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
    
    map< typename Object::Id_Type, vector< pair< string, string > > > tags_by_id;
    collect_tags(tags_by_id, items_db, tag_it, ids_by_coarse, it->val());
    
    // print the result
    while ((item_it != items.end()) &&
        ((item_it->first.val() & 0x7fffff00) == it->val()))
    {
      for (typename vector< Object >::const_iterator it2(item_it->second.begin());
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
    (const map< Index, vector< Attic< Object > > >& items,
     Print_Target& target,
     Resource_Manager& rman, Transaction& transaction,
     const File_Properties* current_meta_file_prop, const File_Properties* attic_meta_file_prop,
     uint32& element_count)
{
  //generate set of relevant coarse indices
  set< Index > coarse_indices;
  map< uint32, vector< Attic< typename Object::Id_Type > > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (items, rman, current_meta_file_prop);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (items, rman, attic_meta_file_prop);
  
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
    
  typename map< Index, vector< Attic< Object > > >::const_iterator
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
    
    map< Attic< typename Object::Id_Type >, vector< pair< string, string > > > tags_by_id;
    collect_tags(tags_by_id, current_tags_db, current_tag_it, attic_tags_db, attic_tag_it,
                 ids_by_coarse, it->val(), typename Object::Id_Type(), typename Object::Id_Type());
    
    // print the result
    while ((item_it != items.end()) &&
        ((item_it->first.val() & 0x7fffff00) == it->val()))
    {
      for (typename vector< Attic< Object > >::const_iterator it2(item_it->second.begin());
          it2 != item_it->second.end(); ++it2)
      {
        if (++element_count > limit)
          return;
        const OSM_Element_Metadata_Skeleton< typename Object::Id_Type >* meta
            = attic_meta_printer.get(item_it->first, it2->id, it2->timestamp);
        if (!meta)
          meta = current_meta_printer.get(item_it->first, it2->id, it2->timestamp);
        print_item(target, item_it->first.val(), *it2,
                          &(tags_by_id[Attic< typename Object::Id_Type >(it2->id, it2->timestamp)]),
                   meta, &(current_meta_printer.users()));
      }
      ++item_it;
    }
  }
}


template< class TComp >
struct Skeleton_Comparator_By_Id {
  bool operator() (const pair< const TComp*, uint32 >& a, 
		   const pair< const TComp*, uint32 >& b)
  {
    return (*a.first < *b.first);
  }
};

template< class TIndex, class TObject >
void by_id
  (const map< TIndex, vector< TObject > >& items, Print_Target& target,
   Transaction& transaction, Print_Statement& stmt, uint32 limit, uint32& element_count)
{
  // order relevant elements by id
  vector< pair< const TObject*, uint32 > > items_by_id;
  for (typename map< TIndex, vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< TObject >());
  
  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    stmt.print_item(target, items_by_id[i].second, *(items_by_id[i].first));
  }
}


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


template< class TIndex, class TObject >
void by_id
  (const std::map< TIndex, std::vector< TObject > >& items,
   const std::map< TIndex, std::vector< Attic< TObject > > >& attic_items,
   Print_Target& target,
   Transaction& transaction, Print_Statement& stmt, uint32 limit, uint32& element_count)
{
  // order relevant elements by id
  std::vector< Maybe_Attic_Ref< TIndex, TObject > > items_by_id;
  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename std::vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(Maybe_Attic_Ref< TIndex, TObject >(it->first, &(*it2), NOW));
  }
  for (typename std::map< TIndex, std::vector< Attic< TObject > > >::const_iterator
      it(attic_items.begin()); it != attic_items.end(); ++it)
  {
    for (typename std::vector< Attic< TObject > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(Maybe_Attic_Ref< TIndex, TObject >(it->first, &(*it2), it2->timestamp));
  }
  sort(items_by_id.begin(), items_by_id.end());
  
  // iterate over the result
  for (uint32 i(0); i < items_by_id.size(); ++i)
  {
    if (++element_count > limit)
      return;
    if (items_by_id[i].timestamp == NOW)
      stmt.print_item(target, items_by_id[i].idx.val(), *items_by_id[i].obj);
    else
      stmt.print_item(target, items_by_id[i].idx.val(),
		      Attic< TObject >(*items_by_id[i].obj, items_by_id[i].timestamp));
  }
}


template< class TIndex, class TObject >
void collect_metadata(set< OSM_Element_Metadata_Skeleton< typename TObject::Id_Type > >& metadata,
		      const map< TIndex, vector< TObject > >& items,
		      typename TObject::Id_Type lower_id_bound, typename TObject::Id_Type upper_id_bound,
		      Meta_Collector< TIndex, typename TObject::Id_Type >& meta_printer)
{
  for (typename map< TIndex, vector< TObject > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (!(it2->id < lower_id_bound) && (it2->id < upper_id_bound))
      {
	const OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >* meta
	    = meta_printer.get(it->first, it2->id);
	if (meta)
	  metadata.insert(*meta);
      }
    }
  }
}


template< class TIndex, class TObject >
void collect_metadata(set< OSM_Element_Metadata_Skeleton< typename TObject::Id_Type > >& metadata,
                      const map< TIndex, vector< Attic< TObject > > >& items,
                      typename TObject::Id_Type lower_id_bound, typename TObject::Id_Type upper_id_bound,
                      Meta_Collector< TIndex, typename TObject::Id_Type >& current_meta_printer,
                      Meta_Collector< TIndex, typename TObject::Id_Type >& attic_meta_printer)
{
  for (typename map< TIndex, vector< Attic< TObject > > >::const_iterator
      it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< Attic< TObject > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (!(it2->id < lower_id_bound) && (it2->id < upper_id_bound))
      {
        const OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >* meta
            = current_meta_printer.get(it->first, it2->id, it2->timestamp);
        if (!meta || !(meta->timestamp < it2->timestamp))
          meta = attic_meta_printer.get(it->first, it2->id, it2->timestamp);
        if (meta)          
          metadata.insert(*meta);
      }
    }
  }
}


template< class TIndex, class TObject >
void Print_Statement::tags_by_id
  (const map< TIndex, vector< TObject > >& items,
   const File_Properties& file_prop,
   uint32 FLUSH_SIZE, Print_Target& target,
   Resource_Manager& rman, Transaction& transaction,
   const File_Properties* meta_file_prop, uint32& element_count)
{
  // order relevant elements by id
  vector< pair< const TObject*, uint32 > > items_by_id;
  for (typename map< TIndex, vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< TObject >());
  
  //generate set of relevant coarse indices
  set< TIndex > coarse_indices;
  map< uint32, vector< typename TObject::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  for (typename set< TIndex >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< TIndex, typename TObject::Id_Type > meta_printer(items, rman, meta_file_prop);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  for (typename TObject::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    // Disable health_check: This ensures that a result will be always printed completely
    //rman.health_check(*this);
    
    map< typename TObject::Id_Type, vector< pair< string, string > > > tags_by_id;
    typename TObject::Id_Type lower_id_bound(items_by_id[id_pos.val()].first->id);
    typename TObject::Id_Type upper_id_bound;
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[(id_pos + FLUSH_SIZE).val()].first->id;
    else
    {
      upper_id_bound = items_by_id[items_by_id.size()-1].first->id;
      ++upper_id_bound;
    }
    
    typename Block_Backend< Tag_Index_Local, typename TObject::Id_Type >::Range_Iterator
        tag_it(items_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    for (typename set< TIndex >::const_iterator
        it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
      collect_tags_framed< typename TObject::Id_Type >(tags_by_id, items_db, tag_it, ids_by_coarse, it->val(),
			  lower_id_bound, upper_id_bound);
    
    // collect metadata if required
    set< OSM_Element_Metadata_Skeleton< typename TObject::Id_Type > > metadata;
    collect_metadata(metadata, items, lower_id_bound, upper_id_bound, meta_printer);
    meta_printer.reset();

    // print the result
    for (typename TObject::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      typename set< OSM_Element_Metadata_Skeleton< typename TObject::Id_Type > >::const_iterator meta_it
          = metadata.lower_bound(OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >
              (items_by_id[i.val()].first->id));
      if (++element_count > limit)
	return;
      print_item(target, items_by_id[i.val()].second, *(items_by_id[i.val()].first),
		 &(tags_by_id[items_by_id[i.val()].first->id.val()]),
		 (meta_it != metadata.end() && meta_it->ref == items_by_id[i.val()].first->id) ?
		     &*meta_it : 0, &(meta_printer.users()));
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
void Print_Statement::tags_by_id_attic
  (const map< Index, vector< Object > >& current_items,
   const map< Index, vector< Attic< Object > > >& attic_items,
   uint32 FLUSH_SIZE, Print_Target& target,
   Resource_Manager& rman, Transaction& transaction,
   const File_Properties* current_meta_file_prop, const File_Properties* attic_meta_file_prop,
   uint32& element_count)
{
  // order relevant elements by id
  std::vector< Maybe_Attic_Ref< Index, Object > > items_by_id;
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it(current_items.begin()); it != current_items.end(); ++it)
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
  
  //generate set of relevant coarse indices
  std::set< Index > current_coarse_indices;
  std::map< uint32, std::vector< typename Object::Id_Type > > current_ids_by_coarse;
  generate_ids_by_coarse(current_coarse_indices, current_ids_by_coarse, current_items);
  
  std::set< Index > attic_coarse_indices;
  std::map< uint32, std::vector< Attic< typename Object::Id_Type > > > attic_ids_by_coarse;
  generate_ids_by_coarse(attic_coarse_indices, attic_ids_by_coarse, attic_items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > current_range_set;
  formulate_range_query(current_range_set, current_coarse_indices);
  
  set< pair< Tag_Index_Local, Tag_Index_Local > > attic_range_set;
  formulate_range_query(attic_range_set, attic_coarse_indices);
  
  for (typename set< Index >::const_iterator
      it(current_coarse_indices.begin()); it != current_coarse_indices.end(); ++it)
    sort(current_ids_by_coarse[it->val()].begin(), current_ids_by_coarse[it->val()].end());
  
  for (typename set< Index >::const_iterator
      it(attic_coarse_indices.begin()); it != attic_coarse_indices.end(); ++it)
    sort(attic_ids_by_coarse[it->val()].begin(), attic_ids_by_coarse[it->val()].end());
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > only_current_meta_printer
      (current_items, rman, current_meta_file_prop);
  
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (attic_items, rman, current_meta_file_prop);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (attic_items, rman, attic_meta_file_prop);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename Object::Id_Type > current_tags_db
      (transaction.data_index(current_local_tags_file_properties< Object >()));
  Block_Backend< Tag_Index_Local, Attic< typename Object::Id_Type > > attic_tags_db
      (transaction.data_index(attic_local_tags_file_properties< Object >()));
      
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
    
    map< typename Object::Id_Type, vector< pair< string, string > > > current_tags_by_id;
    typename Block_Backend< Tag_Index_Local, typename Object::Id_Type >::Range_Iterator
        only_current_tag_it(current_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(current_range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(current_range_set.end())));
    for (typename set< Index >::const_iterator
        it(current_coarse_indices.begin()); it != current_coarse_indices.end(); ++it)
      collect_tags_framed< typename Object::Id_Type >(current_tags_by_id, current_tags_db, only_current_tag_it,
	  current_ids_by_coarse, it->val(), lower_id_bound, upper_id_bound);
      
    map< Attic< typename Object::Id_Type >, vector< pair< string, string > > > attic_tags_by_id;
    typename Block_Backend< Tag_Index_Local, typename Object::Id_Type >::Range_Iterator
        current_tag_it(current_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(attic_range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(attic_range_set.end())));
    typename Block_Backend< Tag_Index_Local, Attic< typename Object::Id_Type > >::Range_Iterator
        attic_tag_it(attic_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(attic_range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(attic_range_set.end())));
    for (typename set< Index >::const_iterator
        it(attic_coarse_indices.begin()); it != attic_coarse_indices.end(); ++it)
      collect_tags(attic_tags_by_id, current_tags_db, current_tag_it, attic_tags_db, attic_tag_it,
                 attic_ids_by_coarse, it->val(), lower_id_bound, upper_id_bound);
    
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
		 &(current_tags_by_id[items_by_id[i.val()].obj->id.val()]),
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
                 &(attic_tags_by_id[Attic< typename Object::Id_Type >
                     (items_by_id[i.val()].obj->id, items_by_id[i.val()].timestamp)]),
                 meta_it != attic_metadata.end() ? &*meta_it : 0, &current_meta_printer.users());
      }
    }
  }
}


Way_Bbox_Geometry_Store::Way_Bbox_Geometry_Store(
    const map< Uint31_Index, vector< Way_Skeleton > >& ways,
    const Statement& query, Resource_Manager& rman,
    double south_, double north_, double west_, double east_)
  : Way_Geometry_Store(ways, query, rman),
    south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{}


Way_Bbox_Geometry_Store::Way_Bbox_Geometry_Store(
    const map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
    const Statement& query, Resource_Manager& rman,
    double south_, double north_, double west_, double east_)
  : Way_Geometry_Store(ways, timestamp, query, rman),
    south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{}


bool Way_Bbox_Geometry_Store::matches_bbox(uint32 ll_upper, uint32 ll_lower) const
{
  if (north < south)
    return true;
  uint32 lat(::ilat(ll_upper, ll_lower));
  int32 lon(::ilon(ll_upper, ll_lower));
  return (lat >= south && lat <= north &&
     ((lon >= west && lon <= east)
            || (east < west && (lon >= west || lon <= east))));
}


std::vector< Quad_Coord > Way_Bbox_Geometry_Store::get_geometry(const Way_Skeleton& way) const
{
  std::vector< Quad_Coord > result = Way_Geometry_Store::get_geometry(way);
  
  if (result.empty())
    ;
  else if (result.size() == 1)
  {
    if (!matches_bbox(result.begin()->ll_upper, result.begin()->ll_lower))
      *result.begin() = Quad_Coord(0u, 0u);
  }
  else
  {
    bool this_matches = matches_bbox(result[0].ll_upper, result[0].ll_lower);
    bool next_matches = matches_bbox(result[1].ll_upper, result[1].ll_lower);
    if (!this_matches && !next_matches)
      result[0] = Quad_Coord(0u, 0u);
    for (uint i = 1; i < result.size() - 1; ++i)
    {
      bool last_matches = this_matches;
      this_matches = next_matches;
      next_matches = matches_bbox(result[i+1].ll_upper, result[i+1].ll_lower);
      if (!last_matches && !this_matches && !next_matches)
        result[i] = Quad_Coord(0u, 0u);
    }
    if (!this_matches && !next_matches)
      result[result.size()-1] = Quad_Coord(0u, 0u);
  }
        
  return result;
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


class Collection_Print_Target : public Print_Target
{
  public:
    Collection_Print_Target(uint32 mode_, Transaction& transaction)
        : Print_Target(mode_, transaction), final_target(0) {}
    virtual ~Collection_Print_Target() {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< Quad_Coord >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds = 0,
                            const std::vector< std::vector< Quad_Coord > >* geometry = 0,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0,
			    Show_New_Elem show_new_elem = visible_void);
                            
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
                            const vector< pair< string, string > >* tags = 0,
                            const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta = 0,
                            const map< uint32, string >* users = 0, const Action& action = KEEP);
    
    virtual void print_item_count(const Output_Item_Count& item_count);

    void set_target(Print_Target* target, bool order_by_id);
    
    void clear_nodes
        (Resource_Manager& rman, const map< uint32, string >* users = 0, bool add_deletion_information = false);
    void clear_ways
        (Resource_Manager& rman, const map< uint32, string >* users = 0, bool add_deletion_information = false);
    void clear_relations
        (Resource_Manager& rman, const map< uint32, string >* users = 0, bool add_deletion_information = false);
    
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
  
    Print_Target* final_target;
    std::vector< Node_Entry > nodes;
    std::vector< Way_Entry > ways;
    std::vector< Relation_Entry > relations;
    std::vector< Node_Entry > new_nodes;
    std::vector< Way_Entry > new_ways;
    std::vector< Relation_Entry > new_relations;
    bool order_by_id;
};

    
void Collection_Print_Target::set_target(Print_Target* target, bool order_by_id_)
{ 
  final_target = target;
  std::sort(nodes.begin(), nodes.end());
  std::sort(ways.begin(), ways.end());
  std::sort(relations.begin(), relations.end());
  order_by_id = order_by_id_;
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Node_Skeleton& skel,
                            const vector< pair< string, string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
                            const map< uint32, string >* users, const Action& action,
			    const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
{
  if (final_target)
  {
    if (order_by_id)
      new_nodes.push_back(Node_Entry(ll_upper, skel,
          meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
          tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
    else
    {
      std::vector< Node_Entry >::iterator nodes_it
          = std::lower_bound(nodes.begin(), nodes.end(), Node_Entry(ll_upper, skel));

      if (nodes_it == nodes.end() || skel.id < nodes_it->elem.id)
      {
        // No old element exists
        final_target->print_item(ll_upper, skel,
                                 (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                 (mode & Print_Target::PRINT_META) ? meta : 0, users, CREATE);
      }
      else
      {
        if (!(nodes_it->idx.val() == ll_upper) || !(nodes_it->elem.ll_lower == skel.ll_lower) ||
            (tags && !(nodes_it->tags == *tags)) || (meta && !(nodes_it->meta.timestamp == meta->timestamp)))
        {
          // The elements differ
          final_target->print_item(nodes_it->idx.val(), nodes_it->elem,
                                   (mode & Print_Target::PRINT_TAGS) ? &nodes_it->tags : 0,
                                   (mode & Print_Target::PRINT_META) ? &nodes_it->meta : 0, users, MODIFY_OLD);
          final_target->print_item(ll_upper, skel,
                                   (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                   (mode & Print_Target::PRINT_META) ? meta : 0, users, MODIFY_NEW);
        }
        nodes_it->idx = 0xffu;
      }
    }
  }
  else
    nodes.push_back(Node_Entry(ll_upper, skel,
        meta ? *meta : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
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


void Collection_Print_Target::clear_nodes
    (Resource_Manager& rman, const map< uint32, string >* users, bool add_deletion_information)
{
  if (order_by_id)
  {
    std::sort(new_nodes.begin(), new_nodes.end());
    std::vector< Node_Entry >::const_iterator old_it = nodes.begin();
    std::vector< Node_Entry >::const_iterator new_it = new_nodes.begin();
    
    std::vector< Node_Skeleton::Id_Type > found_ids;
    std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > > found_meta;
    if (add_deletion_information)
    {
      std::vector< Node_Skeleton::Id_Type > searched_ids;
      while (old_it != nodes.end() || new_it != new_nodes.end())
      {
        if (new_it == new_nodes.end() || (old_it != nodes.end() && old_it->elem.id < new_it->elem.id))
        {
	  searched_ids.push_back(old_it->elem.id);
	  ++old_it;
        }
        else if (old_it != nodes.end() && old_it->elem.id == new_it->elem.id)
        {
	  ++old_it;
	  ++new_it;
        }
        else
	  ++new_it;
      }
    
      std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(searched_ids, rman, true);
      find_still_existing_skeletons< Uint32_Index, Node_Skeleton >(rman, req, searched_ids).swap(found_ids);
      find_meta_elements< Uint32_Index, Node_Skeleton >(rman, req, searched_ids).swap(found_meta);
    }
	
    old_it = nodes.begin();
    new_it = new_nodes.begin();
    
    while (old_it != nodes.end() || new_it != new_nodes.end())
    {
      if (new_it == new_nodes.end() || (old_it != nodes.end() && old_it->elem.id < new_it->elem.id))
      {
	std::map< Node_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
	    meta_it = found_meta.find(old_it->elem.id);
        // No corresponding new element exists, thus the old one has been deleted.
	if (add_deletion_information)
          final_target->print_item(old_it->idx.val(), old_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                 (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, DELETE,
				 ((mode & Print_Target::PRINT_META) && meta_it != found_meta.end() ?
				     &meta_it->second : 0),
				 std::binary_search(found_ids.begin(), found_ids.end(), old_it->elem.id) ?
				     visible_true : visible_false);
	else
          final_target->print_item(old_it->idx.val(), old_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                 (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, DELETE);
	++old_it;
      }
      else if (old_it != nodes.end() && old_it->elem.id == new_it->elem.id)
      {
        if (!(old_it->idx == new_it->idx) || !(old_it->elem.ll_lower == new_it->elem.ll_lower) ||
            !(old_it->tags == new_it->tags) || !(old_it->meta.timestamp == new_it->meta.timestamp))
        {
          // The elements differ
          final_target->print_item(old_it->idx.val(), old_it->elem,
                                   (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                   (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, MODIFY_OLD);
          final_target->print_item(new_it->idx.val(), new_it->elem,
                                   (mode & Print_Target::PRINT_TAGS) ? &new_it->tags : 0,
                                   (mode & Print_Target::PRINT_META) ? &new_it->meta : 0, users, MODIFY_NEW);
        }
	++old_it;
	++new_it;
      }
      else
      {
        // No old element exists
        final_target->print_item(new_it->idx.val(), new_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &new_it->tags : 0,
                                 (mode & Print_Target::PRINT_META) ? &new_it->meta : 0, users, CREATE);
	++new_it;
      }
    }
  }
  else if (add_deletion_information)
  {
    std::vector< Node_Skeleton::Id_Type > searched_ids;
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
      searched_ids.push_back(it->elem.id);
    std::sort(searched_ids.begin(), searched_ids.end());
    searched_ids.erase(std::unique(searched_ids.begin(), searched_ids.end()), searched_ids.end());
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
        // No corresponding new element exists, thus the old one has been deleted.
        final_target->print_item(it->idx.val(), it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &it->tags : 0,
                                 (mode & Print_Target::PRINT_META) ? &it->meta : 0, users, DELETE,
				 ((mode & Print_Target::PRINT_META) && meta_it != found_meta.end() ?
				     &meta_it->second : 0),
				 std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ?
				     visible_true : visible_false);
      }
    }
  }
  else
  {
    for (std::vector< Node_Entry >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
        // No corresponding new element exists, thus the old one has been deleted.
        final_target->print_item(it->idx.val(), it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &it->tags : 0,
                                 (mode & Print_Target::PRINT_META) ? &it->meta : 0, users, DELETE);
      }
    }
  }
}
 
                            
void Collection_Print_Target::print_item(uint32 ll_upper, const Way_Skeleton& skel,
                            const vector< pair< string, string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< Quad_Coord >* geometry,
                            const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
                            const map< uint32, string >* users, const Action& action,
			    const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
{
  if (final_target)
  {
    if (order_by_id)
      new_ways.push_back(Way_Entry(ll_upper, skel,
          geometry ? *geometry : std::vector< Quad_Coord >(),
          meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
          tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
    else
    {
      std::vector< Way_Entry >::iterator ways_it
          = std::lower_bound(ways.begin(), ways.end(), Way_Entry(ll_upper, skel, std::vector< Quad_Coord >()));
    
      if (ways_it == ways.end() || skel.id < ways_it->elem.id)
      {
        // No old element exists
        if (geometry)
        {
          Double_Coords double_coords(*geometry);
          final_target->print_item(ll_upper, skel,
                                 (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? meta : 0, users, CREATE);
        }
        else
          final_target->print_item(ll_upper, skel,
                                 (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                 0, 0,
                                 (mode & Print_Target::PRINT_META) ? meta : 0, users, CREATE);
      }
      else
      {
        if (!(ways_it->idx.val() == ll_upper) || !(ways_it->elem.nds == skel.nds) ||
            (geometry && !(ways_it->geometry == *geometry)) ||
            (tags && !(ways_it->tags == *tags)) || (meta && !(ways_it->meta.timestamp == meta->timestamp)))
        {
          // The elements differ
          Double_Coords double_coords(ways_it->geometry);
          final_target->print_item(ways_it->idx.val(), ways_it->elem,
                               (mode & Print_Target::PRINT_TAGS) ? &ways_it->tags : 0,
                               bound_variant(double_coords, mode),
                               (mode & Print_Target::PRINT_GEOMETRY) ? &ways_it->geometry : 0,
                               (mode & Print_Target::PRINT_META) ? &ways_it->meta : 0, users, MODIFY_OLD);
	  if (geometry)
	  {
            Double_Coords double_coords_new(*geometry);
            final_target->print_item(ll_upper, skel,
                                   (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                   bound_variant(double_coords_new, mode),
                                   (mode & Print_Target::PRINT_GEOMETRY) ? geometry : 0,
                                   (mode & Print_Target::PRINT_META) ? meta : 0, users, MODIFY_NEW);
	  }
	  else
            final_target->print_item(ll_upper, skel,
                                   (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                   0, 0,
                                   (mode & Print_Target::PRINT_META) ? meta : 0, users, MODIFY_NEW);
        }
        ways_it->idx = 0xffu;
      }
    }
  }
  else
    ways.push_back(Way_Entry(ll_upper, skel,
        geometry ? *geometry : std::vector< Quad_Coord >(),
        meta ? *meta : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Collection_Print_Target::clear_ways
    (Resource_Manager& rman, const map< uint32, string >* users, bool add_deletion_information)
{
  if (order_by_id)
  {
    std::sort(new_ways.begin(), new_ways.end());
    std::vector< Way_Entry >::const_iterator old_it = ways.begin();
    std::vector< Way_Entry >::const_iterator new_it = new_ways.begin();
    
    std::vector< Way_Skeleton::Id_Type > found_ids;
    std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > > found_meta;
    if (add_deletion_information)
    {
      std::vector< Way_Skeleton::Id_Type > searched_ids;
      while (old_it != ways.end() || new_it != new_ways.end())
      {
        if (new_it == new_ways.end() || (old_it != ways.end() && old_it->elem.id < new_it->elem.id))
        {
	  searched_ids.push_back(old_it->elem.id);
	  ++old_it;
        }
        else if (old_it != ways.end() && old_it->elem.id == new_it->elem.id)
        {
	  ++old_it;
	  ++new_it;
        }
        else
	  ++new_it;
      }
    
      std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(searched_ids, rman, true);
      find_still_existing_skeletons< Uint31_Index, Way_Skeleton >(rman, req, searched_ids).swap(found_ids);
      find_meta_elements< Uint31_Index, Way_Skeleton >(rman, req, searched_ids).swap(found_meta);
    }
	
    old_it = ways.begin();
    new_it = new_ways.begin();
    
    while (old_it != ways.end() || new_it != new_ways.end())
    {
      if (new_it == new_ways.end() || (old_it != ways.end() && old_it->elem.id < new_it->elem.id))
      {
	std::map< Way_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::const_iterator
	    meta_it = found_meta.find(old_it->elem.id);
        Double_Coords double_coords(old_it->geometry);
        // No corresponding new element exists, thus the old one has been deleted.
	if (add_deletion_information)
          final_target->print_item(old_it->idx.val(), old_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &old_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, DELETE,
				 ((mode & Print_Target::PRINT_META) && meta_it != found_meta.end() ?
				     &meta_it->second : 0),
				 std::binary_search(found_ids.begin(), found_ids.end(), old_it->elem.id) ?
				     visible_true : visible_false);
	else
          final_target->print_item(old_it->idx.val(), old_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &old_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, DELETE);
	++old_it;
      }
      else if (old_it != ways.end() && old_it->elem.id == new_it->elem.id)
      {
        if (!(old_it->idx == new_it->idx) || !(old_it->elem.nds == new_it->elem.nds) ||
            !(old_it->geometry == new_it->geometry) ||
            !(old_it->tags == new_it->tags) || !(old_it->meta.timestamp == new_it->meta.timestamp))
        {
          // The elements differ
          Double_Coords double_coords(old_it->geometry);
          final_target->print_item(old_it->idx.val(), old_it->elem,
                               (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                               bound_variant(double_coords, mode),
                               (mode & Print_Target::PRINT_GEOMETRY) ? &old_it->geometry : 0,
                               (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, MODIFY_OLD);
	  
          Double_Coords double_coords_new(new_it->geometry);
          final_target->print_item(new_it->idx.val(), new_it->elem,
                               (mode & Print_Target::PRINT_TAGS) ? &new_it->tags : 0,
                               bound_variant(double_coords_new, mode),
                               (mode & Print_Target::PRINT_GEOMETRY) ? &new_it->geometry : 0,
                               (mode & Print_Target::PRINT_META) ? &new_it->meta : 0, users, MODIFY_NEW);
        }
	++old_it;
	++new_it;
      }
      else
      {
        // No old element exists
        Double_Coords double_coords(new_it->geometry);
        final_target->print_item(new_it->idx.val(), new_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &new_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &new_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &new_it->meta : 0, users, CREATE);
	++new_it;
      }
    }
  }
  else if (add_deletion_information)
  {
    std::vector< Way_Skeleton::Id_Type > searched_ids;
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
      searched_ids.push_back(it->elem.id);
    std::sort(searched_ids.begin(), searched_ids.end());
    searched_ids.erase(std::unique(searched_ids.begin(), searched_ids.end()), searched_ids.end());
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
        // No corresponding new element exists, thus the old one has been deleted.
        Double_Coords double_coords(it->geometry);
        final_target->print_item(it->idx.val(), it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &it->meta : 0, users, DELETE,
				 ((mode & Print_Target::PRINT_META) && meta_it != found_meta.end() ?
				     &meta_it->second : 0),
				 std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ?
				     visible_true : visible_false);
      }
    }
  }
  else
  {
    for (std::vector< Way_Entry >::const_iterator it = ways.begin(); it != ways.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
        // No corresponding new element exists, thus the old one has been deleted.
        Double_Coords double_coords(it->geometry);
        final_target->print_item(it->idx.val(), it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &it->meta : 0, users, DELETE);
      }
    }
  }
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Relation_Skeleton& skel,
                            const vector< pair< string, string > >* tags,
                            const std::pair< Quad_Coord, Quad_Coord* >* bounds,
                            const std::vector< std::vector< Quad_Coord > >* geometry,
                            const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
                            const map< uint32, string >* users, const Action& action,
			    const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta,
			    Show_New_Elem show_new_elem)
{
  if (final_target)
  {
    if (order_by_id)
      new_relations.push_back(Relation_Entry(ll_upper, skel,
	  geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
	  meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
	  tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
    else
    {
      std::vector< Relation_Entry >::iterator relations_it
	  = std::lower_bound(relations.begin(), relations.end(),
	      Relation_Entry(ll_upper, skel, std::vector< std::vector< Quad_Coord > >()));
    
      if (relations_it == relations.end() || skel.id < relations_it->elem.id)
      {
	// No old element exists
	if (geometry)
	{
	  Double_Coords double_coords(*geometry);
	  final_target->print_item(ll_upper, skel,
                                 (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? meta : 0, users, CREATE);
	}
	else
	  final_target->print_item(ll_upper, skel,
                                 (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                 0, 0,
                                 (mode & Print_Target::PRINT_META) ? meta : 0, users, CREATE);
      }
      else
      {
	if (!(relations_it->idx.val() == ll_upper) || !(relations_it->elem.members == skel.members) ||
	    (geometry && !(relations_it->geometry == *geometry)) ||
	    (tags && !(relations_it->tags == *tags)) || (meta && !(relations_it->meta.timestamp == meta->timestamp)))
	{
	  // The elements differ
	  Double_Coords double_coords(relations_it->geometry);
	  final_target->print_item(relations_it->idx.val(), relations_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &relations_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &relations_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &relations_it->meta : 0, users, MODIFY_OLD);
	  if (geometry)
	  {
	    Double_Coords double_coords_new(*geometry);
	    final_target->print_item(ll_upper, skel,
                                   (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                   bound_variant(double_coords_new, mode),
                                   (mode & Print_Target::PRINT_GEOMETRY) ? geometry : 0,
                                   (mode & Print_Target::PRINT_META) ? meta : 0, users, MODIFY_NEW);
	  }
	  else
	    final_target->print_item(ll_upper, skel,
                                   (mode & Print_Target::PRINT_TAGS) ? tags : 0,
                                   0, 0,
                                   (mode & Print_Target::PRINT_META) ? meta : 0, users, MODIFY_NEW);
	}
	relations_it->idx = 0xffu;
      }
    }
  }
  else
    relations.push_back(Relation_Entry(ll_upper, skel,
        geometry ? *geometry : std::vector< std::vector< Quad_Coord > >(),
        meta ? *meta : OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >(),
        tags ? *tags : std::vector< std::pair< std::string, std::string > >()));
}


void Collection_Print_Target::clear_relations
    (Resource_Manager& rman, const map< uint32, string >* users, bool add_deletion_information)
{
  if (order_by_id)
  {
    std::sort(new_relations.begin(), new_relations.end());
    std::vector< Relation_Entry >::const_iterator old_it = relations.begin();
    std::vector< Relation_Entry >::const_iterator new_it = new_relations.begin();
    
    std::vector< Relation_Skeleton::Id_Type > found_ids;
    std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > > found_meta;
    if (add_deletion_information)
    {
      std::vector< Relation_Skeleton::Id_Type > searched_ids;
      while (old_it != relations.end() || new_it != new_relations.end())
      {
        if (new_it == new_relations.end() || (old_it != relations.end() && old_it->elem.id < new_it->elem.id))
        {
	  searched_ids.push_back(old_it->elem.id);
	  ++old_it;
        }
        else if (old_it != relations.end() && old_it->elem.id == new_it->elem.id)
        {
	  ++old_it;
	  ++new_it;
        }
        else
	  ++new_it;
      }
    
      std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Relation_Skeleton >(searched_ids, rman, true);
      find_still_existing_skeletons< Uint31_Index, Relation_Skeleton >(rman, req, searched_ids).swap(found_ids);
      find_meta_elements< Uint31_Index, Relation_Skeleton >(rman, req, searched_ids).swap(found_meta);
    }
	
    old_it = relations.begin();
    new_it = new_relations.begin();
    
    while (old_it != relations.end() || new_it != new_relations.end())
    {
      if (new_it == new_relations.end() || (old_it != relations.end() && old_it->elem.id < new_it->elem.id))
      {
	std::map< Relation_Skeleton::Id_Type, OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
	    meta_it = found_meta.find(old_it->elem.id);
        // No corresponding new element exists, thus the old one has been deleted.
        Double_Coords double_coords(old_it->geometry);
	if (add_deletion_information)
	  final_target->print_item(old_it->idx.val(), old_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &old_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, DELETE,
				 ((mode & Print_Target::PRINT_META) && meta_it != found_meta.end() ?
				     &meta_it->second : 0),
				 std::binary_search(found_ids.begin(), found_ids.end(), old_it->elem.id) ?
				     visible_true : visible_false);
	else
	  final_target->print_item(old_it->idx.val(), old_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &old_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, DELETE);
	++old_it;
      }
      else if (old_it != relations.end() && old_it->elem.id == new_it->elem.id)
      {
        if (!(old_it->idx == new_it->idx) || !(old_it->elem.members == new_it->elem.members) ||
            !(old_it->geometry == new_it->geometry) ||
            !(old_it->tags == new_it->tags) || !(old_it->meta.timestamp == new_it->meta.timestamp))
        {
          // The elements differ
          Double_Coords double_coords(old_it->geometry);
          final_target->print_item(old_it->idx.val(), old_it->elem,
                               (mode & Print_Target::PRINT_TAGS) ? &old_it->tags : 0,
                               bound_variant(double_coords, mode),
                               (mode & Print_Target::PRINT_GEOMETRY) ? &old_it->geometry : 0,
                               (mode & Print_Target::PRINT_META) ? &old_it->meta : 0, users, MODIFY_OLD);
	  
          Double_Coords double_coords_new(new_it->geometry);
          final_target->print_item(new_it->idx.val(), new_it->elem,
                               (mode & Print_Target::PRINT_TAGS) ? &new_it->tags : 0,
                               bound_variant(double_coords_new, mode),
                               (mode & Print_Target::PRINT_GEOMETRY) ? &new_it->geometry : 0,
                               (mode & Print_Target::PRINT_META) ? &new_it->meta : 0, users, MODIFY_NEW);
        }
	++old_it;
	++new_it;
      }
      else
      {
        // No old element exists
        Double_Coords double_coords(new_it->geometry);
        final_target->print_item(new_it->idx.val(), new_it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &new_it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &new_it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &new_it->meta : 0, users, CREATE);
	++new_it;
      }
    }
  }
  else if (add_deletion_information)
  {
    std::vector< Relation_Skeleton::Id_Type > searched_ids;
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
      searched_ids.push_back(it->elem.id);
    std::sort(searched_ids.begin(), searched_ids.end());
    searched_ids.erase(std::unique(searched_ids.begin(), searched_ids.end()), searched_ids.end());
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
        // No corresponding new element exists, thus the old one has been deleted.
        Double_Coords double_coords(it->geometry);
        final_target->print_item(it->idx.val(), it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &it->meta : 0, users, DELETE,
				 ((mode & Print_Target::PRINT_META) && meta_it != found_meta.end() ?
				     &meta_it->second : 0),
				 std::binary_search(found_ids.begin(), found_ids.end(), it->elem.id) ?
				     visible_true : visible_false);
      }
    }
  }
  else
  {
    for (std::vector< Relation_Entry >::const_iterator it = relations.begin(); it != relations.end(); ++it)
    {
      if (it->idx.val() != 0xffu)
      {
        // No corresponding new element exists, thus the old one has been deleted.
        Double_Coords double_coords(it->geometry);
        final_target->print_item(it->idx.val(), it->elem,
                                 (mode & Print_Target::PRINT_TAGS) ? &it->tags : 0,
                                 bound_variant(double_coords, mode),
                                 (mode & Print_Target::PRINT_GEOMETRY) ? &it->geometry : 0,
                                 (mode & Print_Target::PRINT_META) ? &it->meta : 0, users, DELETE);
      }
    }
  }
}


void Collection_Print_Target::print_item(uint32 ll_upper, const Area_Skeleton& skel,
                            const vector< pair< string, string > >* tags,
                            const OSM_Element_Metadata_Skeleton< Area::Id_Type >* meta,
                            const map< uint32, string >* users, const Action& action) {}
                            

void Collection_Print_Target::print_item_count(const Output_Item_Count & item_count) {}


void Print_Statement::execute(Resource_Manager& rman)
{
  if (rman.area_updater())
    rman.area_updater()->flush();
  
  map< string, Set >::const_iterator mit(rman.sets().find(input));
  uint32 element_count = 0;
  if (mit == rman.sets().end())
    return;
    
  Output_Handle output_handle("xml");
  Print_Target* target = 0;
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
  else if (this->output_handle)
    target = &this->output_handle->get_print_target(mode, *rman.get_transaction());
  else
    target = &output_handle.get_print_target(mode, *rman.get_transaction());
  
  if (collection_mode == collect_rhs)
  {
    collection_print_target->set_target(target, order == order_by_id);
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
  
    if (order == order_by_id)
    {
      if (rman.get_desired_timestamp() == NOW)
        tags_by_id(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		  NODE_FLUSH_SIZE, *target, rman,
		  *rman.get_transaction(),
		  (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
		  element_count);
      else
        tags_by_id_attic(mit->second.nodes, mit->second.attic_nodes,
                   NODE_FLUSH_SIZE, *target, rman,
                   *rman.get_transaction(),
                   (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                   (mode & Print_Target::PRINT_META) ? attic_settings().NODES_META : 0,
                   element_count);
      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_nodes(rman, &user_data_cache->users(), add_deletion_information);
      
      if (rman.get_desired_timestamp() == NOW)
        tags_by_id(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		  WAY_FLUSH_SIZE, *target, rman,
		  *rman.get_transaction(),
		  (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
		  element_count);
      else
        tags_by_id_attic(mit->second.ways, mit->second.attic_ways,
                   WAY_FLUSH_SIZE, *target, rman,
                   *rman.get_transaction(),
                   (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                   (mode & Print_Target::PRINT_META) ? attic_settings().WAYS_META : 0,
                   element_count);
      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_ways(rman, &user_data_cache->users(), add_deletion_information);
      
      if (rman.get_desired_timestamp() == NOW)
        tags_by_id(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		  RELATION_FLUSH_SIZE, *target, rman,
		  *rman.get_transaction(),
		  (mode & Print_Target::PRINT_META) ? meta_settings().RELATIONS_META : 0,
		  element_count);
      else
        tags_by_id_attic(mit->second.relations, mit->second.attic_relations,
                   RELATION_FLUSH_SIZE, *target, rman,
                   *rman.get_transaction(),
                   (mode & Print_Target::PRINT_META) ? meta_settings().RELATIONS_META : 0,
                   (mode & Print_Target::PRINT_META) ? attic_settings().RELATIONS_META : 0,
                   element_count);
      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_relations(rman, &user_data_cache->users(), add_deletion_information);
      
      if (rman.get_area_transaction())
	tags_by_id(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		   AREA_FLUSH_SIZE, *target, rman,
		   *rman.get_area_transaction(), 0, element_count);
    }
    else
    {
      tags_quadtile(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
		    (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                    element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        tags_quadtile_attic(mit->second.attic_nodes,
                      *target, rman, *rman.get_transaction(),
                      (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                      (mode & Print_Target::PRINT_META) ? attic_settings().NODES_META : 0,
                      element_count);
      }
      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_nodes(rman, &user_data_cache->users(), add_deletion_information);
      
      tags_quadtile(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
		    (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                    element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        tags_quadtile_attic(mit->second.attic_ways,
                      *target, rman, *rman.get_transaction(),
                      (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                      (mode & Print_Target::PRINT_META) ? attic_settings().WAYS_META : 0,
                      element_count);
      }
      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_ways(rman, &user_data_cache->users(), add_deletion_information);
      
      tags_quadtile(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
		    (mode & Print_Target::PRINT_META) ?
		        meta_settings().RELATIONS_META : 0, element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        tags_quadtile_attic(mit->second.attic_relations,
                      *target, rman, *rman.get_transaction(),
                      (mode & Print_Target::PRINT_META) ? meta_settings().RELATIONS_META : 0,
                      (mode & Print_Target::PRINT_META) ? attic_settings().RELATIONS_META : 0,
                      element_count);
      }
      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_relations(rman, &user_data_cache->users(), add_deletion_information);
      
      if (rman.get_area_transaction())
      {
        tags_quadtile(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		      *target, rman, *rman.get_area_transaction(), 0, element_count);
      }
    }
    
    delete user_data_cache;
  }
  else if (mode & Print_Target::PRINT_COUNT)
  {
    Output_Item_Count item_count;

    item_count.nodes = count_items(mit->second.nodes, *target, *rman.get_transaction(), *this);
    item_count.nodes += count_items(mit->second.attic_nodes, *target, *rman.get_transaction(), *this);
    item_count.ways = count_items(mit->second.ways, *target, *rman.get_transaction(), *this);
    item_count.ways += count_items(mit->second.attic_ways, *target, *rman.get_transaction(), *this);
    item_count.relations = count_items(mit->second.relations, *target, *rman.get_transaction(), *this);
    item_count.relations += count_items(mit->second.attic_relations, *target, *rman.get_transaction(), *this);
    item_count.areas = rman.get_area_transaction() ?
                            count_items(mit->second.areas, *target, *rman.get_area_transaction(), *this) : 0;
    item_count.total = item_count.nodes + item_count.ways +  item_count.relations + item_count.areas;
    print_item_count(*target, item_count);
  }
  else
  {
    if (order == order_by_id)
    {
      by_id(mit->second.nodes, mit->second.attic_nodes,
	    *target, *rman.get_transaction(), *this, limit, element_count);
      if (collection_mode == collect_rhs)
        collection_print_target->clear_nodes(rman, 0, add_deletion_information);
      
      by_id(mit->second.ways, mit->second.attic_ways,
	    *target, *rman.get_transaction(), *this, limit, element_count);
      if (collection_mode == collect_rhs)
        collection_print_target->clear_ways(rman, 0, add_deletion_information);
      
      by_id(mit->second.relations, mit->second.attic_relations,
	    *target, *rman.get_transaction(), *this, limit, element_count);      
      if (collection_mode == collect_rhs)
        collection_print_target->clear_relations(rman, 0, add_deletion_information);
      
      if (rman.get_area_transaction())
	by_id(mit->second.areas, *target, *rman.get_area_transaction(), *this, limit, element_count);
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
      
      if (rman.get_area_transaction())
	quadtile(mit->second.areas, *target, *rman.get_area_transaction(), *this, limit, element_count);
    }
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
