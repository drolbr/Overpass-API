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

#ifndef DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H
#define DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H

#include "../core/datatypes.h"
#include "../statements/statement.h"
#include "abstract_processing.h"
#include "collect_items.h"
#include "filenames.h"

#include <map>
#include <set>
#include <vector>


class Resource_Manager;
class Statement;


Timeless< Uint31_Index, Relation_Skeleton > relation_relation_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& parents,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_parents,
     const Ranges< Uint31_Index >& children_ranges,
     const std::vector< Relation::Id_Type >& children_ids, bool invert_ids, const uint32* role_id = 0);

Timeless< Uint31_Index, Way_Skeleton > relation_way_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
     const Ranges< Uint31_Index >& way_ranges,
     const std::vector< Way::Id_Type >& way_ids, bool invert_ids, const uint32* role_id = 0);

std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > relation_way_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     const Ranges< Uint31_Index >& way_ranges);

Timeless< Uint32_Index, Node_Skeleton > relation_node_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
     const Ranges< Uint32_Index >& node_ranges,
     const std::vector< Node::Id_Type >& node_ids, bool invert_ids = false, const uint32* role_id = 0);

std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > relation_node_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     const Ranges< Uint32_Index >& node_ranges);

Timeless< Uint32_Index, Node_Skeleton > way_members(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    const std::vector< int >* pos,
    const Ranges< Uint32_Index >& node_ranges,
    const std::vector< Node::Id_Type >& node_ids, bool invert_ids = false);

Timeless< Uint32_Index, Node_Skeleton > way_cnt_members(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit,
    const Ranges< Uint32_Index >& node_ranges,
    const std::vector< Node::Id_Type >& node_ids, bool invert_ids = false);

Timeless< Uint32_Index, Node_Skeleton > way_link_members(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit,
    const Ranges< Uint32_Index >& node_ranges,
    const std::vector< Node::Id_Type >& node_ids, bool invert_ids = false);

template< typename Relation_Skeleton >
std::vector< Node::Id_Type > relation_node_member_ids
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id = 0);

std::vector< Relation::Id_Type > relation_relation_member_ids(
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
    const uint32* role_id = 0);

std::vector< Node::Id_Type > way_nd_ids(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    const std::vector< int >* pos);

std::vector< Node::Id_Type > way_cnt_nd_ids(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit);

std::vector< Node::Id_Type > way_link_nd_ids(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit);

const std::map< uint32, std::string >& relation_member_roles(Transaction& transaction);
uint32 determine_role_id(Transaction& transaction, const std::string& role);


bool add_way_to_area_blocks(const std::vector< Quad_Coord >& coords,
                            uint32 id, std::map< Uint31_Index, std::vector< Area_Block > >& areas);


std::vector< Quad_Coord > make_geometry(const Way_Skeleton& way, const std::vector< Node >& nodes);


void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
                           const Ranges< Uint31_Index >& ranges);
void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways,
                           const Ranges< Uint31_Index >& ranges);


template< typename Relation_Skeleton >
void filter_relations_by_ranges(
    std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations, const Ranges< Uint31_Index >& ranges)
{
  auto ranges_it = ranges.begin();
  typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::iterator it = relations.begin();
  for (; it != relations.end() && ranges_it != ranges.end(); )
  {
    if (!(it->first < ranges_it.upper_bound()))
      ++ranges_it;
    else if (!(it->first < ranges_it.lower_bound()))
      ++it;
    else
    {
      it->second.clear();
      ++it;
    }
  }
  for (; it != relations.end(); ++it)
    it->second.clear();
}


//-----------------------------------------------------------------------------


template < typename Index, typename Skeleton >
Timeless< Index, Skeleton > get_elements_from_db(
    const Ranges< Index >& ranges, const Statement& query, Resource_Manager& rman)
{
  Request_Context context(&query, rman);
  return collect_items_range< Index, Skeleton >(context, ranges, Trivial_Predicate< Skeleton >());
}


template < typename Index, typename Object >
class Collect_Items
{
public:
  Collect_Items(
      const std::vector< typename Object::Id_Type >& ids_, bool invert_ids_,
      const Ranges< Index >& ranges_,
      const Statement& query_, Resource_Manager& rman_)
      : ids(&ids_), invert_ids(invert_ids_), ranges(ranges_), query(&query_), rman(&rman_),
      min_idx(ranges_.empty() ? Index() : ranges_.begin().lower_bound()) {}

