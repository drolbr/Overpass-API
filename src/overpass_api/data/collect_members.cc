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

#include "abstract_processing.h"
#include "collect_members.h"
#include "utils.h"

#include <cstdint>

//-----------------------------------------------------------------------------

template< typename Container >
void way_nd_ids_pos(
    const Container& ways, const std::vector< int >& pos, std::vector< Node::Id_Type >& ids)
{
  for (const auto& i : ways)
  {
    for (const auto& j : i.second)
    {
      auto it3 = pos.begin();
      while (it3 != pos.end() && *it3 < 0)
      {
        if (*it3 + (int)j.nds.size() >= 0)
          ids.push_back(j.nds[*it3 + j.nds.size()]);
        ++it3;
      }
      while (it3 != pos.end() && *it3 == 0)
        ++it3;
      while(it3 != pos.end())
      {
        if (*it3 < (int)j.nds.size()+1)
          ids.push_back(j.nds[*it3-1]);
        ++it3;
      }
    }
  }
}


template< typename Container >
void way_nd_ids_plain(const Container& ways, std::vector< Node::Id_Type >& ids)
{
  for (const auto& i : ways)
  {
    for (const auto& j : i.second)
    {
      for (auto k : j.nds)
        ids.push_back(k);
    }
  }
}


template< typename Container >
void way_nd_ids_once(const Container& ways, std::vector< Node::Id_Type >& ids)
{
  for (const auto& i : ways)
  {
    for (const auto& j : i.second)
    {
      auto size = ids.size();
      for (auto k : j.nds)
        ids.push_back(k);
      std::sort(ids.begin()+size, ids.end());
      ids.erase(std::unique(ids.begin()+size, ids.end()), ids.end());
    }
  }
}


template< typename Container >
void way_nd_firstlast(const Container& ways, std::vector< Node::Id_Type >& ids)
{
  for (const auto& i : ways)
  {
    for (const auto& j : i.second)
    {
      if (j.nds.empty())
        continue;
      ids.push_back(j.nds.front());
      ids.push_back(j.nds.back());
    }
  }
}


std::vector< Node::Id_Type > way_nd_ids(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    const std::vector< int >* pos)
{
  std::vector< Node::Id_Type > ids;
  if (pos)
    way_nd_ids_pos(ways, *pos, ids);
  else
    way_nd_ids_plain(ways, ids);

  if (!attic_ways.empty())
  {
    if (pos)
      way_nd_ids_pos(attic_ways, *pos, ids);
    else
      way_nd_ids_plain(attic_ways, ids);
  }

  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

  return ids;
}


std::vector< Node::Id_Type > way_cnt_nd_ids(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit)
{
  std::vector< Node::Id_Type > ids;
  way_nd_ids_once(ways, ids);
  if (!attic_ways.empty())
    way_nd_ids_once(attic_ways, ids);

  std::sort(ids.begin(), ids.end());

  decltype(ids.size()) lower = 0;
  decltype(ids.size()) upper = 0;
  decltype(ids.size()) target = 0;
  while (lower < ids.size())
  {
    while (upper < ids.size() && ids[lower] == ids[upper])
      ++upper;
    if (lower_limit <= upper - lower && upper - lower <= upper_limit)
      ids[target++] = ids[lower];
    lower = upper;
  }
  ids.resize(target);

  return ids;
}


