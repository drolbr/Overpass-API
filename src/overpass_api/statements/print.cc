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
  Block_Backend< Uint32_Index, String_Object > roles_db
      (transaction.data_index(osm_base_settings().RELATION_ROLES));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
      it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
    roles[it.index().val()] = it.object().val();
}


Generic_Statement_Maker< Print_Statement > Print_Statement::statement_maker("print");


Print_Statement::Print_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_),
      mode(0), order(order_by_id), limit(numeric_limits< unsigned int >::max()),
      output_handle(0)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["mode"] = "body";
  attributes["order"] = "id";
  attributes["limit"] = "";
  
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
}


void Print_Statement::forecast() {}


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


template< class TIndex, class TObject >
void Print_Statement::tags_quadtile
    (const map< TIndex, vector< TObject > >& items,
     const File_Properties& file_prop, Print_Target& target,
     Resource_Manager& rman, Transaction& transaction,
     const File_Properties* meta_file_prop, uint32& element_count)
{
  //generate set of relevant coarse indices
  set< TIndex > coarse_indices;
  map< uint32, vector< typename TObject::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);
  
  // formulate meta query if meta data shall be printed
  Meta_Collector< TIndex, TObject > meta_printer(items, transaction, meta_file_prop);
  
  // iterate over the result
  uint coarse_count = 0;
  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  typename Block_Backend< Tag_Index_Local, typename TObject::Id_Type >::Range_Iterator
    tag_it(items_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  typename map< TIndex, vector< TObject > >::const_iterator
      item_it(items.begin());
  for (typename set< TIndex >::const_iterator
      it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    if (++coarse_count >= 1024)
    {
      coarse_count = 0;
      rman.health_check(*this);
    }
    
    sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
    map< typename TObject::Id_Type, vector< pair< string, string > > > tags_by_id;
    collect_tags(tags_by_id, items_db, tag_it, ids_by_coarse, it->val());
    
    // print the result
    while ((item_it != items.end()) &&
        ((item_it->first.val() & 0x7fffff00) == it->val()))
    {
      for (typename vector< TObject >::const_iterator it2(item_it->second.begin());
          it2 != item_it->second.end(); ++it2)
      {
	if (++element_count > limit)
	  return;
	target.print_item(item_it->first.val(), *it2, &(tags_by_id[it2->id.val()]),
		   meta_printer.get(item_it->first, it2->id), &(meta_printer.users()));
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
    return (a.first->id < b.first->id);
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
		      Meta_Collector< TIndex, TObject>& meta_printer)
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
  Meta_Collector< TIndex, TObject > meta_printer(items, transaction, meta_file_prop);
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  for (typename TObject::Id_Type id_pos; id_pos < items_by_id.size(); id_pos += FLUSH_SIZE)
  {
    rman.health_check(*this);
    
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
          = metadata.find(OSM_Element_Metadata_Skeleton< typename TObject::Id_Type >
              (items_by_id[i.val()].first->id));
      if (++element_count > limit)
	return;
      target.print_item(items_by_id[i.val()].second, *(items_by_id[i.val()].first),
		 &(tags_by_id[items_by_id[i.val()].first->id.val()]),
		 meta_it != metadata.end() ? &*meta_it : 0, &(meta_printer.users()));
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

  if (mode & Print_Target::PRINT_TAGS)
  {
    if (order == order_by_id)
    {
      tags_by_id(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		 NODE_FLUSH_SIZE, *target, rman,
		 *rman.get_transaction(),
		 (mode & Print_Target::PRINT_META) ?
		     meta_settings().NODES_META : 0, element_count);
      tags_by_id(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		 WAY_FLUSH_SIZE, *target, rman,
		 *rman.get_transaction(),
		 (mode & Print_Target::PRINT_META) ?
		     meta_settings().WAYS_META : 0, element_count);
      tags_by_id(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		 RELATION_FLUSH_SIZE, *target, rman,
		 *rman.get_transaction(),
		 (mode & Print_Target::PRINT_META) ?
		     meta_settings().RELATIONS_META : 0, element_count);
      if (rman.get_area_transaction())
      {
	tags_by_id(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		   AREA_FLUSH_SIZE, *target, rman,
		   *rman.get_area_transaction(), 0, element_count);
      }
    }
    else
    {
      tags_quadtile(mit->second.nodes, *osm_base_settings().NODE_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
		    (mode & Print_Target::PRINT_META) ?
		        meta_settings().NODES_META : 0, element_count);
      tags_quadtile(mit->second.ways, *osm_base_settings().WAY_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
		    (mode & Print_Target::PRINT_META) ?
		        meta_settings().WAYS_META : 0, element_count);
      tags_quadtile(mit->second.relations, *osm_base_settings().RELATION_TAGS_LOCAL,
		    *target, rman, *rman.get_transaction(),
		    (mode & Print_Target::PRINT_META) ?
		        meta_settings().RELATIONS_META : 0, element_count);
      if (rman.get_area_transaction())
      {
        tags_quadtile(mit->second.areas, *area_settings().AREA_TAGS_LOCAL,
		      *target, rman, *rman.get_area_transaction(), 0, element_count);
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
      if (rman.get_area_transaction())
	by_id(mit->second.areas, *target, *rman.get_area_transaction(), limit, element_count);
    }
    else
    {
      quadtile(mit->second.nodes, *target, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.ways, *target, *rman.get_transaction(), limit, element_count);
      quadtile(mit->second.relations, *target, *rman.get_transaction(), limit, element_count);
      if (rman.get_area_transaction())
	quadtile(mit->second.areas, *target, *rman.get_area_transaction(), limit, element_count);
    }
  }
  
  rman.health_check(*this);
}


Print_Statement::~Print_Statement() {}
