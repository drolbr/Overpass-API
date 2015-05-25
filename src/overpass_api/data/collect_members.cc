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

#include "abstract_processing.h"
#include "collect_members.h"

//-----------------------------------------------------------------------------


std::vector< Node::Id_Type > way_nd_ids(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  std::vector< Node::Id_Type > ids;
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
  {
    for (std::vector< Way_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      for (std::vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());
  
  return ids;
}


std::vector< Node::Id_Type > way_nd_ids
    (const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
     const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways)
{
  std::vector< Node::Id_Type > ids = way_nd_ids(ways);
  
  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator
      it = attic_ways.begin(); it != attic_ways.end(); ++it)
  {
    for (std::vector< Attic< Way_Skeleton > >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
    {
      for (std::vector< Node::Id_Type >::const_iterator it3 = it2->nds.begin();
           it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());
  
  return ids;
}


inline std::set< std::pair< Uint32_Index, Uint32_Index > > calc_node_children_ranges
    (const std::vector< uint32 >& way_rel_idxs)
{
  std::set< std::pair< Uint32_Index, Uint32_Index > > result;

  std::vector< std::pair< uint32, uint32 > > ranges;
  
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

      ranges.push_back(make_pair(ll_upper(lat<<16, lon<<16),
				 ll_upper((lat+offset-1)<<16, (lon+offset-1)<<16)+1));
      ranges.push_back(make_pair(ll_upper(lat<<16, (lon+offset)<<16),
				 ll_upper((lat+offset-1)<<16, (lon+2*offset-1)<<16)+1));
      ranges.push_back(make_pair(ll_upper((lat+offset)<<16, lon<<16),
				 ll_upper((lat+2*offset-1)<<16, (lon+offset-1)<<16)+1));
      ranges.push_back(make_pair(ll_upper((lat+offset)<<16, (lon+offset)<<16),
				 ll_upper((lat+2*offset-1)<<16, (lon+2*offset-1)<<16)+1));
      for (uint32 i = lat; i <= lat_u; ++i)
      {
	for (uint32 j = lon; j <= lon_u; ++j)
	  result.insert(make_pair(ll_upper(i<<16, j<<16), ll_upper(i<<16, j<<16)+1));
      }
    }
    else
      ranges.push_back(make_pair(*it, (*it) + 1));
  }
  sort(ranges.begin(), ranges.end());
  uint32 pos = 0;
  for (std::vector< std::pair< uint32, uint32 > >::const_iterator it = ranges.begin();
      it != ranges.end(); ++it)
  {
    if (pos < it->first)
      pos = it->first;
    result.insert(make_pair(pos, it->second));
    pos = it->second;
  }
  return result;
}


std::vector< Uint31_Index > collect_relation_req
    (const Statement& stmt, Resource_Manager& rman,
     const std::vector< Relation::Id_Type >& map_ids)
{
  std::vector< Uint31_Index > req;
  
  Random_File< Relation_Skeleton::Id_Type, Uint31_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().RELATIONS));
  for (std::vector< Relation::Id_Type >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(it->val()));
  
  rman.health_check(stmt);
  sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());
  rman.health_check(stmt);
  
  return req;
}


std::vector< Uint31_Index > collect_attic_relation_req
    (const Statement& stmt, Resource_Manager& rman,
     const std::vector< Relation::Id_Type >& map_ids)
{
  uint64 timestamp = rman.get_desired_timestamp();
  std::vector< std::pair< Relation::Id_Type, uint64 > > ids;
  for (std::vector< Relation::Id_Type >::const_iterator it = map_ids.begin(); it != map_ids.end(); ++it)
    ids.push_back(std::make_pair(*it, timestamp));
  
  std::pair< std::vector< Uint31_Index >, std::vector< Uint31_Index > > idxs
      = get_indexes< Uint31_Index, Relation_Skeleton >(ids, rman);
  
  std::vector< Uint31_Index > req;
  std::set_union(idxs.first.begin(), idxs.first.end(), idxs.second.begin(), idxs.second.end(),
                 std::back_inserter(req));
  
  return req;
}


