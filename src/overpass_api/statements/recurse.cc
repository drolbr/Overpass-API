/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "recurse.h"
#include "union.h"

using namespace std;

const unsigned int RECURSE_RELATION_RELATION = 1;
const unsigned int RECURSE_RELATION_BACKWARDS = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;
const unsigned int RECURSE_WAY_NODE = 5;
const unsigned int RECURSE_WAY_RELATION = 6;
const unsigned int RECURSE_NODE_RELATION = 7;
const unsigned int RECURSE_NODE_WAY = 8;
const unsigned int RECURSE_DOWN = 9;
const unsigned int RECURSE_DOWN_REL = 10;
const unsigned int RECURSE_UP = 11;
const unsigned int RECURSE_UP_REL = 12;

Generic_Statement_Maker< Recurse_Statement > Recurse_Statement::statement_maker("recurse");

//-----------------------------------------------------------------------------

template < class TObject, class TPredicateA, class TPredicateB >
class And_Predicate
{
  public:
    And_Predicate(const TPredicateA& predicate_a_, const TPredicateB& predicate_b_)
    : predicate_a(predicate_a_), predicate_b(predicate_b_) {}
    bool match(const TObject& obj) const
    {
      return (predicate_a.match(obj) && predicate_b.match(obj));
    }
    
  private:
    TPredicateA predicate_a;
    TPredicateB predicate_b;
};

template < class TObject, class TPredicateA, class TPredicateB >
class Or_Predicate
{
  public:
    Or_Predicate(const TPredicateA& predicate_a_, const TPredicateB& predicate_b_)
    : predicate_a(predicate_a_), predicate_b(predicate_b_) {}
    bool match(const TObject& obj) const
    {
      return (predicate_a.match(obj) || predicate_b.match(obj));
    }
    
  private:
    TPredicateA predicate_a;
    TPredicateB predicate_b;
};

//-----------------------------------------------------------------------------

template < class TObject >
class Id_Predicate
{
  public:
    Id_Predicate(const vector< uint32 >& ids_) : ids(ids_) {}
    bool match(const TObject& obj) const
    {
      return binary_search(ids.begin(), ids.end(), obj.id);
    }
    
  private:
    const vector< uint32 >& ids;
};

//-----------------------------------------------------------------------------

bool has_a_child_with_id
    (const Relation_Skeleton& relation, const vector< uint32 >& ids, uint32 type)
{
  for (vector< Relation_Entry >::const_iterator it3(relation.members.begin());
      it3 != relation.members.end(); ++it3)
  {
    if ((it3->type == type) &&
        (binary_search(ids.begin(), ids.end(), it3->ref)))
      return true;
  }
  return false;
}

bool has_a_child_with_id
    (const Way_Skeleton& way, const vector< uint32 >& ids)
{
  for (vector< uint32 >::const_iterator it3(way.nds.begin());
      it3 != way.nds.end(); ++it3)
  {
    if (binary_search(ids.begin(), ids.end(), *it3))
      return true;
  }
  return false;
}

class Get_Parent_Rels_Predicate
{
public:
  Get_Parent_Rels_Predicate(const vector< uint32 >& ids_, uint32 child_type_)
    : ids(ids_), child_type(child_type_) {}
  bool match(const Relation_Skeleton& obj) const
  {
    return has_a_child_with_id(obj, ids, child_type);
  }
  
private:
  const vector< uint32 >& ids;
  uint32 child_type;
};

class Get_Parent_Ways_Predicate
{
public:
  Get_Parent_Ways_Predicate(const vector< uint32 >& ids_)
    : ids(ids_) {}
  bool match(const Way_Skeleton& obj) const
  {
    return has_a_child_with_id(obj, ids);
  }
  
private:
  const vector< uint32 >& ids;
};

//-----------------------------------------------------------------------------

