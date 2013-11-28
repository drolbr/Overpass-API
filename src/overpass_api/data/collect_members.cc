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

using namespace std;

//-----------------------------------------------------------------------------

vector< Node::Id_Type > way_nd_ids(const map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  vector< Node::Id_Type > ids;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      for (vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());
  
  return ids;
}

inline set< pair< Uint32_Index, Uint32_Index > > calc_node_children_ranges
    (const vector< uint32 >& way_rel_idxs)
{
  set< pair< Uint32_Index, Uint32_Index > > result;

  vector< pair< uint32, uint32 > > ranges;
  
  for (vector< uint32 >::const_iterator it = way_rel_idxs.begin();
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
  for (vector< pair< uint32, uint32 > >::const_iterator it = ranges.begin();
      it != ranges.end(); ++it)
  {
    if (pos < it->first)
      pos = it->first;
    result.insert(make_pair(pos, it->second));
    pos = it->second;
  }
  return result;
}

vector< Uint31_Index > collect_relation_req
    (const Statement& stmt, Resource_Manager& rman,
     const vector< Relation::Id_Type >& map_ids)
{
  vector< Uint31_Index > req;
  
  Random_File< Uint31_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().RELATIONS));
  for (vector< Relation::Id_Type >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(it->val()));
  
  rman.health_check(stmt);
  sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());
  rman.health_check(stmt);
  
  return req;
}

vector< Uint31_Index > collect_way_req
    (const Statement* stmt, Resource_Manager& rman,
     const vector< uint32 >& map_ids, const vector< uint32 >& parents,
     const vector< Uint31_Index >& children_idxs)
{
  vector< Uint31_Index > req = calc_children(parents);
  
  Random_File< Uint31_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().WAYS));
  for (vector< uint32 >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(*it));
  
  for (vector< Uint31_Index >::const_iterator it = children_idxs.begin();
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

set< pair< Uint32_Index, Uint32_Index > > collect_node_req
    (const Statement* stmt, Resource_Manager& rman,
     const vector< Node::Id_Type >& map_ids, const vector< uint32 >& parents)
{
  set< pair< Uint32_Index, Uint32_Index > > req = calc_node_children_ranges(parents);
  
  Random_File< Uint32_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().NODES));
  for (vector< Node::Id_Type >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
  {
    Uint32_Index idx = random.get(it->val());
    req.insert(make_pair(idx, idx.val() + 1));
  }
  
  if (stmt)
    rman.health_check(*stmt);
  
  return req;
}


vector< Uint31_Index > relation_relation_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< Relation::Id_Type > map_ids;
    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
    // Treat relations with really large indices: get the node indexes from nodes.map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);

  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
    
  return collect_relation_req(stmt, rman, map_ids);
}


template< typename Relation_Skeleton >
vector< Uint31_Index > relation_way_member_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     typename map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > map_ids;
  vector< uint32 > parents;
  vector< Uint31_Index > children_idxs;
    
  for (typename map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (typename vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
	for (vector< Uint31_Index >::const_iterator it3 = it2->way_idxs.begin();
	    it3 != it2->way_idxs.end(); ++it3)
	  children_idxs.push_back(*it3);
      }
    }
    else
      parents.push_back(it->first.val());
  }    
  sort(map_ids.begin(), map_ids.end());
  if (stmt)
    rman.health_check(*stmt);
    
  return collect_way_req(stmt, rman, map_ids, parents, children_idxs);
}


template< typename Relation_Skeleton >
set< pair< Uint32_Index, Uint32_Index > > relation_node_member_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     typename map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< Node::Id_Type > map_ids;
  vector< uint32 > parents;
  
  for (typename map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the node indexes from the segement indexes
      for (typename vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
	bool large_indices = false;
	for (vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
	    it3 != it2->node_idxs.end(); ++it3)
	{
	  if ((it3->val() & 0x80000000) && ((it3->val() & 0xf) == 0))
	  {
	    //Treat relations with really large indices: get the node indexes from nodes.map.
	    large_indices = true;
	    break;
	  }
	}
	
	if (large_indices)
	  filter_for_member_ids(it->second, map_ids, Relation_Entry::NODE);
	else
	{
	  for (vector< Uint31_Index >::const_iterator it3 = it2->node_idxs.begin();
	      it3 != it2->node_idxs.end(); ++it3)
	    parents.push_back(it3->val());
	}
      }
    }
    else
      parents.push_back(it->first.val());
  }    
  sort(map_ids.begin(), map_ids.end());
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, map_ids, parents);
}


