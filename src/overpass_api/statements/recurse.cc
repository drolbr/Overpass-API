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

using namespace std;

const unsigned int RECURSE_RELATION_RELATION = 1;
const unsigned int RECURSE_RELATION_BACKWARDS = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;
const unsigned int RECURSE_WAY_NODE = 5;
const unsigned int RECURSE_WAY_RELATION = 6;
const unsigned int RECURSE_NODE_RELATION = 7;
const unsigned int RECURSE_NODE_WAY = 8;

Generic_Statement_Maker< Recurse_Statement > Recurse_Statement::statement_maker("recurse");

//-----------------------------------------------------------------------------

template < class TIndex, class TObject, class TContainer >
void collect_items(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const TContainer& req, vector< uint32 > ids,
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
    if (binary_search(ids.begin(), ids.end(), it.object().id))
      result[it.index()].push_back(it.object());
  }
}

// template < class TContainer >
// void collect_items_rels(const Statement& stmt, Resource_Manager& rman,
// 		        File_Properties& file_properties,
// 		        const TContainer& req, vector< uint32 > ids,
// 		        map< TIndex, vector< TObject > >& result)
// {
//   uint32 count = 0;
//   
//   Block_Backend< Uint31_Index, Relation_Skeleton, typename TContainer::const_iterator >
//       relations_db(rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
//   for (typename Block_Backend< TIndex, TObject, typename TContainer
//       ::const_iterator >::Discrete_Iterator
//       it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
//   {
//     const Relation_Skeleton& relation(it.object());
//     for (vector< Relation_Entry >::const_iterator it3(relation.members.begin());
//         it3 != relation.members.end(); ++it3)
//     {
//       if ((it3->type == Relation_Entry::RELATION) &&
// 	(binary_search(ids.begin(), ids.end(), it3->ref)))
//       {
// 	relations[it.index()].push_back(relation);
// 	break;
//       }
//     }
//   }
//   
//   Block_Backend< TIndex, TObject, typename TContainer::const_iterator > db
//       (rman.get_transaction()->data_index(&file_properties));
//   for (typename Block_Backend< TIndex, TObject, typename TContainer
//       ::const_iterator >::Discrete_Iterator
//       it(db.discrete_begin(req.begin(), req.end())); !(it == db.discrete_end()); ++it)
//   {
//     if (++count >= 64*1024)
//     {
//       count = 0;
//       rman.health_check(stmt);
//     }
//     if (binary_search(ids.begin(), ids.end(), it.object().id))
//       result[it.index()].push_back(it.object());
//   }
// }

template < class TIndex, class TObject, class TContainer >
void collect_items_range(const Statement& stmt, Resource_Manager& rman,
		   File_Properties& file_properties,
		   const TContainer& req, vector< uint32 > ids,
		   map< TIndex, vector< TObject > >& result)
{
  uint32 count = 0;
  Block_Backend< TIndex, TObject, typename TContainer::const_iterator > db
      (rman.get_transaction()->data_index(&file_properties));
  for (typename Block_Backend< TIndex, TObject, typename TContainer
      ::const_iterator >::Range_Iterator
      it(db.range_begin(req.begin(), req.end())); !(it == db.range_end()); ++it)
  {
    if (++count >= 64*1024)
    {
      count = 0;
      rman.health_check(stmt);
    }
    if (binary_search(ids.begin(), ids.end(), it.object().id))
      result[it.index()].push_back(it.object());
  }
}

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
     const vector< uint32 >& map_ids, const vector< uint32 >& parents)
{
  vector< Uint31_Index > req = calc_children(parents);
  
  Random_File< Uint31_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().WAYS));
  for (vector< uint32 >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(*it));
  
  rman.health_check(stmt);
  sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());
  rman.health_check(stmt);
  
  return req;
}

vector< Uint32_Index > collect_node_req
    (const Statement& stmt, Resource_Manager& rman,
     const vector< uint32 >& map_ids, const vector< uint32 >& parents)
{
  vector< Uint32_Index > req = calc_node_children(parents);
  
  Random_File< Uint32_Index > random
      (rman.get_transaction()->random_index(osm_base_settings().NODES));
  for (vector< uint32 >::const_iterator
      it(map_ids.begin()); it != map_ids.end(); ++it)
    req.push_back(random.get(*it));
  
  rman.health_check(stmt);
  sort(req.begin(), req.end());
  req.erase(unique(req.begin(), req.end()), req.end());
  rman.health_check(stmt);
  
  return req;
}

