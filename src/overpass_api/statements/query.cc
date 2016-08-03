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

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/filenames.h"
#include "../data/filter_by_tags.h"
#include "../data/filter_ids_by_tags.h"
#include "../data/regular_expression.h"
#include "meta_collector.h"
#include "area_query.h"
#include "query.h"

#include <algorithm>
#include <sstream>


//-----------------------------------------------------------------------------

bool Query_Statement::area_query_exists_ = false;

Generic_Statement_Maker< Query_Statement > Query_Statement::statement_maker("query");

Query_Statement::Query_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  if (attributes["type"] == "node")
    type = QUERY_NODE;
  else if (attributes["type"] == "way")
    type = QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = QUERY_RELATION;
  else if (attributes["type"] == "area")
  {
    type = QUERY_AREA;
    area_query_exists_ = true;
  }
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\", \"relation\" or \"area\".";
    add_static_error(temp.str());
  }
  
  if (bbox_limitation)
    constraints.push_back(bbox_limitation);
}

void Query_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  Has_Kv_Statement* has_kv(dynamic_cast<Has_Kv_Statement*>(statement));
  if (has_kv)
  {
    if (has_kv->get_value() != "")
    {
      if (has_kv->get_straight())
        key_values.push_back(make_pair< string, string >
	    (has_kv->get_key(), has_kv->get_value()));
      else
        key_nvalues.push_back(make_pair< string, string >
	    (has_kv->get_key(), has_kv->get_value()));
    }
    else if (has_kv->get_key_regex())
    {
      if (has_kv->get_straight())
	regkey_regexes.push_back(make_pair< Regular_Expression*, Regular_Expression* >
            (has_kv->get_key_regex(), has_kv->get_regex()));
      else
	regkey_nregexes.push_back(make_pair< Regular_Expression*, Regular_Expression* >
            (has_kv->get_key_regex(), has_kv->get_regex()));
    }
    else if (has_kv->get_regex())
    {
      if (has_kv->get_straight())
	key_regexes.push_back(make_pair< string, Regular_Expression* >
            (has_kv->get_key(), has_kv->get_regex()));
      else
	key_nregexes.push_back(make_pair< string, Regular_Expression* >
            (has_kv->get_key(), has_kv->get_regex()));
    }
    else
      keys.push_back(has_kv->get_key());
    return;
  }
  
  Query_Constraint* constraint = statement->get_query_constraint();
  if (constraint)
    constraints.push_back(constraint);
  else
    substatement_error(get_name(), statement);
}


template < typename T >
struct Optional
{
  Optional(T* obj_) : obj(obj_) {}
  ~Optional() { delete obj; }
  
  T* obj;
};


struct Trivial_Regex
{
public:
  bool matches(const std::string&) const { return true; }
};


template< typename Id_Type, typename Iterator, typename Key_Regex, typename Val_Regex >
void filter_id_list(
    std::vector< std::pair< Id_Type, Uint31_Index > >& new_ids, bool& filtered,
    Iterator begin, Iterator end, const Key_Regex& key_regex, const Val_Regex& val_regex)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > old_ids;
  old_ids.swap(new_ids);
  
  for (Iterator it = begin; !(it == end); ++it)
  {
    if (key_regex.matches(it.index().key) && val_regex.matches(it.index().value) && (!filtered ||
	binary_search(old_ids.begin(), old_ids.end(), std::make_pair(it.object().id, Uint31_Index(0u)))))
      new_ids.push_back(std::make_pair(it.object().id, it.object().idx));
  }
  
  sort(new_ids.begin(), new_ids.end());
  filtered = true;  
}


template< typename Id_Type, typename Iterator, typename Key_Regex, typename Val_Regex >
void filter_id_list(
    std::vector< Id_Type >& new_ids, bool& filtered,
    Iterator begin, Iterator end, const Key_Regex& key_regex, const Val_Regex& val_regex)
{
  std::vector< Id_Type > old_ids;
  old_ids.swap(new_ids);
  
  for (Iterator it = begin; !(it == end); ++it)
  {
    if (key_regex.matches(it.index().key) && val_regex.matches(it.index().value) &&
	(!filtered || binary_search(old_ids.begin(), old_ids.end(), it.object())))
      new_ids.push_back(it.object());
  }
  
  sort(new_ids.begin(), new_ids.end());
  filtered = true;  
}


template< typename Id_Type, typename Container >
void filter_id_list(
    std::vector< std::pair< Id_Type, Uint31_Index > >& new_ids, bool& filtered,
    const Container& container)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > old_ids;
  old_ids.swap(new_ids);
  
  for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
  {
    if (!filtered ||
	binary_search(old_ids.begin(), old_ids.end(), std::make_pair(it->first, Uint31_Index(0u))))
      new_ids.push_back(std::make_pair(it->first, it->second.second));
  }
  
  sort(new_ids.begin(), new_ids.end());
  filtered = true;
}


template< typename Skeleton, typename Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > Query_Statement::collect_ids
  (const File_Properties& file_prop, const File_Properties& attic_file_prop, Resource_Manager& rman,
   uint64 timestamp, bool check_keys_late)
{
  if (key_values.empty() && keys.empty() && key_regexes.empty() && regkey_regexes.empty())
    return std::vector< std::pair< Id_Type, Uint31_Index > >();
  
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  Optional< Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > > > attic_tags_db
      (timestamp == NOW ? 0 :
        new Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >
        (rman.get_transaction()->data_index(&attic_file_prop)));
  
  // Handle simple Key-Value pairs
  std::vector< std::pair< Id_Type, Uint31_Index > > new_ids;
  bool filtered = false;
  
  for (vector< pair< string, string > >::const_iterator kvit = key_values.begin();
       kvit != key_values.end(); ++kvit)
  {
    if (timestamp == NOW)
    {
      std::set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
      filter_id_list(new_ids, filtered,
		     tags_db.discrete_begin(tag_req.begin(), tag_req.end()), tags_db.discrete_end(),
		     Trivial_Regex(), Trivial_Regex());
    }
    else
      filter_id_list(new_ids, filtered, collect_attic_kv(kvit, timestamp, tags_db, *attic_tags_db.obj));
      
    rman.health_check(*this);
  }

  if (!check_keys_late)
  {
    // Handle simple Keys Only
    for (vector< string >::const_iterator kit = keys.begin(); kit != keys.end(); ++kit)
    {
      if (timestamp == NOW)
      {
        std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(*kit);
	filter_id_list(new_ids, filtered,
		       tags_db.range_begin(range_req.begin(), range_req.end()), tags_db.range_end(),
			Trivial_Regex(), Trivial_Regex());
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_k(kit, timestamp, tags_db, *attic_tags_db.obj));
      
      rman.health_check(*this);
    }

    // Handle Key-Regular-Expression-Value pairs
    for (vector< pair< string, Regular_Expression* > >::const_iterator krit = key_regexes.begin();
	 krit != key_regexes.end(); ++krit)
    {
      if (timestamp == NOW)
      {
        std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(krit->first);
	filter_id_list(new_ids, filtered,
	    tags_db.range_begin(range_req.begin(), range_req.end()), tags_db.range_end(),
		Trivial_Regex(), *krit->second);
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_kregv(krit, timestamp, tags_db, *attic_tags_db.obj));
      
      rman.health_check(*this);
    }

    // Handle Regular-Key-Regular-Expression-Value pairs
    for (vector< pair< Regular_Expression*, Regular_Expression* > >::const_iterator it = regkey_regexes.begin();
	 it != regkey_regexes.end(); ++it)
    {
      if (timestamp == NOW)
      {
	std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req
	    = get_regk_req< Skeleton >(it->first, rman, *this);
	filter_id_list(new_ids, filtered,
	    tags_db.range_begin(range_req.begin(), range_req.end()), tags_db.range_end(), *it->first, *it->second);
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_regkregv< Skeleton, Id_Type >(
	    it, timestamp, tags_db, *attic_tags_db.obj, rman, *this));
      
      rman.health_check(*this);
    }
  }

  return new_ids;
}