template < class TIndex, class TObject, class TContainer, class TPredicate >
void collect_items_discrete(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const TContainer& req, const TPredicate& predicate,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject, typename TContainer::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< TIndex, TObject, typename TContainer
      ::const_iterator >::Discrete_Iterator
      it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
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

template < class TIndex, class TObject, class TContainer, class TPredicate >
void collect_items_range(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const TContainer& req, const TPredicate& predicate,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject, typename TContainer::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< TIndex, TObject, typename TContainer
      ::const_iterator >::Range_Iterator
      it(db.range_begin(req.begin(), req.end()));
	   !(it == db.range_end()); ++it)
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

template < class TIndex, class TObject, class TPredicate >
void collect_items_flat(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties, const TPredicate& predicate,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject > db
      (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
  for (typename Block_Backend< TIndex, TObject >::Flat_Iterator
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

//-----------------------------------------------------------------------------

template < class TIndex, class TObject, class TPredicate >
void filter_items(const TPredicate& predicate, map< TIndex, vector< TObject > >& data)
{
  for (typename map< TIndex, vector< TObject > >::iterator it = data.begin();
      it != data.end(); ++it)
  {
    vector< TObject > local_into;
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (predicate.match(*iit))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}

template< class TIndex, class TObject >
vector< uint32 > filter_for_ids(const map< TIndex, vector< TObject > >& elems)
{
  vector< uint32 > ids;
  for (typename map< TIndex, vector< TObject > >::const_iterator it = elems.begin();
      it != elems.end(); ++it)
  {
    for (typename vector< TObject >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      ids.push_back(iit->id);
  }
  sort(ids.begin(), ids.end());
  
  return ids;
}

//-----------------------------------------------------------------------------

void filter_for_member_ids(const vector< Relation_Skeleton >& relations, vector< uint32 >& ids,
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

vector< uint32 > filter_for_nd_ids
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end)
{
  vector< uint32 > ids;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(ways_begin); it != ways_end; ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      for (vector< uint32 >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  rman.health_check(stmt);
  sort(ids.begin(), ids.end());
  
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

set< pair< Uint32_Index, Uint32_Index > > collect_node_req
    (const Statement& stmt, Resource_Manager& rman,
     const vector< uint32 >& map_ids, const vector< uint32 >& parents)
{
  set< pair< Uint32_Index, Uint32_Index > > req = calc_node_children_ranges(parents);
  
  Random_File< Uint32_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().NODES));
  for (vector< uint32 >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
  {
    Uint32_Index idx = random.get(*it);
    req.insert(make_pair(idx, idx.val() + 1));
  }
  
  rman.health_check(stmt);
  
  return req;
}

set< pair< Uint32_Index, Uint32_Index > > relation_node_member_indices
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > map_ids;
  vector< uint32 > parents;
  
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
    it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0xfc) != 0))
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
  
  return collect_node_req(stmt, rman, map_ids, parents);
}

set< pair< Uint32_Index, Uint32_Index > > way_nd_indices
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end)
{
  vector< uint32 > map_ids;
  vector< uint32 > parents;
  
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0xfc) != 0))
    {
      // Treat ways with really large indices: get the node indexes from the segement indexes
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
	  for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
	      it2 != it->second.end(); ++it2)
	  {
	    for (vector< uint32 >::const_iterator it3(it2->nds.begin());
	        it3 != it2->nds.end(); ++it3)
	      map_ids.push_back(*it3);
	  }
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
  rman.health_check(stmt);
  
  return collect_node_req(stmt, rman, map_ids, parents);
}

vector< uint32 > relation_member_ids
    (const Statement& stmt, Resource_Manager& rman, uint32 type,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > ids;    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(rels_begin); it != rels_end; ++it)
    filter_for_member_ids(it->second, ids, type);
  
  rman.health_check(stmt);
  sort(ids.begin(), ids.end());
  
  return ids;
}

