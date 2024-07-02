/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#include "../dispatch/resource_manager.h"
#include "abstract_processing.h"
#include "filenames.h"
#include "request_context.h"
#include "timeless.h"

#include <exception>


inline uint64 timestamp_of(const Attic< Node_Skeleton >& skel) { return skel.timestamp; }
inline uint64 timestamp_of(const Attic< Way_Skeleton >& skel) { return skel.timestamp; }
inline uint64 timestamp_of(const Attic< Relation_Skeleton >& skel) { return skel.timestamp; }

inline uint64 timestamp_of(const Node_Skeleton& skel) { return NOW; }
inline uint64 timestamp_of(const Way_Skeleton& skel) { return NOW; }
inline uint64 timestamp_of(const Relation_Skeleton& skel) { return NOW; }


template < typename Index, typename Object, typename Iterator, typename Predicate >
void reconstruct_items(
    Iterator& it, Iterator end, Index index,
    const Predicate& predicate,
    std::vector< Object >& result,
    std::vector< std::pair< typename Object::Id_Type, uint64 > >& timestamp_by_id,
    uint64 timestamp, uint32& count)
{
  while (!(it == end) && it.index() == index)
  {
    ++count;
    if (timestamp < timestamp_of(it.object()))
    {
      timestamp_by_id.push_back(std::make_pair(it.object().id, timestamp_of(it.object())));
      if (predicate.match(it.object()))
        result.push_back(it.object());
    }
    ++it;
  }
}


template < typename Index, typename Object, typename Attic_Iterator, typename Current_Iterator, typename Predicate >
void reconstruct_items(Health_Guard& health_guard,
    Current_Iterator& current_it, Current_Iterator current_end,
    Attic_Iterator& attic_it, Attic_Iterator attic_end, Index& idx,
    const Predicate& predicate,
    std::vector< Object >& result, std::vector< Attic< Object > >& attic_result,
    std::vector< std::pair< typename Object::Id_Type, uint64 > >& timestamp_by_id,
    uint64 timestamp)
{
  std::vector< Object > skels;
  std::vector< Attic< typename Object::Delta > > deltas;
  std::vector< std::pair< typename Object::Id_Type, uint64 > > local_timestamp_by_id;

  while (!(current_it == current_end) && current_it.index() == idx)
  {
    timestamp_by_id.push_back(std::make_pair(current_it.object().id, NOW));
    local_timestamp_by_id.push_back(std::make_pair(current_it.object().id, NOW));
    skels.push_back(current_it.object());
    ++current_it;
  }

  while (!(attic_it == attic_end) && attic_it.index() == idx)
  {
    if (timestamp < attic_it.object().timestamp)
    {
      timestamp_by_id.push_back(std::make_pair(attic_it.object().id, attic_it.object().timestamp));
      local_timestamp_by_id.push_back(std::make_pair(attic_it.object().id, attic_it.object().timestamp));
      deltas.push_back(attic_it.object());
    }
    ++attic_it;
  }

  std::vector< const Attic< typename Object::Delta >* > delta_refs;
  delta_refs.reserve(deltas.size());
  for (typename std::vector< Attic< typename Object::Delta > >::const_iterator it = deltas.begin();
      it != deltas.end(); ++it)
    delta_refs.push_back(&*it);

  std::sort(skels.begin(), skels.end());
  std::sort(delta_refs.begin(), delta_refs.end(), Delta_Ref_Comparator< Attic< typename Object::Delta > >());
  std::sort(local_timestamp_by_id.begin(), local_timestamp_by_id.end());

  std::vector< Attic< Object > > attics;
  typename std::vector< Object >::const_iterator skels_it = skels.begin();
  Object reference;
  for (typename std::vector< const Attic< typename Object::Delta >* >::const_iterator it = delta_refs.begin();
        it != delta_refs.end(); ++it)
  {
    if (!(reference.id == (*it)->id))
    {
      while (skels_it != skels.end() && skels_it->id < (*it)->id)
        ++skels_it;
      if (skels_it != skels.end() && skels_it->id == (*it)->id)
        reference = *skels_it;
      else
        reference = Object();
    }
    try
    {
      Attic< Object > attic_obj = Attic< Object >((*it)->expand(reference), (*it)->timestamp);
      if (attic_obj.id.val() != 0)
      {
        reference = attic_obj;
        typename std::vector< std::pair< typename Object::Id_Type, uint64 > >::const_iterator
            tit = std::lower_bound(local_timestamp_by_id.begin(), local_timestamp_by_id.end(),
                                    std::make_pair((*it)->id, 0ull));
        if (tit != local_timestamp_by_id.end() && tit->first == (*it)->id && tit->second == (*it)->timestamp)
          attics.push_back(attic_obj);
      }
      else
      {
        // Relation_Delta without a reference of the same index
        std::ostringstream out;
        out<<name_of_type< Object >()<<" "<<(*it)->id.val()<<" cannot be expanded at timestamp "
            <<Timestamp((*it)->timestamp).str()<<".";
        health_guard.log_and_display_error(out.str());
      }
    }
    catch (const std::exception& e)
    {
      std::ostringstream out;
      out<<name_of_type< Object >()<<" "<<(*it)->id.val()<<" cannot be expanded at timestamp "
          <<Timestamp((*it)->timestamp).str()<<": "<<e.what();
      health_guard.log_and_display_error(out.str());
    }
  }

  for (typename std::vector< Attic< Object > >::const_iterator it = attics.begin(); it != attics.end(); ++it)
  {
    if (predicate.match(*it))
      attic_result.push_back(*it);
  }

  for (typename std::vector< Object >::const_iterator it = skels.begin(); it != skels.end(); ++it)
  {
    if (predicate.match(*it))
      result.push_back(*it);
  }
}