template< class Id_Type >
vector< Id_Type > Query_Statement::collect_ids
  (const File_Properties& file_prop, Resource_Manager& rman,
   bool check_keys_late)
{
  if (key_values.empty() && keys.empty() && key_regexes.empty() && regkey_regexes.empty())
    return vector< Id_Type >();
 
  Block_Backend< Tag_Index_Global, Id_Type > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  
  // Handle simple Key-Value pairs
  vector< Id_Type > new_ids;
  bool filtered = false;

  for (vector< pair< string, string > >::const_iterator kvit = key_values.begin();
       kvit != key_values.end(); ++kvit)
  {
    std::set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
    filter_id_list(new_ids, filtered,
	tags_db.discrete_begin(tag_req.begin(), tag_req.end()), tags_db.discrete_end(),
	    Trivial_Regex(), Trivial_Regex());
      
    rman.health_check(*this);
  }

  // Handle simple Keys Only
  if (!check_keys_late)
  {
    for (vector< string >::const_iterator kit = keys.begin(); kit != keys.end(); ++kit)
    {
      std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(*kit);
      filter_id_list(new_ids, filtered,
	  tags_db.range_begin(range_req.begin(), range_req.end()), tags_db.range_end(),
	      Trivial_Regex(), Trivial_Regex());
	
      rman.health_check(*this);
    }

    // Handle Key-Regular-Expression-Value pairs
    for (vector< pair< string, Regular_Expression* > >::const_iterator krit = key_regexes.begin();
	 krit != key_regexes.end(); ++krit)
    {
      std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(krit->first);
      filter_id_list(new_ids, filtered,
	  tags_db.range_begin(range_req.begin(), range_req.end()), tags_db.range_end(),
	      Trivial_Regex(), *krit->second);
      
      rman.health_check(*this);
    }

    // Handle Key-Regular-Expression-Value pairs
    for (vector< pair< Regular_Expression*, Regular_Expression* > >::const_iterator it = regkey_regexes.begin();
	 it != regkey_regexes.end(); ++it)
    {
      filter_id_list(new_ids, filtered,
	  tags_db.flat_begin(), tags_db.flat_end(), *it->first, *it->second);
      
      rman.health_check(*this);
    }
  }

  return new_ids;
}


template< class Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > Query_Statement::collect_non_ids
  (const File_Properties& file_prop, const File_Properties& attic_file_prop,
   Resource_Manager& rman, uint64 timestamp)
{
  if (key_nvalues.empty() && key_nregexes.empty())
    return std::vector< std::pair< Id_Type, Uint31_Index > >();
 
  Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  Optional< Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > > > attic_tags_db
      (timestamp == NOW ? 0 :
        new Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >
        (rman.get_transaction()->data_index(&attic_file_prop)));
  
  std::vector< std::pair< Id_Type, Uint31_Index > > new_ids;

  // Handle Key-Non-Value pairs
  for (vector< pair< string, string > >::const_iterator knvit = key_nvalues.begin();
      knvit != key_nvalues.end(); ++knvit)
  {
    if (timestamp == NOW)
    {
      std::set< Tag_Index_Global > tag_req = get_kv_req(knvit->first, knvit->second);
      for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Discrete_Iterator
          it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
          !(it2 == tags_db.discrete_end()); ++it2)
        new_ids.push_back(std::make_pair(it2.object().id, it2.object().idx));
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kv(knvit, timestamp, tags_db, *attic_tags_db.obj);
      
      for (typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::const_iterator
          it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
        new_ids.push_back(std::make_pair(it->first, it->second.second));
    }
    rman.health_check(*this);
  }
  
  // Handle Key-Regular-Expression-Non-Value pairs  
  for (vector< pair< string, Regular_Expression* > >::const_iterator knrit = key_nregexes.begin();
      knrit != key_nregexes.end(); ++knrit)
  {
    if (timestamp == NOW)
    {
      std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(knrit->first);
      for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Range_Iterator
          it2(tags_db.range_begin
          (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
           Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
          !(it2 == tags_db.range_end()); ++it2)
      {
        if (knrit->second->matches(it2.index().value))
          new_ids.push_back(std::make_pair(it2.object().id, it2.object().idx));
      }
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kregv(knrit, timestamp, tags_db, *attic_tags_db.obj);
      
      for (typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::const_iterator
          it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
        new_ids.push_back(std::make_pair(it->first, it->second.second));
    }
    rman.health_check(*this);
  }
  
  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());
  
  return new_ids;
}


template< class Id_Type >
vector< Id_Type > Query_Statement::collect_non_ids
  (const File_Properties& file_prop, Resource_Manager& rman)
{
  if (key_nvalues.empty() && key_nregexes.empty())
    return vector< Id_Type >();
 
  Block_Backend< Tag_Index_Global, Id_Type > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  
  vector< Id_Type > new_ids;

  // Handle Key-Non-Value pairs
  for (vector< pair< string, string > >::const_iterator knvit = key_nvalues.begin();
      knvit != key_nvalues.end(); ++knvit)
  {
    std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(knvit->first);
    for (typename Block_Backend< Tag_Index_Global, Id_Type >::Range_Iterator
        it2(tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
         Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
        !(it2 == tags_db.range_end()); ++it2)
    {
      if (it2.index().value == knvit->second)
        new_ids.push_back(it2.object());
    }
      
    rman.health_check(*this);
  }

  // Handle Key-Regular-Expression-Non-Value pairs  
  for (vector< pair< string, Regular_Expression* > >::const_iterator knrit = key_nregexes.begin();
      knrit != key_nregexes.end(); ++knrit)
  {
    std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(knrit->first);
    for (typename Block_Backend< Tag_Index_Global, Id_Type >::Range_Iterator
        it2(tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
         Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
        !(it2 == tags_db.range_end()); ++it2)
    {
      if (knrit->second->matches(it2.index().value))
        new_ids.push_back(it2.object());
    }

    rman.health_check(*this);
  }

  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());

  return new_ids;
}


void Query_Statement::get_elements_by_id_from_db
    (map< Uint31_Index, vector< Area_Skeleton > >& elements,
     const vector< Area_Skeleton::Id_Type >& ids, bool invert_ids,
     Resource_Manager& rman, File_Properties& file_prop)
{
  if (!invert_ids)
    collect_items_flat(*this, rman, file_prop,
			Id_Predicate< Area_Skeleton >(ids), elements);
  else
    collect_items_flat(*this, rman, file_prop,
			Not_Predicate< Area_Skeleton, Id_Predicate< Area_Skeleton > >
			(Id_Predicate< Area_Skeleton >(ids)), elements);
}


template< typename TIndex, typename TObject >
void clear_empty_indices
    (map< TIndex, vector< TObject > >& modify)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = modify.begin();
      it != modify.end();)
  {
    if (!it->second.empty())
    {
      ++it;
      continue;
    }
    typename map< TIndex, vector< TObject > >::iterator next_it = it;
    if (++next_it == modify.end())
    {
      modify.erase(it);
      break;
    }
    else
    {
      TIndex idx = next_it->first;
      modify.erase(it);
      it = modify.find(idx);
    }
  }
}