std::vector< Node::Id_Type > way_link_nd_ids(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit)
{
  std::vector< Node::Id_Type > ids;
  way_nd_ids_plain(ways, ids);
  if (!attic_ways.empty())
    way_nd_ids_plain(attic_ways, ids);

  std::sort(ids.begin(), ids.end());

  std::vector< Node::Id_Type > fl_ids;
  way_nd_firstlast(ways, fl_ids);
  if (!attic_ways.empty())
    way_nd_firstlast(attic_ways, fl_ids);

  std::sort(fl_ids.begin(), fl_ids.end());

  decltype(ids.size()) lower = 0;
  decltype(ids.size()) upper = 0;
  decltype(fl_ids.size()) fl_lower = 0;
  decltype(fl_ids.size()) fl_upper = 0;
  decltype(ids.size()) target = 0;
  while (lower < ids.size())
  {
    while (upper < ids.size() && ids[lower] == ids[upper])
      ++upper;
    while (fl_lower < fl_ids.size() && fl_ids[fl_lower] < ids[lower])
      ++fl_lower;
    fl_upper = fl_lower;
    while (fl_upper < fl_ids.size() && fl_ids[fl_upper] == ids[lower])
      ++fl_upper;
    if (lower_limit <= 2*(upper - lower) + fl_lower - fl_upper
        && 2*(upper - lower) + fl_lower - fl_upper <= upper_limit)
      ids[target++] = ids[lower];
    lower = upper;
    fl_lower = fl_upper;
  }
  ids.resize(target);

  return ids;
}


Ranges< Uint32_Index > calc_node_children_ranges(const std::vector< uint32 >& way_rel_idxs)
{
  Ranges< Uint32_Index > result;

  for (std::vector< uint32 >::const_iterator it = way_rel_idxs.begin();
      it != way_rel_idxs.end(); ++it)
  {
    if (*it & 0x80000000)
    {
      uint32 lat = 0;
      uint32 lon = 0;
      uint32 lat_u = 0;
      uint32 lon_u = 0;
      uint32 offset = 0;

      if (*it & 0x00000001)
      {
	lat = upper_ilat(*it & 0x2aaaaaa8);
	lon = upper_ilon(*it & 0x55555554);
	offset = 2;
      }
      else if (*it & 0x00000002)
      {
	lat = upper_ilat(*it & 0x2aaaaa80);
	lon = upper_ilon(*it & 0x55555540);
	offset = 8;
      }
      else if (*it & 0x00000004)
      {
	lat = upper_ilat(*it & 0x2aaaa800);
	lon = upper_ilon(*it & 0x55555400);
	offset = 0x20;
      }
      else if (*it & 0x00000008)
      {
	lat = upper_ilat(*it & 0x2aaa8000);
	lon = upper_ilon(*it & 0x55554000);
	offset = 0x80;
      }
      else if (*it & 0x00000010)
      {
	lat = upper_ilat(*it & 0x2aa80000);
	lon = upper_ilon(*it & 0x55540000);
	offset = 0x200;
      }
      else if (*it & 0x00000020)
      {
	lat = upper_ilat(*it & 0x2a800000);
	lon = upper_ilon(*it & 0x55400000);
	offset = 0x800;
      }
      else if (*it & 0x00000040)
      {
	lat = upper_ilat(*it & 0x28000000);
	lon = upper_ilon(*it & 0x54000000);
	offset = 0x2000;
      }
      else // *it == 0x80000080
      {
	lat = 0;
	lon = 0;
	offset = 0x8000;
      }

      result.push_back(ll_upper(lat<<16, lon<<16), ll_upper((lat+offset-1)<<16, (lon+offset-1)<<16)+1);
      result.push_back(ll_upper(lat<<16, (lon+offset)<<16), ll_upper((lat+offset-1)<<16, (lon+2*offset-1)<<16)+1);
      result.push_back(ll_upper((lat+offset)<<16, lon<<16), ll_upper((lat+2*offset-1)<<16, (lon+offset-1)<<16)+1);
      result.push_back(
          ll_upper((lat+offset)<<16, (lon+offset)<<16), ll_upper((lat+2*offset-1)<<16, (lon+2*offset-1)<<16)+1);
      for (uint32 i = lat; i <= lat_u; ++i)
      {
        for (uint32 j = lon; j <= lon_u; ++j)
          result.push_back(ll_upper(i<<16, j<<16), ll_upper(i<<16, j<<16)+1);
      }
    }
    else
      result.push_back(*it, (*it) + 1);
  }
  result.sort();

  return result;
}