template < typename Object >
void filter_items_by_timestamp(
    const std::vector< std::pair< typename Object::Id_Type, uint64 > >& timestamp_by_id,
    std::vector< Object >& result)
{
  typename std::vector< Object >::iterator target_it = result.begin();
  for (typename std::vector< Object >::iterator it2 = result.begin();
        it2 != result.end(); ++it2)
  {
    typename std::vector< std::pair< typename Object::Id_Type, uint64 > >::const_iterator
        tit = std::lower_bound(timestamp_by_id.begin(), timestamp_by_id.end(),
            std::make_pair(it2->id, 0ull));
    if (tit != timestamp_by_id.end() && tit->first == it2->id && tit->second == timestamp_of(*it2))
    {
      *target_it = *it2;
      ++target_it;
    }
  }
  result.erase(target_it, result.end());
}


template < typename Index, typename Object >
void filter_items_by_timestamp(
    const std::vector< std::pair< typename Object::Id_Type, uint64 > >& timestamp_by_id,
    std::map< Index, std::vector< Object > >& result)
{
  for (typename std::map< Index, std::vector< Object > >::iterator it = result.begin();
       it != result.end(); ++it)
    filter_items_by_timestamp(timestamp_by_id, it->second);
}


template< typename Object >
void check_for_duplicated_objects(
    const std::vector< std::pair< typename Object::Id_Type, uint64 > >& timestamp_by_id,
    Health_Guard& health_guard)
{
  // Debug-Feature. Can be disabled once no further bugs appear
  for (typename std::vector< std::pair< typename Object::Id_Type, uint64 > >::size_type i = 0;
      i+1 < timestamp_by_id.size(); ++i)
  {
    if (timestamp_by_id[i].second == timestamp_by_id[i+1].second
      && timestamp_by_id[i].first == timestamp_by_id[i+1].first)
    {
      std::ostringstream out;
      out<<name_of_type< Object >()<<" "<<timestamp_by_id[i].first.val()
          <<" appears multiple times at timestamp "<<Timestamp(timestamp_by_id[i].second).str();
      health_guard.log_and_display_error(out.str());
    }
  }
}