template< typename Id_Type >
void filter_ids_by_ntags
  (const map< string, pair< vector< Regular_Expression* >, vector< string > > >& keys,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   uint32 coarse_index,
   vector< Id_Type >& new_ids)
{
  vector< Id_Type > removed_ids;
  string last_key, last_value;  
  bool key_relevant = false;
  bool valid = false;
  map< string, pair< vector< Regular_Expression* >, vector< string > > >::const_iterator
      key_it = keys.begin();
  
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
  {
    if (tag_it.index().key != last_key)
    {
      last_value = void_tag_value() + " ";
      
      if (key_relevant)
      {
        ++key_it;
        sort(removed_ids.begin(), removed_ids.end());
        removed_ids.erase(unique(removed_ids.begin(), removed_ids.end()), removed_ids.end());
        new_ids.erase(set_difference(new_ids.begin(), new_ids.end(),
                      removed_ids.begin(), removed_ids.end(), new_ids.begin()), new_ids.end());
      }
      key_relevant = false;
      
      last_key = tag_it.index().key;
      while (key_it != keys.end() && last_key > key_it->first)
        ++key_it;
      
      if (key_it == keys.end())
        break;
      
      if (last_key == key_it->first)
      {
        key_relevant = true;
        removed_ids.clear();
      }
    }
    
    if (key_relevant)
    {
      if (tag_it.index().value != last_value)
      {
        valid = false;
        for (vector< Regular_Expression* >::const_iterator rit = key_it->second.first.begin();
            rit != key_it->second.first.end(); ++rit)
          valid |= (*rit)->matches(tag_it.index().value);
        for (vector< string >::const_iterator rit = key_it->second.second.begin();
            rit != key_it->second.second.end(); ++rit)
          valid |= (*rit == tag_it.index().value);
        last_value = tag_it.index().value;
      }
      
      if (valid)
        removed_ids.push_back(tag_it.object());
    }

    ++tag_it;
  }
  while ((!(tag_it == items_db.range_end())) &&
      (((tag_it.index().index) & 0x7fffff00) == coarse_index))
    ++tag_it;
  
  sort(removed_ids.begin(), removed_ids.end());
  removed_ids.erase(unique(removed_ids.begin(), removed_ids.end()), removed_ids.end());
  new_ids.erase(set_difference(new_ids.begin(), new_ids.end(),
                removed_ids.begin(), removed_ids.end(), new_ids.begin()), new_ids.end());
  
  sort(new_ids.begin(), new_ids.end());
}


