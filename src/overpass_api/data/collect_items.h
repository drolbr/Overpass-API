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


#ifndef DE__OSM3S___OVERPASS_API__DATA__COLLECT_ITEMS_H
#define DE__OSM3S___OVERPASS_API__DATA__COLLECT_ITEMS_H

#include "filenames.h"


template < class Index, class Object, class Current_Iterator, class Attic_Iterator, class Predicate >
void collect_items_by_timestamp(const Statement* stmt, Resource_Manager& rman,
                   Current_Iterator current_begin, Current_Iterator current_end,
                   Attic_Iterator attic_begin, Attic_Iterator attic_end,
                   const Predicate& predicate,
                   map< Index, vector< Object > >& result,
                   map< Index, vector< Attic< Object > > >& attic_result)
{
  std::vector< std::pair< typename Object::Id_Type, uint64 > > timestamp_by_id;
  uint64 timestamp = rman.get_desired_timestamp();
  
  uint32 count = 0;
  for (Current_Iterator it = current_begin; !(it == current_end); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      if (stmt)
        rman.health_check(*stmt);
    }
    timestamp_by_id.push_back(std::make_pair(it.object().id, NOW));
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }

  count = 0;
  for (Attic_Iterator it = attic_begin; !(it == attic_end); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      if (stmt)
        rman.health_check(*stmt);
    }
    if (timestamp < it.object().timestamp)
      timestamp_by_id.push_back(std::make_pair(it.object().id, it.object().timestamp));
    if (predicate.match(it.object()))
      attic_result[it.index()].push_back(it.object());
  }
  
  std::sort(timestamp_by_id.begin(), timestamp_by_id.end());
  
  for (typename std::map< Index, std::vector< Object > >::iterator it = result.begin();
       it != result.end(); ++it)
  {
    typename std::vector< Object >::iterator target_it = it->second.begin();
    for (typename std::vector< Object >::iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      typename std::vector< std::pair< typename Object::Id_Type, uint64 > >::const_iterator
          tit = std::lower_bound(timestamp_by_id.begin(), timestamp_by_id.end(),
              std::make_pair(it2->id, 0ull));
      if (tit != timestamp_by_id.end() && tit->first == it2->id && tit->second == NOW)
      {
        *target_it = *it2;
        ++target_it;
      }
    }
    it->second.erase(target_it, it->second.end());
  }
  
  for (typename std::map< Index, std::vector< Attic< Object > > >::iterator it = attic_result.begin();
       it != attic_result.end(); ++it)
  {
    typename std::vector< Attic< Object > >::iterator target_it = it->second.begin();
    for (typename std::vector< Attic< Object > >::iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      typename std::vector< std::pair< typename Object::Id_Type, uint64 > >::const_iterator
          tit = std::lower_bound(timestamp_by_id.begin(), timestamp_by_id.end(),
              std::make_pair(it2->id, 0ull));
      if (tit != timestamp_by_id.end() && tit->first == it2->id && tit->second == it2->timestamp)
      {
        *target_it = *it2;
        ++target_it;
      }
    }
    it->second.erase(target_it, it->second.end());
  }
}


template < class Index, class Object, class Container, class Predicate >
void collect_items_discrete(const Statement* stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const Container& req, const Predicate& predicate,
		   map< Index, vector< Object > >& result)
{
  uint32 count = 0;
  Block_Backend< Index, Object, typename Container::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< Index, Object, typename Container
      ::const_iterator >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      if (stmt)
        rman.health_check(*stmt);
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }
}


template < class Index, class Object, class Container, class Predicate >
void collect_items_discrete(Transaction& transaction,
                   File_Properties& file_properties,
                   const Container& req, const Predicate& predicate,
                   map< Index, vector< Object > >& result)
{
  uint32 count = 0;
  Block_Backend< Index, Object, typename Container::const_iterator > db
      (transaction.data_index(&file_properties));
  for (typename Block_Backend< Index, Object, typename Container
      ::const_iterator >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }
}


template < class Index, class Object, class Container, class Predicate >
void collect_items_discrete_by_timestamp(const Statement* stmt, Resource_Manager& rman,
                   const Container& req, const Predicate& predicate,
                   map< Index, vector< Object > >& result,
                   map< Index, vector< Attic< Object > > >& attic_result)
{
  Block_Backend< Index, Object, typename Container::const_iterator > current_db
      (rman.get_transaction()->data_index(current_skeleton_file_properties< Object >()));
  Block_Backend< Index, Attic< Object >, typename Container::const_iterator > attic_db
      (rman.get_transaction()->data_index(attic_skeleton_file_properties< Object >()));
  collect_items_by_timestamp(stmt, rman,
      current_db.discrete_begin(req.begin(), req.end()), current_db.discrete_end(),
      attic_db.discrete_begin(req.begin(), req.end()), attic_db.discrete_end(),
      predicate, result, attic_result);
}


template < class Index, class Object, class Container, class Predicate >
void collect_items_range(const Statement* stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const Container& req, const Predicate& predicate,
		   map< Index, vector< Object > >& result)
{
  uint32 count = 0;
  Block_Backend< Index, Object, typename Container::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< Index, Object, typename Container
      ::const_iterator >::Range_Iterator
      it(db.range_begin(req.begin(), req.end()));
	   !(it == db.range_end()); ++it)
  {
    if (++count >= 64*1024 && stmt)
    {
      count = 0;
      rman.health_check(*stmt);
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }
}


template < class Index, class Object, class Container, class Predicate >
void collect_items_range_by_timestamp(const Statement* stmt, Resource_Manager& rman,
                   const Container& req, const Predicate& predicate,
                   map< Index, vector< Object > >& result,
                   map< Index, vector< Attic< Object > > >& attic_result)
{
  Block_Backend< Index, Object, typename Container::const_iterator > current_db
      (rman.get_transaction()->data_index(current_skeleton_file_properties< Object >()));
  Block_Backend< Index, Attic< Object >, typename Container::const_iterator > attic_db
      (rman.get_transaction()->data_index(attic_skeleton_file_properties< Object >()));
  collect_items_by_timestamp(stmt, rman,
      current_db.range_begin(req.begin(), req.end()), current_db.range_end(),
      attic_db.range_begin(req.begin(), req.end()), attic_db.range_end(),
      predicate, result, attic_result);
}


template < class Index, class Object, class Predicate >
void collect_items_flat(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties, const Predicate& predicate,
		   map< Index, vector< Object > >& result)
{
  uint32 count = 0;
  Block_Backend< Index, Object > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< Index, Object >::Flat_Iterator
      it(db.flat_begin()); !(it == db.flat_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      rman.health_check(stmt);
    }
    if (predicate.match(it.object()))
      result[it.index()].push_back(it.object());
  }
}


template < class Index, class Object, class Predicate >
void collect_items_flat_by_timestamp(const Statement& stmt, Resource_Manager& rman,
                   const Predicate& predicate,
                   map< Index, vector< Object > >& result,
                   map< Index, vector< Attic< Object > > >& attic_result)
{
  Block_Backend< Index, Object > current_db
      (rman.get_transaction()->data_index(current_skeleton_file_properties< Object >()));
  Block_Backend< Index, Attic< Object > > attic_db
      (rman.get_transaction()->data_index(attic_skeleton_file_properties< Object >()));
  collect_items_by_timestamp(&stmt, rman,
      current_db.flat_begin(), current_db.flat_end(),
      attic_db.flat_begin(), attic_db.flat_end(),
      predicate, result, attic_result);
}


#endif
