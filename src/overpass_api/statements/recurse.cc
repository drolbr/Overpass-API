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

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
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
const unsigned int RECURSE_DOWN = 9;
const unsigned int RECURSE_DOWN_REL = 10;
const unsigned int RECURSE_UP = 11;
const unsigned int RECURSE_UP_REL = 12;

Generic_Statement_Maker< Recurse_Statement > Recurse_Statement::statement_maker("recurse");

//-----------------------------------------------------------------------------


template< class TIndex, class TObject, class Id_Type >
vector< Id_Type > extract_children_ids(const map< TIndex, vector< TObject > >& elems)
{
  vector< Id_Type > ids;
  
  {
    for (typename map< TIndex, vector< TObject > >::const_iterator
        it(elems.begin()); it != elems.end(); ++it)
    {
      for (typename vector< TObject >::const_iterator it2(it->second.begin());
          it2 != it->second.end(); ++it2)
        ids.push_back(Id_Type(it2->id.val()));
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


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< Relation_Entry::Ref_Type > ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
			 Get_Parent_Rels_Predicate(ids, source_type), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
                         Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids)
{
  vector< Relation_Entry::Ref_Type > children_ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  if (!invert_ids)
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
	    Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
	    (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(children_ids, source_type)), result);
  else
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
	    Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
	    Get_Parent_Rels_Predicate >
	    (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	      (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Predicate(children_ids, source_type)), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources, uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > children_ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  if (!invert_ids)
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(children_ids, source_type, role_id)), result);
  else
    collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Role_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Role_Predicate(children_ids, source_type, role_id)), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources,
     const map< TSourceIndex, vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
  for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
    req.insert(*it);
  
  collect_items_discrete_by_timestamp(&stmt, rman, req,
      Get_Parent_Rels_Predicate(ids, source_type), result, attic_result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources,
     const map< TSourceIndex, vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result,
     uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
  for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
    req.insert(*it);
  
  collect_items_discrete_by_timestamp(&stmt, rman, req,
      Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), result, attic_result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources,
     const map< TSourceIndex, vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result,
     const vector< Relation::Id_Type >& ids, bool invert_ids)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > children_ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(children_ids));
  for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
    req.insert(*it);
  
  if (!invert_ids)
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(children_ids, source_type)), result, attic_result);
  else
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Predicate(children_ids, source_type)), result, attic_result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< TSourceIndex, vector< TSourceObject > >& sources,
     const map< TSourceIndex, vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result,
     const vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > children_ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(children_ids));
  for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
    req.insert(*it);
  
  if (!invert_ids)
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(children_ids, source_type, role_id)), result, attic_result);
  else
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Role_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Role_Predicate(children_ids, source_type, role_id)), result, attic_result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result)
{
  vector< Uint64 > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Predicate(ids, Relation_Entry::RELATION), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result, uint32 role_id)
{
  vector< Uint64 > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Role_Predicate(ids, Relation_Entry::RELATION, role_id), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids)
{
  vector< Uint64 > children_ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  if (!invert_ids)
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
	    Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
	    (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
        result);
  else
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
	    Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
	    Get_Parent_Rels_Predicate >
	    (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	      (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
        result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     const vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  vector< Uint64 > children_ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);    
  rman.health_check(stmt);
  
  if (!invert_ids)
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(children_ids, Relation_Entry::RELATION, role_id)),
        result);
  else
    collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Role_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Role_Predicate(children_ids, Relation_Entry::RELATION, role_id)),
        result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
  
  collect_items_flat_by_timestamp(stmt, rman,
      Get_Parent_Rels_Predicate(ids, Relation_Entry::RELATION), result, attic_result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
  
  collect_items_flat_by_timestamp(stmt, rman,
      Get_Parent_Rels_Role_Predicate(ids, Relation_Entry::RELATION, role_id), result, attic_result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result,
     const vector< Relation::Id_Type >& ids, bool invert_ids)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > children_ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(children_ids));
  
  if (!invert_ids)
    collect_items_flat_by_timestamp(stmt, rman,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
        result, attic_result);
  else
    collect_items_flat_by_timestamp(stmt, rman,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Predicate(children_ids, Relation_Entry::RELATION)),
        result, attic_result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint31_Index, vector< Relation_Skeleton > >& sources,
     const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_sources,
     map< Uint31_Index, vector< Relation_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result,
     const vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  
  vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  
  vector< Uint64 > children_ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(children_ids));
  
  if (!invert_ids)
    collect_items_flat_by_timestamp(stmt, rman,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(children_ids, Relation_Entry::RELATION, role_id)),
        result, attic_result);
  else
    collect_items_flat_by_timestamp(stmt, rman,
        And_Predicate< Relation_Skeleton,
            Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
            Get_Parent_Rels_Role_Predicate >
            (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
              (Id_Predicate< Relation_Skeleton >(ids)),
            Get_Parent_Rels_Role_Predicate(children_ids, Relation_Entry::RELATION, role_id)),
        result, attic_result);
}


