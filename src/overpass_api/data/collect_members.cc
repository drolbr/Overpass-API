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

using namespace std;

//-----------------------------------------------------------------------------

void filter_for_member_ids(const vector< Relation_Skeleton >& relations, vector< Uint32_Index >& ids,
		 uint32 type)
{
  for (vector< Relation_Skeleton >::const_iterator it2(relations.begin());
      it2 != relations.end(); ++it2)
  {
    for (vector< Relation_Entry >::const_iterator it3(it2->members.begin());
        it3 != it2->members.end(); ++it3)
    {
      if (it3->type == type)
	ids.push_back(it3->ref);
    }
  }
}

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
    (const Statement& stmt, Resource_Manager& rman,
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
  
  rman.health_check(stmt);
  sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());
  rman.health_check(stmt);
  
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

vector< Uint31_Index > relation_way_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > map_ids;
  vector< uint32 > parents;
  vector< Uint31_Index > children_idxs;
    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat relations with really large indices: get the ways indexes explicitly
      for (vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
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
  rman.health_check(stmt);
    
  return collect_way_req(stmt, rman, map_ids, parents, children_idxs);
}

set< pair< Uint32_Index, Uint32_Index > > relation_node_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< Node::Id_Type > map_ids;
  vector< uint32 > parents;
  
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat ways with really large indices: get the node indexes from the segement indexes
      for (vector< Relation_Skeleton >::const_iterator it2 = it->second.begin();
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
  rman.health_check(stmt);
  
  return collect_node_req(&stmt, rman, map_ids, parents);
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
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
    {
      // Treat ways with really large indices: get the node indexes from the segment indexes
      for (vector< Way_Skeleton >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
      {
	bool large_indices = false;
	for (vector< Uint31_Index >::const_iterator it3 = it2->segment_idxs.begin();
	    it3 != it2->segment_idxs.end(); ++it3)
	{
	  if ((it3->val() & 0x80000000) && ((it3->val() & 0xf) == 0))
	  {
	    //Treat ways with really large indices: get the node indexes from nodes.map.
	    large_indices = true;
	    break;
	  }
	}
	
	if (large_indices)
	{
	  for (vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
	      it3 != it2->nds.end(); ++it3)
	    map_ids.push_back(*it3);
	}
	else
	{
	  for (vector< Uint31_Index >::const_iterator it3 = it2->segment_idxs.begin();
	      it3 != it2->segment_idxs.end(); ++it3)
	    parents.push_back(it3->val());
	}
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

vector< Uint32_Index > relation_member_ids
    (const Statement& stmt, Resource_Manager& rman, uint32 type,
     const map< Uint31_Index, vector< Relation_Skeleton > >& rels)
{
  vector< Uint32_Index > ids;    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(rels.begin()); it != rels.end(); ++it)
    filter_for_member_ids(it->second, ids, type);
  
  rman.health_check(stmt);
  sort(ids.begin(), ids.end());
  
  return ids;
}

map< Uint31_Index, vector< Relation_Skeleton > > relation_relation_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& parents,
     const set< pair< Uint31_Index, Uint31_Index > >* children_ranges,
     const vector< Relation::Id_Type >* children_ids, bool invert_ids)
{
  vector< Relation::Id_Type > intersect_ids;
  if (children_ids)
  {
    vector< Relation::Id_Type > children_ids_ = relation_member_ids
        (stmt, rman, Relation_Entry::RELATION, parents);
    intersect_ids.resize(children_ids->size(), Relation::Id_Type(0u));
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
    intersect_ids
        = relation_member_ids(stmt, rman, Relation_Entry::RELATION, parents);
    
  map< Uint31_Index, vector< Relation_Skeleton > > result;
  if (children_ranges)
    collect_items_range(&stmt, rman, *osm_base_settings().RELATIONS, *children_ranges,
			Id_Predicate< Relation_Skeleton >(intersect_ids), result);
  else
  {    
    vector< Uint31_Index > req =
        relation_relation_member_indices(stmt, rman, parents.begin(), parents.end());
    collect_items_discrete(stmt, rman, *osm_base_settings().RELATIONS, req,
			Id_Predicate< Relation_Skeleton >(intersect_ids), result);
  }
  return result;
}
 
map< Uint31_Index, vector< Way_Skeleton > > relation_way_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint31_Index, Uint31_Index > >* way_ranges,
     const vector< Way::Id_Type >* way_ids, bool invert_ids)
{
  vector< Way::Id_Type > intersect_ids;
  if (way_ids)
  {
    vector< Way::Id_Type > children_ids = relation_member_ids
        (stmt, rman, Relation_Entry::WAY, relations);
    intersect_ids.resize(way_ids->size(), Way::Id_Type(0u));
    if (!invert_ids)
    intersect_ids.erase(set_intersection
        (way_ids->begin(), way_ids->end(), children_ids.begin(), children_ids.end(),
	intersect_ids.begin()), intersect_ids.end());
  }
  else
    intersect_ids
        = relation_member_ids(stmt, rman, Relation_Entry::WAY, relations);
    
  map< Uint31_Index, vector< Way_Skeleton > > result;
  if (way_ranges)
    collect_items_range(&stmt, rman, *osm_base_settings().WAYS, *way_ranges,
			Id_Predicate< Way_Skeleton >(intersect_ids), result);
  else
  {    
    vector< Uint31_Index > req =
        relation_way_member_indices(stmt, rman, relations.begin(), relations.end());
    collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req,
			Id_Predicate< Way_Skeleton >(intersect_ids), result);
  }
  return result;
}
 
map< Uint32_Index, vector< Node_Skeleton > > relation_node_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const vector< Node::Id_Type >* node_ids, bool invert_ids)
{
  vector< Node::Id_Type > intersect_ids;
  if (node_ids)
  {
    vector< Node::Id_Type > children_ids = relation_member_ids
        (stmt, rman, Relation_Entry::NODE, relations);
    intersect_ids.resize(node_ids->size(), Node::Id_Type(0u));
    if (!invert_ids)
      intersect_ids.erase(set_intersection
          (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
	  intersect_ids.begin()), intersect_ids.end());
    else
      intersect_ids.erase(set_intersection
          (children_ids.begin(), children_ids.end(),
	   node_ids->begin(), node_ids->end(),
	  intersect_ids.begin()), intersect_ids.end());
  }
  else
    intersect_ids
        = relation_member_ids(stmt, rman, Relation_Entry::NODE, relations);
    
  map< Uint32_Index, vector< Node_Skeleton > > result;
  if (node_ranges)
    collect_items_range(&stmt, rman, *osm_base_settings().NODES, *node_ranges,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  else
  {    
    set< pair< Uint32_Index, Uint32_Index > > req =
        relation_node_member_indices(stmt, rman, relations.begin(), relations.end());
    collect_items_range(&stmt, rman, *osm_base_settings().NODES, req,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  }
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
    intersect_ids.resize(children_ids.size(), Node::Id_Type(0u));
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