template< typename Id_Type >
void filter_ids_by_ntags
  (const map< string, pair< vector< Regular_Expression* >, vector< string > > >& keys,
   uint64 timestamp,
   const Block_Backend< Tag_Index_Local, Id_Type >& items_db,
   typename Block_Backend< Tag_Index_Local, Id_Type >::Range_Iterator& tag_it,
   const Block_Backend< Tag_Index_Local, Attic< Id_Type > >& attic_items_db,
   typename Block_Backend< Tag_Index_Local, Attic< Id_Type > >::Range_Iterator& attic_tag_it,
   uint32 coarse_index,
   std::vector< Id_Type >& new_ids)
{
  for (map< string, pair< vector< Regular_Expression* >, vector< string > > >::const_iterator
      key_it = keys.begin(); key_it != keys.end(); ++key_it)
  {
    std::map< Id_Type, std::pair< uint64, uint64 > > timestamps;
    for (typename std::vector< Id_Type >::const_iterator it = new_ids.begin(); it != new_ids.end(); ++it)
      timestamps[*it];
    
    while ((!(tag_it == items_db.range_end())) &&
        ((tag_it.index().index) & 0x7fffff00) == coarse_index &&
        tag_it.index().key < key_it->first)
      ++tag_it;
    while ((!(attic_tag_it == attic_items_db.range_end())) &&
        ((attic_tag_it.index().index) & 0x7fffff00) == coarse_index &&
        attic_tag_it.index().key < key_it->first)
      ++attic_tag_it;
    
    bool valid = false;
    std::string last_value = void_tag_value() + " ";
    while ((!(tag_it == items_db.range_end())) &&
        ((tag_it.index().index) & 0x7fffff00) == coarse_index &&
        tag_it.index().key == key_it->first)
    {
      if (std::binary_search(new_ids.begin(), new_ids.end(), tag_it.object()))
      {
        std::pair< uint64, uint64 >& timestamp_ref = timestamps[tag_it.object()];
        timestamp_ref.second = NOW;
      
        if (tag_it.index().value != last_value)
        {
          valid = false;
          for (vector< Regular_Expression* >::const_iterator rit = key_it->second.first.begin();
              rit != key_it->second.first.end(); ++rit)
            valid |= (*rit)->matches(tag_it.index().value);
          for (vector< string >::const_iterator rit = key_it->second.second.begin();
              rit != key_it->second.second.end(); ++rit)
            valid |= (*rit == tag_it.index().value);
          last_value = tag_it.index().value;
        }
      
        if (valid)
          timestamp_ref.first = NOW;
      }
      ++tag_it;
    }
    
    last_value = void_tag_value() + " ";
    while ((!(attic_tag_it == attic_items_db.range_end())) &&
        ((attic_tag_it.index().index) & 0x7fffff00) == coarse_index &&
        attic_tag_it.index().key == key_it->first)
    {
      if (std::binary_search(new_ids.begin(), new_ids.end(), Id_Type(attic_tag_it.object())))
      {
        std::pair< uint64, uint64 >& timestamp_ref = timestamps[attic_tag_it.object()];
        if (timestamp < attic_tag_it.object().timestamp &&
            (timestamp_ref.second == 0 || timestamp_ref.second > attic_tag_it.object().timestamp))
          timestamp_ref.second = attic_tag_it.object().timestamp;
      
        if (attic_tag_it.index().value != last_value)
        {
          valid = false;
          if (attic_tag_it.index().value != void_tag_value())
          {
            for (vector< Regular_Expression* >::const_iterator rit = key_it->second.first.begin();
                rit != key_it->second.first.end(); ++rit)
              valid |= (*rit)->matches(attic_tag_it.index().value);
            for (vector< string >::const_iterator rit = key_it->second.second.begin();
                rit != key_it->second.second.end(); ++rit)
              valid |= (*rit == attic_tag_it.index().value);
          }
          last_value = attic_tag_it.index().value;
        }
      
        if (valid && timestamp < attic_tag_it.object().timestamp &&
            (timestamp_ref.first == 0 || timestamp_ref.first > attic_tag_it.object().timestamp))
          timestamp_ref.first = attic_tag_it.object().timestamp;
      }
      ++attic_tag_it;
    }
    
    new_ids.clear();
    new_ids.reserve(timestamps.size());
    for (typename std::map< Id_Type, std::pair< uint64, uint64 > >::const_iterator
        it = timestamps.begin(); it != timestamps.end(); ++it)
    {
      if (!(0 < it->second.first && it->second.first <= it->second.second))
        new_ids.push_back(it->first);
    }
  }
  while ((!(tag_it == items_db.range_end())) &&
      ((tag_it.index().index) & 0x7fffff00) == coarse_index)
    ++tag_it;
  while ((!(attic_tag_it == attic_items_db.range_end())) &&
      ((attic_tag_it.index().index) & 0x7fffff00) == coarse_index)
    ++attic_tag_it;
}