void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result)
{
  vector< Uint64 > ids = extract_children_ids< Uint32_Index, Node_Skeleton, Uint64 >(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  collect_items_discrete(&stmt, rman, *osm_base_settings().WAYS, req,
      Get_Parent_Ways_Predicate(ids), result);
}


void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const vector< Way::Id_Type >& ids, bool invert_ids)
{
  vector< Uint64 > children_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Uint64 >(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  if (!invert_ids)
    collect_items_discrete(&stmt, rman, *osm_base_settings().WAYS, req,
        And_Predicate< Way_Skeleton,
	    Id_Predicate< Way_Skeleton >, Get_Parent_Ways_Predicate >
	    (Id_Predicate< Way_Skeleton >(ids), Get_Parent_Ways_Predicate(children_ids)), result);
  else
    collect_items_discrete(&stmt, rman, *osm_base_settings().WAYS, req,
        And_Predicate< Way_Skeleton,
	    Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >,
	    Get_Parent_Ways_Predicate >
	    (Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >
	      (Id_Predicate< Way_Skeleton >(ids)),
	     Get_Parent_Ways_Predicate(children_ids)), result);
}


void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     const map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic_nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Way_Skeleton > > >& attic_result)
{
  vector< Uint64 > current_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Uint64 >(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  vector< Uint64 > attic_ids = extract_children_ids< Uint32_Index, Attic< Node_Skeleton >, Uint64 >
      (attic_nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > attic_req = extract_parent_indices(attic_nodes);
  rman.health_check(stmt);
  
  vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
  for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
    req.insert(*it);
  
  collect_items_discrete_by_timestamp(&stmt, rman, req,
      Get_Parent_Ways_Predicate(ids), result, attic_result);
}


void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     const map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     const map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic_nodes,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     map< Uint31_Index, vector< Attic< Way_Skeleton > > >& attic_result,
     const vector< Way::Id_Type >& ids, bool invert_ids)
{
  vector< Uint64 > current_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Uint64 >(nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > req = extract_parent_indices(nodes);
  rman.health_check(stmt);
  
  vector< Uint64 > attic_ids = extract_children_ids< Uint32_Index, Attic< Node_Skeleton >, Uint64 >
      (attic_nodes);    
  rman.health_check(stmt);
  set< Uint31_Index > attic_req = extract_parent_indices(attic_nodes);
  rman.health_check(stmt);
  
  vector< Uint64 > children_ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(children_ids));
  for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
    req.insert(*it);
  
  if (!invert_ids)
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        And_Predicate< Way_Skeleton,
            Id_Predicate< Way_Skeleton >, Get_Parent_Ways_Predicate >
            (Id_Predicate< Way_Skeleton >(ids), Get_Parent_Ways_Predicate(children_ids)),
        result, attic_result);
  else
    collect_items_discrete_by_timestamp(&stmt, rman, req,
        And_Predicate< Way_Skeleton,
            Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >,
            Get_Parent_Ways_Predicate >
            (Not_Predicate< Way_Skeleton, Id_Predicate< Way_Skeleton > >
              (Id_Predicate< Way_Skeleton >(ids)),
             Get_Parent_Ways_Predicate(children_ids)),
        result, attic_result);
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		   const vector< Node::Id_Type >& ids, bool invert_ids,
		   map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels);
    else
      nodes = relation_node_members(&query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels, &ranges);
    else
      nodes = relation_node_members(&query, rman, rels, &ranges, &ids, invert_ids);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                   const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
                   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                   const vector< Node::Id_Type >& ids, bool invert_ids,
                   uint64 timestamp,
                   map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                   map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic_nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids), nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, &ranges), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, &ranges, &ids, invert_ids), nodes, attic_nodes);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                   const vector< Node::Id_Type >& ids, bool invert_ids,
                   map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                   uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels, 0, 0, false, &role_id);
    else
      nodes = relation_node_members(&query, rman, rels, 0, &ids, invert_ids, &role_id);
  }
  else
  {
    if (ids.empty())
      nodes = relation_node_members(&query, rman, rels, &ranges, 0, false, &role_id);
    else
      nodes = relation_node_members(&query, rman, rels, &ranges, &ids, invert_ids, &role_id);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                   const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
                   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                   const vector< Node::Id_Type >& ids, bool invert_ids,
                   uint64 timestamp,
                   map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                   map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic_nodes,
                   uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, 0, 0, false, &role_id), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids, &role_id), nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, &ranges, 0, false, &role_id), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, timestamp, &ranges, &ids, invert_ids, &role_id), nodes, attic_nodes);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const map< Uint31_Index, vector< Way_Skeleton > >& rels,
		   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
		   const vector< Node::Id_Type >& ids, bool invert_ids,
		   map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      nodes = way_members(&query, rman, rels);
    else if (!invert_ids)
      nodes = way_members(&query, rman, rels, 0, &ids);
    else
      nodes = way_members(&query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      nodes = way_members(&query, rman, rels, &ranges);
    else if (!invert_ids)
      nodes = way_members(&query, rman, rels, &ranges, &ids);
    else
      nodes = way_members(&query, rman, rels, 0, &ids, invert_ids);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const map< Uint31_Index, vector< Way_Skeleton > >& rels,
                   const map< Uint31_Index, vector< Attic< Way_Skeleton > > >& attic_rels,
                   const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                   const vector< Node::Id_Type >& ids, bool invert_ids,
                   uint64 timestamp,
                   map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                   map< Uint32_Index, vector< Attic< Node_Skeleton > > >& attic_nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp), nodes, attic_nodes);
    else if (!invert_ids)
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, 0, &ids), nodes, attic_nodes);
    else
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids),
                      nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, &ranges), nodes, attic_nodes);
    else if (!invert_ids)
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, &ranges, &ids),
                      nodes, attic_nodes);
    else
      swap_components(way_members(&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids),
                      nodes, attic_nodes);
  }
}


void collect_ways(const Statement& query, Resource_Manager& rman,
		  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  const vector< Way::Id_Type >& ids, bool invert_ids,
		  map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  if (ranges.empty())
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels);
    else
      ways = relation_way_members(&query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels, &ranges);
    else
      ways = relation_way_members(&query, rman, rels, &ranges, &ids, invert_ids);
  }
}


void collect_ways(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Way::Id_Type >& ids, bool invert_ids,
                  uint64 timestamp,
                  map< Uint31_Index, vector< Way_Skeleton > >& ways,
                  map< Uint31_Index, vector< Attic< Way_Skeleton > > >& attic_ways)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp), ways, attic_ways);
    else
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids), ways, attic_ways);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, &ranges), ways, attic_ways);
    else
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, &ranges, &ids, invert_ids), ways, attic_ways);
  }
}


void collect_ways(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Way::Id_Type >& ids, bool invert_ids,
                  map< Uint31_Index, vector< Way_Skeleton > >& ways,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels, 0, 0, false, &role_id);
    else
      ways = relation_way_members(&query, rman, rels, 0, &ids, invert_ids, &role_id);
  }
  else
  {
    if (ids.empty())
      ways = relation_way_members(&query, rman, rels, &ranges, 0, false, &role_id);
    else
      ways = relation_way_members(&query, rman, rels, &ranges, &ids, invert_ids, &role_id);
  }
}


void collect_ways(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Way::Id_Type >& ids, bool invert_ids,
                  uint64 timestamp,
                  map< Uint31_Index, vector< Way_Skeleton > >& ways,
                  map< Uint31_Index, vector< Attic< Way_Skeleton > > >& attic_ways,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, 0, 0, false, &role_id), ways, attic_ways);
    else
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids, &role_id), ways, attic_ways);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, &ranges, 0, false, &role_id), ways, attic_ways);
    else
      swap_components(relation_way_members
          (&query, rman, rels, attic_rels, timestamp, &ranges, &ids, invert_ids, &role_id), ways, attic_ways);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
		  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
		  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
		  const vector< Relation::Id_Type >& ids, bool invert_ids,
		  map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  if (ranges.empty())
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels);
    else
      relations = relation_relation_members(query, rman, rels, 0, &ids, invert_ids);
  }
  else
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels, &ranges);
    else
      relations = relation_relation_members(query, rman, rels, &ranges, &ids, invert_ids);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Relation::Id_Type >& ids, bool invert_ids,
                  uint64 timestamp,
                  map< Uint31_Index, vector< Relation_Skeleton > >& relations,
                  map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_relations)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids), relations, attic_relations);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, &ranges), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, &ranges, &ids, invert_ids), relations, attic_relations);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Relation::Id_Type >& ids, bool invert_ids,
                  map< Uint31_Index, vector< Relation_Skeleton > >& relations,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels, 0, 0, false, &role_id);
    else
      relations = relation_relation_members(query, rman, rels, 0, &ids, invert_ids, &role_id);
  }
  else
  {
    if (ids.empty())
      relations = relation_relation_members(query, rman, rels, &ranges, 0, false, &role_id);
    else
      relations = relation_relation_members(query, rman, rels, &ranges, &ids, invert_ids, &role_id);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
                  const map< Uint31_Index, vector< Relation_Skeleton > >& rels,
                  const map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_rels,
                  const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                  const vector< Relation::Id_Type >& ids, bool invert_ids,
                  uint64 timestamp,
                  map< Uint31_Index, vector< Relation_Skeleton > >& relations,
                  map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_relations,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, 0, 0, false, &role_id), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, 0, &ids, invert_ids, &role_id),
                      relations, attic_relations);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, &ranges, 0, false, &role_id), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, timestamp, &ranges, &ids, invert_ids, &role_id),
                      relations, attic_relations);
  }
}