  bool get_chunk(
      std::map< Index, std::vector< Object > >& elements,
      std::map< Index, std::vector< Attic< Object > > >& attic_elements);

private:
  const std::vector< typename Object::Id_Type >* ids;
  bool invert_ids;
  Ranges< Index > ranges;
  const Statement* query;
  Resource_Manager* rman;
  Index min_idx;
};


template < typename Index, typename Object, typename Predicate >
bool get_elements_by_id_from_db_generic(
    std::map< Index, std::vector< Object > >& elements,
    std::map< Index, std::vector< Attic< Object > > >& attic_elements,
    const Predicate& pred,
    const Ranges< Index >& ranges, Index& cur_idx,
    const Statement& query, Resource_Manager& rman)
{
  if (ranges.empty())
    return false;
  Request_Context context(&query, rman);
  return collect_items_range(context, ranges, pred, cur_idx, elements, attic_elements);
}


template < typename Index, typename Object >
bool Collect_Items< Index, Object >::get_chunk(
    std::map< Index, std::vector< Object > >& elements,
    std::map< Index, std::vector< Attic< Object > > >& attic_elements)
{
  elements.clear();
  attic_elements.clear();
  if (invert_ids)
  {
    if (ids->empty())
      return get_elements_by_id_from_db_generic(
          elements, attic_elements, Trivial_Predicate< Object >(), ranges, min_idx, *query, *rman);
    else
      return get_elements_by_id_from_db_generic(
          elements, attic_elements, Not_Predicate< Object, Id_Predicate< Object > >(Id_Predicate< Object >(*ids)),
          ranges, min_idx, *query, *rman);
  }
  return get_elements_by_id_from_db_generic(
      elements, attic_elements, Id_Predicate< Object >(*ids), ranges, min_idx, *query, *rman);
}