Ranges< Uint32_Index > relation_node_member_indices(
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& current_rels,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels)
{
  std::vector< uint32 > parents;

  for (auto it = current_rels.begin(); it != current_rels.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the node indexes from the segement indexes
      for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
            it3 != it2->node_idxs.end(); ++it3)
          parents.push_back(it3->val());
      }
    }
    else
      parents.push_back(it->first.val());
  }
  for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the node indexes from the segement indexes
      for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
            it3 != it2->node_idxs.end(); ++it3)
          parents.push_back(it3->val());
      }
    }
    else
      parents.push_back(it->first.val());
  }

  return calc_node_children_ranges(parents);
}


Ranges< Uint31_Index > relation_way_member_indices
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& current_rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels)
{
  std::vector< Uint31_Index > parents;
  std::vector< Uint31_Index > children_idxs;

  for (auto it = current_rels.begin(); it != current_rels.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->way_idxs.begin();
            it3 != it2->way_idxs.end(); ++it3)
          children_idxs.push_back(*it3);
      }
    }
    else
      parents.push_back(it->first);
  }
  for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        for (std::vector< Uint31_Index >::const_iterator it3 = it2->way_idxs.begin();
            it3 != it2->way_idxs.end(); ++it3)
          children_idxs.push_back(*it3);
      }
    }
    else
      parents.push_back(it->first);
  }

  std::sort(children_idxs.begin(), children_idxs.end());
  children_idxs.erase(std::unique(children_idxs.begin(), children_idxs.end()), children_idxs.end());

  return collect_way_req(parents, children_idxs);
}


Ranges< Uint31_Index > uint31_to_ranges(const std::vector< Uint31_Index >& arg)
{
  Ranges< Uint31_Index > result;
  for (auto i : arg)
    result.push_back(i, inc(i));
  return result;
}


Ranges< Uint31_Index > collect_way_req(
    const std::vector< Uint31_Index >& parents, const std::vector< Uint31_Index >& children_idxs)
{
  Ranges< Uint31_Index > req = calc_children_(parents);

  for (std::vector< Uint31_Index >::const_iterator it = children_idxs.begin();
      it != children_idxs.end(); ++it)
    req.push_back(*it, inc(*it));

  req.sort();
  return req;
}


template< typename Index, typename Skeleton >
std::vector< Index > get_indexes(
    Request_Context& context, const std::vector< typename Skeleton::Id_Type >& ids)
{
  std::vector< Index > result;

  Random_File< typename Skeleton::Id_Type, Index > current(
      context.random_index(current_skeleton_file_properties< Skeleton >()));
  for (auto it = ids.begin(); it != ids.end(); ++it)
    result.push_back(current.get(it->val()));

  if (context.get_desired_timestamp() != NOW)
  {
    Random_File< typename Skeleton::Id_Type, Index > attic_random(
        context.random_index(attic_skeleton_file_properties< Skeleton >()));
    std::vector< typename Skeleton::Id_Type > idx_list_ids;
    for (auto it = ids.begin(); it != ids.end(); ++it)
    {
      if (attic_random.get(it->val()).val() == 0)
        ;
      else if (attic_random.get(it->val()) == 0xff)
        idx_list_ids.push_back(it->val());
      else
        result.push_back(attic_random.get(it->val()));
    }

    Block_Backend< typename Skeleton::Id_Type, Index > idx_list_db
        (context.data_index(attic_idx_list_properties< Skeleton >()));
    for (auto it = idx_list_db.discrete_begin(idx_list_ids.begin(), idx_list_ids.end());
        !(it == idx_list_db.discrete_end()); ++it)
      result.push_back(it.object());
  }

  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  return result;
}


Ranges< Uint31_Index > collect_relation_req(
    Request_Context& context, const std::vector< Relation::Id_Type >& map_ids)
{
  std::vector< Uint31_Index > req;

  Random_File< Relation_Skeleton::Id_Type, Uint31_Index > random(
      context.random_index(osm_base_settings().RELATIONS));
  for (std::vector< Relation::Id_Type >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(it->val()));

  std::sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());

  return uint31_to_ranges(req);
}


Ranges< Uint31_Index > collect_attic_relation_req(
    Request_Context& context, const std::vector< Relation::Id_Type >& map_ids)
{
  std::vector< Uint31_Index > req = get_indexes< Uint31_Index, Relation_Skeleton >(context, map_ids);
  return uint31_to_ranges(req);
}