template< class TIndex, class TObject >
void Query_Statement::filter_by_tags
    (std::map< TIndex, std::vector< TObject > >& items,
     std::map< TIndex, std::vector< Attic< TObject > > >* attic_items,
     uint64 timestamp, const File_Properties& file_prop, const File_Properties* attic_file_prop,
     Resource_Manager& rman, Transaction& transaction)
{
  // generate set of relevant coarse indices
  set< TIndex > coarse_indices;
  map< uint32, vector< typename TObject::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  if (timestamp != NOW)
    generate_ids_by_coarse(coarse_indices, ids_by_coarse, *attic_items);
  
  // formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);

  // prepare straight keys
  map< string, vector< Regular_Expression* > > key_union;
  for (vector< string >::const_iterator it = keys.begin(); it != keys.end();
      ++it)
    key_union[*it];
  for (vector< pair< string, Regular_Expression* > >::const_iterator
      it = key_regexes.begin(); it != key_regexes.end(); ++it)
    key_union[it->first].push_back(it->second);
  
  // iterate over the result
  std::map< TIndex, std::vector< TObject > > result;
  std::map< TIndex, std::vector< Attic< TObject > > > attic_result;
  uint coarse_count = 0;
  
  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  typename Block_Backend< Tag_Index_Local, typename TObject::Id_Type >::Range_Iterator
    tag_it(items_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    
  typename std::map< TIndex, std::vector< TObject > >::const_iterator item_it
      = items.begin();
      
  if (timestamp == NOW)
  {
    for (typename set< TIndex >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
    
      sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
      
      filter_ids_by_tags(key_union, regkey_regexes, items_db, tag_it,
                         it->val() & 0x7fffff00, ids_by_coarse[it->val() & 0x7fffff00]);

      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
	     eit != item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(),
              ids_by_coarse[it->val()].end(), eit->id))
	    result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }
  else
  {
    Block_Backend< Tag_Index_Local, Attic< typename TObject::Id_Type > > attic_items_db
        (transaction.data_index(attic_file_prop));
    typename Block_Backend< Tag_Index_Local, Attic< typename TObject::Id_Type > >::Range_Iterator
      attic_tag_it(attic_items_db.range_begin
      (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
       Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    
    typename std::map< TIndex, std::vector< Attic< TObject > > >::const_iterator attic_item_it
        = attic_items->begin();

    for (typename set< TIndex >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
    
      sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());

      filter_ids_by_tags(key_union, regkey_regexes, timestamp, items_db, tag_it, attic_items_db, attic_tag_it,
                         it->val() & 0x7fffff00, ids_by_coarse[it->val() & 0x7fffff00]);
    
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    
      while ((attic_item_it != attic_items->end()) &&
          ((attic_item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< Attic< TObject > >::const_iterator eit = attic_item_it->second.begin();
             eit != attic_item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end(), eit->id))
            attic_result[attic_item_it->first.val()].push_back(*eit);
        }
        ++attic_item_it;
      }
    }
  }
    
  items.swap(result);
  if (attic_items)
    attic_items->swap(attic_result);

  if (key_nregexes.empty() && key_nvalues.empty())
    return;
  
  // prepare negated keys
  map< string, pair< vector< Regular_Expression* >, vector< string > > > nkey_union;
  for (vector< pair< string, Regular_Expression* > >::const_iterator
      it = key_nregexes.begin(); it != key_nregexes.end(); ++it)
    nkey_union[it->first].first.push_back(it->second);
  for (vector< pair< string, string > >::const_iterator
      it = key_nvalues.begin(); it != key_nvalues.end(); ++it)
    nkey_union[it->first].second.push_back(it->second);
  
  // iterate over the result
  result.clear();
  attic_result.clear();
  coarse_count = 0;
  typename Block_Backend< Tag_Index_Local, typename TObject::Id_Type >::Range_Iterator
      ntag_it(items_db.range_begin
      (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
       Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  item_it = items.begin();
  
  if (timestamp == NOW)
  {
    for (typename set< TIndex >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
    
      sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
      filter_ids_by_ntags(nkey_union, items_db, ntag_it,
                          it->val() & 0x7fffff00, ids_by_coarse[it->val() & 0x7fffff00]);
    
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(),
              ids_by_coarse[it->val()].end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }
  else
  {
    Block_Backend< Tag_Index_Local, Attic< typename TObject::Id_Type > > attic_items_db
        (transaction.data_index(attic_file_prop));
    typename Block_Backend< Tag_Index_Local, Attic< typename TObject::Id_Type > >::Range_Iterator
      attic_ntag_it(attic_items_db.range_begin
      (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
       Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    
    typename std::map< TIndex, std::vector< Attic< TObject > > >::const_iterator attic_item_it
        = attic_items->begin();

    for (typename set< TIndex >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
    
      sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
      filter_ids_by_ntags(nkey_union, timestamp, items_db, ntag_it, attic_items_db, attic_ntag_it,
                          it->val() & 0x7fffff00, ids_by_coarse[it->val() & 0x7fffff00]);
    
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(),
              ids_by_coarse[it->val()].end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    
      while ((attic_item_it != attic_items->end()) &&
          ((attic_item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< Attic< TObject > >::const_iterator eit = attic_item_it->second.begin();
             eit != attic_item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(),
              ids_by_coarse[it->val()].end(), eit->id))
            attic_result[attic_item_it->first.val()].push_back(*eit);
        }
        ++attic_item_it;
      }
    }
  }
  
  items.swap(result);
  if (attic_items)
    attic_items->swap(attic_result);
}


template< class TIndex, class TObject >
void Query_Statement::filter_by_tags
    (std::map< TIndex, std::vector< TObject > >& items,
     const File_Properties& file_prop,
     Resource_Manager& rman, Transaction& transaction)
{
  // generate set of relevant coarse indices
  set< TIndex > coarse_indices;
  map< uint32, vector< typename TObject::Id_Type > > ids_by_coarse;
  generate_ids_by_coarse(coarse_indices, ids_by_coarse, items);
  
  // formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  formulate_range_query(range_set, coarse_indices);

  // prepare straight keys
  map< string, vector< Regular_Expression* > > key_union;
  for (vector< string >::const_iterator it = keys.begin(); it != keys.end();
      ++it)
    key_union[*it];
  for (vector< pair< string, Regular_Expression* > >::const_iterator
      it = key_regexes.begin(); it != key_regexes.end(); ++it)
    key_union[it->first].push_back(it->second);
  
  // iterate over the result
  std::map< TIndex, std::vector< TObject > > result;
  std::map< TIndex, std::vector< Attic< TObject > > > attic_result;
  uint coarse_count = 0;
  
  Block_Backend< Tag_Index_Local, typename TObject::Id_Type > items_db
      (transaction.data_index(&file_prop));
  typename Block_Backend< Tag_Index_Local, typename TObject::Id_Type >::Range_Iterator
    tag_it(items_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
    
  typename std::map< TIndex, std::vector< TObject > >::const_iterator item_it
      = items.begin();
      
  {
    for (typename set< TIndex >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
    
      sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());

      filter_ids_by_tags_old(ids_by_coarse, key_union, regkey_regexes, items_db, tag_it, it->val());
    
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
	     eit != item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(),
              ids_by_coarse[it->val()].end(), eit->id))
	    result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }
    
  items.swap(result);

  if (key_nregexes.empty() && key_nvalues.empty())
    return;
  
  // prepare negated keys
  map< string, pair< vector< Regular_Expression* >, vector< string > > > nkey_union;
  for (vector< pair< string, Regular_Expression* > >::const_iterator
      it = key_nregexes.begin(); it != key_nregexes.end(); ++it)
    nkey_union[it->first].first.push_back(it->second);
  for (vector< pair< string, string > >::const_iterator
      it = key_nvalues.begin(); it != key_nvalues.end(); ++it)
    nkey_union[it->first].second.push_back(it->second);
  
  // iterate over the result
  result.clear();
  attic_result.clear();
  coarse_count = 0;
  typename Block_Backend< Tag_Index_Local, typename TObject::Id_Type >::Range_Iterator
      ntag_it(items_db.range_begin
      (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
       Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  item_it = items.begin();
  
  {
    for (typename set< TIndex >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    {
      if (++coarse_count >= 1024)
      {
        coarse_count = 0;
        rman.health_check(*this);
      }
    
      sort(ids_by_coarse[it->val()].begin(), ids_by_coarse[it->val()].end());
    
      filter_ids_by_ntags(nkey_union, items_db, ntag_it,
                          it->val() & 0x7fffff00, ids_by_coarse[it->val() & 0x7fffff00]);
    
      while ((item_it != items.end()) &&
          ((item_it->first.val() & 0x7fffff00) == it->val()))
      {
        for (typename std::vector< TObject >::const_iterator eit = item_it->second.begin();
             eit != item_it->second.end(); ++eit)
        {
          if (binary_search(ids_by_coarse[it->val()].begin(),
              ids_by_coarse[it->val()].end(), eit->id))
            result[item_it->first.val()].push_back(*eit);
        }
        ++item_it;
      }
    }
  }
  
  items.swap(result);
}


template< typename Skeleton, typename Id_Type, typename Index >
void Query_Statement::progress_1(vector< Id_Type >& ids, std::vector< Index >& range_vec,
                                 bool& invert_ids, uint64 timestamp,
                                 Answer_State& answer_state, bool check_keys_late,
                                 const File_Properties& file_prop, const File_Properties& attic_file_prop,
                                 Resource_Manager& rman)
{
  ids.clear();
  range_vec.clear();
  if (!key_values.empty()
     || (!keys.empty() && !check_keys_late)
     || (!key_regexes.empty() && !check_keys_late)
     || (!regkey_regexes.empty() && !check_keys_late))
  {
    std::vector< std::pair< Id_Type, Uint31_Index > > id_idxs =
        collect_ids< Skeleton, Id_Type >(file_prop, attic_file_prop, rman, timestamp, check_keys_late);
    
    if (!key_nvalues.empty() || !key_nregexes.empty())
    {
      std::vector< std::pair< Id_Type, Uint31_Index > > non_ids
          = collect_non_ids< Id_Type >(file_prop, attic_file_prop, rman, timestamp);
      std::vector< std::pair< Id_Type, Uint31_Index > > diff_ids(id_idxs.size());
      diff_ids.erase(set_difference(id_idxs.begin(), id_idxs.end(), non_ids.begin(), non_ids.end(),
		     diff_ids.begin()), diff_ids.end());
      ids.clear();
      range_vec.clear();
      for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator it = diff_ids.begin();
          it != diff_ids.end(); ++it)
      {
        ids.push_back(it->first);
        range_vec.push_back(it->second);
      }
    }
    else
    {
      for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator it = id_idxs.begin();
          it != id_idxs.end(); ++it)
      {
        ids.push_back(it->first);
        range_vec.push_back(it->second);
      }
    }
      
    if (ids.empty())
      answer_state = data_collected;
  }
  else if ((!key_nvalues.empty() || !key_nregexes.empty()) && !check_keys_late)
  {
    invert_ids = true;
    std::vector< std::pair< Id_Type, Uint31_Index > > id_idxs =
        collect_non_ids< Id_Type >(file_prop, attic_file_prop, rman, timestamp);
    for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator it = id_idxs.begin();
        it != id_idxs.end(); ++it)
      ids.push_back(it->first);
  }
}


template< class Id_Type >
void Query_Statement::progress_1(vector< Id_Type >& ids, bool& invert_ids,
                                 Answer_State& answer_state, bool check_keys_late,
                                 const File_Properties& file_prop,
                                 Resource_Manager& rman)
{
  if (!key_values.empty()
     || (!keys.empty() && !check_keys_late)
     || (!key_regexes.empty() && !check_keys_late)
     || (!regkey_regexes.empty() && !check_keys_late))
  {
    collect_ids< Id_Type >(file_prop, rman, check_keys_late).swap(ids);
    if (!key_nvalues.empty() || !key_nregexes.empty() || !regkey_nregexes.empty())
    {
      vector< Id_Type > non_ids = collect_non_ids< Id_Type >(file_prop, rman);
      vector< Id_Type > diff_ids(ids.size(), Id_Type());
      diff_ids.erase(set_difference(ids.begin(), ids.end(), non_ids.begin(), non_ids.end(),
                     diff_ids.begin()), diff_ids.end());
      ids.swap(diff_ids);
    }
    if (ids.empty())
      answer_state = data_collected;
  }
  else if ((!key_nvalues.empty() || !key_nregexes.empty() || !regkey_nregexes.empty()) && !check_keys_late)
  {
    invert_ids = true;
    collect_non_ids< Id_Type >(file_prop, rman).swap(ids);
  }
}


template< class Id_Type >
void Query_Statement::collect_nodes(vector< Id_Type >& ids,
				 bool& invert_ids, Answer_State& answer_state, Set& into,
				 Resource_Manager& rman)
{
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end() && answer_state < data_collected; ++it)
  {
    if ((*it)->collect_nodes(rman, into, ids, invert_ids))
      answer_state = data_collected;
  }
}


template< class Id_Type >
void Query_Statement::collect_elems(vector< Id_Type >& ids,
				 bool& invert_ids, Answer_State& answer_state, Set& into,
				 Resource_Manager& rman)
{
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end() && answer_state < data_collected; ++it)
  {
    if ((*it)->collect(rman, into, type, ids, invert_ids))
      answer_state = data_collected;
  }
}


template< typename Index >
std::set< std::pair< Index, Index > > intersect_ranges
    (const std::set< std::pair< Index, Index > >& range_a,
     std::vector< Index >& range_vec)
{
  std::set< std::pair< Index, Index > > result;
  
  unsigned long long sum = 0;
  for (typename std::set< std::pair< Index, Index > >::const_iterator it = range_a.begin();
       it != range_a.end(); ++it)
    sum += difference(it->first, it->second);
  
  if (sum/256 < range_vec.size())
    return range_a;
  
  std::sort(range_vec.begin(), range_vec.end());
  
  typename std::set< std::pair< Index, Index > >::const_iterator it_a = range_a.begin();
  typename std::vector< Index >::const_iterator it_vec = range_vec.begin();
  
  while (it_a != range_a.end() && it_vec != range_vec.end())
  {
    if (!(it_a->first < Index(it_vec->val() + 0x100)))
      ++it_vec;
    else if (!(*it_vec < it_a->second))
      ++it_a;
    else if (Index(it_vec->val() + 0x100) < it_a->second)
    {
      result.insert(std::make_pair(std::max(it_a->first, *it_vec), Index(it_vec->val() + 0x100)));
      ++it_vec;
    }
    else
    {
      result.insert(std::make_pair(std::max(it_a->first, *it_vec), it_a->second));
      ++it_a;
    }
  }
  
  return result;
}


void Query_Statement::execute(Resource_Manager& rman)
{
  Answer_State answer_state = nothing;
  Set into;
  uint64 timestamp = rman.get_desired_timestamp();
  if (timestamp == 0)
    timestamp = NOW;
  
  set_progress(1);
  rman.health_check(*this);

  bool check_keys_late = false;
  for (vector< Query_Constraint* >::iterator it = constraints.begin(); it != constraints.end(); ++it)
    check_keys_late |= (*it)->delivers_data(rman);

  {
    vector< Node::Id_Type > node_ids;
    vector< Uint32_Index > ids;
    bool invert_ids = false;

    set< pair< Uint32_Index, Uint32_Index > > range_req_32;
    std::vector< Uint32_Index > range_vec_32;
    set< pair< Uint31_Index, Uint31_Index > > range_req_31;
    std::vector< Uint31_Index > range_vec_31;

    if (type == QUERY_NODE)
    {
      progress_1< Node_Skeleton, Node::Id_Type, Uint32_Index >(
	  node_ids, range_vec_32, invert_ids, timestamp, answer_state, check_keys_late,
          *osm_base_settings().NODE_TAGS_GLOBAL, *attic_settings().NODE_TAGS_GLOBAL, rman);
      collect_nodes(node_ids, invert_ids, answer_state, into, rman);
    }
    else if (type == QUERY_WAY)
    {
      progress_1< Way_Skeleton, Way::Id_Type, Uint31_Index >(
	  ids, range_vec_31, invert_ids, timestamp, answer_state, check_keys_late,
          *osm_base_settings().WAY_TAGS_GLOBAL, *attic_settings().WAY_TAGS_GLOBAL, rman);
      collect_elems(ids, invert_ids, answer_state, into, rman);
    }
    else if (type == QUERY_RELATION)
    {
      progress_1< Relation_Skeleton, Relation::Id_Type, Uint31_Index >(
	  ids, range_vec_31, invert_ids, timestamp, answer_state, check_keys_late,
          *osm_base_settings().RELATION_TAGS_GLOBAL,  *attic_settings().RELATION_TAGS_GLOBAL, rman);
      collect_elems(ids, invert_ids, answer_state, into, rman);
    }
    else if (type == QUERY_AREA)
    {
      progress_1(ids, invert_ids, answer_state,
		 check_keys_late, *area_settings().AREA_TAGS_GLOBAL, rman);
      collect_elems(ids, invert_ids, answer_state, into, rman);
    }
    
    set_progress(2);
    rman.health_check(*this);

    if (type == QUERY_NODE)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
	vector< Node::Id_Type > constraint_node_ids;
	if ((*it)->get_node_ids(rman, constraint_node_ids))
	{
	  if (node_ids.empty())
	    node_ids.swap(constraint_node_ids);
	  else
	  {
	    vector< Node::Id_Type > new_ids(node_ids.size());
	    new_ids.erase(std::set_intersection(
	        node_ids.begin(), node_ids.end(), constraint_node_ids.begin(), constraint_node_ids.end(),
	        new_ids.begin()), new_ids.end());
	    node_ids.swap(new_ids);
	  }
	  
	  if (node_ids.empty())
	    answer_state = data_collected;
	}
      }
    }
    else if (type == QUERY_WAY)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
	vector< Way::Id_Type > constraint_way_ids;
	if ((*it)->get_way_ids(rman, constraint_way_ids))
	{
	  if (ids.empty())
	    ids.swap(constraint_way_ids);
	  else
	  {
	    vector< Way::Id_Type > new_ids(ids.size());
	    new_ids.erase(std::set_intersection(
	        ids.begin(), ids.end(), constraint_way_ids.begin(), constraint_way_ids.end(),
	        new_ids.begin()), new_ids.end());
	    ids.swap(new_ids);
	  }
	  
	  if (ids.empty())
	    answer_state = data_collected;
	}
      }
    }
    else if (type == QUERY_RELATION)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
	vector< Relation::Id_Type > constraint_relation_ids;
	if ((*it)->get_relation_ids(rman, constraint_relation_ids))
	{
	  if (ids.empty())
	    ids.swap(constraint_relation_ids);
	  else
	  {
	    vector< Relation::Id_Type > new_ids(ids.size());
	    new_ids.erase(std::set_intersection(
	        ids.begin(), ids.end(), constraint_relation_ids.begin(), constraint_relation_ids.end(),
	        new_ids.begin()), new_ids.end());
	    ids.swap(new_ids);
	  }
	  
	  if (ids.empty())
	    answer_state = data_collected;
	}
      }
    }

    if (type == QUERY_NODE)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
        set< pair< Uint32_Index, Uint32_Index > > range_req;
        if ((*it)->get_ranges(rman, range_req))
        {
          if (answer_state < ranges_collected)
            range_req.swap(range_req_32);
          else
            intersect_ranges(range_req_32, range_req).swap(range_req_32);
	  answer_state = ranges_collected;
        }
      }
      
      if (!range_vec_32.empty())
      {
        if (answer_state < ranges_collected)
        {
          answer_state = ranges_collected;
          range_req_32.clear();
          range_req_32.insert(std::make_pair(Uint32_Index(0u), Uint32_Index(0xffffffff)));
          intersect_ranges(range_req_32, range_vec_32).swap(range_req_32);
        }
        else
          intersect_ranges(range_req_32, range_vec_32).swap(range_req_32);
      }
    }
    else if (type == QUERY_WAY || type == QUERY_RELATION)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
        set< pair< Uint31_Index, Uint31_Index > > range_req;
	if ((*it)->get_ranges(rman, range_req))
        {
          if (answer_state < ranges_collected)
            range_req.swap(range_req_31);
          else
            intersect_ranges(range_req_31, range_req).swap(range_req_31);
	  answer_state = ranges_collected;
        }
      }
      
      if (!range_vec_31.empty())
      {
        if (answer_state < ranges_collected)
        {
          answer_state = ranges_collected;
          range_req_31.clear();
          range_req_31.insert(std::make_pair(Uint31_Index(0u), Uint31_Index(0xffffffff)));
          intersect_ranges(range_req_31, range_vec_31).swap(range_req_31);
        }
        else
          intersect_ranges(range_req_31, range_vec_31).swap(range_req_31);
      }
    }
    
    set_progress(3);
    rman.health_check(*this);

    if (type == QUERY_NODE)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
	if ((*it)->get_data(*this, rman, into, range_req_32, node_ids, invert_ids, timestamp))
	  answer_state = data_collected;
      }
    }
    else if (type == QUERY_WAY || type == QUERY_RELATION || type == QUERY_AREA)
    {
      for (vector< Query_Constraint* >::iterator it = constraints.begin();
          it != constraints.end() && answer_state < data_collected; ++it)
      {
	if ((*it)->get_data(*this, rman, into, range_req_31, type, ids, invert_ids, timestamp))
	  answer_state = data_collected;
      }
    }
  
    set_progress(4);
    rman.health_check(*this);
    