set< pair< Uint32_Index, Uint32_Index > > way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end)
{
  vector< Node::Id_Type > map_ids;
  vector< uint32 > parents;
  
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x1) == 0)) // Adapt 0x3
    {
      // Treat ways with really large indices: get the node indexes from the segment indexes
      for (vector< Way_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
// 	bool large_indices = false;
// 	for (vector< Uint31_Index >::const_iterator it3 = it2->segment_idxs.begin();
// 	    it3 != it2->segment_idxs.end(); ++it3)
// 	{
// 	  if ((it3->val() & 0x80000000) && ((it3->val() & 0xf) == 0))
// 	  {
// 	    //Treat ways with really large indices: get the node indexes from nodes.map.
// 	    large_indices = true;
// 	    break;
// 	  }
// 	}
// 	
// 	if (large_indices)
// 	{
// 	  for (vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
// 	      it3 != it2->nds.end(); ++it3)
// 	    map_ids.push_back(*it3);
// 	}
// 	else
// 	{
// 	  for (vector< Uint31_Index >::const_iterator it3 = it2->segment_idxs.begin();
// 	      it3 != it2->segment_idxs.end(); ++it3)
// 	    parents.push_back(it3->val());
// 	}        
        for (vector< Quad_Coord >::const_iterator it3 = it2->geometry.begin();
            it3 != it2->geometry.end(); ++it3)
          parents.push_back(it3->ll_upper);
      }
    }
    else
      parents.push_back(it->first.val());
  }
  sort(map_ids.begin(), map_ids.end());
  sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, map_ids, parents);
}


vector< Relation::Id_Type > relation_relation_member_ids
    (Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& rels, const uint32* role_id)
{
  vector< Relation::Id_Type > ids;    
  if (role_id)
  {
    for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION, *role_id);
  }
  else
  {
    for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
        it(rels.begin()); it != rels.end(); ++it)
      filter_for_member_ids(it->second, ids, Relation_Entry::RELATION);
  }
  
  sort(ids.begin(), ids.end());
  
  return ids;
}


map< Uint31_Index, vector< Relation_Skeleton > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& parents,
     const set< pair< Uint31_Index, Uint31_Index > >* children_ranges,
     const vector< Relation::Id_Type >* children_ids, bool invert_ids, const uint32* role_id)
{
  vector< Relation::Id_Type > intersect_ids;
  if (children_ids)
  {
    vector< Relation::Id_Type > children_ids_ = relation_relation_member_ids(rman, parents, role_id);
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
    
  map< Uint31_Index, vector< Relation_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
  
  if (children_ranges)
    collect_items_range(&stmt, rman, *osm_base_settings().RELATIONS, *children_ranges,
			Id_Predicate< Relation_Skeleton >(intersect_ids), result);
  else
  {    
    vector< Uint31_Index > req =
        relation_relation_member_indices(stmt, rman, parents.begin(), parents.end());
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
			Id_Predicate< Relation_Skeleton >(intersect_ids), result);
  }
  return result;
}


template< typename Index, typename Skeleton >
void keep_matching_skeletons
    (map< Index, vector< Attic< Skeleton > > >& result,
     const map< Index, vector< Skeleton > >& current,
     const map< Index, vector< Attic< Skeleton > > >& attic,
     uint64 timestamp)
{
  std::map< typename Skeleton::Id_Type, uint64 > timestamp_by_id;
  
  result.clear();
  
  for (typename map< Index, vector< Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (typename vector< Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      timestamp_by_id[it2->id] = NOW;
  }
  
  for (typename map< Index, vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (typename vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      uint64& stored_timestamp = timestamp_by_id[it2->id];
      if (it2->timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > it2->timestamp))
        stored_timestamp = it2->timestamp;
    }
  }
  
  for (typename map< Index, vector< Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (typename vector< Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == NOW)
        result[it->first].push_back(Attic< Skeleton >(*it2, NOW));
    }
  }
  
  for (typename map< Index, vector< Attic< Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (typename vector< Attic< Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == it2->timestamp)
        result[it->first].push_back(Attic< Skeleton >(*it2, it2->timestamp));
    }
  }
}