Ranges< Uint31_Index > relation_relation_member_indices
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& current_rels,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels)
{
  std::vector< Relation::Id_Type > map_ids;

  for (auto it = current_rels.begin(); it != current_rels.end(); ++it)
    // Treat relations with really large indices: get the node indexes from nodes.std::map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);

  std::sort(map_ids.begin(), map_ids.end());
  map_ids.erase(std::unique(map_ids.begin(), map_ids.end()), map_ids.end());
  context.get_health_guard().check();
  Ranges< Uint31_Index > current = collect_relation_req(context, map_ids);
  context.get_health_guard().check();

  map_ids.clear();
  for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
    // Treat relations with really large indices: get the node indexes from nodes.std::map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);

  std::sort(map_ids.begin(), map_ids.end());
  map_ids.erase(std::unique(map_ids.begin(), map_ids.end()), map_ids.end());
  if (!map_ids.empty())
  {
    context.get_health_guard().check();
    Ranges< Uint31_Index > attic = collect_attic_relation_req(context, map_ids);
    return current.union_(attic);
  }
  return current;
}


Ranges< Uint32_Index > way_nd_indices(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways)
{
  std::vector< uint32 > parents;

  for (auto it = ways.begin(); it != ways.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x1) == 0)) // Adapt 0x3
    {
      // Treat ways with really large indices: get the node indexes from the segment indexes
      for (std::vector< Way_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
        for (std::vector< Quad_Coord >::const_iterator it3 = it2->geometry.begin();
            it3 != it2->geometry.end(); ++it3)
          parents.push_back(it3->ll_upper);
      }
    }
    else
      parents.push_back(it->first.val());
  }
  for (auto it = attic_ways.begin(); it != attic_ways.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x1) == 0)) // Adapt 0x3
    {
      // Treat ways with really large indices: get the node indexes from the segment indexes
      for (std::vector< Attic< Way_Skeleton > >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
        for (std::vector< Quad_Coord >::const_iterator it3 = it2->geometry.begin();
            it3 != it2->geometry.end(); ++it3)
          parents.push_back(it3->ll_upper);
      }
    }
    else
      parents.push_back(it->first.val());
  }
  std::sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());

  return calc_node_children_ranges(parents);
}


Ranges< Uint32_Index > way_covered_indices(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways)
{
  std::vector< uint32 > parents;

  for (auto it = ways.begin(); it != ways.end(); ++it)
    parents.push_back(it->first.val());
  for (auto it = attic_ways.begin(); it != attic_ways.end(); ++it)
    parents.push_back(it->first.val());

  std::sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());

  return calc_node_children_ranges(parents);
}


std::vector< Relation::Id_Type > relation_relation_member_ids(
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
    const uint32* role_id)
{
  std::vector< Relation::Id_Type > ids;
  if (role_id)
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
  }
  else
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
  }

  if (role_id)
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
    for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
  }
  else
  {
    for (auto it = rels.begin(); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
    for (auto it = attic_rels.begin(); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
  }

  std::sort(ids.begin(), ids.end());

  return ids;
}


template< typename Id_Type >
void sieve_first_arg(
    std::vector< Id_Type >& working_ids, const std::vector< Id_Type >& extra_ids, bool invert_ids)
{
  std::vector< Id_Type > copy_ids = working_ids;
  if (!invert_ids)
    working_ids.erase(std::set_intersection
        (extra_ids.begin(), extra_ids.end(), copy_ids.begin(), copy_ids.end(),
         working_ids.begin()), working_ids.end());
  else
    working_ids.erase(std::set_difference
        (copy_ids.begin(), copy_ids.end(), extra_ids.begin(), extra_ids.end(),
         working_ids.begin()), working_ids.end());
}


Timeless< Uint31_Index, Relation_Skeleton > relation_relation_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& parents,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_parents,
     const Ranges< Uint31_Index >& children_ranges,
     const std::vector< Relation::Id_Type >& children_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Relation::Id_Type > intersect_ids = relation_relation_member_ids(parents, attic_parents, role_id);
  context.get_health_guard().check();
  sieve_first_arg(intersect_ids, children_ids, invert_ids);

  return collect_items_range< Uint31_Index, Relation_Skeleton >(context,
      children_ranges.is_global()
      ? relation_relation_member_indices(context, parents, attic_parents) : children_ranges,
      Id_Predicate< Relation_Skeleton >(intersect_ids));
}