//     std::cout<<"progress 4\n";
//     for (vector< Relation::Id_Type >::const_iterator it = ids.begin(); it != ids.end(); ++it)
//       std::cout<<it->val()<<'\n';
//     for (vector< Node::Id_Type >::const_iterator it = node_ids.begin(); it != node_ids.end(); ++it)
//       std::cout<<it->val()<<'\n';
//     for (set< pair< Uint32_Index, Uint32_Index > >::const_iterator it = range_req_32.begin();
//         it != range_req_32.end(); ++it)
//       std::cout<<hex<<it->first.val()<<'\t'<<it->second.val()<<'\n';
//     std::cout<<dec<<answer_state<<'\n';
    
    if (type == QUERY_NODE)
    {
      if (answer_state < data_collected)
      {
        if (range_req_32.empty() && answer_state < ranges_collected && !invert_ids)
	{
          std::vector< Uint32_Index > req = get_indexes_< Uint32_Index, Node_Skeleton >(node_ids, rman);  
          for (std::vector< Uint32_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
            range_req_32.insert(std::make_pair(*it, ++Uint32_Index(*it)));
	}
        ::get_elements_by_id_from_db< Uint32_Index, Node_Skeleton >
            (into.nodes, into.attic_nodes,
             node_ids, invert_ids, timestamp, range_req_32, *this, rman,
             *osm_base_settings().NODES, *attic_settings().NODES);
      }
    }
    else if (type == QUERY_WAY)
    {
      if (answer_state < data_collected)
      {
        if (range_req_31.empty() && answer_state < ranges_collected && !invert_ids)
	{
          std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Way_Skeleton >(ids, rman);  
          for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
            range_req_31.insert(std::make_pair(*it, inc(*it)));
	}
	::get_elements_by_id_from_db< Uint31_Index, Way_Skeleton >
	    (into.ways, into.attic_ways,
             ids, invert_ids, timestamp, range_req_31, *this, rman,
             *osm_base_settings().WAYS, *attic_settings().WAYS);
      }
    }
    else if (type == QUERY_RELATION)
    {
      if (answer_state < data_collected)
      {
        if (range_req_31.empty() && answer_state < ranges_collected && !invert_ids)
	{
          std::vector< Uint31_Index > req = get_indexes_< Uint31_Index, Relation_Skeleton >(ids, rman);  
          for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
            range_req_31.insert(std::make_pair(*it, inc(*it)));
	}
	::get_elements_by_id_from_db< Uint31_Index, Relation_Skeleton >
	    (into.relations, into.attic_relations,
             ids, invert_ids, timestamp, range_req_31, *this, rman,
             *osm_base_settings().RELATIONS, *attic_settings().RELATIONS);
      }
    }
    else if (type == QUERY_AREA)
    {
      if (answer_state < data_collected)
	get_elements_by_id_from_db(into.areas, ids, invert_ids, rman, *area_settings().AREAS);
    }
  }
  
  set_progress(5);
  rman.health_check(*this);