template < typename Index, typename Object, typename Current_Iterator, typename Attic_Iterator, typename Predicate >
bool collect_items_by_timestamp(Health_Guard&& health_guard,
                   Current_Iterator current_begin, Current_Iterator current_end,
                   Attic_Iterator attic_begin, Attic_Iterator attic_end,
                   const Predicate& predicate, Index* cur_idx, uint64 timestamp,
                   std::map< Index, std::vector< Object > >& result,
                   std::map< Index, std::vector< Attic< Object > > >& attic_result)
{
  uint32 count = 0;
  while (!(current_begin == current_end) || !(attic_begin == attic_end))
  {
    std::vector< std::pair< typename Object::Id_Type, uint64 > > timestamp_by_id;

    bool too_much_data = false;
    if (++count >= 128*1024)
    {
      count = 0;
      health_guard.check(0, eval_map(result));
      health_guard.check(0, eval_map(attic_result));
    }
    Index index =
        (attic_begin == attic_end ||
            (!(current_begin == current_end) && current_begin.index() < attic_begin.index())
        ? current_begin.index() : attic_begin.index());
    if (too_much_data && cur_idx)
    {
      *cur_idx = index;
      return true;
    }

    reconstruct_items(
        current_begin, current_end, index, predicate, result[index], timestamp_by_id, timestamp, count);
    reconstruct_items(
        attic_begin, attic_end, index, predicate, attic_result[index], timestamp_by_id, timestamp, count);

    std::sort(timestamp_by_id.begin(), timestamp_by_id.end());

    filter_items_by_timestamp(timestamp_by_id, result[index]);
    filter_items_by_timestamp(timestamp_by_id, attic_result[index]);

    //check_for_duplicated_objects< Object >(timestamp_by_id, rman);
  }
  return false;
}


template < typename Index, typename Current_Iterator, typename Attic_Iterator, typename Predicate >
bool collect_items_by_timestamp(Health_Guard&& health_guard,
                   Current_Iterator current_begin, Current_Iterator current_end,
                   Attic_Iterator attic_begin, Attic_Iterator attic_end,
                   const Predicate& predicate, Index* cur_idx, uint64 timestamp,
                   std::map< Index, std::vector< Relation_Skeleton > >& result,
                   std::map< Index, std::vector< Attic< Relation_Skeleton > > >& attic_result)
{
  uint32 count = 0;
  while (!(current_begin == current_end) || !(attic_begin == attic_end))
  {
    std::vector< std::pair< Relation_Skeleton::Id_Type, uint64 > > timestamp_by_id;

    bool too_much_data = false;
    if (++count >= 128*1024)
    {
      count = 0;
      health_guard.check(0, eval_map(result));
      health_guard.check(0, eval_map(attic_result));
    }
    Index index =
        (attic_begin == attic_end ||
            (!(current_begin == current_end) && current_begin.index() < attic_begin.index())
        ? current_begin.index() : attic_begin.index());
    if (too_much_data && cur_idx)
    {
      *cur_idx = index;
      return true;
    }

    reconstruct_items(health_guard, current_begin, current_end, attic_begin, attic_end, index,
                      predicate, result[index], attic_result[index], timestamp_by_id, timestamp);

    std::sort(timestamp_by_id.begin(), timestamp_by_id.end());

    filter_items_by_timestamp(timestamp_by_id, result[index]);
    filter_items_by_timestamp(timestamp_by_id, attic_result[index]);

    //check_for_duplicated_objects< Relation_Skeleton >(timestamp_by_id, rman);
  }
  return false;
}