template< typename Index, typename Skeleton >
void filter_attic_elements
    (Resource_Manager& rman, uint64 timestamp,
     std::map< Index, std::vector< Skeleton > >& current,
     std::map< Index, std::vector< Attic< Skeleton > > >& attic)
{
  if (timestamp != NOW)
  {
    std::vector< Index > idx_set;
    for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current.begin();
         it != current.end(); ++it)
      idx_set.push_back(it->first);
    for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
         it != attic.end(); ++it)
      idx_set.push_back(it->first);
    std::sort(idx_set.begin(), idx_set.end());
    idx_set.erase(std::unique(idx_set.begin(), idx_set.end()), idx_set.end());

    // Remove elements that have been deleted at the given point of time
    std::map< Index, std::vector< typename Skeleton::Id_Type > > deleted_items;

    Block_Backend< Index, Attic< typename Skeleton::Id_Type >, typename std::vector< Index >::const_iterator >
        undeleted_db(rman.get_transaction()->data_index(attic_undeleted_file_properties< Skeleton >()));
    for (auto it = undeleted_db.discrete_begin(idx_set.begin(), idx_set.end());
        !(it == undeleted_db.discrete_end()); ++it)
    {
      if (it.object().timestamp <= timestamp)
        continue;

      typename std::map< Index, std::vector< Skeleton > >::iterator cit = current.find(it.index());
      if (cit != current.end())
      {
        for (typename std::vector< Skeleton >::iterator it2 = cit->second.begin(); it2 != cit->second.end(); )
        {
          if (it2->id == it.object())
          {
            *it2 = cit->second.back();
            cit->second.pop_back();
          }
          else
            ++it2;
        }
      }

      typename std::map< Index, std::vector< Attic< Skeleton > > >::iterator ait = attic.find(it.index());
      if (ait != attic.end())
      {
        for (typename std::vector< Attic< Skeleton > >::iterator it2 = ait->second.begin();
             it2 != ait->second.end(); )
        {
          if (it2->id == it.object() && it.object().timestamp < it2->timestamp)
          {
            *it2 = ait->second.back();
            ait->second.pop_back();
          }
          else
            ++it2;
        }
      }
    }


    // Confirm elements that are backed by meta data
    // Update element's expiration timestamp if a meta exists that is older than the current
    // expiration date and younger than timestamp
    std::map< Index, std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > > >
        timestamp_by_id_by_idx;
    for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current.begin();
         it != current.end(); ++it)
    {
      std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > >& entry
          = timestamp_by_id_by_idx[it->first];
      for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
        entry[it2->id] = std::make_pair(0, NOW);
    }
    for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
         it != attic.end(); ++it)
    {
      std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > >& entry
          = timestamp_by_id_by_idx[it->first];
      for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
        entry[it2->id] = std::make_pair(0, it2->timestamp);
    }

    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        attic_meta_db(rman.get_transaction()->data_index
          (attic_meta_file_properties< Skeleton >()));
    for (auto it = attic_meta_db.discrete_begin(idx_set.begin(), idx_set.end());
        !(it == attic_meta_db.discrete_end()); ++it)
    {
      auto tit = timestamp_by_id_by_idx[it.index()].find(it.object().ref);
      if (tit != timestamp_by_id_by_idx[it.index()].end())
      {
        if (timestamp < it.object().timestamp)
          tit->second.second = std::min(tit->second.second, it.object().timestamp);
        else
          tit->second.first = std::max(tit->second.first, it.object().timestamp);
      }
    }

    // Same thing with current meta data
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        meta_db(rman.get_transaction()->data_index
          (current_meta_file_properties< Skeleton >()));

    for (auto it = meta_db.discrete_begin(idx_set.begin(), idx_set.end());
        !(it == meta_db.discrete_end()); ++it)
    {
      auto tit = timestamp_by_id_by_idx[it.index()].find(it.object().ref);
      if (tit != timestamp_by_id_by_idx[it.index()].end())
      {
        if (timestamp < it.object().timestamp)
          tit->second.second = std::min(tit->second.second, it.object().timestamp);
        else
          tit->second.first = std::max(tit->second.first, it.object().timestamp);
      }
    }

    // Filter current: only keep elements that have already existed at timestamp
    for (typename std::map< Index, std::vector< Skeleton > >::iterator it = current.begin();
         it != current.end(); ++it)
    {
      std::vector< Skeleton > result;
      std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > >& entry
          = timestamp_by_id_by_idx[it->first];

      for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
      {
        if (entry[it2->id].first > 0)
        {
          if (entry[it2->id].second == NOW)
            result.push_back(*it2);
          else
            attic[it->first].push_back(Attic< Skeleton >(*it2, entry[it2->id].second));
        }
      }

      result.swap(it->second);
    }

    // Filter attic: only keep elements that have already existed at timestamp
    for (typename std::map< Index, std::vector< Attic< Skeleton > > >::iterator it = attic.begin();
         it != attic.end(); ++it)
    {
      std::vector< Attic< Skeleton > > result;
      std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > >& entry
          = timestamp_by_id_by_idx[it->first];

      for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
      {
        if (entry[it2->id].first > 0)
        {
          result.push_back(*it2);
          result.back().timestamp = entry[it2->id].second;
        }
      }

      result.swap(it->second);
    }
  }
}


//-----------------------------------------------------------------------------


template< typename Ref_Type, typename Relation_Skeleton >
void filter_for_member_ids(const std::vector< Relation_Skeleton >& relations,
                           std::vector< Ref_Type >& ids, uint32 type)
{
  for (typename std::vector< Relation_Skeleton >::const_iterator it2(relations.begin());
      it2 != relations.end(); ++it2)
  {
    for (std::vector< Relation_Entry >::const_iterator it3(it2->members.begin());
        it3 != it2->members.end(); ++it3)
    {
      if (it3->type == type)
        ids.push_back(Ref_Type(it3->ref.val()));
    }
  }
}


template< typename Ref_Type, typename Relation_Skeleton >
void filter_for_member_ids(const std::vector< Relation_Skeleton >& relations,
                           std::vector< Ref_Type >& ids, uint32 type, uint32 role_id)
{
  for (typename std::vector< Relation_Skeleton >::const_iterator it2(relations.begin());
      it2 != relations.end(); ++it2)
  {
    for (std::vector< Relation_Entry >::const_iterator it3(it2->members.begin());
        it3 != it2->members.end(); ++it3)
    {
      if (it3->type == type && it3->role == role_id)
        ids.push_back(Ref_Type(it3->ref.val()));
    }
  }
}


template< typename Relation_Skeleton >
std::vector< Node::Id_Type > relation_node_member_ids
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id)
{
  std::vector< Node::Id_Type > ids;
  if (role_id)
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE, *role_id);
  }
  else
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE);
  }

  if (role_id)
  {
    for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE, *role_id);
  }
  else
  {
    for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE);
  }

  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