std::vector< Uint31_Index > collect_way_req
    (const Statement* stmt, Resource_Manager& rman,
     const std::vector< uint32 >& parents,
     const std::vector< uint32 >& map_ids,
     const std::vector< Uint31_Index >& children_idxs)
{
  std::vector< Uint31_Index > req = calc_children(parents);
  
  Random_File< Way_Skeleton::Id_Type, Uint31_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().WAYS));
  for (std::vector< uint32 >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(*it));
  
  for (std::vector< Uint31_Index >::const_iterator it = children_idxs.begin();
      it != children_idxs.end(); ++it)
    req.push_back(*it);

  if (stmt)
    rman.health_check(*stmt);
  sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());
  if (stmt)
    rman.health_check(*stmt);
  
  return req;
}


std::set< std::pair< Uint32_Index, Uint32_Index > > collect_node_req
    (const Statement* stmt, Resource_Manager& rman,
     const std::vector< Node::Id_Type >& map_ids, const std::vector< uint32 >& parents)
{
  std::set< std::pair< Uint32_Index, Uint32_Index > > req = calc_node_children_ranges(parents);
  
  Random_File< Node_Skeleton::Id_Type, Uint32_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().NODES));
  for (std::vector< Node::Id_Type >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
  {
    Uint32_Index idx = random.get(it->val());
    req.insert(make_pair(idx, idx.val() + 1));
  }
  
  if (stmt)
    rman.health_check(*stmt);
  
  return req;
}


std::vector< Uint31_Index > relation_relation_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end)
{
  std::vector< Relation::Id_Type > map_ids;
    
  for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
    // Treat relations with really large indices: get the node indexes from nodes.map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);

  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
    
  return collect_relation_req(stmt, rman, map_ids);
}


std::vector< Uint31_Index > relation_relation_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_begin,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator rels_end,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator attic_rels_begin,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator attic_rels_end)
{
  std::vector< Relation::Id_Type > map_ids;
    
  for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
    // Treat relations with really large indices: get the node indexes from nodes.map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);

  sort(map_ids.begin(), map_ids.end());
  map_ids.erase(std::unique(map_ids.begin(), map_ids.end()), map_ids.end());
  rman.health_check(stmt);
  std::vector< Uint31_Index > current = collect_relation_req(stmt, rman, map_ids);

  map_ids.clear();
  for (std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
      it = attic_rels_begin; it != attic_rels_end; ++it)
    // Treat relations with really large indices: get the node indexes from nodes.map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);
  
  sort(map_ids.begin(), map_ids.end());
  map_ids.erase(std::unique(map_ids.begin(), map_ids.end()), map_ids.end());
  rman.health_check(stmt);
  std::vector< Uint31_Index > attic = collect_attic_relation_req(stmt, rman, map_ids);
  
  std::vector< Uint31_Index > result;
  std::set_union(current.begin(), current.end(), attic.begin(), attic.end(), back_inserter(result));
  return result;
}


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
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_end)
{
  std::vector< uint32 > parents;
  
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
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
  sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, std::vector< Node::Id_Type >(), parents);
}


std::set< std::pair< Uint32_Index, Uint32_Index > > way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_begin,
     std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator ways_end,
     std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_ways_begin,
     std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_ways_end)
{
  std::vector< uint32 > parents;
  
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
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
  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator
    it(attic_ways_begin); it != attic_ways_end; ++it)
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
  sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, std::vector< Node::Id_Type >(), parents);
}


std::vector< Relation::Id_Type > relation_relation_member_ids
    (Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels, const uint32* role_id)
{
  std::vector< Relation::Id_Type > ids;    
  if (role_id)
  {
    for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
  }
  else
  {
    for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
  }
  
  sort(ids.begin(), ids.end());
  
  return ids;
}