Timeless< Uint31_Index, Way_Skeleton > relation_way_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
     const Ranges< Uint31_Index >& way_ranges,
     const std::vector< Way::Id_Type >& way_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Way::Id_Type > intersect_ids = relation_way_member_ids(relations, attic_relations, role_id);
  context.get_health_guard().check();
  sieve_first_arg(intersect_ids, way_ids, invert_ids);

  return collect_items_range< Uint31_Index, Way_Skeleton >(context, way_ranges.intersect(
      relation_way_member_indices(relations, attic_relations)),
      Id_Predicate< Way_Skeleton >(intersect_ids));
}


std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > relation_way_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     const Ranges< Uint31_Index >& way_ranges)
{
  std::vector< Way::Id_Type > intersect_ids = relation_way_member_ids({}, relations);
  context.get_health_guard().check();

  auto timeless = collect_items_range< Uint31_Index, Way_Skeleton >(context, way_ranges.intersect(
      relation_way_member_indices({}, relations)),
      Id_Predicate< Way_Skeleton >(intersect_ids));

  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > result;
  keep_matching_skeletons(result, timeless.current, timeless.attic, context.get_desired_timestamp());
  return result;
}


Timeless< Uint32_Index, Node_Skeleton > paired_items_range(
    Request_Context& context,
    const std::vector< Node::Id_Type >& target_ids, const Ranges< Uint32_Index >& ranges)
{
  auto result = collect_items_range< Uint32_Index, Node_Skeleton >(
      context, ranges, Id_Predicate< Node_Skeleton >(target_ids));
  keep_matching_skeletons(result.current, result.attic, context.get_desired_timestamp());

  return result;
}


Timeless< Uint32_Index, Node_Skeleton > relation_node_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
     const Ranges< Uint32_Index >& node_ranges,
     const std::vector< Node::Id_Type >& node_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Node::Id_Type > intersect_ids = relation_node_member_ids(relations, attic_relations, role_id);
  context.get_health_guard().check();
  sieve_first_arg(intersect_ids, node_ids, invert_ids);

  return paired_items_range(
      context, intersect_ids, node_ranges.intersect(
          relation_node_member_indices(relations, attic_relations)));
}


std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > relation_node_members
    (Request_Context& context,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     const Ranges< Uint32_Index >& node_ranges)
{
  std::vector< Node::Id_Type > intersect_ids = relation_node_member_ids(relations, {});
  context.get_health_guard().check();

  // Retrieve all nodes referred by the ways.
  auto timeless = collect_items_range< Uint32_Index, Node_Skeleton >(
      context, node_ranges, Id_Predicate< Node_Skeleton >(intersect_ids));

  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > result;
  keep_matching_skeletons(result, timeless.current, timeless.attic, context.get_desired_timestamp());
  return result;
}


Timeless< Uint32_Index, Node_Skeleton > way_members(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    const std::vector< int >* pos,
    const Ranges< Uint32_Index >& node_ranges,
    const std::vector< Node::Id_Type >& node_ids, bool invert_ids)
{
  std::vector< Node::Id_Type > intersect_ids = way_nd_ids(ways, attic_ways, pos);
  context.get_health_guard().check();
  sieve_first_arg(intersect_ids, node_ids, invert_ids);

  return paired_items_range(context, intersect_ids,
      !node_ranges.is_global() ? node_ranges : way_nd_indices(ways, attic_ways));
}


Timeless< Uint32_Index, Node_Skeleton > way_cnt_members(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit,
    const Ranges< Uint32_Index >& node_ranges,
    const std::vector< Node::Id_Type >& node_ids, bool invert_ids)
{
  std::vector< Node::Id_Type > intersect_ids = way_cnt_nd_ids(ways, attic_ways, lower_limit, upper_limit);
  context.get_health_guard().check();
  sieve_first_arg(intersect_ids, node_ids, invert_ids);

  return paired_items_range(context, intersect_ids,
      !node_ranges.is_global() ? node_ranges : way_nd_indices(ways, attic_ways));
}