map< Uint32_Index, vector< Node_Skeleton > > relation_node_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& relations,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const vector< uint32 >* node_ids)
{
  vector< uint32 > intersect_ids;
  if (node_ids)
  {
    vector< uint32 > children_ids = relation_member_ids
        (stmt, rman, Relation_Entry::NODE, relations.begin(), relations.end());
    intersect_ids.resize(node_ids->size());
    intersect_ids.erase(set_intersection
        (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
	intersect_ids.begin()), intersect_ids.end());
  }
  else
    intersect_ids
        = relation_member_ids(stmt, rman, Relation_Entry::NODE, relations.begin(), relations.end());
    
  map< Uint32_Index, vector< Node_Skeleton > > result;
  if (node_ranges)
    collect_items_range(stmt, rman, *osm_base_settings().NODES, *node_ranges,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  else
  {    
    set< pair< Uint32_Index, Uint32_Index > > req =
        relation_node_member_indices(stmt, rman, relations.begin(), relations.end());
    collect_items_range(stmt, rman, *osm_base_settings().NODES, req,
			Id_Predicate< Node_Skeleton >(intersect_ids), result);
  }
  return result;
}
 
map< Uint32_Index, vector< Node_Skeleton > > way_members
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Way_Skeleton > >& ways,
     const set< pair< Uint32_Index, Uint32_Index > >* node_ranges,
     const vector< uint32 >* node_ids)
{  
  vector< uint32 > intersect_ids;
  
  if (node_ids)
  {
    vector< uint32 > children_ids = filter_for_nd_ids(stmt, rman, ways.begin(), ways.end());
    intersect_ids.resize(node_ids->size());
    intersect_ids.erase(set_intersection
        (node_ids->begin(), node_ids->end(), children_ids.begin(), children_ids.end(),
         intersect_ids.begin()), intersect_ids.end());
  }
  else
    intersect_ids = filter_for_nd_ids(stmt, rman, ways.begin(), ways.end());
  
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
 
//-----------------------------------------------------------------------------

vector< Uint31_Index > collect_relation_req
    (const Statement& stmt, Resource_Manager& rman,
     const vector< uint32 >& map_ids)
{
  vector< Uint31_Index > req;
  
  Random_File< Uint31_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().RELATIONS));
  for (vector< uint32 >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(*it));
  
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

template< class TIndex, class TObject >
vector< uint32 > extract_children_ids(const map< TIndex, vector< TObject > >& elems)
{
  vector< uint32 > ids;
  
  {
    for (typename map< TIndex, vector< TObject > >::const_iterator
        it(elems.begin()); it != elems.end(); ++it)
    {
      for (typename vector< TObject >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        ids.push_back(it2->id);
    }
  }
  
  sort(ids.begin(), ids.end());
  
  return ids;
}

template< class TIndex, class TObject >
set< Uint31_Index > extract_parent_indices(const map< TIndex, vector< TObject > >& elems)
{
  vector< uint32 > children;
  {
    for (typename map< TIndex, vector< TObject > >::const_iterator
        it(elems.begin()); it != elems.end(); ++it)
      children.push_back(it->first.val());
  }
  
  vector< uint32 > parents = calc_parents(children);
  
  set< Uint31_Index > req;
  for (vector< uint32 >::const_iterator it = parents.begin(); it != parents.end(); ++it)
    req.insert(Uint31_Index(*it));
  
  return req;
}

vector< Uint31_Index > collect_indices_31
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
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0xfc) != 0))
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