vector< Relation::Id_Type > relation_relation_member_ids
    (Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
     const uint32* role_id)
{
  std::vector< Relation::Id_Type > ids;    
  if (role_id)
  {
    for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
    for (std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
        it(attic_rels.begin()); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
  }
  else
  {
    for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
    for (std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
        it(attic_rels.begin()); it != attic_rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
  }
  
  sort(ids.begin(), ids.end());
  
  return ids;
}


std::map< Uint31_Index, std::vector< Relation_Skeleton > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& parents,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* children_ranges,
     const std::vector< Relation::Id_Type >* children_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Relation::Id_Type > intersect_ids;
  if (children_ids)
  {
    std::vector< Relation::Id_Type > children_ids_ = relation_relation_member_ids(rman, parents, role_id);
    rman.health_check(stmt);
    intersect_ids.resize(children_ids_.size(), Relation::Id_Type(0u));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (children_ids->begin(), children_ids->end(),
	   children_ids_.begin(), children_ids_.end(),
	  intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids_.begin(), children_ids_.end(),
	   children_ids->begin(), children_ids->end(),
	  intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = relation_relation_member_ids(rman, parents, role_id);
    rman.health_check(stmt);
  }
    
  std::map< Uint31_Index, std::vector< Relation_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
  
  if (children_ranges)
    collect_items_range(&stmt, rman, *osm_base_settings().RELATIONS, *children_ranges,
			Id_Predicate< Relation_Skeleton >(intersect_ids), result);
  else
  {    
    std::vector< Uint31_Index > req =
        relation_relation_member_indices(stmt, rman, parents.begin(), parents.end());
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
			Id_Predicate< Relation_Skeleton >(intersect_ids), result);
  }
  return result;
}


std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
    std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& parents,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_parents,
     uint64 timestamp,
     const set< pair< Uint31_Index, Uint31_Index > >* children_ranges,
     const vector< Relation::Id_Type >* children_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Relation::Id_Type > intersect_ids;
  if (children_ids)
  {
    std::vector< Relation::Id_Type > children_ids_
        = relation_relation_member_ids(rman, parents, attic_parents, role_id);
    rman.health_check(stmt);
    intersect_ids.resize(children_ids_.size(), Relation::Id_Type(0u));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (children_ids->begin(), children_ids->end(),
           children_ids_.begin(), children_ids_.end(),
          intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids_.begin(), children_ids_.end(),
           children_ids->begin(), children_ids->end(),
          intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = relation_relation_member_ids(rman, parents, attic_parents, role_id);
    rman.health_check(stmt);
  }
    
  std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > result;
  if (intersect_ids.empty())
    return result;
  
  if (children_ranges)
    collect_items_range_by_timestamp(&stmt, rman, *children_ranges,
        Id_Predicate< Relation_Skeleton >(intersect_ids), result.first, result.second);
  else
  {    
    std::vector< Uint31_Index > req =
        relation_relation_member_indices(stmt, rman,
            parents.begin(), parents.end(), attic_parents.begin(), attic_parents.end());
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        Id_Predicate< Relation_Skeleton >(intersect_ids), result.first, result.second);
  }
  return result;
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
  
  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      timestamp_by_id[it2->id] = NOW;
  }
  
  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      uint64& stored_timestamp = timestamp_by_id[it2->id];
      if (it2->timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > it2->timestamp))
        stored_timestamp = it2->timestamp;
    }
  }
  
  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (typename std::vector< Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == NOW)
        result[it->first].push_back(Attic< Skeleton >(*it2, NOW));
    }
  }
  
  for (typename std::map< Index, std::vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (typename std::vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == it2->timestamp)
        result[it->first].push_back(Attic< Skeleton >(*it2, it2->timestamp));
    }
  }
}