template < typename Index, typename Current_Iterator, typename Attic_Iterator, typename Predicate >
bool collect_items_by_timestamp(Health_Guard&& health_guard,
                   Current_Iterator current_begin, Current_Iterator current_end,
                   Attic_Iterator attic_begin, Attic_Iterator attic_end,
                   const Predicate& predicate, Index* cur_idx, uint64 timestamp,
                   std::map< Index, std::vector< Way_Skeleton > >& result,
                   std::map< Index, std::vector< Attic< Way_Skeleton > > >& attic_result)
{
  uint32 count = 0;
  while (!(current_begin == current_end) || !(attic_begin == attic_end))
  {
    std::vector< std::pair< Way_Skeleton::Id_Type, uint64 > > timestamp_by_id;

    bool too_much_data = false;
    if (++count >= 128*1024)
    {
      count = 0;
      health_guard.check(0, eval_map(result));
      health_guard.check(0, eval_map(attic_result));
    }
    Index index =
        (attic_begin == attic_end ||
            (!(current_begin == current_end) && current_begin.index() < attic_begin.index())
        ? current_begin.index() : attic_begin.index());
    if (too_much_data && cur_idx)
    {
      *cur_idx = index;
      return true;
    }

    reconstruct_items(health_guard, current_begin, current_end, attic_begin, attic_end, index,
                      predicate, result[index], attic_result[index], timestamp_by_id, timestamp);

    std::sort(timestamp_by_id.begin(), timestamp_by_id.end());

    filter_items_by_timestamp(timestamp_by_id, result[index]);
    filter_items_by_timestamp(timestamp_by_id, attic_result[index]);

    //check_for_duplicated_objects< Way_Skeleton >(timestamp_by_id, rman);
  }
  return false;
}


// Used by Set_Comparison
template < typename Index, typename Object, typename Container, typename Predicate >
void collect_items_discrete(Request_Context& context,
                   const Container& req, const Predicate& predicate, uint64 timestamp,
                   std::map< Index, std::vector< Object > >& result,
                   std::map< Index, std::vector< Attic< Object > > >& attic_result)
{
  Block_Backend< Index, Object, typename Container::const_iterator > current_db(
      context.data_index(current_skeleton_file_properties< Object >()));
  if (timestamp == NOW)
  {
    for (auto it = current_db.discrete_begin(req.begin(), req.end()); !(it == current_db.discrete_end()); ++it)
    {
      if (predicate.match(it.handle()))
        result[it.index()].push_back(it.object());
    }
  }
  else
  {
    Block_Backend< Index, Attic< typename Object::Delta >, typename Container::const_iterator > attic_db(
        context.data_index(attic_skeleton_file_properties< Object >()));

    collect_items_by_timestamp(context.get_health_guard(),
        current_db.discrete_begin(req.begin(), req.end()), current_db.discrete_end(),
        attic_db.discrete_begin(req.begin(), req.end()), attic_db.discrete_end(),
        predicate, (Index*)0, timestamp, result, attic_result);
  }
}


template < typename Index, typename Object, typename Container, typename Predicate >
Timeless< Index, Object > collect_items_discrete(
    Request_Context& context, const Container& req, const Predicate& predicate)
{
  std::map< Index, std::vector< Object > > current;
  std::map< Index, std::vector< Attic< Object > > > attic;
  collect_items_discrete(context, req, predicate, context.get_desired_timestamp(), current, attic);
  Timeless< Index, Object > result;
  result.swap(current, attic);
  return result;
}


template< typename Index, typename Object, typename Predicate >
Timeless< Index, Object > collect_items_range(Request_Context& context,
    const Ranges< Index >& ranges, const Predicate& predicate)
{
  Timeless< Index, Object > result;
  if (ranges.empty() || !predicate.possible())
    return result;

  std::map< Index, std::vector< Object > > current;
  std::map< Index, std::vector< Attic< Object > > > attic;
  
  Block_Backend< Index, Object > current_db
      (context.data_index(current_skeleton_file_properties< Object >()));
  if (context.get_desired_timestamp() == NOW)
  {
    for (auto it = current_db.range_begin(ranges); !(it == current_db.range_end()); ++it)
    {
      if (predicate.match(it.handle()))
        current[it.index()].push_back(it.object());
    }
  }
  else
  {
    Block_Backend< Index, Attic< typename Object::Delta > > attic_db
        (context.data_index(attic_skeleton_file_properties< Object >()));
    collect_items_by_timestamp(context.get_health_guard(),
        current_db.range_begin(ranges), current_db.range_end(),
        attic_db.range_begin(ranges), attic_db.range_end(),
        predicate, (Index*)0, context.get_desired_timestamp(), current, attic);
  }
  result.swap(current, attic);
  return result;
}