void extract_ids(const vector< Relation_Skeleton >& relations, vector< uint32 >& ids,
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

void extract_ids(const vector< Way_Skeleton >& ways, vector< uint32 >& ids)
{
  for (vector< Way_Skeleton >::const_iterator it2(ways.begin());
      it2 != ways.end(); ++it2)
  {
    for (vector< uint32 >::const_iterator it3(it2->nds.begin());
        it3 != it2->nds.end(); ++it3)
      ids.push_back(*it3);
  }
}

bool has_a_child_with_id
    (const Relation_Skeleton& relation, vector< uint32 >& ids, uint32 type)
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
    (const Way_Skeleton& way, vector< uint32 >& ids)
{
  for (vector< uint32 >::const_iterator it3(way.nds.begin());
      it3 != way.nds.end(); ++it3)
  {
    if (binary_search(ids.begin(), ids.end(), *it3))
      return true;
  }
  return false;
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
    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0xf) == 0))
      // Treat relations with really large indices: get the node indexes from nodes.map.
      extract_ids(it->second, map_ids, Relation_Entry::WAY);
    else
      parents.push_back(it->first.val());      
  }    
  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
    
  return collect_way_req(stmt, rman, map_ids, parents);
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
    extract_ids(it->second, map_ids, Relation_Entry::RELATION);

  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
    
  return collect_relation_req(stmt, rman, map_ids);
}

vector< Uint32_Index > collect_indices_32
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > map_ids;
  vector< uint32 > parents;
  
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
    it = rels_begin; it != rels_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0xf) == 0))
      // Treat relations with really large indices: get the node indexes from nodes.map.
      extract_ids(it->second, map_ids, Relation_Entry::NODE);
    else
      parents.push_back(it->first.val());      
  }    
  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
  
  return collect_node_req(stmt, rman, map_ids, parents);
}

vector< Uint32_Index > collect_indices_from_way
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end)
{
  vector< uint32 > map_ids;
  vector< uint32 > parents;
  
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0xf) == 0))
      // Treat ways with really large indices: get the node indexes from nodes.map.
      extract_ids(it->second, map_ids);
    else
      parents.push_back(it->first.val());
  }
  sort(map_ids.begin(), map_ids.end());
  rman.health_check(stmt);
  
  return collect_node_req(stmt, rman, map_ids, parents);
}

vector< uint32 > collect_ids
    (const Statement& stmt, Resource_Manager& rman, uint32 type,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end)
{
  vector< uint32 > ids;    
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(rels_begin); it != rels_end; ++it)
    extract_ids(it->second, ids, type);
  
  rman.health_check(stmt);
  sort(ids.begin(), ids.end());
  
  return ids;
}