Timeless< Uint32_Index, Node_Skeleton > way_link_members(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
    unsigned int lower_limit, unsigned int upper_limit,
    const Ranges< Uint32_Index >& node_ranges,
    const std::vector< Node::Id_Type >& node_ids, bool invert_ids)
{
  std::vector< Node::Id_Type > intersect_ids = way_link_nd_ids(ways, attic_ways, lower_limit, upper_limit);
  context.get_health_guard().check();
  sieve_first_arg(intersect_ids, node_ids, invert_ids);

  return paired_items_range(context, intersect_ids,
      !node_ranges.is_global() ? node_ranges : way_nd_indices(ways, attic_ways));
}

//-----------------------------------------------------------------------------

const std::map< uint32, std::string >& relation_member_roles(Transaction& transaction)
{
  static std::map< uint32, std::string > roles;

  if (roles.empty())
  {
    Block_Backend< Uint32_Index, String_Object > roles_db
        (transaction.data_index(osm_base_settings().RELATION_ROLES));
    for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
        it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
      roles[it.index().val()] = it.object().val();
  }

  return roles;
}


uint32 determine_role_id(Transaction& transaction, const std::string& role)
{
  const std::map< uint32, std::string >& roles = relation_member_roles(transaction);
  for (std::map< uint32, std::string >::const_iterator it = roles.begin(); it != roles.end(); ++it)
  {
    if (it->second == role)
      return it->first;
  }
  return std::numeric_limits< uint32 >::max();
}


//-----------------------------------------------------------------------------


bool add_way_to_area_blocks(const std::vector< Quad_Coord >& coords,
                            uint32 id, std::map< Uint31_Index, std::vector< Area_Block > >& areas)
{
  bool wraps_around_date_line = false;

  if (coords.size() < 2)
    return false;

  uint32 cur_idx = 0;
  std::vector< uint64 > cur_polyline;
  for (std::vector< Quad_Coord >::const_iterator it = coords.begin(); it != coords.end(); ++it)
  {
    if ((it->ll_upper & 0xffffff00) != cur_idx)
    {
      if (cur_idx != 0)
      {
        if (cur_polyline.size() > 1)
          areas[cur_idx].push_back(Area_Block(id, cur_polyline));

        std::vector< Aligned_Segment > aligned_segments;
        wraps_around_date_line ^= Area::calc_aligned_segments
            (aligned_segments, cur_polyline.back(),
             ((uint64)it->ll_upper<<32) | it->ll_lower);
        cur_polyline.clear();
        for (std::vector< Aligned_Segment >::const_iterator
            it(aligned_segments.begin()); it != aligned_segments.end(); ++it)
        {
          cur_polyline.push_back((((uint64)it->ll_upper_)<<32) | it->ll_lower_a);
          cur_polyline.push_back((((uint64)it->ll_upper_)<<32) | it->ll_lower_b);
          areas[it->ll_upper_].push_back(Area_Block(id, cur_polyline));
          cur_polyline.clear();
        }
      }
      cur_idx = (it->ll_upper & 0xffffff00);
    }
    cur_polyline.push_back(((uint64)it->ll_upper<<32) | it->ll_lower);
  }
  if ((cur_idx != 0) && (cur_polyline.size() > 1))
    areas[cur_idx].push_back(Area_Block(id, cur_polyline));

  return wraps_around_date_line;
}


std::vector< Quad_Coord > make_geometry(const Way_Skeleton& way, const std::vector< Node >& nodes)
{
  std::vector< Quad_Coord > result;

  for (std::vector< Node::Id_Type >::const_iterator it3(way.nds.begin());
      it3 != way.nds.end(); ++it3)
  {
    const Node* node = binary_search_for_id(nodes, *it3);
    if (node == 0)
    {
      result.clear();
      return result;
    }
    result.push_back(Quad_Coord(node->index, node->ll_lower_));
  }

  return result;
}