std::map< Uint31_Index, std::vector< Way_Skeleton > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* way_ranges,
     const std::vector< Way::Id_Type >* way_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Way::Id_Type > intersect_ids;
  if (way_ids)
  {
    std::vector< Way::Id_Type > children_ids = relation_way_member_ids(rman, relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
    intersect_ids.resize(children_ids.size(), Way::Id_Type(0u));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (way_ids->begin(), way_ids->end(), children_ids.begin(), children_ids.end(),
	  intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids.begin(), children_ids.end(),
	  way_ids->begin(), way_ids->end(),
	  intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = relation_way_member_ids(rman, relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
  }
      
  std::map< Uint31_Index, std::vector< Way_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
  
  if (way_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().WAYS, *way_ranges,
			Id_Predicate< Way_Skeleton >(intersect_ids), result);
  else
  {    
    std::vector< Uint31_Index > req =
        relation_way_member_indices< Relation_Skeleton >(stmt, rman, relations.begin(), relations.end());
    collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req,
			Id_Predicate< Way_Skeleton >(intersect_ids), result);
  }
  return result;
}


std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_relations,
     uint64 timestamp,
     const set< pair< Uint31_Index, Uint31_Index > >* way_ranges,
     const vector< Way::Id_Type >* way_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Way::Id_Type > intersect_ids;
  if (way_ids)
  {
    std::vector< Way::Id_Type > children_ids
        = relation_way_member_ids(rman, relations, attic_relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
    intersect_ids.resize(children_ids.size(), Way::Id_Type(0u));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (way_ids->begin(), way_ids->end(), children_ids.begin(), children_ids.end(),
          intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids.begin(), children_ids.end(),
          way_ids->begin(), way_ids->end(),
          intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = relation_way_member_ids(rman, relations, attic_relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
  }
    
  std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > result;
  if (intersect_ids.empty())
    return result;
  
  if (way_ranges)
    collect_items_range_by_timestamp(stmt, rman,
                   *way_ranges, Id_Predicate< Way_Skeleton >(intersect_ids),
                   result.first, result.second);
  else
    collect_items_discrete_by_timestamp(stmt, rman,
        relation_way_member_indices< Relation_Skeleton >
            (stmt, rman, relations.begin(), relations.end(), attic_relations.begin(), attic_relations.end()),
        Id_Predicate< Way_Skeleton >(intersect_ids), result.first, result.second);
  
  return result;
}


std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     uint64 timestamp,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >* way_ranges)
{
  std::vector< Way::Id_Type > intersect_ids = relation_way_member_ids(rman, relations);
  if (stmt)
    rman.health_check(*stmt);
  
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > result;
  if (intersect_ids.empty())
    return result;

  // Retrieve all ways referred by the ways.
  std::map< Uint31_Index, std::vector< Way_Skeleton > > current;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic;
  
  if (way_ranges)
    collect_items_range_by_timestamp(stmt, rman,
                   *way_ranges, Id_Predicate< Way_Skeleton >(intersect_ids),
                   current, attic);
  else
    collect_items_discrete_by_timestamp(stmt, rman,
        relation_way_member_indices< Attic< Relation_Skeleton > >
            (stmt, rman, relations.begin(), relations.end()),
        Id_Predicate< Way_Skeleton >(intersect_ids), current, attic);
  
  keep_matching_skeletons(result, current, attic, timestamp);
    
  return result;
}


std::map< Uint32_Index, std::vector< Node_Skeleton > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const std::vector< Node::Id_Type >* node_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Node::Id_Type > intersect_ids;
  if (node_ids)
  {
    std::vector< Node::Id_Type > children_ids = relation_node_member_ids(rman, relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
    intersect_ids.resize(children_ids.size(), Node::Id_Type(0ull));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
	  intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids.begin(), children_ids.end(),
	  node_ids->begin(), node_ids->end(),
	  intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = relation_node_member_ids(rman, relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
  }
  
  std::map< Uint32_Index, std::vector< Node_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
    
  if (node_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  else
  {    
    std::set< std::pair< Uint32_Index, Uint32_Index > > req =
        relation_node_member_indices< Relation_Skeleton >(stmt, rman, relations.begin(), relations.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  }
  return result;
}


std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_relations,
     uint64 timestamp,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const vector< Node::Id_Type >* node_ids, bool invert_ids, const uint32* role_id)
{
  std::vector< Node::Id_Type > intersect_ids;
  if (node_ids)
  {
    std::vector< Node::Id_Type > children_ids
        = relation_node_member_ids(rman, relations, attic_relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
    intersect_ids.resize(children_ids.size(), Node::Id_Type(0ull));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
          intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids.begin(), children_ids.end(),
          node_ids->begin(), node_ids->end(),
          intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = relation_node_member_ids(rman, relations, attic_relations, role_id);
    if (stmt)
      rman.health_check(*stmt);
  }
  
  std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
      std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > result;
  if (intersect_ids.empty())
    return result;
  
  if (node_ranges)
  {
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
                        Id_Predicate< Node_Skeleton >(intersect_ids), result.first);
    collect_items_range(stmt, rman, *attic_settings().NODES, *node_ranges,
                        Id_Predicate< Attic< Node_Skeleton > >(intersect_ids), result.second);
  }
  else
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > req =
        relation_node_member_indices< Relation_Skeleton >(stmt, rman,
            relations.begin(), relations.end(), attic_relations.begin(), attic_relations.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
                        Id_Predicate< Node_Skeleton >(intersect_ids), result.first);
    collect_items_range(stmt, rman, *attic_settings().NODES, req,
                        Id_Predicate< Attic< Node_Skeleton > >(intersect_ids), result.second);
  }
  keep_matching_skeletons(result.first, result.second, timestamp);
  
  return result;
}


std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     uint64 timestamp,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges)
{
  std::vector< Node::Id_Type > intersect_ids = relation_node_member_ids(rman, relations);
  if (stmt)
    rman.health_check(*stmt);
  
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > result;
  if (intersect_ids.empty())
    return result;

  // Retrieve all nodes referred by the ways.
  std::map< Uint32_Index, std::vector< Node_Skeleton > > current;
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > attic;
  
  if (node_ranges)
  {
    collect_items_range(stmt, rman, *osm_base_settings().NODES,
                        *node_ranges, Id_Predicate< Node_Skeleton >(intersect_ids), current);
    collect_items_range(stmt, rman, *attic_settings().NODES,
                        *node_ranges, Id_Predicate< Node_Skeleton >(intersect_ids), attic);
  }
  else
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > req =
        relation_node_member_indices< Attic< Relation_Skeleton > >
            (stmt, rman, relations.begin(), relations.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES,
                        req, Id_Predicate< Node_Skeleton >(intersect_ids), current);
    collect_items_range(stmt, rman, *attic_settings().NODES,
                        req, Id_Predicate< Node_Skeleton >(intersect_ids), attic);
  }
  
    
  keep_matching_skeletons(result, current, attic, timestamp);
    
  return result;
}


std::map< Uint32_Index, std::vector< Node_Skeleton > > way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const std::vector< Node::Id_Type >* node_ids, bool invert_ids)
{  
  std::vector< Node::Id_Type > intersect_ids;
  
  if (node_ids)
  {
    std::vector< Node::Id_Type > children_ids = way_nd_ids(ways);
    if (stmt)
      rman.health_check(*stmt);
    intersect_ids.resize(children_ids.size(), Node::Id_Type(0ull));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
           intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids.begin(), children_ids.end(),
	   node_ids->begin(), node_ids->end(),
           intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = way_nd_ids(ways);
    if (stmt)
      rman.health_check(*stmt);
  }

  std::map< Uint32_Index, std::vector< Node_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
  
  if (node_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  else
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > req =
        way_nd_indices(stmt, rman, ways.begin(), ways.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  }
  
  return result;
}


std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
     const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways,
     uint64 timestamp,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const std::vector< Node::Id_Type >* node_ids, bool invert_ids)
{  
  std::vector< Node::Id_Type > intersect_ids;
  
  if (node_ids)
  {
    std::vector< Node::Id_Type > children_ids = way_nd_ids(ways, attic_ways);
    if (stmt)
      rman.health_check(*stmt);
    intersect_ids.resize(children_ids.size(), Node::Id_Type(0ull));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
           intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_difference
          (children_ids.begin(), children_ids.end(),
           node_ids->begin(), node_ids->end(),
           intersect_ids.begin()), intersect_ids.end());
  }
  else
  {
    intersect_ids = way_nd_ids(ways, attic_ways);
    if (stmt)
      rman.health_check(*stmt);
  }

  std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
      std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > result;
  if (intersect_ids.empty())
    return result;
  
  if (node_ranges)
  {
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
                        Id_Predicate< Node_Skeleton >(intersect_ids), result.first);
    collect_items_range(stmt, rman, *attic_settings().NODES, *node_ranges,
                        Id_Predicate< Attic< Node_Skeleton > >(intersect_ids), result.second);
  }
  else
  {
    std::set< std::pair< Uint32_Index, Uint32_Index > > req =
        way_nd_indices(stmt, rman, ways.begin(), ways.end(), attic_ways.begin(), attic_ways.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
                        Id_Predicate< Node_Skeleton >(intersect_ids), result.first);
    collect_items_range(stmt, rman, *attic_settings().NODES, req,
                        Id_Predicate< Attic< Node_Skeleton > >(intersect_ids), result.second);
  }
  keep_matching_skeletons(result.first, result.second, timestamp);
  
  return result;
}