inline std::vector< Way::Id_Type > relation_way_member_ids
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id = 0)
{
  std::vector< Way::Id_Type > ids;
  if (role_id)
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY, *role_id);
  }
  else
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY);
  }

  if (role_id)
  {
    for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY, *role_id);
  }
  else
  {
    for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY);
  }

  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


template< typename Index, typename Skeleton >
void keep_matching_skeletons
    (std::map< Index, std::vector< Attic< Skeleton > > >& result,
     const std::map< Index, std::vector< Skeleton > >& current,
     const std::map< Index, std::vector< Attic< Skeleton > > >& attic,
     uint64 timestamp)
{
  std::map< typename Skeleton::Id_Type, uint64 > timestamp_by_id;

  result.clear();

  for (auto it = current.begin(); it != current.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      timestamp_by_id[it2->id] = NOW;
  }

  for (auto it = attic.begin(); it != attic.end(); ++it)
  {
    for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      uint64& stored_timestamp = timestamp_by_id[it2->id];
      if (it2->timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > it2->timestamp))
        stored_timestamp = it2->timestamp;
    }
  }

  for (auto it = current.begin(); it != current.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == NOW)
        result[it->first].push_back(Attic< Skeleton >(*it2, NOW));
    }
  }

  for (auto it = attic.begin(); it != attic.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == it2->timestamp)
        result[it->first].push_back(Attic< Skeleton >(*it2, it2->timestamp));
    }
  }
}


void keep_matching_skeletons
    (std::vector< Node >& result,
     const std::map< Uint32_Index, std::vector< Node_Skeleton > >& current,
     const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic,
     uint64 timestamp);


template< typename TIndex, typename TObject >
void item_filter_map
    (std::map< TIndex, std::vector< TObject > >& modify,
     const std::map< TIndex, std::vector< TObject > >& read)
{
  for (typename std::map< TIndex, std::vector< TObject > >::iterator it = modify.begin();
      it != modify.end(); ++it)
  {
    sort(it->second.begin(), it->second.end());
    typename std::map< TIndex, std::vector< TObject > >::const_iterator
        from_it = read.find(it->first);
    if (from_it == read.end())
    {
      it->second.clear();
      continue;
    }
    std::vector< TObject > local_into;
    for (typename std::vector< TObject >::const_iterator iit = from_it->second.begin();
        iit != from_it->second.end(); ++iit)
    {
      if (std::binary_search(it->second.begin(), it->second.end(), *iit))
        local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename First, typename Second >
void swap_components(std::pair< First, Second > pair, First& first, Second& second)
{
  first.swap(pair.first);
  second.swap(pair.second);
}


template< typename Index, typename Skeleton, typename Order_By_Id >
std::vector< std::pair< Index, const Skeleton* > > order_by_id
    (const std::map< Index, std::vector< Skeleton > >& skels,
     const Order_By_Id& order_by_id)
{
  std::vector< std::pair< Index, const Skeleton* > > skels_by_id;
  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = skels.begin();
      it != skels.end(); ++it)
  {
    for (typename std::vector< Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      skels_by_id.push_back(std::make_pair(it->first, &*iit));
  }
  std::sort(skels_by_id.begin(), skels_by_id.end(), order_by_id);

  return skels_by_id;
}


template< typename Index, typename Skeleton, typename Order_By_Id >
std::vector< std::pair< Index, const Skeleton* > > order_attic_by_id
    (const std::map< Index, std::vector< Attic< Skeleton > > >& skels,
     const Order_By_Id& order_by_id)
{
  std::vector< std::pair< Index, const Skeleton* > > skels_by_id;
  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = skels.begin();
      it != skels.end(); ++it)
  {
    for (typename std::vector< Attic< Skeleton > >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      skels_by_id.push_back(std::make_pair(it->first, &*iit));
  }
  std::sort(skels_by_id.begin(), skels_by_id.end(), order_by_id);

  return skels_by_id;
}


template< typename Relation_Skeleton >
void filter_relations_expensive(const std::vector< std::pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id,
				const std::vector< std::pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id,
				std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations)
{
  for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::iterator it = relations.begin();
      it != relations.end(); ++it)
  {
    std::vector< Relation_Skeleton > local_into;
    for (typename std::vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      for (std::vector< Relation_Entry >::const_iterator nit = iit->members.begin();
          nit != iit->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE)
        {
          const std::pair< Uint32_Index, const Node_Skeleton* >* second_nd =
              binary_search_for_pair_id(node_members_by_id, nit->ref);
          if (second_nd)
          {
            local_into.push_back(*iit);
            break;
          }
        }
        else if (nit->type == Relation_Entry::WAY)
        {
          const std::pair< Uint31_Index, const Way_Skeleton* >* second_nd =
              binary_search_for_pair_id(way_members_by_id, nit->ref32());
          if (second_nd)
          {
            local_into.push_back(*iit);
            break;
          }
        }
      }
    }
    it->second.swap(local_into);
  }
}


Ranges< Uint31_Index > collect_way_req(
    const std::vector< Uint31_Index >& parents,
    const std::vector< Uint31_Index >& children_idxs);


Ranges< Uint31_Index > relation_way_member_indices
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& current_rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels);


