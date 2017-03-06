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

#ifndef DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H
#define DE__OSM3S___OVERPASS_API__DATA__COLLECT_MEMBERS_H

#include "../core/datatypes.h"
#include "../statements/statement.h"
#include "abstract_processing.h"
#include "filenames.h"

#include <map>
#include <set>
#include <vector>


class Resource_Manager;
class Statement;


std::map< Uint31_Index, std::vector< Relation_Skeleton > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& parents,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* children_ranges = 0,
     const std::vector< Relation::Id_Type >* children_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
    std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& parents,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_parents,
     uint64 timestamp,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* children_ranges = 0,
     const std::vector< Relation::Id_Type >* children_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

std::map< Uint31_Index, std::vector< Way_Skeleton > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* way_ranges = 0,
     const std::vector< Way::Id_Type >* way_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
     uint64 timestamp,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* way_ranges = 0,
     const std::vector< Way::Id_Type >* way_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     uint64 timestamp,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* way_ranges = 0);

std::map< Uint32_Index, std::vector< Node_Skeleton > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const std::vector< Node::Id_Type >* node_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
     uint64 timestamp,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const std::vector< Node::Id_Type >* node_ids = 0, bool invert_ids = false, const uint32* role_id = 0);

std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     uint64 timestamp,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges = 0);

std::map< Uint32_Index, std::vector< Node_Skeleton > > way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const std::vector< Node::Id_Type >* node_ids = 0, bool invert_ids = false);

std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
     const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
     uint64 timestamp,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges = 0,
     const std::vector< Node::Id_Type >* node_ids = 0, bool invert_ids = false);

template< typename Relation_Skeleton >
std::vector< Node::Id_Type > relation_node_member_ids
    (Resource_Manager& rman, const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const uint32* role_id = 0);

template< typename Relation_Skeleton >
std::vector< Node::Id_Type > relation_node_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id = 0);

template< typename Relation_Skeleton >
std::vector< Way::Id_Type > relation_way_member_ids
    (Resource_Manager& rman, const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const uint32* role_id = 0);

template< typename Relation_Skeleton >
std::vector< Way::Id_Type > relation_way_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id = 0);

std::vector< Relation::Id_Type > relation_relation_member_ids
    (Resource_Manager& rman, const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const uint32* role_id = 0);

std::vector< Relation::Id_Type > relation_relation_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id = 0);

std::vector< Node::Id_Type > way_nd_ids(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways);
std::vector< Node::Id_Type > way_nd_ids
    (const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
     const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways);

const std::map< uint32, std::string >& relation_member_roles(Transaction& transaction);
uint32 determine_role_id(Transaction& transaction, const std::string& role);


bool add_way_to_area_blocks(const std::vector< Quad_Coord >& coords,
                            uint32 id, std::map< Uint31_Index, std::vector< Area_Block > >& areas);


std::vector< Quad_Coord > make_geometry(const Way_Skeleton& way, const std::vector< Node >& nodes);


void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
                           const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);
void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways,
                           const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);