template< typename Skeleton >
uint count_relations(const map< Uint31_Index, vector< Skeleton > >& relations)
{
  uint result = 0;
  for (typename map< Uint31_Index, vector< Skeleton > >::const_iterator it = relations.begin();
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
    result = relation_relation_members(query, rman, source);
    indexed_set_union(result, source);
    uint new_rel_count = count_relations(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}


void relations_loop(const Statement& query, Resource_Manager& rman,
                    map< Uint31_Index, vector< Relation_Skeleton > > source,
                    map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_source,
                    uint64 timestamp,
                    map< Uint31_Index, vector< Relation_Skeleton > >& result,
                    map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result)
{
  uint old_rel_count = count_relations(source) + count_relations(attic_source);
  while (true)
  {
    std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
        std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > result_pair
        = relation_relation_members(query, rman, source, attic_source, timestamp);
    indexed_set_union(result_pair.first, source);
    indexed_set_union(result_pair.second, attic_source);
    keep_matching_skeletons(result_pair.first, result_pair.second, timestamp);
    uint new_rel_count = count_relations(result_pair.first) + count_relations(result_pair.second);
    if (new_rel_count == old_rel_count)
    {
      result.swap(result_pair.first);
      attic_result.swap(result_pair.second);
      return;
    }
    old_rel_count = new_rel_count;
    source.swap(result_pair.first);
    attic_source.swap(result_pair.second);
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


void relations_up_loop(const Statement& query, Resource_Manager& rman,
                    map< Uint31_Index, vector< Relation_Skeleton > > source,
                    map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_source,
                    uint64 timestamp,
                    map< Uint31_Index, vector< Relation_Skeleton > >& result,
                    map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& attic_result)
{
  uint old_rel_count = count_relations(source) + count_relations(attic_source);
  while (true)
  {
    result.clear();
    attic_result.clear();
    collect_relations(query, rman, source, attic_source, result, attic_result);
    indexed_set_union(result, source);
    indexed_set_union(attic_result, attic_source);
    keep_matching_skeletons(result, attic_result, timestamp);
    uint new_rel_count = count_relations(result) + count_relations(attic_result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
    attic_source.swap(attic_result);
  }
}


//-----------------------------------------------------------------------------

class Recurse_Constraint : public Query_Constraint
{
  public:
    Recurse_Constraint(Recurse_Statement& stmt_) : stmt(&stmt_) {}

    bool delivers_data(Resource_Manager& rman) { return true; }
    
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const set< pair< Uint32_Index, Uint32_Index > >& ranges,
                          const vector< Node::Id_Type >& ids,
                          bool invert_ids, uint64 timestamp);
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const set< pair< Uint31_Index, Uint31_Index > >& ranges,
                          int type,
                          const vector< Uint32_Index >& ids,
                          bool invert_ids, uint64 timestamp);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Recurse_Constraint() {}
    
  private:
    Recurse_Statement* stmt;
};

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint32_Index, Uint32_Index > >& ranges,
     const vector< Node::Id_Type >& ids,
     bool invert_ids, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
    return true;
  
  if (timestamp == NOW)
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == numeric_limits< uint32 >::max())
        return true;
    
      if (stmt->get_type() == RECURSE_RELATION_NODE)
        ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, into.nodes, role_id);
      return true;
    }
  
    if (stmt->get_type() == RECURSE_RELATION_NODE)
      ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, into.nodes);
    else if (stmt->get_type() == RECURSE_WAY_NODE)
      ::collect_nodes(query, rman, mit->second.ways, ranges, ids, invert_ids, into.nodes);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
      ::collect_nodes(query, rman, mit->second.relations, ranges, ids, invert_ids, rel_nodes);
      rel_ways = relation_way_members(&query, rman, mit->second.relations);
      ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, mit->second.relations, rel_rels);
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways;
      ::collect_nodes(query, rman, rel_rels, ranges, ids, invert_ids, rel_nodes);
      rel_ways = relation_way_members(&query, rman, rel_rels);
      ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
  }
  else
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == numeric_limits< uint32 >::max())
        return true;
    
      if (stmt->get_type() == RECURSE_RELATION_NODE)
        ::collect_nodes(query, rman, mit->second.relations, mit->second.attic_relations,
                        ranges, ids, invert_ids, timestamp, into.nodes, into.attic_nodes, role_id);
      return true;
    }
  
    if (stmt->get_type() == RECURSE_RELATION_NODE)
      ::collect_nodes(query, rman, mit->second.relations, mit->second.attic_relations,
                      ranges, ids, invert_ids, timestamp, into.nodes, into.attic_nodes);
    else if (stmt->get_type() == RECURSE_WAY_NODE)
      ::collect_nodes(query, rman, mit->second.ways, mit->second.attic_ways,
                      ranges, ids, invert_ids, timestamp, into.nodes, into.attic_nodes);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
      map< Uint32_Index, vector< Attic< Node_Skeleton > > > rel_attic_nodes;
      ::collect_nodes(query, rman, mit->second.relations, mit->second.attic_relations,
                      ranges, ids, invert_ids, timestamp, rel_nodes, rel_attic_nodes);
      
      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > rel_ways
          = relation_way_members(&query, rman, mit->second.relations, mit->second.attic_relations, timestamp);
      ::collect_nodes(query, rman, rel_ways.first, rel_ways.second,
                      ranges, ids, invert_ids, timestamp, into.nodes, into.attic_nodes);
      indexed_set_union(into.nodes, rel_nodes);
      indexed_set_union(into.attic_nodes, rel_attic_nodes);
      keep_matching_skeletons(into.nodes, into.attic_nodes, timestamp);
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_rel_rels;
      relations_loop(query, rman,
                     mit->second.relations, mit->second.attic_relations, timestamp,
                     rel_rels, attic_rel_rels);
      
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes;
      map< Uint32_Index, vector< Attic< Node_Skeleton > > > rel_attic_nodes;
      ::collect_nodes(query, rman, rel_rels, attic_rel_rels,
                      ranges, ids, invert_ids, timestamp, rel_nodes, rel_attic_nodes);
      
      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > rel_ways
          = relation_way_members(&query, rman, rel_rels, attic_rel_rels, timestamp);
      ::collect_nodes(query, rman, rel_ways.first, rel_ways.second,
                      ranges, ids, invert_ids, timestamp, into.nodes, into.attic_nodes);
      
      indexed_set_union(into.nodes, rel_nodes);
      indexed_set_union(into.attic_nodes, rel_attic_nodes);
      keep_matching_skeletons(into.nodes, into.attic_nodes, timestamp);
    }
  }    
  return true;
}

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const set< pair< Uint31_Index, Uint31_Index > >& ranges,
     int type,
     const vector< Uint32_Index >& ids,
     bool invert_ids, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
    return true;
  
  if (timestamp == NOW)
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == numeric_limits< uint32 >::max())
        return true;

      if (stmt->get_type() == RECURSE_RELATION_WAY)
        collect_ways(query, rman, mit->second.relations, ranges, ids, invert_ids, into.ways, role_id);
      else if (stmt->get_type() == RECURSE_RELATION_RELATION)
        collect_relations(query, rman, mit->second.relations, ranges,
                        ids, invert_ids, into.relations, role_id);
      else if (stmt->get_type() == RECURSE_NODE_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, role_id);
        else
          collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY, into.relations, role_id);
        else
          collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY,
                        into.relations, ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      {
        if (ids.empty())
          collect_relations(query, rman, mit->second.relations, into.relations, role_id);
        else
          collect_relations(query, rman, mit->second.relations, into.relations,
                        ids, invert_ids, role_id);
      }
      else
        return false;
      
      return true;
    }
  
    if (stmt->get_type() == RECURSE_RELATION_WAY)
      collect_ways(query, rman, mit->second.relations, ranges, ids, invert_ids, into.ways);
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
      collect_relations(query, rman, mit->second.relations, ranges,
		      ids, invert_ids, into.relations);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      if (type != QUERY_WAY)
        return true;
      collect_ways(query, rman, mit->second.relations, ranges, ids, invert_ids, into.ways);
      return true;
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, mit->second.relations, rel_rels);
      if (type == QUERY_WAY)
        collect_ways(query, rman, rel_rels, ranges, ids, invert_ids, into.ways);
      else
      {
        if (!ids.empty())
        {
	  if (!invert_ids)
	    filter_items
	        (Id_Predicate< Relation_Skeleton >(ids), rel_rels);
	  else
	    filter_items
	        (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	        (Id_Predicate< Relation_Skeleton >(ids)), rel_rels);
        }
        into.relations.swap(rel_rels);
      }
    }
    else if (stmt->get_type() == RECURSE_NODE_WAY)
    {
      if (ids.empty())
        collect_ways(query, rman, mit->second.nodes, into.ways);
      else
        collect_ways(query, rman, mit->second.nodes, into.ways, ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations);
      else
        collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, ids, invert_ids);
      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY, into.relations);
      else
        collect_relations(query, rman, mit->second.ways, Relation_Entry::WAY,
			  into.relations, ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.relations, into.relations);
      else
        collect_relations(query, rman, mit->second.relations, into.relations,
			  ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_UP)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
	  collect_ways(query, rman, mit->second.nodes, into.ways);
        else
	  collect_ways(query, rman, mit->second.nodes, into.ways,
		       ids, invert_ids);
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
	  collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations,
			  ids, invert_ids);
      
        map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
        if (ids.empty())
	  collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
        else
	  collect_relations(query, rman, mit->second.nodes, Relation_Entry::NODE, node_rels,
			  ids, invert_ids);
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
	  collect_ways(query, rman, mit->second.nodes, into.ways, ids, invert_ids);
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
        {
	  if (!invert_ids)
	    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
	  else
	    filter_items
	        (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
	        (Id_Predicate< Relation_Skeleton >(ids)), into.relations);
        }
      }
    }
    else
      return false;
    
    return true;
  }
  else
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == numeric_limits< uint32 >::max())
        return true;

      if (stmt->get_type() == RECURSE_RELATION_WAY)
        collect_ways(query, rman, mit->second.relations, mit->second.attic_relations,
                     ranges, ids, invert_ids, timestamp, into.ways, into.attic_ways, role_id);
      else if (stmt->get_type() == RECURSE_RELATION_RELATION)
        collect_relations(query, rman, mit->second.relations, mit->second.attic_relations,
                     ranges, ids, invert_ids, timestamp, into.relations, into.attic_relations, role_id);
      else if (stmt->get_type() == RECURSE_NODE_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                            Relation_Entry::NODE, into.relations, into.attic_relations, role_id);
        else
          collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                            Relation_Entry::NODE, into.relations, into.attic_relations,
                            ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, mit->second.ways, mit->second.attic_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations, role_id);
        else
          collect_relations(query, rman, mit->second.ways, mit->second.attic_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations,
                            ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      {
        if (ids.empty())
          collect_relations(query, rman, mit->second.relations, mit->second.attic_relations,
                            into.relations, into.attic_relations, role_id);
        else
          collect_relations(query, rman, mit->second.relations, mit->second.attic_relations,
                            into.relations, into.attic_relations, ids, invert_ids, role_id);
      }
      else
        return false;
      
      return true;
    }
  
    if (stmt->get_type() == RECURSE_RELATION_WAY)
      collect_ways(query, rman, mit->second.relations, mit->second.attic_relations,
                   ranges, ids, invert_ids, timestamp, into.ways, into.attic_ways);
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
      collect_relations(query, rman, mit->second.relations, mit->second.attic_relations,
                     ranges, ids, invert_ids, timestamp, into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      if (type != QUERY_WAY)
        return true;
      collect_ways(query, rman, mit->second.relations, mit->second.attic_relations,
                   ranges, ids, invert_ids, timestamp, into.ways, into.attic_ways);
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_rel_rels;
      relations_loop(query, rman,
                     mit->second.relations, mit->second.attic_relations, timestamp,
                     rel_rels, attic_rel_rels);
      if (type == QUERY_WAY)
        collect_ways(query, rman, rel_rels, attic_rel_rels,
                     ranges, ids, invert_ids, timestamp, into.ways, into.attic_ways);
      else
      {
        if (!ids.empty())
        {
          if (!invert_ids)
          {
            filter_items(Id_Predicate< Relation_Skeleton >(ids), rel_rels);
            filter_items(Id_Predicate< Relation_Skeleton >(ids), attic_rel_rels);
          }
          else
          {
            filter_items
                (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
                (Id_Predicate< Relation_Skeleton >(ids)), rel_rels);
            filter_items
                (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
                (Id_Predicate< Relation_Skeleton >(ids)), attic_rel_rels);
          }
        }
        into.relations.swap(rel_rels);
        into.attic_relations.swap(attic_rel_rels);
        
        keep_matching_skeletons(into.relations, into.attic_relations, timestamp);
      }
    }
    else if (stmt->get_type() == RECURSE_NODE_WAY)
    {
      if (ids.empty())
        collect_ways(query, rman,
                     mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways);
      else
        collect_ways(query, rman,
                     mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways,
                     ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                          Relation_Entry::NODE, into.relations, into.attic_relations);
      else
        collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                          Relation_Entry::NODE, into.relations, into.attic_relations,
                          ids, invert_ids);
      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, mit->second.ways, mit->second.attic_ways,
                          Relation_Entry::WAY, into.relations, into.attic_relations);
      else
        collect_relations(query, rman, mit->second.ways, mit->second.attic_ways,
                          Relation_Entry::WAY, into.relations, into.attic_relations,
                          ids, invert_ids);
      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
        if (ids.empty())
          collect_relations(query, rman, mit->second.relations, mit->second.attic_relations,
                            into.relations, into.attic_relations);
        else
          collect_relations(query, rman, mit->second.relations, mit->second.attic_relations,
                            into.relations, into.attic_relations, ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_UP)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
          collect_ways(query, rman,
                       mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways);
        else
          collect_ways(query, rman,
                       mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways,
                       ids, invert_ids);
      }
      else
      {
        map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
        map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_rel_ways = mit->second.attic_ways;
        map< Uint31_Index, vector< Way_Skeleton > > node_ways;
        map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_node_ways;
        collect_ways(query, rman,
                     mit->second.nodes, mit->second.attic_nodes, node_ways, attic_node_ways);
        indexed_set_union(rel_ways, node_ways);
        indexed_set_union(attic_rel_ways, attic_node_ways);
        if (ids.empty())
          collect_relations(query, rman, rel_ways, attic_rel_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations);
        else
          collect_relations(query, rman, rel_ways, attic_rel_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations,
                            ids, invert_ids);
      
        map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
        map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_node_rels;
        if (ids.empty())
          collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                            Relation_Entry::NODE, node_rels, attic_node_rels);
        else
          collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                            Relation_Entry::NODE, node_rels, attic_node_rels,
                            ids, invert_ids);
        indexed_set_union(into.relations, node_rels);
        indexed_set_union(into.attic_relations, attic_node_rels);
      }
    }
    else if (stmt->get_type() == RECURSE_UP_REL)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
          collect_ways(query, rman,
                       mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways);
        else
          collect_ways(query, rman,
                       mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways,
                       ids, invert_ids);
      }
      else
      {
        map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
        map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_rel_ways = mit->second.attic_ways;
        map< Uint31_Index, vector< Way_Skeleton > > node_ways;
        map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_node_ways;
        collect_ways(query, rman,
                     mit->second.nodes, mit->second.attic_nodes, node_ways, attic_node_ways);
        indexed_set_union(rel_ways, node_ways);
        indexed_set_union(attic_rel_ways, attic_node_ways);
        
        map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
        map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_way_rels;
        collect_relations(query, rman, rel_ways, attic_rel_ways,
                          Relation_Entry::WAY, way_rels, attic_way_rels);
        
        map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
        map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_rel_rels = mit->second.attic_relations;
        indexed_set_union(rel_rels, way_rels);
        indexed_set_union(attic_rel_rels, attic_way_rels);
      
        map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
        map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_node_rels;
        collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                          Relation_Entry::NODE, node_rels, attic_node_rels);
        indexed_set_union(rel_rels, node_rels);
        indexed_set_union(attic_rel_rels, attic_node_rels);
        
        relations_up_loop(query, rman, rel_rels, attic_rel_rels, timestamp,
                          into.relations, into.attic_relations);

        if (!ids.empty())
        {
          if (!invert_ids)
          {
            filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
            filter_items(Id_Predicate< Relation_Skeleton >(ids), into.attic_relations);
          }
          else
          {
            filter_items
                (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
                (Id_Predicate< Relation_Skeleton >(ids)), into.relations);
            filter_items
                (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
                (Id_Predicate< Relation_Skeleton >(ids)), into.attic_relations);
          }
        }
      }
    }
    else
      return false;
    
    return true;
  }
}