vector< uint32 > collect_ids
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end)
{
  vector< uint32 > ids;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(ways_begin); it != ways_end; ++it)
    extract_ids(it->second, ids);
  
  rman.health_check(stmt);
  sort(ids.begin(), ids.end());
  
  return ids;
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< Uint31_Index > req = collect_indices_31_rels(stmt, rman, rels_begin, rels_end);  
  vector< uint32 > ids = collect_ids(stmt, rman, Relation_Entry::RELATION, rels_begin, rels_end);  
  collect_items(stmt, rman, *osm_base_settings().RELATIONS, req, ids, result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const set< pair< Uint31_Index, Uint31_Index > >& relation_ranges)
{
  vector< uint32 > ids = collect_ids(stmt, rman, Relation_Entry::RELATION, rels_begin, rels_end);  
  collect_items_range(stmt, rman, *osm_base_settings().RELATIONS, relation_ranges, ids, result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< Uint31_Index > req = collect_indices_31_rels(stmt, rman, rels_begin, rels_end);  
  vector< uint32 > children_ids = collect_ids(stmt, rman, Relation_Entry::RELATION,
					      rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items(stmt, rman, *osm_base_settings().RELATIONS, req, ids, result);
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids,
     const set< pair< Uint31_Index, Uint31_Index > >& relation_ranges)
{
  vector< uint32 > children_ids = collect_ids(stmt, rman, Relation_Entry::RELATION,
					      rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_range(stmt, rman, *osm_base_settings().RELATIONS, relation_ranges, ids, result);
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
  
  Block_Backend< Uint31_Index, Relation_Skeleton > targets_db
      (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Discrete_Iterator
      it(targets_db.discrete_begin(req.begin(), req.end()));
      !(it == targets_db.discrete_end()); ++it)
  {
    if (has_a_child_with_id(it.object(), ids, source_type))
      result[it.index()].push_back(it.object());
  }
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
  
  Block_Backend< Uint31_Index, Relation_Skeleton > targets_db
      (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Discrete_Iterator
      it(targets_db.discrete_begin(req.begin(), req.end()));
      !(it == targets_db.discrete_end()); ++it)
  {
    if (binary_search(ids.begin(), ids.end(), it.object().id)
        && has_a_child_with_id(it.object(), children_ids, source_type))
      result[it.index()].push_back(it.object());
  }
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< uint32 > ids = extract_children_ids(sources);    
  rman.health_check(stmt);
  
  Block_Backend< Uint31_Index, Relation_Skeleton > targets_db
      (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
      it(targets_db.flat_begin()); !(it == targets_db.flat_end()); ++it)
  {
    if (has_a_child_with_id(it.object(), ids, Relation_Entry::RELATION))
      result[it.index()].push_back(it.object());
  }
}

void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< uint32 > children_ids = extract_children_ids(sources);    
  rman.health_check(stmt);
  
  Block_Backend< Uint31_Index, Relation_Skeleton > targets_db
      (rman.get_transaction()->data_index(osm_base_settings().RELATIONS));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
      it(targets_db.flat_begin()); !(it == targets_db.flat_end()); ++it)
  {
    if (binary_search(ids.begin(), ids.end(), it.object().id)
        && has_a_child_with_id(it.object(), children_ids, Relation_Entry::RELATION))
      result[it.index()].push_back(it.object());
  }
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result)
{
  vector< Uint31_Index > req = collect_indices_31(stmt, rman, rels_begin, rels_end);
  vector< uint32 > ids = collect_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  collect_items(stmt, rman, *osm_base_settings().WAYS, req, ids, result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const set< pair< Uint31_Index, Uint31_Index > >& way_ranges)
{
  vector< uint32 > ids = collect_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  collect_items_range(stmt, rman, *osm_base_settings().WAYS, way_ranges, ids, result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< Uint31_Index > req = collect_indices_31(stmt, rman, rels_begin, rels_end);
  vector< uint32 > children_ids = collect_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items(stmt, rman, *osm_base_settings().WAYS, req, intersect_ids, result);
}

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< uint32 >& ids,
     const set< pair< Uint31_Index, Uint31_Index > >& way_ranges)
{
  vector< uint32 > children_ids = collect_ids(stmt, rman, Relation_Entry::WAY, rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_range(stmt, rman, *osm_base_settings().WAYS, way_ranges, intersect_ids, result);
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
  
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (rman.get_transaction()->data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
      it(ways_db.discrete_begin(req.begin(), req.end()));
      !(it == ways_db.discrete_end()); ++it)
  {
    if (has_a_child_with_id(it.object(), ids))
      result[it.index()].push_back(it.object());
  }
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
  
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (rman.get_transaction()->data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
      it(ways_db.discrete_begin(req.begin(), req.end()));
      !(it == ways_db.discrete_end()); ++it)
  {
    if (binary_search(ids.begin(), ids.end(), it.object().id)
        && has_a_child_with_id(it.object(), children_ids))
      result[it.index()].push_back(it.object());
  }
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const set< pair< Uint32_Index, Uint32_Index > >& node_ranges)
{
  vector< uint32 > ids = collect_ids(stmt, rman, Relation_Entry::NODE, rels_begin, rels_end);
  collect_items_range(stmt, rman, *osm_base_settings().NODES, node_ranges, ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result)
{
  vector< Uint32_Index > req = collect_indices_32(stmt, rman, rels_begin, rels_end);
  vector< uint32 > ids = collect_ids(stmt, rman, Relation_Entry::NODE, rels_begin, rels_end);
  collect_items(stmt, rman, *osm_base_settings().NODES, req, ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const vector< uint32 >& ids,
     const set< pair< Uint32_Index, Uint32_Index > >& node_ranges)
{
  vector< uint32 > children_ids = collect_ids
      (stmt, rman, Relation_Entry::NODE, rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_range(stmt, rman, *osm_base_settings().NODES, node_ranges, intersect_ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< Uint32_Index > req = collect_indices_32(stmt, rman, rels_begin, rels_end);
  vector< uint32 > children_ids = collect_ids
      (stmt, rman, Relation_Entry::NODE, rels_begin, rels_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items(stmt, rman, *osm_base_settings().NODES, req, intersect_ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const set< pair< Uint32_Index, Uint32_Index > >& node_ranges)
{
  vector< uint32 > ids = collect_ids(stmt, rman, ways_begin, ways_end);
  collect_items_range(stmt, rman, *osm_base_settings().NODES, node_ranges, ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result)
{
  vector< Uint32_Index > req = collect_indices_from_way(stmt, rman, ways_begin, ways_end);
  vector< uint32 > ids = collect_ids(stmt, rman, ways_begin, ways_end);
  collect_items(stmt, rman, *osm_base_settings().NODES, req, ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const vector< uint32 >& ids,
     const set< pair< Uint32_Index, Uint32_Index > >& node_ranges)
{
  vector< uint32 > children_ids = collect_ids(stmt, rman, ways_begin, ways_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items_range(stmt, rman, *osm_base_settings().NODES, node_ranges, intersect_ids, result);
}

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const vector< uint32 >& ids)
{
  vector< Uint32_Index > req = collect_indices_from_way(stmt, rman, ways_begin, ways_end);
  vector< uint32 > children_ids = collect_ids(stmt, rman, ways_begin, ways_end);
  vector< uint32 > intersect_ids(ids.size());
  intersect_ids.erase(set_intersection
      (ids.begin(), ids.end(), children_ids.begin(), children_ids.end(), intersect_ids.begin()),
      intersect_ids.end());
  collect_items(stmt, rman, *osm_base_settings().NODES, req, intersect_ids, result);
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
  {
    if (ranges.empty())
    {
      if (ids.empty())
        collect_nodes(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		      into.nodes);
      else
        collect_nodes(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		      into.nodes, ids);
    }
    else
    {
      if (ids.empty())
	collect_nodes(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		      into.nodes, ranges);
      else
	collect_nodes(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		      into.nodes, ids, ranges);
    }
  }
  else if (stmt->get_type() == RECURSE_WAY_NODE)
  {
    if (ranges.empty())
    {
      if (ids.empty())
	collect_nodes(query, rman, mit->second.ways.begin(), mit->second.ways.end(),
		      into.nodes);
      else
	collect_nodes(query, rman, mit->second.ways.begin(), mit->second.ways.end(),
		      into.nodes, ids);
    }
    else
    {
      if (ids.empty())
	collect_nodes(query, rman, mit->second.ways.begin(), mit->second.ways.end(),
		      into.nodes, ranges);
      else
	collect_nodes(query, rman, mit->second.ways.begin(), mit->second.ways.end(),
		      into.nodes, ids, ranges);
    }
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
  {
    if (ranges.empty())
    {
      if (ids.empty())
	collect_ways(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		     into.ways);
      else
	collect_ways(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		     into.ways, ids);
    }
    else
    {
      if (ids.empty())
	collect_ways(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		     into.ways, ranges);
      else
	collect_ways(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
		     into.ways, ids, ranges);
    }
  }
  else if (stmt->get_type() == RECURSE_RELATION_RELATION)
  {
    if (ranges.empty())
    {
      if (ids.empty())
	collect_relations(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
			  into.relations);
      else
	collect_relations(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
			  into.relations, ids);
    }
    else
    {
      if (ids.empty())
	collect_relations(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
			  into.relations, ranges);
      else
	collect_relations(query, rman, mit->second.relations.begin(), mit->second.relations.end(),
			  into.relations, ids, ranges);
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
  
  vector< uint32 > ids;
  if (stmt->get_type() == RECURSE_WAY_NODE)
    ids = collect_ids(*stmt, rman, mit->second.ways.begin(), mit->second.ways.end());
  else if (stmt->get_type() == RECURSE_RELATION_NODE)
    ids = collect_ids(*stmt, rman, Relation_Entry::NODE,
		      mit->second.relations.begin(), mit->second.relations.end());
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = into.nodes.begin();
      it != into.nodes.end(); ++it)
  {
    vector< Node_Skeleton > local_into;
    for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (binary_search(ids.begin(), ids.end(), iit->id))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }  
  
  if (stmt->get_type() == RECURSE_RELATION_WAY)
  {
    vector< uint32 > ids = collect_ids(*stmt, rman, Relation_Entry::WAY,
	mit->second.relations.begin(), mit->second.relations.end());
  
    for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = into.ways.begin();
        it != into.ways.end(); ++it)
    {
      vector< Way_Skeleton > local_into;
      for (vector< Way_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
	if (binary_search(ids.begin(), ids.end(), iit->id))
	  local_into.push_back(*iit);
      }
      it->second.swap(local_into);
    }
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY)
  {
    vector< uint32 > ids = extract_children_ids(mit->second.nodes);

    for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = into.ways.begin();
        it != into.ways.end(); ++it)
    {
      vector< Way_Skeleton > local_into;
      for (vector< Way_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
	if (has_a_child_with_id(*iit, ids))
	  local_into.push_back(*iit);
      }
      it->second.swap(local_into);
    }
  }
  else
    into.ways.clear();
    
  ids.clear();
  if (stmt->get_type() == RECURSE_RELATION_RELATION)
  {
    vector< uint32 > ids = collect_ids(*stmt, rman, Relation_Entry::RELATION,
	mit->second.relations.begin(), mit->second.relations.end());

    for (map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = into.relations.begin();
        it != into.relations.end(); ++it)
    {
      vector< Relation_Skeleton > local_into;
      for (vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
        if (binary_search(ids.begin(), ids.end(), iit->id))
  	  local_into.push_back(*iit);
      }
      it->second.swap(local_into);
    }
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
    
    for (map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = into.relations.begin();
        it != into.relations.end(); ++it)
    {
      vector< Relation_Skeleton > local_into;
      for (vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
	if (has_a_child_with_id(*iit, ids, source_type))
  	  local_into.push_back(*iit);
      }
      it->second.swap(local_into);
    }
  }
  else
    into.relations.clear();
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
  else if (attributes["type"] == "way-relation")
    type = RECURSE_WAY_RELATION;
  else if (attributes["type"] == "node-relation")
    type = RECURSE_NODE_RELATION;
  else if (attributes["type"] == "node-way")
    type = RECURSE_NODE_WAY;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"relation-relation\", \"relation-backwards\","
	<<"\"relation-way\", \"relation-node\", \"way-node\", \"way-relation\","
	<<"\"node-relation\" or \"node-way\".";
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
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(into.nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways(into.ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(into.relations);

  if (type == RECURSE_RELATION_RELATION)
    collect_relations(*this, rman, mit->second.relations.begin(), mit->second.relations.end(),
		      relations);
  else if (type == RECURSE_RELATION_BACKWARDS)
    collect_relations(*this, rman, mit->second.relations, relations);
  else if (type == RECURSE_RELATION_WAY)
    collect_ways(*this, rman, mit->second.relations.begin(), mit->second.relations.end(), ways);
  else if (type == RECURSE_RELATION_NODE)
    collect_nodes(*this, rman, mit->second.relations.begin(), mit->second.relations.end(), nodes);
  else if (type == RECURSE_WAY_NODE)
    collect_nodes(*this, rman, mit->second.ways.begin(), mit->second.ways.end(), nodes);
  else if (type == RECURSE_WAY_RELATION)
    collect_relations(*this, rman, mit->second.ways, Relation_Entry::WAY, relations);
  else if (type == RECURSE_NODE_WAY)
    collect_ways(*this, rman, mit->second.nodes, ways);
  else if (type == RECURSE_NODE_RELATION)
    collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, relations);

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