vector< Uint31_Index > collect_indices_31_rels
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > map_ids;
    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
    // Treat relations with really large indices: get the node indexes from nodes.map.
    filter_for_member_ids(it->second, map_ids, Relation_Entry::RELATION);

  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
    
  return collect_relation_req(stmt, rman, map_ids);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< Uint31_Index > req = collect_indices_31_rels(stmt, rman, rels_begin, rels_end);  
  vector< uint32 > ids = relation_member_ids(stmt, rman, Relation_Entry::RELATION, rels_begin, rels_end);  
  collect_items_discrete(stmt, rman, *osm_base_settings().RELATIONS, req, Id_Predicate< Relation_Skeleton >(ids), result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const set< pair< Uint31_Index, Uint31_Index > >& relation_ranges)
{
  vector< uint32 > ids = relation_member_ids(stmt, rman, Relation_Entry::RELATION, rels_begin, rels_end);  
  collect_items_range(stmt, rman, *osm_base_settings().RELATIONS, relation_ranges, Id_Predicate< Relation_Skeleton >(ids), result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< Uint31_Index > req = collect_indices_31_rels(stmt, rman, rels_begin, rels_end);  
  vector< uint32 > children_ids = relation_member_ids(stmt, rman, Relation_Entry::RELATION,
					      rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_discrete(stmt, rman, *osm_base_settings().RELATIONS, req, Id_Predicate< Relation_Skeleton >(ids), result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids,
     const set< pair< Uint31_Index, Uint31_Index > >& relation_ranges)
{
  vector< uint32 > children_ids = relation_member_ids(stmt, rman, Relation_Entry::RELATION,
					      rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_range(stmt, rman, *osm_base_settings().RELATIONS, relation_ranges, Id_Predicate< Relation_Skeleton >(ids), result);
}

template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< uint32 > ids = extract_children_ids(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(stmt, rman, *osm_base_settings().RELATIONS, req,
			 Get_Parent_Rels_Predicate(ids, source_type), result);
}

template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< uint32 > children_ids = extract_children_ids(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);
  
  collect_items_discrete(stmt, rman, *osm_base_settings().RELATIONS, req,
      And_Predicate< Relation_Skeleton,
	  Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
	  (Id_Predicate< Relation_Skeleton >(ids),
          Get_Parent_Rels_Predicate(children_ids, source_type)), result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< uint32 > ids = extract_children_ids(sources);    
  rman.health_check(stmt);
  
  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Predicate(ids, Relation_Entry::RELATION), result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< uint32 > children_ids = extract_children_ids(sources);    
  rman.health_check(stmt);
  
  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      And_Predicate< Relation_Skeleton,
	  Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
	  (Id_Predicate< Relation_Skeleton >(ids),
          Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
      result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result)
{
  vector< Uint31_Index > req = collect_indices_31(stmt, rman, rels_begin, rels_end);
  vector< uint32 > ids = relation_member_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req, Id_Predicate< Way_Skeleton >(ids), result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const set< pair< Uint31_Index, Uint31_Index > >& way_ranges)
{
  vector< uint32 > ids = relation_member_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  collect_items_range(stmt, rman, *osm_base_settings().WAYS, way_ranges, Id_Predicate< Way_Skeleton >(ids), result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< Uint31_Index > req = collect_indices_31(stmt, rman, rels_begin, rels_end);
  vector< uint32 > children_ids = relation_member_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req, Id_Predicate< Way_Skeleton >(intersect_ids), result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< uint32 >& ids,
     const set< pair< Uint31_Index, Uint31_Index > >& way_ranges)
{
  vector< uint32 > children_ids = relation_member_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_range(stmt, rman, *osm_base_settings().WAYS, way_ranges, Id_Predicate< Way_Skeleton >(intersect_ids), result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result)
{
  vector< uint32 > ids = extract_children_ids(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req,
      Get_Parent_Ways_Predicate(ids), result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< uint32 > children_ids = extract_children_ids(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  collect_items_discrete(stmt, rman, *osm_base_settings().WAYS, req,
      And_Predicate< Way_Skeleton,
	  Id_Predicate< Way_Skeleton >, Get_Parent_Ways_Predicate >
	  (Id_Predicate< Way_Skeleton >(ids), Get_Parent_Ways_Predicate(children_ids)), result);
}

void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		   const vector< uint32 >& ids,
		   map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = relation_node_members(query, rman, rels);
    else
      nodes = relation_node_members(query, rman, rels, 0, &ids);
  }
  else
  {
    if (ids.empty())
      nodes = relation_node_members(query, rman, rels, &ranges);
    else
      nodes = relation_node_members(query, rman, rels, &ranges, &ids);
  }
}

void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const map< Uint31_Index, vector< Way_Skeleton > >& rels,
		   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		   const vector< uint32 >& ids,
		   map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = way_members(query, rman, rels);
    else
      nodes = way_members(query, rman, rels, 0, &ids);
  }
  else
  {
    if (ids.empty())
      nodes = way_members(query, rman, rels, &ranges);
    else
      nodes = way_members(query, rman, rels, &ranges, &ids);
  }
}

void collect_ways(const Statement& query, Resource_Manager& rman,
		  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  const vector< uint32 >& ids,
		  map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  if (ranges.empty())
  {
    if (ids.empty())
      collect_ways(query, rman, rels.begin(), rels.end(), ways);
    else
      collect_ways(query, rman, rels.begin(), rels.end(), ways, ids);
  }
  else
  {
    if (ids.empty())
      collect_ways(query, rman, rels.begin(), rels.end(), ways, ranges);
    else
      collect_ways(query, rman, rels.begin(), rels.end(), ways, ids, ranges);
  }
}

void collect_relations(const Statement& query, Resource_Manager& rman,
		  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  const vector< uint32 >& ids,
		  map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  if (ranges.empty())
  {
    if (ids.empty())
      collect_relations(query, rman, rels.begin(), rels.end(), relations);
    else
      collect_relations(query, rman, rels.begin(), rels.end(), relations, ids);
  }
  else
  {
    if (ids.empty())
      collect_relations(query, rman, rels.begin(), rels.end(), relations, ranges);
    else
      collect_relations(query, rman, rels.begin(), rels.end(), relations, ids, ranges);
  }
}

uint count_relations(const map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  uint result = 0;
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator it = relations.begin();
      it != relations.end(); ++it)
    result += it->second.size();
  return result;
}

void relations_loop(const Statement& query, Resource_Manager& rman,
		    map< Uint31_Index, vector< Relation_Skeleton > > source,
		    map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  uint old_rel_count = count_relations(source);
  while (true)
  {
    collect_relations(query, rman, source.begin(), source.end(), result);
    indexed_set_union(result, source);
    uint new_rel_count = count_relations(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}

void relations_up_loop(const Statement& query, Resource_Manager& rman,
		    map< Uint31_Index, vector< Relation_Skeleton > > source,
		    map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  uint old_rel_count = count_relations(source);
  while (true)
  {
    result.clear();
    collect_relations(query, rman, source, result);
    indexed_set_union(result, source);
    uint new_rel_count = count_relations(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}

//-----------------------------------------------------------------------------

class Recurse_Constraint : public Query_Constraint
{
  public:
    Recurse_Constraint(Recurse_Statement& stmt_) : stmt(&stmt_) {}
    bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
		  const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		  const vector< uint32 >& ids);
    bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  int type, const vector< uint32 >& ids);
    void filter(Resource_Manager& rman, Set& into);
    void filter(const Statement& query, Resource_Manager& rman, Set& into);
    virtual ~Recurse_Constraint() {}
    
  private:
    Recurse_Statement* stmt;
};

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint32_Index, Uint32_Index > >& ranges,
     const vector< uint32 >& ids)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
    return true;
  
  if (stmt->get_type() == RECURSE_RELATION_NODE)
    collect_nodes(query, rman, mit->second.relations, ranges, ids, into.nodes);
  else if (stmt->get_type() == RECURSE_WAY_NODE)
    collect_nodes(query, rman, mit->second.ways, ranges, ids, into.nodes);
  else if (stmt->get_type() == RECURSE_DOWN)
  {
    map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
    collect_nodes(query, rman, mit->second.relations, ranges, ids, rel_nodes);
    collect_ways(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		 rel_ways);
    collect_nodes(query, rman, rel_ways, ranges, ids, into.nodes);
    indexed_set_union(into.nodes, rel_nodes);
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
    relations_loop(query, rman, mit->second.relations, rel_rels);
    map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
    collect_nodes(query, rman, rel_rels, ranges, ids, rel_nodes);
    collect_ways(query, rman, rel_rels.begin(), rel_rels.end(), rel_ways);
    collect_nodes(query, rman, rel_ways, ranges, ids, into.nodes);
    indexed_set_union(into.nodes, rel_nodes);
  }
  return true;
}

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint31_Index, Uint31_Index > >& ranges,
     int type, const vector< uint32 >& ids)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
    return true;
  
  if (stmt->get_type() == RECURSE_RELATION_WAY)
    collect_ways(query, rman, mit->second.relations, ranges, ids, into.ways);
  else if (stmt->get_type() == RECURSE_RELATION_RELATION)
    collect_relations(query, rman, mit->second.relations, ranges, ids, into.relations);
  else if (stmt->get_type() == RECURSE_DOWN)
  {
    if (type != QUERY_WAY)
      return true;
    collect_ways(query, rman, mit->second.relations, ranges, ids, into.ways);
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
    relations_loop(query, rman, mit->second.relations, rel_rels);
    if (type == QUERY_WAY)
      collect_ways(query, rman, rel_rels, ranges, ids, into.ways);
    else
    {
      if (!ids.empty())
	filter_items(Id_Predicate< Relation_Skeleton >(ids), rel_rels);
      into.relations.swap(rel_rels);
    }
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY)
  {
    if (ids.empty())
      collect_ways(query, rman, mit->second.nodes, into.ways);
    else
      collect_ways(query, rman, mit->second.nodes, into.ways, ids);
  }
  else if (stmt->get_type() == RECURSE_NODE_RELATION)
  {
    if (ids.empty())
      collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations);
    else
      collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, ids);
  }
  else if (stmt->get_type() == RECURSE_WAY_RELATION)
  {
    if (ids.empty())
      collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY, into.relations);
    else
      collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY, into.relations, ids);
  }
  else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
  {
    if (ids.empty())
      collect_relations(query, rman, mit->second.relations, into.relations);
    else
      collect_relations(query, rman, mit->second.relations, into.relations, ids);
  }
  else if (stmt->get_type() == RECURSE_UP)
  {
    if (type == QUERY_WAY)
    {
      if (ids.empty())
	collect_ways(query, rman, mit->second.nodes, into.ways);
      else
	collect_ways(query, rman, mit->second.nodes, into.ways, ids);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, mit->second.nodes, node_ways);    
      indexed_set_union(rel_ways, node_ways);
      if (ids.empty())
	collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations);
      else
	collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations, ids);
      
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      if (ids.empty())
	collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
      else
	collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels, ids);
      indexed_set_union(into.relations, node_rels);
    }
  }
  else if (stmt->get_type() == RECURSE_UP_REL)
  {
    if (type == QUERY_WAY)
    {
      if (ids.empty())
	collect_ways(query, rman, mit->second.nodes, into.ways);
      else
	collect_ways(query, rman, mit->second.nodes, into.ways, ids);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, mit->second.nodes, node_ways);    
      indexed_set_union(rel_ways, node_ways);
      map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
      collect_relations(query, rman, rel_ways, Relation_Entry::WAY, way_rels);
    
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
      indexed_set_union(rel_rels, way_rels);
    
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
      indexed_set_union(rel_rels, node_rels);
    
      relations_up_loop(query, rman, rel_rels, into.relations);

      if (!ids.empty())
	filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
    }
  }
  return true;
}