std::vector< Uint31_Index > segment_idxs(const std::vector< Quad_Coord >& geometry)
{
  std::vector< uint32 > nd_idxs;

  for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    nd_idxs.push_back(it->ll_upper);

  return calc_segment_idxs(nd_idxs);
}


template< typename Object >
void filter_ways_by_ranges_generic
    (std::map< Uint31_Index, std::vector< Object > >& ways,
    const Ranges< Uint31_Index >& ranges)
{
  auto ranges_it = ranges.begin();
  typename std::map< Uint31_Index, std::vector< Object > >::iterator it = ways.begin();
  auto ranges_begin = ranges.begin();
  for (; it != ways.end() && ranges_it != ranges.end(); )
  {
    if (!(it->first < ranges_it.upper_bound()))
      ++ranges_it;
    else if (!(it->first < ranges_it.lower_bound()))
    {
      if ((it->first.val() & 0x80000000) == 0 || (it->first.val() & 0x1) != 0) // Adapt 0x3
        ++it;
      else
      {
        std::vector< Object > filtered_ways;
        while (!(Uint31_Index(it->first.val() & 0x7fffff00) < ranges_begin.upper_bound()))
          ++ranges_begin;
        for (typename std::vector< Object >::const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2)
        {
          auto ranges_it2 = ranges_begin;
          std::vector< Uint31_Index > segment_idxs_ = segment_idxs(it2->geometry);
          for (std::vector< Uint31_Index >::const_iterator it3 = segment_idxs_.begin();
               it3 != segment_idxs_.end() && ranges_it2 != ranges.end(); )
          {
            if (!(*it3 < ranges_it2.upper_bound()))
              ++ranges_it2;
            else if (!(*it3 < ranges_it2.lower_bound()))
            {
              // A relevant index is found; thus the way is relevant.
              filtered_ways.push_back(*it2);
              break;
            }
            else
              ++it3;
          }
        }

        filtered_ways.swap(it->second);
        ++it;
      }
    }
    else
    {
      // The index of the way is not in the current set of ranges.
      // Thus it cannot be in the result set.
      it->second.clear();
      ++it;
    }
  }
  for (; it != ways.end(); ++it)
    it->second.clear();
}


void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
                           const Ranges< Uint31_Index >& ranges)
{
  filter_ways_by_ranges_generic(ways, ranges);
}


void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways,
                           const Ranges< Uint31_Index >& ranges)
{
  filter_ways_by_ranges_generic(ways, ranges);
}


void keep_matching_skeletons
    (std::vector< Node >& result,
     const std::map< Uint32_Index, std::vector< Node_Skeleton > >& current,
     const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic,
     uint64 timestamp)
{
  std::map< Node_Skeleton::Id_Type, uint64 > timestamp_by_id;

  result.clear();

  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      timestamp_by_id[it2->id] = NOW;
  }

  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (std::vector< Attic<Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      uint64& stored_timestamp = timestamp_by_id[it2->id];
      if (it2->timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > it2->timestamp))
        stored_timestamp = it2->timestamp;
    }
  }

  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == NOW)
        result.push_back(Node(it2->id, it->first.val(), it2->ll_lower));
    }
  }

  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (std::vector< Attic<Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == it2->timestamp)
        result.push_back(Node(it2->id, it->first.val(), it2->ll_lower));
    }
  }

  std::sort(result.begin(), result.end(), Node_Comparator_By_Id());
}


Timeless< Uint31_Index, Way_Skeleton > collect_ways(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
    const Ranges< Uint31_Index >& ranges,
    const std::vector< Way::Id_Type >& ids, bool invert_ids,
    uint32* role_id)
{
  return relation_way_members(context, rels, attic_rels, ranges, ids, invert_ids, role_id);
}