//-----------------------------------------------------------------------------

const std::map< uint32, string >& relation_member_roles(Transaction& transaction)
{
  static std::map< uint32, string > roles;
  
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


uint32 determine_role_id(Transaction& transaction, const string& role)
{
  const std::map< uint32, string >& roles = relation_member_roles(transaction);
  for (std::map< uint32, string >::const_iterator it = roles.begin(); it != roles.end(); ++it)
  {
    if (it->second == role)
      return it->first;
  }
  return numeric_limits< uint32 >::max();
}


//-----------------------------------------------------------------------------


void add_way_to_area_blocks(const std::vector< Quad_Coord >& coords,
                            uint32 id, std::map< Uint31_Index, std::vector< Area_Block > >& areas)
{
  if (coords.size() < 2)
    return;
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
        Area::calc_aligned_segments
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
    const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  std::set< std::pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
  typename std::map< Uint31_Index, std::vector< Object > >::iterator it = ways.begin();
  std::set< std::pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_begin = ranges.begin();
  for (; it != ways.end() && ranges_it != ranges.end(); )
  {
    if (!(it->first < ranges_it->second))
      ++ranges_it;
    else if (!(it->first < ranges_it->first))
    {
      if ((it->first.val() & 0x80000000) == 0 || (it->first.val() & 0x1) != 0) // Adapt 0x3
        ++it;
      else
      {
        std::vector< Object > filtered_ways;
        while (!(Uint31_Index(it->first.val() & 0x7fffff00) < ranges_begin->second))
          ++ranges_begin;
        for (typename std::vector< Object >::const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2)
        {
          std::set< std::pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it2 = ranges_begin;
          std::vector< Uint31_Index > segment_idxs_ = segment_idxs(it2->geometry);
          for (std::vector< Uint31_Index >::const_iterator it3 = segment_idxs_.begin();
               it3 != segment_idxs_.end() && ranges_it2 != ranges.end(); )
          {
            if (!(*it3 < ranges_it2->second))
              ++ranges_it2;
            else if (!(*it3 < ranges_it2->first))
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
                           const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  filter_ways_by_ranges_generic(ways, ranges);
}


void filter_ways_by_ranges(std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways,
                           const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  filter_ways_by_ranges_generic(ways, ranges);
}


template< typename Object >
std::vector< Node::Id_Type > small_way_nd_ids(const std::map< Uint31_Index, std::vector< Object > >& ways)
{
  std::vector< Node::Id_Type > ids;
  for (typename std::map< Uint31_Index, std::vector< Object > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x1) == 0))
      continue;
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      for (std::vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());
  
  return ids;
}


template< typename Object >
std::set< std::pair< Uint32_Index, Uint32_Index > > small_way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename std::map< Uint31_Index, std::vector< Object > >::const_iterator ways_begin,
     typename std::map< Uint31_Index, std::vector< Object > >::const_iterator ways_end)
{
  std::vector< uint32 > parents;
  
  for (typename std::map< Uint31_Index, std::vector< Object > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
  {
    if (!(it->first.val() & 0x80000000) || ((it->first.val() & 0x1) != 0)) // Adapt 0x3
      parents.push_back(it->first.val());
  }
  sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());
  
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, std::vector< Node::Id_Type >(), parents);
}


std::map< Uint32_Index, std::vector< Node_Skeleton > > small_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  std::map< Uint32_Index, std::vector< Node_Skeleton > > result;
  
  collect_items_range(stmt, rman, *osm_base_settings().NODES,
                      small_way_nd_indices< Way_Skeleton >(stmt, rman, ways.begin(), ways.end()),
                      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), result);
  
  return result;
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


Way_Geometry_Store::Way_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways, const Statement& query, Resource_Manager& rman)
{
  // Retrieve all nodes referred by the ways.
  std::map< Uint32_Index, std::vector< Node_Skeleton > > way_members_ = small_way_members(&query, rman, ways);
  
  // Order node ids by id.
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it = way_members_.begin();
      it != way_members_.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());
}


Way_Geometry_Store::Way_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
     const Statement& query, Resource_Manager& rman)
{
  // Retrieve all nodes referred by the ways.
  std::map< Uint32_Index, std::vector< Node_Skeleton > > current;
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > attic;
  collect_items_range_by_timestamp(&query, rman, 
      small_way_nd_indices< Attic< Way_Skeleton > >(&query, rman, ways.begin(), ways.end()),
      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), current, attic);
  
  keep_matching_skeletons(nodes, current, attic, timestamp);
}


std::vector< Quad_Coord > Way_Geometry_Store::get_geometry(const Way_Skeleton& way) const
{
  if (way.geometry.empty())
    return make_geometry(way, nodes);
  else
    return way.geometry;
}