void Recurse_Constraint::filter(Resource_Manager& rman, Set& into)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
  {
    into.nodes.clear();
    into.ways.clear();
    into.relations.clear();
    return;
  }
  
  if (stmt->get_type() == RECURSE_DOWN || stmt->get_type() == RECURSE_DOWN_REL)
    return;
  
  vector< uint32 > ids;
  if (stmt->get_type() == RECURSE_WAY_NODE)
    ids = filter_for_nd_ids(*stmt, rman, mit->second.ways.begin(), mit->second.ways.end());
  else if (stmt->get_type() == RECURSE_RELATION_NODE)
    ids = relation_member_ids(*stmt, rman, Relation_Entry::NODE,
		      mit->second.relations.begin(), mit->second.relations.end());
  
  filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
  
  if (stmt->get_type() == RECURSE_RELATION_WAY)
  {
    vector< uint32 > ids = relation_member_ids(*stmt, rman, Relation_Entry::WAY,
	mit->second.relations.begin(), mit->second.relations.end());
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_UP
      || stmt->get_type() == RECURSE_UP_REL)
  {
    vector< uint32 > ids = extract_children_ids(mit->second.nodes);
    filter_items(Get_Parent_Ways_Predicate(ids), into.ways);
  }
  else
    into.ways.clear();
    
  if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
    return;
  
  ids.clear();
  if (stmt->get_type() == RECURSE_RELATION_RELATION)
  {
    vector< uint32 > ids = relation_member_ids(*stmt, rman, Relation_Entry::RELATION,
	mit->second.relations.begin(), mit->second.relations.end());
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
  }
  else if (stmt->get_type() == RECURSE_NODE_RELATION
      || stmt->get_type() == RECURSE_WAY_RELATION
      || stmt->get_type() == RECURSE_RELATION_BACKWARDS)
  {
    uint32 source_type;    
    if (stmt->get_type() == RECURSE_NODE_RELATION)
      source_type = Relation_Entry::NODE;
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      source_type = Relation_Entry::WAY;
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      source_type = Relation_Entry::RELATION;
    
    vector< uint32 > ids;
    if (stmt->get_type() == RECURSE_NODE_RELATION)
      ids = extract_children_ids(mit->second.nodes);
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      ids = extract_children_ids(mit->second.ways);
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      ids = extract_children_ids(mit->second.relations);
    
    filter_items(Get_Parent_Rels_Predicate(ids, source_type), into.relations);
  }
  else
    into.relations.clear();
}

