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

using namespace std;

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
      output_handle(0), way_geometry_store(0)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["geometry"] = "skeleton";
  attributes["limit"] = "";
  attributes["mode"] = "body";
  attributes["order"] = "id";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  
  if (attributes["mode"] == "ids_only")
    mode = Print_Target::PRINT_IDS;
  else if (attributes["mode"] == "skeleton")
    mode = Print_Target::PRINT_IDS
        |Print_Target::PRINT_COORDS|Print_Target::PRINT_NDS|Print_Target::PRINT_MEMBERS;
  else if (attributes["mode"] == "body")
    mode = Print_Target::PRINT_IDS
        |Print_Target::PRINT_COORDS|Print_Target::PRINT_NDS|Print_Target::PRINT_MEMBERS
	|Print_Target::PRINT_TAGS;
  else if (attributes["mode"] == "meta")
    mode = Print_Target::PRINT_IDS
        |Print_Target::PRINT_COORDS|Print_Target::PRINT_NDS|Print_Target::PRINT_MEMBERS
	|Print_Target::PRINT_TAGS|Print_Target::PRINT_VERSION|Print_Target::PRINT_META;
  else
  {
    mode = 0;
    ostringstream temp;
    temp<<"For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\", \"body\", or \"meta\".";
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
  else
  {
    ostringstream temp;
    temp<<"For the attribute \"geometry\" of the element \"print\""
        <<" the only allowed values are \"skeleton\", \"full\", or \"bounds\".";
    add_static_error(temp.str());
  }
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
     Transaction& transaction, uint32 limit, uint32& element_count)
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
      target.print_item(item_it->first.val(), *it2);
    }
    ++item_it;
  }
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Node_Skeleton& skel,
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
{
  target.print_item(ll_upper, skel, tags, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Way_Skeleton& skel,
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
{
  if (way_geometry_store)
  {
    std::vector< Quad_Coord > geometry = way_geometry_store->get_geometry(skel);
    if (geometry.size() == skel.nds.size())
      target.print_item(ll_upper, skel, tags, 0, &geometry, meta, users);
    else
      target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
  }
  else
    target.print_item(ll_upper, skel, tags, 0, 0, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Relation_Skeleton& skel,
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Relation_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
{
  target.print_item(ll_upper, skel, tags, 0, meta, users);
}


void Print_Statement::print_item(Print_Target& target, uint32 ll_upper, const Area_Skeleton& skel,
                    const vector< pair< string, string > >* tags,
                    const OSM_Element_Metadata_Skeleton< Area_Skeleton::Id_Type >* meta,
                    const map< uint32, string >* users)
{
  target.print_item(ll_upper, skel, tags, meta, users);
}


template< class Index, class Object >
void Print_Statement::tags_quadtile
    (const map< Index, vector< Object > >& items,
     const File_Properties& file_prop, Print_Target& target,
     Resource_Manager& rman, Transaction& transaction,
     Geometry_Store_Manager< typename Object::Id_Type, Index >& geometry_store,
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
  Meta_Collector< Index, typename Object::Id_Type > meta_printer(items, transaction, meta_file_prop);
  
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
     Geometry_Store_Manager< typename Object::Id_Type, Index >& geometry_store,
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
      (items, transaction, current_meta_file_prop);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (items, transaction, attic_meta_file_prop);
  
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
   Transaction& transaction, uint32 limit, uint32& element_count)
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
    target.print_item(items_by_id[i].second, *(items_by_id[i].first));
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
   Geometry_Store_Manager< typename TObject::Id_Type, TIndex >& geometry_store,
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
  Meta_Collector< TIndex, typename TObject::Id_Type > meta_printer(items, transaction, meta_file_prop);
  
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
  (const map< Index, vector< Attic< Object > > >& items,
   uint32 FLUSH_SIZE, Print_Target& target,
   Resource_Manager& rman, Transaction& transaction,
   Geometry_Store_Manager< typename Object::Id_Type, Index >& geometry_store,
   const File_Properties* current_meta_file_prop, const File_Properties* attic_meta_file_prop,
   uint32& element_count)
{
  // order relevant elements by id
  vector< pair< const Attic< Object >*, uint32 > > items_by_id;
  for (typename map< Index, vector< Attic< Object > > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    for (typename vector< Attic< Object > >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      items_by_id.push_back(make_pair(&(*it2), it->first.val()));
  }
  sort(items_by_id.begin(), items_by_id.end(),
       Skeleton_Comparator_By_Id< Attic< Object > >());
  
  //generate set of relevant coarse indices
  set< Index > coarse_indices;
  map< uint32, vector< Attic< typename Object::Id_Type > > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  for (typename set< Index >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< Index, typename Object::Id_Type > current_meta_printer
      (items, transaction, current_meta_file_prop);
  Meta_Collector< Index, typename Object::Id_Type > attic_meta_printer
      (items, transaction, attic_meta_file_prop);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename Object::Id_Type > current_tags_db
      (transaction.data_index(current_local_tags_file_properties< Object >()));
  Block_Backend< Tag_Index_Local, Attic< typename Object::Id_Type > > attic_tags_db
      (transaction.data_index(attic_local_tags_file_properties< Object >()));
      
  for (typename Object::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    // Disable health_check: This ensures that a result will be always printed completely
    //rman.health_check(*this);
    
    map< Attic< typename Object::Id_Type >, vector< pair< string, string > > > tags_by_id;
    typename Object::Id_Type lower_id_bound(items_by_id[id_pos.val()].first->id);
    typename Object::Id_Type upper_id_bound;
    if (id_pos + FLUSH_SIZE < items_by_id.size())
      upper_id_bound = items_by_id[(id_pos + FLUSH_SIZE).val()].first->id;
    else
    {
      upper_id_bound = items_by_id[items_by_id.size()-1].first->id;
      ++upper_id_bound;
    }
    
    typename Block_Backend< Tag_Index_Local, typename Object::Id_Type >::Range_Iterator
        current_tag_it(current_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    typename Block_Backend< Tag_Index_Local, Attic< typename Object::Id_Type > >::Range_Iterator
        attic_tag_it(attic_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    for (typename set< Index >::const_iterator
        it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
      collect_tags(tags_by_id, current_tags_db, current_tag_it, attic_tags_db, attic_tag_it,
                 ids_by_coarse, it->val(), lower_id_bound, upper_id_bound);
    
    // collect metadata if required
    set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > metadata;
    collect_metadata(metadata, items, lower_id_bound, upper_id_bound,
                     current_meta_printer, attic_meta_printer);
    attic_meta_printer.reset();
    current_meta_printer.reset();

    // print the result
    for (typename Object::Id_Type i(id_pos);
         (i < id_pos + FLUSH_SIZE) && (i < items_by_id.size()); ++i)
    {
      typename set< OSM_Element_Metadata_Skeleton< typename Object::Id_Type > >::const_iterator meta_it
          = find_matching_metadata(metadata,
                                   items_by_id[i.val()].first->id, items_by_id[i.val()].first->timestamp);
      if (++element_count > limit)
        return;
      print_item(target, items_by_id[i.val()].second, *(items_by_id[i.val()].first),
                 &(tags_by_id[Attic< typename Object::Id_Type >
                     (items_by_id[i.val()].first->id, items_by_id[i.val()].first->timestamp)]),
                 meta_it != metadata.end() ? &*meta_it : 0, &current_meta_printer.users());
    }
  }
}


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
  if (this->output_handle)
    target = &this->output_handle->get_print_target(mode, *rman.get_transaction());
  else
    target = &output_handle.get_print_target(mode, *rman.get_transaction());
  
  delete way_geometry_store;
  way_geometry_store = new Way_Geometry_Store(mit->second.ways, *this, rman);
  delete attic_way_geometry_store;
  attic_way_geometry_store = new Way_Geometry_Store(mit->second.attic_ways, rman.get_desired_timestamp(), *this, rman);

  if (mode & Print_Target::PRINT_TAGS)
  {
    if (order == order_by_id)
    {
      Geometry_Store_Manager< Node_Skeleton::Id_Type, Uint32_Index > geometry_store_manager;
      tags_by_id(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		 NODE_FLUSH_SIZE, *target, rman,
		 *rman.get_transaction(), geometry_store_manager,
		 (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                 element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        Geometry_Store_Manager< Node_Skeleton::Id_Type, Uint32_Index > geometry_store_manager;
        tags_by_id_attic(mit->second.attic_nodes,
                   NODE_FLUSH_SIZE, *target, rman,
                   *rman.get_transaction(), geometry_store_manager,
                   (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                   (mode & Print_Target::PRINT_META) ? attic_settings().NODES_META : 0,
                   element_count);
      }
      
      Geometry_Store_Manager< Way_Skeleton::Id_Type, Uint31_Index > way_geometry_store_manager;
      tags_by_id(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		 WAY_FLUSH_SIZE, *target, rman,
                 *rman.get_transaction(), way_geometry_store_manager,
		 (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                 element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        Geometry_Store_Manager< Way_Skeleton::Id_Type, Uint31_Index > way_geometry_store_manager;
        tags_by_id_attic(mit->second.attic_ways,
                   WAY_FLUSH_SIZE, *target, rman,
                   *rman.get_transaction(), way_geometry_store_manager,
                   (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                   (mode & Print_Target::PRINT_META) ? attic_settings().WAYS_META : 0,
                   element_count);
      }
      
      Geometry_Store_Manager< Relation_Skeleton::Id_Type, Uint31_Index > rel_geometry_store_manager;
      tags_by_id(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		 RELATION_FLUSH_SIZE, *target, rman,
                 *rman.get_transaction(), rel_geometry_store_manager,
		 (mode & Print_Target::PRINT_META) ? meta_settings().RELATIONS_META : 0,
                 element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        Geometry_Store_Manager< Relation_Skeleton::Id_Type, Uint31_Index > rel_geometry_store_manager;
        tags_by_id_attic(mit->second.attic_relations,
                   RELATION_FLUSH_SIZE, *target, rman,
                   *rman.get_transaction(),
                   rel_geometry_store_manager,
                   (mode & Print_Target::PRINT_META) ? meta_settings().RELATIONS_META : 0,
                   (mode & Print_Target::PRINT_META) ? attic_settings().RELATIONS_META : 0,
                   element_count);
      }
      
      if (rman.get_area_transaction())
      {
        Geometry_Store_Manager< Area_Skeleton::Id_Type, Uint31_Index > geometry_store_manager;
	tags_by_id(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		   AREA_FLUSH_SIZE, *target, rman,
		   *rman.get_area_transaction(),
                   geometry_store_manager,
                   0, element_count);
      }
    }
    else
    {
      Geometry_Store_Manager< Node_Skeleton::Id_Type, Uint32_Index > geometry_store_manager;
      tags_quadtile(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
                    geometry_store_manager,
		    (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                    element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        Geometry_Store_Manager< Node_Skeleton::Id_Type, Uint32_Index > geometry_store_manager;
        tags_quadtile_attic(mit->second.attic_nodes,
                      *target, rman, *rman.get_transaction(),
                      geometry_store_manager,
                      (mode & Print_Target::PRINT_META) ? meta_settings().NODES_META : 0,
                      (mode & Print_Target::PRINT_META) ? attic_settings().NODES_META : 0,
                      element_count);
      }
      
      Geometry_Store_Manager< Way_Skeleton::Id_Type, Uint31_Index > way_geometry_store_manager;
      tags_quadtile(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
                    way_geometry_store_manager,
		    (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                    element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        Geometry_Store_Manager< Way_Skeleton::Id_Type, Uint31_Index > way_geometry_store_manager;
        tags_quadtile_attic(mit->second.attic_ways,
                      *target, rman, *rman.get_transaction(),
                      way_geometry_store_manager,
                      (mode & Print_Target::PRINT_META) ? meta_settings().WAYS_META : 0,
                      (mode & Print_Target::PRINT_META) ? attic_settings().WAYS_META : 0,
                      element_count);
      }
      
      Geometry_Store_Manager< Relation_Skeleton::Id_Type, Uint31_Index > rel_geometry_store_manager;
      tags_quadtile(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
                    rel_geometry_store_manager,
		    (mode & Print_Target::PRINT_META) ?
		        meta_settings().RELATIONS_META : 0, element_count);
      
      if (rman.get_desired_timestamp() != NOW)
      {
        Geometry_Store_Manager< Relation_Skeleton::Id_Type, Uint31_Index > rel_geometry_store_manager;
        tags_quadtile_attic(mit->second.attic_relations,
                      *target, rman, *rman.get_transaction(),
                      rel_geometry_store_manager,
                      (mode & Print_Target::PRINT_META) ? meta_settings().RELATIONS_META : 0,
                      (mode & Print_Target::PRINT_META) ? attic_settings().RELATIONS_META : 0,
                      element_count);
      }
      
      if (rman.get_area_transaction())
      {
        Geometry_Store_Manager< Area_Skeleton::Id_Type, Uint31_Index > geometry_store_manager;
        tags_quadtile(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		      *target, rman, *rman.get_area_transaction(),
                      geometry_store_manager,
                      0, element_count);
      }
    }
  }
  else
  {
    if (order == order_by_id)
    {
      by_id(mit->second.nodes, *target, *rman.get_transaction(), limit, element_count);
      by_id(mit->second.ways, *target, *rman.get_transaction(), limit, element_count);
      by_id(mit->second.relations, *target, *rman.get_transaction(), limit, element_count);
      
      by_id(mit->second.attic_nodes, *target, *rman.get_transaction(), limit, element_count);
      by_id(mit->second.attic_ways, *target, *rman.get_transaction(), limit, element_count);
      
      if (rman.get_area_transaction())
	by_id(mit->second.areas, *target, *rman.get_area_transaction(), limit, element_count);
    }
    else
    {
      quadtile(mit->second.nodes, *target, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.ways, *target, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.relations, *target, *rman.get_transaction(), limit, element_count);
      
      quadtile(mit->second.attic_nodes, *target, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.attic_ways, *target, *rman.get_transaction(), limit, element_count);
      
      if (rman.get_area_transaction())
	quadtile(mit->second.areas, *target, *rman.get_area_transaction(), limit, element_count);
    }
  }
  
  rman.health_check(*this);
}


Print_Statement::~Print_Statement()
{
  delete way_geometry_store;
}