template < typename Index, typename Object, typename Predicate >
bool collect_items_range(Request_Context& context,
    const Ranges< Index >& ranges, const Predicate& predicate, Index& cur_idx,
    std::map< Index, std::vector< Object > >& result,
    std::map< Index, std::vector< Attic< Object > > >& attic_result)
{
  Ranges< Index > shortened = ranges.skip_start(cur_idx);
  Block_Backend< Index, Object > current_db(
      context.data_index(current_skeleton_file_properties< Object >()));

  if (context.get_desired_timestamp() == NOW)
  {
    uint32 count = 0;
    bool too_much_data = false;

    for (auto it = current_db.range_begin(shortened); !(it == current_db.range_end()); ++it)
    {
      if (too_much_data && !(cur_idx == it.index()))
      {
        cur_idx = it.index();
        return true;
      }
      if (++count >= 256*1024)
      {
        count = 0;
        too_much_data = context.get_health_guard().check(0, eval_map(result));
        cur_idx = it.index();
      }
      if (predicate.match(it.handle()))
        result[it.index()].push_back(it.object());
    }

    return false;
  }
  else
  {
    Block_Backend< Index, Attic< typename Object::Delta > > attic_db(
        context.data_index(attic_skeleton_file_properties< Object >()));

    return collect_items_by_timestamp(context.get_health_guard(),
        current_db.range_begin(shortened), current_db.range_end(),
        attic_db.range_begin(shortened), attic_db.range_end(),
        predicate, &cur_idx, context.get_desired_timestamp(), result, attic_result);
  }
}


// Current called olny for areas and only from Query_Statement.
template < typename Index, typename Object, typename Predicate >
void collect_items_flat(Request_Context& context,
		   File_Properties& file_properties, const Predicate& predicate,
		   std::map< Index, std::vector< Object > >& result)
{
  uint32 count = 0;
  Block_Backend< Index, Object > db(context.data_index(&file_properties));
  for (typename Block_Backend< Index, Object >::Flat_Iterator
      it(db.flat_begin()); !(it == db.flat_end()); ++it)
  {
    if (++count >= 256*1024)
    {
      count = 0;
      context.get_health_guard().check(0, eval_map(result));
    }
    if (predicate.match(it.handle()))
      result[it.index()].push_back(it.object());
  }
}


template < typename Index, typename Object, typename Predicate >
Timeless< Index, Object > collect_items_flat(
    Request_Context& context, const Predicate& predicate)
{
  std::map< Index, std::vector< Object > > current;
  std::map< Index, std::vector< Attic< Object > > > attic;

  Block_Backend< Index, Object > current_db
      (context.data_index(current_skeleton_file_properties< Object >()));

  if (context.get_desired_timestamp() == NOW)
  {
    uint32 count = 0;
    for (auto it = current_db.flat_begin(); !(it == current_db.flat_end()); ++it)
    {
      if (++count >= 256*1024)
      {
        count = 0;
        context.get_health_guard().check(0, eval_map(current));
      }
      if (predicate.match(it.handle()))
        current[it.index()].push_back(it.object());
    }
  }
  else
  {
    Block_Backend< Index, Attic< typename Object::Delta > > attic_db
        (context.data_index(attic_skeleton_file_properties< Object >()));

    collect_items_by_timestamp(context.get_health_guard(),
        current_db.flat_begin(), current_db.flat_end(),
        attic_db.flat_begin(), attic_db.flat_end(),
        predicate, (Index*)0, context.get_desired_timestamp(), current, attic);
  }

  Timeless< Index, Object > result;
  result.swap(current, attic);
  return result;
}


#endif