void Recurse_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  
  if (stmt->get_type() != RECURSE_DOWN && stmt->get_type() != RECURSE_DOWN_REL)
    return;
  
  vector< uint32 > ids;
  if (stmt->get_type() == RECURSE_DOWN)
  {
    vector< uint32 > rel_ids
        = relation_member_ids(*stmt, rman, Relation_Entry::NODE,
		      mit->second.relations.begin(), mit->second.relations.end());;
    map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
    collect_ways(query, rman, mit->second.relations, set< pair< Uint31_Index, Uint31_Index > >(),
		 ids, intermediate_ways);
    vector< uint32 > way_ids
        = filter_for_nd_ids(*stmt, rman, intermediate_ways.begin(), intermediate_ways.end());
    set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

    ids = relation_member_ids(*stmt, rman, Relation_Entry::WAY,
	mit->second.relations.begin(), mit->second.relations.end());
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
    
    into.relations.clear();
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
    relations_loop(query, rman, mit->second.relations, rel_rels);
    vector< uint32 > rel_ids
        = relation_member_ids(*stmt, rman, Relation_Entry::NODE, rel_rels.begin(), rel_rels.end());
    map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
    collect_ways(query, rman, rel_rels, set< pair< Uint31_Index, Uint31_Index > >(),
		 ids, intermediate_ways);
    vector< uint32 > way_ids
        = filter_for_nd_ids(*stmt, rman, intermediate_ways.begin(), intermediate_ways.end());
    set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

    ids = relation_member_ids(*stmt, rman, Relation_Entry::WAY,
		      rel_rels.begin(), rel_rels.end());
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);

    filter_items(Id_Predicate< Relation_Skeleton >(filter_for_ids(rel_rels)), into.relations);
  }
  else if (stmt->get_type() == RECURSE_UP && !into.relations.empty())
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    map< Uint31_Index, vector< Way_Skeleton > > node_ways;
    collect_ways(query, rman, mit->second.nodes, node_ways);    
    indexed_set_union(rel_ways, node_ways);
    
    vector< uint32 > node_ids = extract_children_ids(mit->second.nodes);
    vector< uint32 > way_ids = extract_children_ids(rel_ways);
    
    filter_items(
        Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
        (Get_Parent_Rels_Predicate(node_ids, Relation_Entry::NODE),
	 Get_Parent_Rels_Predicate(way_ids, Relation_Entry::WAY)), into.relations);
  }
  else if (stmt->get_type() == RECURSE_UP_REL && !into.relations.empty())
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    map< Uint31_Index, vector< Way_Skeleton > > node_ways;
    collect_ways(query, rman, mit->second.nodes, node_ways);    
    indexed_set_union(rel_ways, node_ways);
    map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
    collect_relations(query, rman, rel_ways, Relation_Entry::WAY, way_rels);
    
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
    indexed_set_union(rel_rels, way_rels);
    
    map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
    collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
    indexed_set_union(rel_rels, node_rels);
    
    relations_up_loop(query, rman, rel_rels, rel_rels);

    ids = extract_children_ids(rel_rels);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
  }
}