map< Uint31_Index, vector< Way_Skeleton > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint31_Index, Uint31_Index > >* way_ranges,
     const vector< Way::Id_Type >* way_ids, bool invert_ids, const uint32* role_id)
{
  vector< Way::Id_Type > intersect_ids;
  if (way_ids)
  {
    vector< Way::Id_Type > children_ids = relation_way_member_ids(rman, relations, role_id);
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
    
  map< Uint31_Index, vector< Way_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
  
  if (way_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().WAYS, *way_ranges,
			Id_Predicate< Way_Skeleton >(intersect_ids), result);
  else
  {    
    vector< Uint31_Index > req =
        relation_way_member_indices< Relation_Skeleton >(stmt, rman, relations.begin(), relations.end());
    collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req,
			Id_Predicate< Way_Skeleton >(intersect_ids), result);
  }
  return result;
}


map< Uint31_Index, vector< Attic< Way_Skeleton > > > relation_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& relations,
     uint64 timestamp,
     const set< pair< Uint31_Index, Uint31_Index > >* way_ranges)
{
  vector< Way::Id_Type > intersect_ids = relation_way_member_ids(rman, relations);
  if (stmt)
    rman.health_check(*stmt);
  
  map< Uint31_Index, vector< Attic< Way_Skeleton > > > result;
  if (intersect_ids.empty())
    return result;

  // Retrieve all ways referred by the ways.
  map< Uint31_Index, vector< Way_Skeleton > > current;
  map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic;
  
  if (way_ranges)
  {
    collect_items_range(stmt, rman, *osm_base_settings().WAYS,
                        *way_ranges, Id_Predicate< Way_Skeleton >(intersect_ids), current);
    collect_items_range(stmt, rman, *attic_settings().WAYS,
                        *way_ranges, Id_Predicate< Way_Skeleton >(intersect_ids), attic);
  }
  else
  {
    vector< Uint31_Index > req =
        relation_way_member_indices< Attic< Relation_Skeleton > >
            (stmt, rman, relations.begin(), relations.end());
    collect_items_discrete(stmt, rman, *osm_base_settings().WAYS,
                        req, Id_Predicate< Way_Skeleton >(intersect_ids), current);
    collect_items_discrete(stmt, rman, *attic_settings().WAYS,
                        req, Id_Predicate< Way_Skeleton >(intersect_ids), attic);
  }
  
  keep_matching_skeletons(result, current, attic, timestamp);
    
  return result;
}