//   std::cout<<'\n';
//   for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = into.nodes.begin();
//        it != into.nodes.end(); ++it)
//   {
//     for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
//       std::cout<<it2->id.val()<<'\n';
//   }
//   std::cout<<'\n';
//   for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::const_iterator
//        it = into.attic_nodes.begin(); it != into.attic_nodes.end(); ++it)
//   {
//     for (vector< Attic< Node_Skeleton > >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
//       std::cout<<it2->id.val()<<'\t'<<it2->timestamp<<'\n';
//   }
//   std::cout<<'\n';
//   std::cout<<'\n';
//   for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator it = into.ways.begin();
//        it != into.ways.end(); ++it)
//   {
//     for (vector< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
//       std::cout<<it2->id.val()<<'\n';
//   }
//   std::cout<<'\n';
//   for (map< Uint31_Index, vector< Attic< Way_Skeleton > > >::const_iterator
//        it = into.attic_ways.begin(); it != into.attic_ways.end(); ++it)
//   {
//     for (vector< Attic< Way_Skeleton > >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
//       std::cout<<it2->id.val()<<'\t'<<it2->timestamp<<'\n';
//   }
//   std::cout<<'\n';
  
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(rman, into, timestamp);
  
  set_progress(6);
  rman.health_check(*this);
  
  filter_attic_elements(rman, timestamp, into.nodes, into.attic_nodes);
  filter_attic_elements(rman, timestamp, into.ways, into.attic_ways);
  filter_attic_elements(rman, timestamp, into.relations, into.attic_relations);
  
  set_progress(7);
  rman.health_check(*this);
    
  if (check_keys_late)
  {
    filter_by_tags(into.nodes, &into.attic_nodes, timestamp,
                   *osm_base_settings().NODE_TAGS_LOCAL, attic_settings().NODE_TAGS_LOCAL,
		   rman, *rman.get_transaction());
    filter_by_tags(into.ways, &into.attic_ways, timestamp,
                   *osm_base_settings().WAY_TAGS_LOCAL, attic_settings().WAY_TAGS_LOCAL,
		   rman, *rman.get_transaction());
    filter_by_tags(into.relations, &into.attic_relations, timestamp,
                   *osm_base_settings().RELATION_TAGS_LOCAL, attic_settings().RELATION_TAGS_LOCAL,
		   rman, *rman.get_transaction());
    if (rman.get_area_transaction())
      filter_by_tags(into.areas,
                     *area_settings().AREA_TAGS_LOCAL,
		     rman, *rman.get_transaction());
  }
  
  set_progress(8);
  rman.health_check(*this);
       
  for (vector< Query_Constraint* >::iterator it = constraints.begin();
      it != constraints.end(); ++it)
    (*it)->filter(*this, rman, into, timestamp);
    
  set_progress(9);
  rman.health_check(*this);
  
  clear_empty_indices(into.nodes);
  clear_empty_indices(into.attic_nodes);
  clear_empty_indices(into.ways);
  clear_empty_indices(into.attic_ways);
  clear_empty_indices(into.relations);
  clear_empty_indices(into.areas);

  transfer_output(rman, into);
  rman.health_check(*this);
}