void Recurse_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  if (mit == rman.sets().end())
  {
    into.nodes.clear();
    into.ways.clear();
    into.relations.clear();
    into.areas.clear();
    return;
  }
  
  if (stmt->get_type() == RECURSE_DOWN || stmt->get_type() == RECURSE_DOWN_REL)
    return;
  
  if (stmt->get_role())
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
    if (role_id == numeric_limits< uint32 >::max())
      return;

    vector< Node::Id_Type > ids;
    if (stmt->get_type() == RECURSE_RELATION_NODE)
    {
      if (timestamp == NOW)
        ids = relation_node_member_ids(rman, mit->second.relations, &role_id);
      else
        ids = relation_node_member_ids(rman, mit->second.relations, mit->second.attic_relations, &role_id);
    }
  
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.attic_nodes);
  
    if (stmt->get_type() == RECURSE_RELATION_WAY)
    {
      vector< Way::Id_Type > ids;
      if (timestamp == NOW)
        relation_way_member_ids(rman, mit->second.relations, &role_id).swap(ids);
      else
        relation_way_member_ids(rman, mit->second.relations, mit->second.attic_relations, &role_id).swap(ids);
      filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
      filter_items(Id_Predicate< Way_Skeleton >(ids), into.attic_ways);
    }
    else
      into.ways.clear();
    
    if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
      return;
  
    ids.clear();
    if (stmt->get_type() == RECURSE_RELATION_RELATION)
    {
      vector< Relation::Id_Type > ids;
      if (timestamp == NOW)
        relation_relation_member_ids(rman, mit->second.relations, &role_id).swap(ids);
      else
        relation_relation_member_ids(rman, mit->second.relations, mit->second.attic_relations, &role_id)
            .swap(ids);
      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.attic_relations);
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
    
      vector< Relation_Entry::Ref_Type > ids;
      if (stmt->get_type() == RECURSE_NODE_RELATION)
      {
        vector< Node::Id_Type > current_ids = extract_children_ids
            < Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
        vector< Node::Id_Type > attic_ids = extract_children_ids
            < Uint32_Index, Attic< Node_Skeleton >, Relation_Entry::Ref_Type >(mit->second.attic_nodes);
        ids.clear();
        std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
      }
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
      {
        vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
            < Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(mit->second.ways);
        vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
            < Uint31_Index, Attic< Way_Skeleton >, Relation_Entry::Ref_Type >(mit->second.attic_ways);
        ids.clear();
        std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
      }
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
        ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(mit->second.relations);
    
      filter_items(Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), into.relations);
      filter_items(Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), into.attic_relations);
    }
    else
      into.relations.clear();
  
    return;
  }
  
  vector< Node::Id_Type > ids;
  if (stmt->get_type() == RECURSE_WAY_NODE)
  {
    if (timestamp == NOW)
      ids = way_nd_ids(mit->second.ways);
    else
      ids = way_nd_ids(mit->second.ways, mit->second.attic_ways);
    rman.health_check(*stmt);
  }
  else if (stmt->get_type() == RECURSE_RELATION_NODE)
  {
    if (timestamp == NOW)
      ids = relation_node_member_ids(rman, mit->second.relations);
    else
      ids = relation_node_member_ids(rman, mit->second.relations, mit->second.attic_relations);
    rman.health_check(*stmt);
  }
  
  filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
  filter_items(Id_Predicate< Node_Skeleton >(ids), into.attic_nodes);
  
  if (stmt->get_type() == RECURSE_RELATION_WAY)
  {
    vector< Way::Id_Type > ids;
    if (timestamp == NOW)
      relation_way_member_ids(rman, mit->second.relations).swap(ids);
    else
      relation_way_member_ids(rman, mit->second.relations, mit->second.attic_relations).swap(ids);
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.attic_ways);
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_UP
      || stmt->get_type() == RECURSE_UP_REL)
  {
    vector< Node::Id_Type > current_ids = extract_children_ids
        < Uint32_Index, Node_Skeleton, Node::Id_Type >(mit->second.nodes);
    vector< Node::Id_Type > attic_ids = extract_children_ids
        < Uint32_Index, Attic< Node_Skeleton >, Node::Id_Type >(mit->second.attic_nodes);
    vector< Node::Id_Type > ids;
    std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                   std::back_inserter(ids));
    filter_items(Get_Parent_Ways_Predicate(ids), into.ways);
    filter_items(Get_Parent_Ways_Predicate(ids), into.attic_ways);
  }
  else
    into.ways.clear();
    
  if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
    return;
  
  ids.clear();
  if (stmt->get_type() == RECURSE_RELATION_RELATION)
  {
    vector< Relation::Id_Type > ids;
    if (timestamp == NOW)
      relation_relation_member_ids(rman, mit->second.relations).swap(ids);
    else
      relation_relation_member_ids(rman, mit->second.relations, mit->second.attic_relations).swap(ids);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.attic_relations);
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
    
    vector< Relation_Entry::Ref_Type > ids;
    if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      vector< Node::Id_Type > current_ids = extract_children_ids
          < Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
      vector< Node::Id_Type > attic_ids = extract_children_ids
          < Uint32_Index, Attic< Node_Skeleton >, Relation_Entry::Ref_Type >(mit->second.attic_nodes);
      ids.clear();
      std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
               std::back_inserter(ids));
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
          < Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(mit->second.ways);
      vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
          < Uint31_Index, Attic< Way_Skeleton >, Relation_Entry::Ref_Type >(mit->second.attic_ways);
      ids.clear();
      std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
               std::back_inserter(ids));
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
      vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
          < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(mit->second.relations);
      vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
          < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(mit->second.attic_relations);
      ids.clear();
      std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
               std::back_inserter(ids));
    }
    
    filter_items(Get_Parent_Rels_Predicate(ids, source_type), into.relations);
    filter_items(Get_Parent_Rels_Predicate(ids, source_type), into.attic_relations);
  }
  else
    into.relations.clear();
  
  //TODO: areas
}