template< typename Relation_Skeleton >
void filter_relations_by_ranges(std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
                                const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  std::set< std::pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
  typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::iterator it = relations.begin();
  for (; it != relations.end() && ranges_it != ranges.end(); )
  {
    if (!(it->first < ranges_it->second))
      ++ranges_it;
    else if (!(it->first < ranges_it->first))
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


template < typename TIndex, typename TObject >
void get_elements_by_id_from_db
    (std::map< TIndex, std::vector< TObject > >& elements,
     std::map< TIndex, std::vector< Attic< TObject > > >& attic_elements,
     const std::vector< typename TObject::Id_Type >& ids, bool invert_ids, uint64 timestamp,
     const std::set< std::pair< TIndex, TIndex > >& range_req,
     const Statement& query, Resource_Manager& rman,
     File_Properties& file_prop, File_Properties& attic_file_prop)
{
  elements.clear();
  attic_elements.clear();
  if (ids.empty())
  {
    if (timestamp == NOW)
      collect_items_range(&query, rman, file_prop, range_req,
          Trivial_Predicate< TObject >(), elements);
    else
      collect_items_range_by_timestamp(&query, rman, range_req,
          Trivial_Predicate< TObject >(), elements, attic_elements);
  }
  else if (!invert_ids)
  {
    if (timestamp == NOW)
      collect_items_range(&query, rman, file_prop, range_req,
          Id_Predicate< TObject >(ids), elements);
    else
      collect_items_range_by_timestamp(&query, rman, range_req,
          Id_Predicate< TObject >(ids), elements, attic_elements);
  }
  else if (!range_req.empty())
  {
    if (timestamp == NOW)
      collect_items_range(&query, rman, file_prop, range_req,
          Not_Predicate< TObject, Id_Predicate< TObject > >(Id_Predicate< TObject >(ids)),
          elements);
    else
      collect_items_range_by_timestamp(&query, rman, range_req,
          Not_Predicate< TObject, Id_Predicate< TObject > >(Id_Predicate< TObject >(ids)),
          elements, attic_elements);
  }
  else
  {
    if (timestamp == NOW)
      collect_items_flat(query, rman, file_prop,
          Not_Predicate< TObject, Id_Predicate< TObject > >(Id_Predicate< TObject >(ids)),
          elements);
    else
      collect_items_flat_by_timestamp(query, rman,
          Not_Predicate< TObject, Id_Predicate< TObject > >(Id_Predicate< TObject >(ids)),
          elements, attic_elements);
  }
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
    for (typename Block_Backend< Index, Attic< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = undeleted_db.discrete_begin(idx_set.begin(), idx_set.end());
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
    for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = attic_meta_db.discrete_begin(idx_set.begin(), idx_set.end());
        !(it == attic_meta_db.discrete_end()); ++it)
    {
      typename std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > >::iterator
          tit = timestamp_by_id_by_idx[it.index()].find(it.object().ref);
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

    for (typename Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >,
            typename std::vector< Index >::const_iterator >
        ::Discrete_Iterator
        it = meta_db.discrete_begin(idx_set.begin(), idx_set.end());
        !(it == meta_db.discrete_end()); ++it)
    {
      typename std::map< typename Skeleton::Id_Type, std::pair< uint64, uint64 > >::iterator
          tit = timestamp_by_id_by_idx[it.index()].find(it.object().ref);
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
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels, const uint32* role_id)
{
  std::vector< Node::Id_Type > ids;
  if (role_id)
  {
    for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE, *role_id);
  }
  else
  {
    for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE);
  }

  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


template< typename Relation_Skeleton >
std::vector< Node::Id_Type > relation_node_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id)
{
  std::vector< Node::Id_Type > ids = relation_node_member_ids(rman, rels, role_id);
  if (role_id)
  {
    for (typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
        it(attic_rels.begin()); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE, *role_id);
  }
  else
  {
    for (typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
        it(attic_rels.begin()); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::NODE);
  }

  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


template< typename Relation_Skeleton >
std::vector< Way::Id_Type > relation_way_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels, const uint32* role_id)
{
  std::vector< Way::Id_Type > ids;
  if (role_id)
  {
    for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY, *role_id);
  }
  else
  {
    for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY);
  }

  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


template< typename Relation_Skeleton >
std::vector< Way::Id_Type > relation_way_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id)
{
  std::vector< Way::Id_Type > ids = relation_way_member_ids(rman, rels, role_id);
  if (role_id)
  {
    for (typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
        it(attic_rels.begin()); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY, *role_id);
  }
  else
  {
    for (typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
        it(attic_rels.begin()); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::WAY);
  }

  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


template< typename Index, typename Skeleton >
void keep_matching_skeletons
    (std::map< Index, std::vector< Skeleton > >& current,
     std::map< Index, std::vector< Attic< Skeleton > > >& attic,
     uint64 timestamp)
{
  std::map< typename Skeleton::Id_Type, uint64 > timestamp_by_id;

  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
      timestamp_by_id[it2->id] = NOW;
  }

  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (typename std::vector< Attic<Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      uint64& stored_timestamp = timestamp_by_id[it2->id];
      if (it2->timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > it2->timestamp))
        stored_timestamp = it2->timestamp;
    }
  }

  for (typename std::map< Index, std::vector< Skeleton > >::iterator it = current.begin();
       it != current.end(); ++it)
  {
    std::vector< Skeleton > local_into;
    for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == NOW)
        local_into.push_back(*it2);
    }
    local_into.swap(it->second);
  }

  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    std::vector< Attic< Skeleton > > local_into;
    for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == it2->timestamp)
        local_into.push_back(*it2);
    }
    local_into.swap(it->second);
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
  sort(skels_by_id.begin(), skels_by_id.end(), order_by_id);

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
  sort(skels_by_id.begin(), skels_by_id.end(), order_by_id);

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