//-----------------------------------------------------------------------------

Recurse_Statement::Recurse_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  output = attributes["into"];
  
  if (attributes["type"] == "relation-relation")
    type = RECURSE_RELATION_RELATION;
  else if (attributes["type"] == "relation-backwards")
    type = RECURSE_RELATION_BACKWARDS;
  else if (attributes["type"] == "relation-way")
    type = RECURSE_RELATION_WAY;
  else if (attributes["type"] == "relation-node")
    type = RECURSE_RELATION_NODE;
  else if (attributes["type"] == "way-node")
    type = RECURSE_WAY_NODE;
  else if (attributes["type"] == "down")
    type = RECURSE_DOWN;
  else if (attributes["type"] == "down-rel")
    type = RECURSE_DOWN_REL;
  else if (attributes["type"] == "way-relation")
    type = RECURSE_WAY_RELATION;
  else if (attributes["type"] == "node-relation")
    type = RECURSE_NODE_RELATION;
  else if (attributes["type"] == "node-way")
    type = RECURSE_NODE_WAY;
  else if (attributes["type"] == "up")
    type = RECURSE_UP;
  else if (attributes["type"] == "up-rel")
    type = RECURSE_UP_REL;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"relation-relation\", \"relation-backwards\","
	<<"\"relation-way\", \"relation-node\", \"way-node\", \"way-relation\","
	<<"\"node-relation\", \"node-way\", \"down\", \"down-rel\", \"up\", or \"up-rel\".";
    add_static_error(temp.str());
  }
}