Ranges< Uint32_Index > calc_node_children_ranges(const std::vector< uint32 >& way_rel_idxs);


Ranges< Uint32_Index > relation_node_member_indices(
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& current_rels,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels);


Ranges< Uint32_Index > way_nd_indices(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways);


Ranges< Uint32_Index > way_covered_indices(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways);


struct Order_By_Node_Id
{
  bool operator() (const std::pair< Uint32_Index, const Node_Skeleton* >& a,
		   const std::pair< Uint32_Index, const Node_Skeleton* >& b)
  {
    return (a.second->id < b.second->id);
  }
};


struct Order_By_Way_Id
{
  bool operator() (const std::pair< Uint31_Index, const Way_Skeleton* >& a,
		   const std::pair< Uint31_Index, const Way_Skeleton* >& b)
  {
    return (a.second->id < b.second->id);
  }
};


template< typename Index, typename Object, typename Id_Type = Relation_Entry::Ref_Type >
std::vector< Id_Type > extract_ids(const std::map< Index, std::vector< Object > >& elems)
{
  std::vector< Id_Type > ids;

  for (auto it = elems.begin(); it != elems.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      ids.push_back(Id_Type(it2->id.val()));
  }

  std::sort(ids.begin(), ids.end());

  return ids;
}


template< typename Index, typename Object, typename Id_Type = Relation_Entry::Ref_Type >
std::vector< Id_Type > extract_ids(
    const std::map< Index, std::vector< Object > >& current,
    const std::map< Index, std::vector< Attic< Object > > >& attic)
{
  std::vector< Id_Type > ids;

  for (auto it = current.begin(); it != current.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      ids.push_back(Id_Type(it2->id.val()));
  }
  for (auto it = attic.begin(); it != attic.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      ids.push_back(Id_Type(it2->id.val()));
  }

  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


template< class TIndex, class TObject >
std::set< Uint31_Index > extract_parent_indices(const std::map< TIndex, std::vector< TObject > >& elems)
{
  std::vector< uint32 > children;
  {
    for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
        it(elems.begin()); it != elems.end(); ++it)
      children.push_back(it->first.val());
  }

  std::vector< uint32 > parents = calc_parents(children);

  std::set< Uint31_Index > req;
  for (std::vector< uint32 >::const_iterator it = parents.begin(); it != parents.end(); ++it)
    req.insert(Uint31_Index(*it));

  return req;
}


Timeless< Uint31_Index, Way_Skeleton > collect_ways(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
    const Ranges< Uint31_Index >& ranges,
    const std::vector< Way::Id_Type >& ids, bool invert_ids,
    uint32* role_id = 0);


Timeless< Uint31_Index, Way_Skeleton > collect_ways(
    Request_Context& context,
    const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
    const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_nodes,
    const std::vector< int >* pos,
    const std::vector< Way::Id_Type >& ids, bool invert_ids);


void add_nw_member_objects(Request_Context& context, const Set& input_set, Set& into,
    const Ranges< Uint32_Index >* ranges_32 = 0, const Ranges< Uint31_Index >* ranges_31 = 0);


template< typename Index, typename Object >
void filter_elems_for_closed_ways(std::map< Index, std::vector< Object > >& arg)
{
  for (typename std::map< Index, std::vector< Object > >::iterator it1 = arg.begin(); it1 != arg.end(); ++it1)
  {
    std::vector< Object > into;
    for (typename std::vector< Object >::iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
    {
      if (!it2->nds.empty() && it2->nds.front() == it2->nds.back())
        into.push_back(*it2);
    }
    into.swap(it1->second);
  }
}


#endif