std::vector< Uint31_Index > collect_way_req
    (const Statement* stmt, Resource_Manager& rman,
     const std::vector< uint32 >& parents,
     const std::vector< uint32 >& map_ids,
     const std::vector< Uint31_Index >& children_idxs);


template< typename Relation_Skeleton >
std::vector< Uint31_Index > relation_way_member_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end)
{
  std::vector< uint32 > parents;
  std::vector< Uint31_Index > children_idxs;

  for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (typename std::vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
	for (std::vector< Uint31_Index >::const_iterator it3 = it2->way_idxs.begin();
	    it3 != it2->way_idxs.end(); ++it3)
	  children_idxs.push_back(*it3);
      }
    }
    else
      parents.push_back(it->first.val());
  }
  if (stmt)
    rman.health_check(*stmt);
  std::sort(children_idxs.begin(), children_idxs.end());
  children_idxs.erase(std::unique(children_idxs.begin(), children_idxs.end()), children_idxs.end());

  return collect_way_req(stmt, rman, parents, std::vector< uint32 >(), children_idxs);
}


template< typename Relation_Skeleton >
std::vector< Uint31_Index > relation_way_member_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end,
     typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
         attic_rels_begin,
     typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
         attic_rels_end)
{
  std::vector< uint32 > parents;
  std::vector< Uint31_Index > children_idxs;

  for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (typename std::vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->way_idxs.begin();
            it3 != it2->way_idxs.end(); ++it3)
          children_idxs.push_back(*it3);
      }
    }
    else
      parents.push_back(it->first.val());
  }
  for (typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
      it = attic_rels_begin; it != attic_rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (typename std::vector< Attic< Relation_Skeleton > >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->way_idxs.begin();
            it3 != it2->way_idxs.end(); ++it3)
          children_idxs.push_back(*it3);
      }
    }
    else
      parents.push_back(it->first.val());
  }
  if (stmt)
    rman.health_check(*stmt);
  std::sort(children_idxs.begin(), children_idxs.end());
  children_idxs.erase(std::unique(children_idxs.begin(), children_idxs.end()), children_idxs.end());

  return collect_way_req(stmt, rman, parents, std::vector< uint32 >(), children_idxs);
}


std::vector< Uint31_Index > relation_relation_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end);


std::vector< Uint31_Index > relation_relation_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator attic_rels_begin,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator attic_rels_end);


std::set< std::pair< Uint32_Index, Uint32_Index > > collect_node_req
    (const Statement* stmt, Resource_Manager& rman,
     const std::vector< Node::Id_Type >& map_ids, const std::vector< uint32 >& parents);


template< typename Relation_Skeleton >
std::set< std::pair< Uint32_Index, Uint32_Index > > relation_node_member_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end)
{
  std::vector< uint32 > parents;

  for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the node indexes from the segement indexes
      for (typename std::vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
	for (std::vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
	    it3 != it2->node_idxs.end(); ++it3)
	  parents.push_back(it3->val());
      }
    }
    else
      parents.push_back(it->first.val());
  }
  if (stmt)
    rman.health_check(*stmt);

  return collect_node_req(stmt, rman, std::vector< Node::Id_Type >(), parents);
}


template< typename Relation_Skeleton >
std::set< std::pair< Uint32_Index, Uint32_Index > > relation_node_member_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end,
     typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
         attic_rels_begin,
     typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
         attic_rels_end)
{
  std::vector< uint32 > parents;

  for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the node indexes from the segement indexes
      for (typename std::vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
            it3 != it2->node_idxs.end(); ++it3)
          parents.push_back(it3->val());
      }
    }
    else
      parents.push_back(it->first.val());
  }
  for (typename std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
      it = attic_rels_begin; it != attic_rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the node indexes from the segement indexes
      for (typename std::vector< Attic< Relation_Skeleton > >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
            it3 != it2->node_idxs.end(); ++it3)
          parents.push_back(it3->val());
      }
    }
    else
      parents.push_back(it->first.val());
  }
  if (stmt)
    rman.health_check(*stmt);

  return collect_node_req(stmt, rman, std::vector< Node::Id_Type >(), parents);
}


std::set< std::pair< Uint32_Index, Uint32_Index > > way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_begin,
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_end);


std::set< std::pair< Uint32_Index, Uint32_Index > > way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_begin,
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_end,
     std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_ways_begin,
     std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_ways_end);


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

#endif