//-----------------------------------------------------------------------------

Generic_Statement_Maker< Has_Kv_Statement > Has_Kv_Statement::statement_maker("has-kv");

Has_Kv_Statement::Has_Kv_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Statement(line_number_), regex(0), key_regex(0), straight(true)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["regk"] = "";
  attributes["v"] = "";
  attributes["regv"] = "";
  attributes["modv"] = "";
  attributes["case"] = "sensitive";
  attributes["key-case"] = "sensitive";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
  value = attributes["v"];
  
  if (key == "" && attributes["regk"] == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
  
  bool case_sensitive = false;
  if (attributes["case"] != "ignore")
  {
    if (attributes["case"] != "sensitive")
      add_static_error("For the attribute \"case\" of the element \"has-kv\""
	  " the only allowed values are \"sensitive\" or \"ignore\".");
    case_sensitive = true;
  }
  
  if (attributes["regk"] != "")
  {
    if (key != "")
    {
      ostringstream temp("");
      temp<<"In the element \"has-kv\" only one of the attributes \"k\" and \"regk\""
            " can be nonempty.";
      add_static_error(temp.str());
    }
    if (value != "")
    {
      ostringstream temp("");
      temp<<"In the element \"has-kv\" the attribute \"regk\" must be combined with \"regv\".";
      add_static_error(temp.str());
    }
    
    try
    {
      key_regex = new Regular_Expression(attributes["regk"], case_sensitive);
    }
    catch (Regular_Expression_Error e)
    {
      add_static_error("Invalid regular expression: \"" + attributes["regk"] + "\"");
    }
  }
  
  if (attributes["regv"] != "")
  {
    if (value != "")
    {
      ostringstream temp("");
      temp<<"In the element \"has-kv\" only one of the attributes \"v\" and \"regv\""
            " can be nonempty.";
      add_static_error(temp.str());
    }
    
    try
    {
      regex = new Regular_Expression(attributes["regv"], case_sensitive);
    }
    catch (Regular_Expression_Error e)
    {
      add_static_error("Invalid regular expression: \"" + attributes["regv"] + "\"");
    }
  }
  
  if (attributes["modv"] == "" || attributes["modv"] == "not")
  {
    if (attributes["modv"] == "not")
    {
      if (attributes["regk"] == "")
        straight = false;
      else
	add_static_error("In the element \"has-kv\" regular expressions on keys cannot be combined"
	  " with negation.");
    }
  }
  else
  {
    ostringstream temp("");
    temp<<"In the element \"has-kv\" the attribute \"modv\" can only be empty or set to \"not\".";
    add_static_error(temp.str());
  }
}


Has_Kv_Statement::~Has_Kv_Statement()
{
  delete regex;
}