void Recurse_Statement::forecast()
{
}

void Recurse_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();  
  
  map< string, Set >::const_iterator mit(rman.sets().find(input));
  if (mit == rman.sets().end())
  {
    rman.sets()[output].nodes.clear();
    rman.sets()[output].ways.clear();
    rman.sets()[output].relations.clear();
    rman.sets()[output].areas.clear();
    
    return;
  }

  Set into;

  if (type == RECURSE_RELATION_RELATION)
    collect_relations(*this, rman, mit->second.relations.begin(), mit->second.relations.end(),
		      into.relations);
  else if (type == RECURSE_RELATION_BACKWARDS)
    collect_relations(*this, rman, mit->second.relations, into.relations);
  else if (type == RECURSE_RELATION_WAY)
    collect_ways(*this, rman, mit->second.relations.begin(), mit->second.relations.end(),
		 into.ways);
  else if (type == RECURSE_RELATION_NODE)
    into.nodes = relation_node_members(*this, rman, mit->second.relations);
  else if (type == RECURSE_WAY_NODE)
    into.nodes = way_members(*this, rman, mit->second.ways);
  else if (type == RECURSE_DOWN)
  {
    map< Uint32_Index, vector< Node_Skeleton > > rel_nodes
        = relation_node_members(*this, rman, mit->second.relations);
    collect_ways(*this, rman, mit->second.relations.begin(), mit->second.relations.end(),
		 into.ways);
    into.nodes = way_members(*this, rman, into.ways);
    indexed_set_union(into.nodes, rel_nodes);
  }
  else if (type == RECURSE_DOWN_REL)
  {
    relations_loop(*this, rman, mit->second.relations, into.relations);    
    map< Uint32_Index, vector< Node_Skeleton > > rel_nodes
        = relation_node_members(*this, rman, into.relations);
    collect_ways(*this, rman, into.relations.begin(), into.relations.end(), into.ways);
    into.nodes = way_members(*this, rman, into.ways);
    indexed_set_union(into.nodes, rel_nodes);
  }
  else if (type == RECURSE_WAY_RELATION)
    collect_relations(*this, rman, mit->second.ways, Relation_Entry::WAY, into.relations);
  else if (type == RECURSE_NODE_WAY)
    collect_ways(*this, rman, mit->second.nodes, into.ways);
  else if (type == RECURSE_NODE_RELATION)
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, into.relations);
  else if (type == RECURSE_UP)
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    collect_ways(*this, rman, mit->second.nodes, into.ways);
    
    indexed_set_union(rel_ways, into.ways);
    collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, into.relations);
    
    map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
    indexed_set_union(into.relations, node_rels);
  }
  else if (type == RECURSE_UP_REL)
  {
    map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
    collect_ways(*this, rman, mit->second.nodes, into.ways);
    
    map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
    map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
    indexed_set_union(rel_ways, into.ways);
    collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, way_rels);
    indexed_set_union(rel_rels, way_rels);
    
    map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
    indexed_set_union(rel_rels, node_rels);

    relations_up_loop(*this, rman, rel_rels, into.relations);    
  }
  
  into.nodes.swap(rman.sets()[output].nodes);
  into.ways.swap(rman.sets()[output].ways);
  into.relations.swap(rman.sets()[output].relations);
  rman.sets()[output].areas.clear();
  
  stopwatch.stop(Stopwatch::NO_DISK);
  stopwatch.report(get_name());
  rman.health_check(*this);
}

Recurse_Statement::~Recurse_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

Query_Constraint* Recurse_Statement::get_query_constraint()
{
  constraints.push_back(new Recurse_Constraint(*this));
  return constraints.back();
}