void Recurse_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  map< string, Set >::const_iterator mit = rman.sets().find(stmt->get_input());
  
  if (mit == rman.sets().end())
    return;
  if (stmt->get_type() != RECURSE_DOWN && stmt->get_type() != RECURSE_DOWN_REL
      && stmt->get_type() != RECURSE_UP && stmt->get_type() != RECURSE_UP_REL)
    return;
  
  if (stmt->get_type() == RECURSE_DOWN)
  {
    if (timestamp == NOW)
    {
      vector< Node::Id_Type > rel_ids
          = relation_node_member_ids(rman, mit->second.relations);
      map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
      collect_ways(query, rman, mit->second.relations, set< pair< Uint31_Index, Uint31_Index > >(),
	  vector< Way::Id_Type >(), false, intermediate_ways);
      vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways);
      rman.health_check(*stmt);
    
      vector< Node::Id_Type > ids;
      set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

      filter_items(Id_Predicate< Way_Skeleton >(relation_way_member_ids(rman, mit->second.relations)),
		   into.ways);
    }
    else
    {
      vector< Node::Id_Type > rel_ids
          = relation_node_member_ids(rman, mit->second.relations, mit->second.attic_relations);
      map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > intermediate_attic_ways;
      collect_ways(query, rman, mit->second.relations, mit->second.attic_relations,
          set< pair< Uint31_Index, Uint31_Index > >(), vector< Way::Id_Type >(), false, timestamp,
          intermediate_ways, intermediate_attic_ways);
      vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways, intermediate_attic_ways);
      rman.health_check(*stmt);
    
      vector< Node::Id_Type > ids;
      set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
      filter_items(Id_Predicate< Attic< Node_Skeleton > >(ids), into.attic_nodes);

      vector< Way::Id_Type > rel_way_ids
          = relation_way_member_ids(rman, mit->second.relations, mit->second.attic_relations);
      filter_items(Id_Predicate< Way_Skeleton >(rel_way_ids), into.ways);
      filter_items(Id_Predicate< Attic< Way_Skeleton > >(rel_way_ids), into.attic_ways);
    }
    
    into.attic_relations.clear();
    into.relations.clear();
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    if (timestamp == NOW)
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, mit->second.relations, rel_rels);
      vector< Node::Id_Type > rel_ids
          = relation_node_member_ids(rman, rel_rels);
      map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
      collect_ways(query, rman, rel_rels, set< pair< Uint31_Index, Uint31_Index > >(),
		   vector< Way::Id_Type >(), false, intermediate_ways);
      vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways);
      rman.health_check(*stmt);
    
      vector< Node::Id_Type > ids;
      set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
  
      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

      filter_items(Id_Predicate< Way_Skeleton >(relation_way_member_ids(rman, rel_rels)), into.ways);

      filter_items(Id_Predicate< Relation_Skeleton >(filter_for_ids(rel_rels)), into.relations);
    }
    else
    {
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_rel_rels;
      relations_loop(query, rman,
                     mit->second.relations, mit->second.attic_relations, timestamp,
                     rel_rels, attic_rel_rels);      
            
      vector< Node::Id_Type > rel_node_ids
          = relation_node_member_ids(rman, rel_rels, attic_rel_rels);
      map< Uint31_Index, vector< Way_Skeleton > > intermediate_ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > intermediate_attic_ways;
      collect_ways(query, rman, rel_rels, attic_rel_rels,
          set< pair< Uint31_Index, Uint31_Index > >(), vector< Way::Id_Type >(), false, timestamp,
          intermediate_ways, intermediate_attic_ways);
      vector< Node::Id_Type > way_node_ids = way_nd_ids(intermediate_ways, intermediate_attic_ways);
      rman.health_check(*stmt);
    
      vector< Node::Id_Type > ids;
      set_union(way_node_ids.begin(), way_node_ids.end(), rel_node_ids.begin(), rel_node_ids.end(),
                back_inserter(ids));
  
      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
      filter_items(Id_Predicate< Attic< Node_Skeleton > >(ids), into.attic_nodes);

      vector< Way::Id_Type > rel_way_ids
          = relation_way_member_ids(rman, mit->second.relations, mit->second.attic_relations);
      filter_items(Id_Predicate< Way_Skeleton >(rel_way_ids), into.ways);
      filter_items(Id_Predicate< Attic< Way_Skeleton > >(rel_way_ids), into.attic_ways);
      
      item_filter_map(into.relations, rel_rels);
      item_filter_map(into.attic_relations, attic_rel_rels);
    }
  }
  else if (stmt->get_type() == RECURSE_UP && !into.relations.empty())
  {
    if (timestamp == NOW)
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, mit->second.nodes, node_ways);    
      indexed_set_union(rel_ways, node_ways);
    
      vector< Relation_Entry::Ref_Type > node_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
      vector< Relation_Entry::Ref_Type > way_ids = extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(rel_ways);
    
      filter_items(
          Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
          (Get_Parent_Rels_Predicate(node_ids, Relation_Entry::NODE),
	   Get_Parent_Rels_Predicate(way_ids, Relation_Entry::WAY)), into.relations);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_rel_ways = mit->second.attic_ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_node_ways;
      collect_ways(query, rman, mit->second.nodes, mit->second.attic_nodes, node_ways, attic_node_ways);
      indexed_set_union(rel_ways, node_ways);
      indexed_set_union(attic_rel_ways, attic_node_ways);
      
      vector< Relation_Entry::Ref_Type > current_node_ids =
          extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(mit->second.nodes);
      vector< Relation_Entry::Ref_Type > attic_node_ids =
          extract_children_ids< Uint32_Index, Attic< Node_Skeleton >, Relation_Entry::Ref_Type >(mit->second.attic_nodes);
      vector< Relation_Entry::Ref_Type > node_ids;
      std::set_union(current_node_ids.begin(), current_node_ids.end(),
                     attic_node_ids.begin(), attic_node_ids.end(),
                     std::back_inserter(node_ids));
      
      vector< Relation_Entry::Ref_Type > current_way_ids =
          extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(rel_ways);
      vector< Relation_Entry::Ref_Type > attic_way_ids =
          extract_children_ids< Uint31_Index, Attic< Way_Skeleton >, Relation_Entry::Ref_Type >(attic_rel_ways);
      vector< Relation_Entry::Ref_Type > way_ids;
      std::set_union(current_way_ids.begin(), current_way_ids.end(),
                     attic_way_ids.begin(), attic_way_ids.end(),
                     std::back_inserter(way_ids));
      
      filter_items(
          Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
          (Get_Parent_Rels_Predicate(node_ids, Relation_Entry::NODE),
           Get_Parent_Rels_Predicate(way_ids, Relation_Entry::WAY)), into.relations);
      filter_items(
          Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
          (Get_Parent_Rels_Predicate(node_ids, Relation_Entry::NODE),
           Get_Parent_Rels_Predicate(way_ids, Relation_Entry::WAY)), into.attic_relations);
    }
  }
  else if (stmt->get_type() == RECURSE_UP_REL && !into.relations.empty())
  {
    if (timestamp == NOW)
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

      vector< Relation::Id_Type > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation::Id_Type >(rel_rels);
      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_rel_ways = mit->second.attic_ways;
      map< Uint31_Index, vector< Way_Skeleton > > node_ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_node_ways;
      collect_ways(query, rman,
                   mit->second.nodes, mit->second.attic_nodes, node_ways, attic_node_ways);
      indexed_set_union(rel_ways, node_ways);
      indexed_set_union(attic_rel_ways, attic_node_ways);
        
      map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_way_rels;
      collect_relations(query, rman, rel_ways, attic_rel_ways,
                        Relation_Entry::WAY, way_rels, attic_way_rels);
        
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_rel_rels = mit->second.attic_relations;
      indexed_set_union(rel_rels, way_rels);
      indexed_set_union(attic_rel_rels, attic_way_rels);
      
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_node_rels;
      collect_relations(query, rman, mit->second.nodes, mit->second.attic_nodes,
                        Relation_Entry::NODE, node_rels, attic_node_rels);
      indexed_set_union(rel_rels, node_rels);
      indexed_set_union(attic_rel_rels, attic_node_rels);
        
      relations_up_loop(query, rman, rel_rels, attic_rel_rels, timestamp, rel_rels, attic_rel_rels);

      vector< Relation::Id_Type > current_ids = extract_children_ids
          < Uint31_Index, Relation_Skeleton, Relation::Id_Type >(rel_rels);
      vector< Relation::Id_Type > attic_ids = extract_children_ids
          < Uint31_Index, Attic< Relation_Skeleton >, Relation::Id_Type >(attic_rel_rels);
    
      vector< Relation::Id_Type > ids;
      std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                     std::back_inserter(ids));

      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.attic_relations);
    }
  }
  
  //TODO: areas
}