map< Uint32_Index, vector< Node_Skeleton > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const vector< Node::Id_Type >* node_ids, bool invert_ids, const uint32* role_id)
{
  vector< Node::Id_Type > intersect_ids;
  if (node_ids)
  {
    vector< Node::Id_Type > children_ids = relation_node_member_ids(rman, relations, role_id);
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
  
  map< Uint32_Index, vector< Node_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
    
  if (node_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  else
  {    
    set< pair< Uint32_Index, Uint32_Index > > req =
        relation_node_member_indices< Relation_Skeleton >(stmt, rman, relations.begin(), relations.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  }
  return result;
}


map< Uint32_Index, vector< Attic< Node_Skeleton > > > relation_node_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& relations,
     uint64 timestamp,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges)
{
  vector< Node::Id_Type > intersect_ids = relation_node_member_ids(rman, relations);
  if (stmt)
    rman.health_check(*stmt);
  
  map< Uint32_Index, vector< Attic< Node_Skeleton > > > result;
  if (intersect_ids.empty())
    return result;

  // Retrieve all nodes referred by the ways.
  map< Uint32_Index, vector< Node_Skeleton > > current;
  map< Uint32_Index, vector< Attic< Node_Skeleton > > > attic;
  
  if (node_ranges)
  {
    collect_items_range(stmt, rman, *osm_base_settings().NODES,
                        *node_ranges, Id_Predicate< Node_Skeleton >(intersect_ids), current);
    collect_items_range(stmt, rman, *attic_settings().NODES,
                        *node_ranges, Id_Predicate< Node_Skeleton >(intersect_ids), attic);
  }
  else
  {
    set< pair< Uint32_Index, Uint32_Index > > req =
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


map< Uint32_Index, vector< Node_Skeleton > > way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Way_Skeleton > >& ways,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const vector< Node::Id_Type >* node_ids, bool invert_ids)
{  
  vector< Node::Id_Type > intersect_ids;
  
  if (node_ids)
  {
    vector< Node::Id_Type > children_ids = way_nd_ids(ways);
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

  map< Uint32_Index, vector< Node_Skeleton > > result;
  if (intersect_ids.empty())
    return result;
  
  if (node_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  else
  {
    set< pair< Uint32_Index, Uint32_Index > > req =
        way_nd_indices(stmt, rman, ways.begin(), ways.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  }
  
  return result;
}

//-----------------------------------------------------------------------------

const map< uint32, string >& relation_member_roles(Transaction& transaction)
{
  static map< uint32, string > roles;
  
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
  const map< uint32, string >& roles = relation_member_roles(transaction);
  for (map< uint32, string >::const_iterator it = roles.begin(); it != roles.end(); ++it)
  {
    if (it->second == role)
      return it->first;
  }
  return numeric_limits< uint32 >::max();
}


//-----------------------------------------------------------------------------


void add_way_to_area_blocks(const vector< Quad_Coord >& coords,
                            uint32 id, map< Uint31_Index, vector< Area_Block > >& areas)
{
  if (coords.size() < 2)
    return;
  uint32 cur_idx = 0;
  vector< uint64 > cur_polyline;
  for (vector< Quad_Coord >::const_iterator it = coords.begin(); it != coords.end(); ++it)
  {
    if ((it->ll_upper & 0xffffff00) != cur_idx)
    {
      if (cur_idx != 0)
      {
        if (cur_polyline.size() > 1)
          areas[cur_idx].push_back(Area_Block(id, cur_polyline));
            
        vector< Aligned_Segment > aligned_segments;
        Area::calc_aligned_segments
            (aligned_segments, cur_polyline.back(),
             ((uint64)it->ll_upper<<32) | it->ll_lower);
        cur_polyline.clear();
        for (vector< Aligned_Segment >::const_iterator
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


vector< Quad_Coord > make_geometry(const Way_Skeleton& way, const vector< Node >& nodes)
{
  vector< Quad_Coord > result;
  
  for (vector< Node::Id_Type >::const_iterator it3(way.nds.begin());
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


vector< Uint31_Index > segment_idxs(const vector< Quad_Coord >& geometry)
{
  vector< uint32 > nd_idxs;
  
  for (vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    nd_idxs.push_back(it->ll_upper);

  return calc_segment_idxs(nd_idxs);
}


template< typename Object >
void filter_ways_by_ranges_generic
    (map< Uint31_Index, vector< Object > >& ways,
    const set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
  typename map< Uint31_Index, vector< Object > >::iterator it = ways.begin();
  set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_begin = ranges.begin();
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
        vector< Object > filtered_ways;
        while (!(Uint31_Index(it->first.val() & 0x7fffff00) < ranges_begin->second))
          ++ranges_begin;
        for (typename vector< Object >::const_iterator it2 = it->second.begin();
             it2 != it->second.end(); ++it2)
        {
          set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it2 = ranges_begin;
          vector< Uint31_Index > segment_idxs_ = segment_idxs(it2->geometry);
          for (vector< Uint31_Index >::const_iterator it3 = segment_idxs_.begin();
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


void filter_ways_by_ranges(map< Uint31_Index, vector< Way_Skeleton > >& ways,
                           const set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  filter_ways_by_ranges_generic(ways, ranges);
}


void filter_ways_by_ranges(map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways,
                           const set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  filter_ways_by_ranges_generic(ways, ranges);
}


template< typename Object >
vector< Node::Id_Type > small_way_nd_ids(const map< Uint31_Index, vector< Object > >& ways)
{
  vector< Node::Id_Type > ids;
  for (typename map< Uint31_Index, vector< Object > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x1) == 0))
      continue;
    for (typename vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      for (vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());
  
  return ids;
}


template< typename Object >
set< pair< Uint32_Index, Uint32_Index > > small_way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename map< Uint31_Index, vector< Object > >::const_iterator ways_begin,
     typename map< Uint31_Index, vector< Object > >::const_iterator ways_end)
{
  vector< uint32 > parents;
  
  for (typename map< Uint31_Index, vector< Object > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
  {
    if (!(it->first.val() & 0x80000000) || ((it->first.val() & 0x1) != 0)) // Adapt 0x3
      parents.push_back(it->first.val());
  }
  sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());
  
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, vector< Node::Id_Type >(), parents);
}


map< Uint32_Index, vector< Node_Skeleton > > small_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  map< Uint32_Index, vector< Node_Skeleton > > result;
  
  collect_items_range(stmt, rman, *osm_base_settings().NODES,
                      small_way_nd_indices< Way_Skeleton >(stmt, rman, ways.begin(), ways.end()),
                      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), result);
  
  return result;
}


void keep_matching_skeletons
    (std::vector< Node >& result,
     const map< Uint32_Index, vector< Node_Skeleton > >& current,
     const map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic,
     uint64 timestamp)
{
  std::map< Node_Skeleton::Id_Type, uint64 > timestamp_by_id;
  
  result.clear();
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      timestamp_by_id[it2->id] = NOW;
  }
  
  for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (vector< Attic<Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      uint64& stored_timestamp = timestamp_by_id[it2->id];
      if (it2->timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > it2->timestamp))
        stored_timestamp = it2->timestamp;
    }
  }
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = current.begin();
       it != current.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == NOW)
        result.push_back(Node(it2->id, it->first.val(), it2->ll_lower));
    }
  }
  
  for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::const_iterator it = attic.begin();
       it != attic.end(); ++it)
  {
    for (vector< Attic<Node_Skeleton > >::const_iterator it2 = it->second.begin();
         it2 != it->second.end(); ++it2)
    {
      if (timestamp_by_id[it2->id] == it2->timestamp)
        result.push_back(Node(it2->id, it->first.val(), it2->ll_lower));
    }
  }

  std::sort(result.begin(), result.end(), Node_Comparator_By_Id());
}


Way_Geometry_Store::Way_Geometry_Store
    (const map< Uint31_Index, vector< Way_Skeleton > >& ways, const Statement& query, Resource_Manager& rman)
{
  // Retrieve all nodes referred by the ways.
  map< Uint32_Index, vector< Node_Skeleton > > way_members_ = small_way_members(&query, rman, ways);
  
  // Order node ids by id.
  for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = way_members_.begin();
      it != way_members_.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());
}


Way_Geometry_Store::Way_Geometry_Store
    (const map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
     const Statement& query, Resource_Manager& rman)
{
  // Retrieve all nodes referred by the ways.
  map< Uint32_Index, vector< Node_Skeleton > > current;
  collect_items_range(&query, rman, *osm_base_settings().NODES,
                      small_way_nd_indices< Attic< Way_Skeleton > >(&query, rman, ways.begin(), ways.end()),
                      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), current);
  
  map< Uint32_Index, vector< Attic< Node_Skeleton > > > attic;
  collect_items_range(&query, rman, *attic_settings().NODES,
                      small_way_nd_indices< Attic< Way_Skeleton > >(&query, rman, ways.begin(), ways.end()),
                      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), attic);
  
  keep_matching_skeletons(nodes, current, attic, timestamp);
}


vector< Quad_Coord > Way_Geometry_Store::get_geometry(const Way_Skeleton& way) const
{
  if (way.geometry.empty())
    return make_geometry(way, nodes);
  else
    return way.geometry;
}