Timeless< Uint31_Index, Way_Skeleton > collect_ways(
    Request_Context& context,
    const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
    const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_nodes,
    const std::vector< int >* pos,
    const std::vector< Way::Id_Type >& ids, bool invert_ids)
{
  Timeless< Uint31_Index, Way_Skeleton > result;
  std::vector< Uint64 > children_ids;
  std::set< Uint31_Index > req;

  if (context.get_desired_timestamp() == NOW)
  {
    extract_ids(nodes).swap(children_ids);
    extract_parent_indices(nodes).swap(req);
  }
  else
  {
    std::vector< Uint64 > current_ids = extract_ids(nodes);
    extract_parent_indices(nodes).swap(req);

    std::vector< Uint64 > attic_ids = extract_ids(attic_nodes);
    std::set< Uint31_Index > attic_req = extract_parent_indices(attic_nodes);

    std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                  std::back_inserter(children_ids));
    for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
      req.insert(*it);
  }

  if (!invert_ids)
    collect_items_discrete(context, req,
        And_Predicate< Way_Skeleton,
            Id_Predicate< Way_Skeleton >, Get_Parent_Ways_Predicate >
            (Id_Predicate< Way_Skeleton >(ids), Get_Parent_Ways_Predicate(children_ids, pos)),
        result.current, result.attic);
  else if (ids.empty())
    collect_items_discrete(context, req,
        Get_Parent_Ways_Predicate(children_ids, pos), result.current, result.attic);
  else
    collect_items_discrete(context, req,
        And_Predicate< Way_Skeleton,
            Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >,
            Get_Parent_Ways_Predicate >
            (Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >
              (Id_Predicate< Way_Skeleton >(ids)),
            Get_Parent_Ways_Predicate(children_ids, pos)),
        result.current, result.attic);

  return result;
}


void add_nw_member_objects(Request_Context& context, const Set& input_set, Set& into,
    const Ranges< Uint32_Index >* ranges_32_, const Ranges< Uint31_Index >* ranges_31_)
{
  Ranges< Uint32_Index > ranges_32(ranges_32_ ? *ranges_32_ : Ranges< Uint32_Index >::global());
  Ranges< Uint31_Index > ranges_31(ranges_31_ ? *ranges_31_ : Ranges< Uint31_Index >::global());
  Timeless< Uint32_Index, Node_Skeleton > rel_nodes
      = relation_node_members(context, input_set.relations, input_set.attic_relations, ranges_32, {}, true);

  if (context.get_desired_timestamp() == NOW)
  {
    relation_way_members(context, input_set.relations, {}, ranges_31, {}, true)
        .swap(into.ways, into.attic_ways);
    std::map< Uint31_Index, std::vector< Way_Skeleton > > source_ways = input_set.ways;
    sort_second(source_ways);
    sort_second(into.ways);
    indexed_set_union(source_ways, into.ways);
    way_members(context, source_ways, {}, 0, Ranges< Uint32_Index >::global(), {}, true)
        .swap(into.nodes, into.attic_nodes);
    sort_second(into.nodes);
    sort_second(rel_nodes.current);
    indexed_set_union(into.nodes, rel_nodes.current);
  }
  else
  {
    rel_nodes.swap(into.nodes, into.attic_nodes);

    Timeless< Uint31_Index, Way_Skeleton > all_ways
        = relation_way_members(context, input_set.relations, input_set.attic_relations, ranges_31, {}, true);
    into.ways = all_ways.current;
    into.attic_ways = all_ways.attic;
    all_ways.sort();

    std::map< Uint31_Index, std::vector< Way_Skeleton > > source_ways = input_set.ways;
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > source_attic_ways = input_set.attic_ways;
    sort_second(source_ways);
    sort_second(source_attic_ways);
    indexed_set_union(all_ways.current, source_ways);
    indexed_set_union(all_ways.attic, source_attic_ways);

    Timeless< Uint32_Index, Node_Skeleton > more_nodes
        = way_members(context, all_ways.current, all_ways.attic, 0, Ranges< Uint32_Index >::global(), {}, true);
    more_nodes.sort();
    sort_second(into.nodes);
    sort_second(into.attic_nodes);
    indexed_set_union(into.nodes, more_nodes.current);
    indexed_set_union(into.attic_nodes, more_nodes.attic);
    keep_matching_skeletons(into.nodes, into.attic_nodes, context.get_desired_timestamp());
  }
}