//-----------------------------------------------------------------------------

Recurse_Statement::Recurse_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_), restrict_to_role(false)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["role"] = "";
  attributes["role-restricted"] = "no";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  set_output(attributes["into"]);
  
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
  if (attributes["role"] != "" || attributes["role-restricted"] == "yes")
  {
    if (type != RECURSE_RELATION_RELATION && type != RECURSE_RELATION_BACKWARDS
        && type != RECURSE_RELATION_WAY && type != RECURSE_WAY_RELATION
        && type != RECURSE_RELATION_NODE && type != RECURSE_NODE_RELATION)
    {
      ostringstream temp;
      temp<<"A role can only be specified for values \"relation-relation\", \"relation-backwards\","
          <<"\"relation-way\", \"relation-node\", \"way-relation\","
          <<"or \"node-relation\".";
      add_static_error(temp.str());
    }
    else
    {
      role = attributes["role"];
      restrict_to_role = true;
    }
  }
}


void Recurse_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  map< string, Set >::const_iterator mit(rman.sets().find(input));
  if (mit == rman.sets().end())
  {
    transfer_output(rman, into);
    return;
  }

  if (restrict_to_role)
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), role);
    if (role_id == numeric_limits< uint32 >::max())
    {
      transfer_output(rman, into);
      return;
    }
        
    if (type == RECURSE_RELATION_RELATION)
    {
      if (rman.get_desired_timestamp() == NOW)
        into.relations = relation_relation_members(*this, rman, mit->second.relations,
                                                 0, 0, false, &role_id);
      else
      {
        std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
            std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > all_relations
            = relation_relation_members(*this, rman, mit->second.relations, mit->second.attic_relations,
                                        rman.get_desired_timestamp(), 0, 0, false, &role_id);
        into.relations.swap(all_relations.first);
        into.attic_relations.swap(all_relations.second);
      }
    }
    else if (type == RECURSE_RELATION_WAY)
    {
      if (rman.get_desired_timestamp() == NOW)
        into.ways = relation_way_members(this, rman, mit->second.relations,
                                         0, 0, false, &role_id);
      else
      {
        std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
            std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
            = relation_way_members(this, rman, mit->second.relations, mit->second.attic_relations,
                                   rman.get_desired_timestamp(), 0, 0, false, &role_id);
        into.ways.swap(all_ways.first);
        into.attic_ways.swap(all_ways.second);
      }
    }
    else if (type == RECURSE_RELATION_NODE)
    {
      if (rman.get_desired_timestamp() == NOW)
        into.nodes = relation_node_members(this, rman, mit->second.relations,
                                           0, 0, false, &role_id);
      else
      {
        std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
            std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
            = relation_node_members(this, rman, mit->second.relations, mit->second.attic_relations,
                                    rman.get_desired_timestamp(), 0, 0, false, &role_id);
        into.nodes.swap(all_nodes.first);
        into.attic_nodes.swap(all_nodes.second);
      }
    }
    else if (type == RECURSE_RELATION_BACKWARDS)
    {
      if (rman.get_desired_timestamp() == NOW)
        collect_relations(*this, rman, mit->second.relations, into.relations, role_id);
      else
        collect_relations(*this, rman, mit->second.relations, mit->second.attic_relations,
                          into.relations, into.attic_relations, role_id);
    }
    else if (type == RECURSE_NODE_RELATION)
    {
      if (rman.get_desired_timestamp() == NOW)
        collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, into.relations, role_id);
      else
        collect_relations(*this, rman, mit->second.nodes, mit->second.attic_nodes, Relation_Entry::NODE,
                          into.relations, into.attic_relations, role_id);
    }
    else if (type == RECURSE_WAY_RELATION)
    {
      if (rman.get_desired_timestamp() == NOW)
        collect_relations(*this, rman, mit->second.ways, Relation_Entry::WAY, into.relations, role_id);
      else
        collect_relations(*this, rman, mit->second.ways, mit->second.attic_ways, Relation_Entry::WAY,
                          into.relations, into.attic_relations, role_id);
    }
  }
  else if (type == RECURSE_RELATION_RELATION)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.relations = relation_relation_members(*this, rman, mit->second.relations);
    else
    {
      std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > all_relations
          = relation_relation_members(*this, rman, mit->second.relations, mit->second.attic_relations,
                                      rman.get_desired_timestamp());
      into.relations.swap(all_relations.first);
      into.attic_relations.swap(all_relations.second);
    }
  }
  else if (type == RECURSE_RELATION_BACKWARDS)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_relations(*this, rman, mit->second.relations, into.relations);
    else
      collect_relations(*this, rman, mit->second.relations, mit->second.attic_relations,
                        into.relations, into.attic_relations);
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.ways = relation_way_members(this, rman, mit->second.relations);
    else
    {
      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
          = relation_way_members(this, rman, mit->second.relations, mit->second.attic_relations,
                                 rman.get_desired_timestamp());
      into.ways.swap(all_ways.first);
      into.attic_ways.swap(all_ways.second);
    }
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.nodes = relation_node_members(this, rman, mit->second.relations);
    else
    {
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
          = relation_node_members(this, rman, mit->second.relations, mit->second.attic_relations,
                                  rman.get_desired_timestamp());
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
    }
  }
  else if (type == RECURSE_WAY_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.nodes = way_members(this, rman, mit->second.ways);
    else
    {
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
         std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
         = way_members(this, rman, mit->second.ways, mit->second.attic_ways, rman.get_desired_timestamp());
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
    }
  }
  else if (type == RECURSE_DOWN)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes
          = relation_node_members(this, rman, mit->second.relations);
      into.ways = relation_way_members(this, rman, mit->second.relations);
      map< Uint31_Index, vector< Way_Skeleton > > source_ways = mit->second.ways;
      indexed_set_union(source_ways, into.ways);
      into.nodes = way_members(this, rman, source_ways);
      indexed_set_union(into.nodes, rel_nodes);
    }
    else
    {
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
          = relation_node_members(this, rman, mit->second.relations, mit->second.attic_relations,
                                  rman.get_desired_timestamp());
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
      
      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
          = relation_way_members(this, rman, mit->second.relations, mit->second.attic_relations,
                                 rman.get_desired_timestamp());
      into.ways = all_ways.first;
      into.attic_ways = all_ways.second;
      
      map< Uint31_Index, vector< Way_Skeleton > > source_ways = mit->second.ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > source_attic_ways = mit->second.attic_ways;
      indexed_set_union(all_ways.first, source_ways);
      indexed_set_union(all_ways.second, source_attic_ways);
      
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > more_nodes
          = way_members(this, rman, all_ways.first, all_ways.second, rman.get_desired_timestamp());
      indexed_set_union(into.nodes, more_nodes.first);
      indexed_set_union(into.attic_nodes, more_nodes.second);
      keep_matching_skeletons(into.nodes, into.attic_nodes, rman.get_desired_timestamp());
    }
  }
  else if (type == RECURSE_DOWN_REL)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      relations_loop(*this, rman, mit->second.relations, into.relations);    
      map< Uint32_Index, vector< Node_Skeleton > > rel_nodes
          = relation_node_members(this, rman, into.relations);
      into.ways = relation_way_members(this, rman, into.relations);
      map< Uint31_Index, vector< Way_Skeleton > > source_ways = mit->second.ways;
      indexed_set_union(source_ways, into.ways);
      into.nodes = way_members(this, rman, source_ways);
      indexed_set_union(into.nodes, rel_nodes);
    }
    else
    {
      relations_loop(*this, rman,
                     mit->second.relations, mit->second.attic_relations, rman.get_desired_timestamp(),
                     into.relations, into.attic_relations);
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
          = relation_node_members(this, rman, into.relations, into.attic_relations,
                                  rman.get_desired_timestamp());
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
      
      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
          = relation_way_members(this, rman, into.relations, into.attic_relations,
                                 rman.get_desired_timestamp());
      into.ways = all_ways.first;
      into.attic_ways = all_ways.second;
      
      map< Uint31_Index, vector< Way_Skeleton > > source_ways = mit->second.ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > source_attic_ways = mit->second.attic_ways;
      indexed_set_union(all_ways.first, source_ways);
      indexed_set_union(all_ways.second, source_attic_ways);
      
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > more_nodes
          = way_members(this, rman, all_ways.first, all_ways.second, rman.get_desired_timestamp());
      indexed_set_union(into.nodes, more_nodes.first);
      indexed_set_union(into.attic_nodes, more_nodes.second);
      keep_matching_skeletons(into.nodes, into.attic_nodes, rman.get_desired_timestamp());
    }
  }
  else if (type == RECURSE_NODE_WAY)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_ways(*this, rman, mit->second.nodes, into.ways);
    else
      collect_ways(*this, rman,
                   mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways);
  }
  else if (type == RECURSE_NODE_RELATION)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, into.relations);
    else
      collect_relations(*this, rman, mit->second.nodes, mit->second.attic_nodes, Relation_Entry::NODE,
                        into.relations, into.attic_relations);
  }
  else if (type == RECURSE_WAY_RELATION)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_relations(*this, rman, mit->second.ways, Relation_Entry::WAY, into.relations);
    else
      collect_relations(*this, rman, mit->second.ways, mit->second.attic_ways, Relation_Entry::WAY,
                        into.relations, into.attic_relations);
  }
  else if (type == RECURSE_UP)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      collect_ways(*this, rman, mit->second.nodes, into.ways);
    
      indexed_set_union(rel_ways, into.ways);
      collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, into.relations);
    
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      collect_relations(*this, rman, mit->second.nodes, Relation_Entry::NODE, node_rels);
      indexed_set_union(into.relations, node_rels);
    }
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_rel_ways = mit->second.attic_ways;
      collect_ways(*this, rman, mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways);
    
      indexed_set_union(rel_ways, into.ways);
      indexed_set_union(attic_rel_ways, into.attic_ways);
      collect_relations(*this, rman, rel_ways, attic_rel_ways,
                        Relation_Entry::WAY, into.relations, into.attic_relations);
      
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_node_rels;
      collect_relations(*this, rman, mit->second.nodes, mit->second.attic_nodes,
                        Relation_Entry::NODE, node_rels, attic_node_rels);      
      indexed_set_union(into.relations, node_rels);
      indexed_set_union(into.attic_relations, attic_node_rels);
    }
  }
  else if (type == RECURSE_UP_REL)
  {
    if (rman.get_desired_timestamp() == NOW)
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
    else
    {
      map< Uint31_Index, vector< Way_Skeleton > > rel_ways = mit->second.ways;
      map< Uint31_Index, vector< Attic< Way_Skeleton > > > attic_rel_ways = mit->second.attic_ways;
      collect_ways(*this, rman,
                   mit->second.nodes, mit->second.attic_nodes, into.ways, into.attic_ways);
      indexed_set_union(rel_ways, into.ways);
      indexed_set_union(attic_rel_ways, into.attic_ways);
        
      map< Uint31_Index, vector< Relation_Skeleton > > way_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_way_rels;
      collect_relations(*this, rman, rel_ways, attic_rel_ways,
                        Relation_Entry::WAY, way_rels, attic_way_rels);
        
      map< Uint31_Index, vector< Relation_Skeleton > > rel_rels = mit->second.relations;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_rel_rels = mit->second.attic_relations;
      indexed_set_union(rel_rels, way_rels);
      indexed_set_union(attic_rel_rels, attic_way_rels);
      
      map< Uint31_Index, vector< Relation_Skeleton > > node_rels;
      map< Uint31_Index, vector< Attic< Relation_Skeleton > > > attic_node_rels;
      collect_relations(*this, rman, mit->second.nodes, mit->second.attic_nodes,
                        Relation_Entry::NODE, node_rels, attic_node_rels);
      indexed_set_union(rel_rels, node_rels);
      indexed_set_union(attic_rel_rels, attic_node_rels);
        
      relations_up_loop(*this, rman, rel_rels, attic_rel_rels, rman.get_desired_timestamp(),
                        into.relations, into.attic_relations);
    }
  }

  if (rman.get_desired_timestamp() != NOW)
  {
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.ways, into.attic_ways);
    filter_attic_elements(rman, rman.get_desired_timestamp(), into.relations, into.attic_relations);
  }
  
  transfer_output(rman, into);
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
