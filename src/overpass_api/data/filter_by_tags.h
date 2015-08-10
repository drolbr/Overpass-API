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

#ifndef DE__OSM3S___OVERPASS_API__DATA__FILTER_BY_TAGS_H
#define DE__OSM3S___OVERPASS_API__DATA__FILTER_BY_TAGS_H


#include "regular_expression.h"


std::set< Tag_Index_Global > get_kv_req(const string& key, const string& value)
{
  std::set< Tag_Index_Global > result;
  Tag_Index_Global idx;
  idx.key = key;
  idx.value = value;
  result.insert(idx);
  return result;
}


std::set< pair< Tag_Index_Global, Tag_Index_Global > > get_k_req(const string& key)
{
  std::set< pair< Tag_Index_Global, Tag_Index_Global > > result;
  pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
  idx_pair.first.key = key;
  idx_pair.first.value = "";
  idx_pair.second.key = key + (char)0;
  idx_pair.second.value = "";
  result.insert(idx_pair);
  return result;
}


template< typename Skeleton >
std::set< pair< Tag_Index_Global, Tag_Index_Global > > get_regk_req
    (Regular_Expression* key, Resource_Manager& rman, Statement& stmt)
{
  std::set< pair< Tag_Index_Global, Tag_Index_Global > > result;
  
  Block_Backend< Uint32_Index, String_Object > db
      (rman.get_transaction()->data_index(*key_file_properties< Skeleton >()));
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
       it(db.flat_begin()); !(it == db.flat_end()); ++it)
  {
    if (key->matches(it.object().val()))
    {
      pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
      idx_pair.first.key = it.object().val();
      idx_pair.first.value = "";
      idx_pair.second.key = it.object().val() + (char)0;
      idx_pair.second.value = "";
      result.insert(idx_pair);
    }
  }
  rman.health_check(stmt);
  
  return result;
}


template< typename Id_Type >
bool operator<(const std::pair< Id_Type, Uint31_Index >& lhs, const std::pair< Id_Type, Uint31_Index >& rhs)
{
  return lhs.first < rhs.first;
}


template< typename Id_Type >
bool operator==(const std::pair< Id_Type, Uint31_Index >& lhs, const std::pair< Id_Type, Uint31_Index >& rhs)
{
  return lhs.first == rhs.first;
}


template< class Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_kv(
    vector< pair< string, string > >::const_iterator kvit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db)
{
  std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id;
  std::set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
      
  for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Discrete_Iterator
      it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
      !(it2 == tags_db.discrete_end()); ++it2)
    timestamp_per_id[it2.object().id] = std::make_pair(NOW, it2.object().idx);
      
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Discrete_Iterator
      it2(attic_tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
      !(it2 == attic_tags_db.discrete_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp)
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }
      
  std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(kvit->first);
      
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Range_Iterator
      it2(attic_tags_db.range_begin(Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
          Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp)
    {
      typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
        if (it2.object().timestamp < it->second.first)
          timestamp_per_id.erase(it);
      }
    }
  }
  
  return timestamp_per_id;
}


template< class Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_k(
    vector< string >::const_iterator kit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db)
{
  std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id;
  std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(*kit);
      
  for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Range_Iterator
      it2(tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
      Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == tags_db.range_end()); ++it2)
    timestamp_per_id[it2.object().id] = std::make_pair(NOW, it2.object().idx);
      
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Range_Iterator
      it2(attic_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
      Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp && it2.index().value != void_tag_value())
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }
      
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Range_Iterator
      it2(attic_tags_db.range_begin(Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
          Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp && it2.index().value == void_tag_value())
    {
      typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
        if (it2.object().timestamp < it->second.first)
          timestamp_per_id.erase(it);
      }
    }
  }
  
  return timestamp_per_id;
}


template< class Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_kregv(
    vector< pair< string, Regular_Expression* > >::const_iterator krit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db)
{
  std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id;
  std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req = get_k_req(krit->first);
      
  for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Range_Iterator
      it2(tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
      Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == tags_db.range_end()); ++it2)
  {
    if (krit->second->matches(it2.index().value))
      timestamp_per_id[it2.object().id] = std::make_pair(NOW, it2.object().idx);
  }
      
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Range_Iterator
      it2(attic_tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
      Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp && krit->second->matches(it2.index().value))
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }
      
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Range_Iterator
      it2(attic_tags_db.range_begin(Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
          Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp)
    {
      typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
        if (it2.object().timestamp < it->second.first)
          timestamp_per_id.erase(it);
      }
    }
  }
  
  return timestamp_per_id;
}


template< typename Skeleton, typename Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_regkregv(
    vector< pair< Regular_Expression*, Regular_Expression* > >::const_iterator krit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db,
    Resource_Manager& rman, Statement& stmt)
{
  std::map< Id_Type, std::map< std::string, std::pair< uint64, Uint31_Index > > > timestamp_per_id;
  std::set< pair< Tag_Index_Global, Tag_Index_Global > > range_req
      = get_regk_req< Skeleton >(krit->first, rman, stmt);
      
  std::string last_key = void_tag_value();
  bool matches = false;
  for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Range_Iterator
      it2(tags_db.range_begin
        (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
      Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
      !(it2 == tags_db.range_end()); ++it2)
  {
    if (it2.index().key != last_key)
    {
      last_key = it2.index().key;
      matches = krit->first->matches(it2.index().key);
    }  
    if (matches && krit->second->matches(it2.index().value))
      timestamp_per_id[it2.object().id][last_key] = std::make_pair(NOW, it2.object().idx);
  }
      
  last_key = void_tag_value();
  matches = false;
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Flat_Iterator
      it2(attic_tags_db.flat_begin()); !(it2 == attic_tags_db.flat_end()); ++it2)
  {
    if (it2.index().key != last_key)
    {
      last_key = it2.index().key;
      matches = krit->first->matches(it2.index().key);
    }
    if (it2.object().timestamp > timestamp && matches && krit->second->matches(it2.index().value))
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id][last_key];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }
      
  last_key = void_tag_value();
  matches = false;
  for (typename Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >::Flat_Iterator
      it2(attic_tags_db.flat_begin()); !(it2 == attic_tags_db.flat_end()); ++it2)
  {
    if (it2.index().key != last_key)
    {
      last_key = it2.index().key;
      matches = krit->first->matches(it2.index().key);
    }
    if (matches && it2.object().timestamp > timestamp)
    {
      typename std::map< Id_Type, std::map< std::string, std::pair< uint64, Uint31_Index > > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
	typename std::map< std::string, std::pair< uint64, Uint31_Index > >::iterator
	    it3 = it->second.find(last_key);
	if (it3 != it->second.end())
	{
	  if (it2.object().timestamp < it3->second.first)
	    it->second.erase(it3);
	}
      }
    }
  }

  std::map< Id_Type, std::pair< uint64, Uint31_Index > > result;
  for (typename std::map< Id_Type, std::map< std::string, std::pair< uint64, Uint31_Index > > >::const_iterator
      it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
  {
    if (!it->second.empty())
      result[it->first] = it->second.begin()->second;
  }

  return result;
}


#endif
