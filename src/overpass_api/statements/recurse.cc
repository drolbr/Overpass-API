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
#include "../data/utils.h"
#include "recurse.h"


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


Recurse_Statement::Statement_Maker Recurse_Statement::statement_maker;
Recurse_Statement::Criterion_Maker_1 Recurse_Statement::criterion_maker_1;
Recurse_Statement::Criterion_Maker_2 Recurse_Statement::criterion_maker_2;


Statement* Recurse_Statement::Criterion_Maker_1::create_criterion(const Token_Node_Ptr& input_tree,
    const std::string& result_type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  Token_Node_Ptr tree_it = input_tree;
  uint line_nr = tree_it->line_col.first;

  std::string from = "_";
  std::string role;
  bool role_found = false;

  if (tree_it->token == ":" && tree_it->rhs)
  {
    role_found = true;
    role = decode_json(tree_it.rhs()->token, error_output);
    tree_it = tree_it.lhs();
  }

  if (tree_it->token == "." && tree_it->rhs)
  {
    from = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
  }

  std::string type = tree_it->token;
  std::map< std::string, std::string > attributes;
  attributes["from"] = from;
  attributes["into"] = into;

  if (type == "r")
  {
    if (result_type == "node")
      attributes["type"] = "relation-node";
    else if (result_type == "way")
      attributes["type"] = "relation-way";
    else if (result_type == "relation")
      attributes["type"] = "relation-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'r' produces nodes, ways, or relations.", line_nr);
  }
  else if (type == "w")
  {
    if (result_type == "node")
      attributes["type"] = "way-node";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'w' produces nodes.", line_nr);
  }
  else if (type == "br")
  {
    if (result_type == "relation")
      attributes["type"] = "relation-backwards";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'br' produces relations.", line_nr);
  }
  else if (type == "bw")
  {
    if (result_type == "relation")
      attributes["type"] = "way-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'bw' produces relations.", line_nr);
  }
  else if (type == "bn")
  {
    if (result_type == "way")
      attributes["type"] = "node-way";
    else if (result_type == "relation")
      attributes["type"] = "node-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion from type 'bn' produces ways or relations.", line_nr);
  }
  else
    return 0;

  if (role_found)
  {
    attributes["role"] = role;
    attributes["role-restricted"] = "yes";
  }
  return new Recurse_Statement(line_nr, attributes, global_settings);
}


Statement* Recurse_Statement::Criterion_Maker_2::create_criterion(const Token_Node_Ptr& input_tree,
    const std::string& result_type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  Token_Node_Ptr tree_it = input_tree;
  uint line_nr = tree_it->line_col.first;

  std::string from = "_";

  if (tree_it->token == "." && tree_it->rhs)
  {
    from = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
  }

  std::string type = tree_it->token;
  std::map< std::string, std::string > attributes;
  attributes["from"] = from;
  attributes["into"] = into;

  if (type == ">")
    attributes["type"] = "down";
  else if (type == ">>")
    attributes["type"] = "down-rel";
  else if (type == "<")
    attributes["type"] = "up";
  else if (type == "<<")
    attributes["type"] = "up-rel";
  else
    return 0;

  return new Recurse_Statement(line_nr, attributes, global_settings);
}


//-----------------------------------------------------------------------------


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources, uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result)
{
  std::vector< Relation_Entry::Ref_Type > ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
			 Get_Parent_Rels_Predicate(ids, source_type), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources, uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  collect_items_discrete(&stmt, rman, *osm_base_settings().RELATIONS, req,
                         Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), result);
}


template< class TSourceIndex, class TSourceObject >
void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources, uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids)
{
  std::vector< Relation_Entry::Ref_Type > children_ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
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
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources, uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > children_ids = extract_children_ids< TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
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
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources,
     const std::map< TSourceIndex, std::vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > ids;
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
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources,
     const std::map< TSourceIndex, std::vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result,
     uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > ids;
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
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources,
     const std::map< TSourceIndex, std::vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > children_ids;
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
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources,
     const std::map< TSourceIndex, std::vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < TSourceIndex, TSourceObject, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > req = extract_parent_indices(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < TSourceIndex, Attic< TSourceObject >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);
  std::set< Uint31_Index > attic_req = extract_parent_indices(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > children_ids;
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
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result)
{
  std::vector< Uint64 > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);
  rman.health_check(stmt);

  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Predicate(ids, Relation_Entry::RELATION), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result, uint32 role_id)
{
  std::vector< Uint64 > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);
  rman.health_check(stmt);

  collect_items_flat(stmt, rman, *osm_base_settings().RELATIONS,
      Get_Parent_Rels_Role_Predicate(ids, Relation_Entry::RELATION, role_id), result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids)
{
  std::vector< Uint64 > children_ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);
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
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  std::vector< Uint64 > children_ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Uint64 >(sources);
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
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));

  collect_items_flat_by_timestamp(stmt, rman,
      Get_Parent_Rels_Predicate(ids, Relation_Entry::RELATION), result, attic_result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > ids;
  std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));

  collect_items_flat_by_timestamp(stmt, rman,
      Get_Parent_Rels_Role_Predicate(ids, Relation_Entry::RELATION, role_id), result, attic_result);
}


void collect_relations
    (const Statement& stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > children_ids;
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
     const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
     const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_sources,
     std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
     std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
      < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(sources);
  rman.health_check(stmt);

  std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
      < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(attic_sources);
  rman.health_check(stmt);

  std::vector< Uint64 > children_ids;
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


void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
		   const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
		   const std::vector< Node::Id_Type >& ids, bool invert_ids,
		   std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes)
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
                   const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
                   const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
                   const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
                   const std::vector< Node::Id_Type >& ids, bool invert_ids,
                   std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
                   std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_node_members(&query, rman, rels, attic_rels), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, 0, &ids, invert_ids), nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, &ranges), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, &ranges, &ids, invert_ids), nodes, attic_nodes);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
                   const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
                   const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
                   const std::vector< Node::Id_Type >& ids, bool invert_ids,
                   std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
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
                   const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
                   const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
                   const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
                   const std::vector< Node::Id_Type >& ids, bool invert_ids,
                   std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
                   std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_nodes,
                   uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, 0, 0, false, &role_id), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, 0, &ids, invert_ids, &role_id), nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, &ranges, 0, false, &role_id), nodes, attic_nodes);
    else
      swap_components(relation_node_members
          (&query, rman, rels, attic_rels, &ranges, &ids, invert_ids, &role_id), nodes, attic_nodes);
  }
}


void collect_nodes(const Statement& query, Resource_Manager& rman,
		   const std::map< Uint31_Index, std::vector< Way_Skeleton > >& rels,
		   const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
		   const std::vector< Node::Id_Type >& ids, bool invert_ids,
		   std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes)
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
                   const std::map< Uint31_Index, std::vector< Way_Skeleton > >& rels,
                   const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_rels,
                   const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
                   const std::vector< Node::Id_Type >& ids, bool invert_ids,
                   std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
                   std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_nodes)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(way_members(&query, rman, rels, attic_rels), nodes, attic_nodes);
    else if (!invert_ids)
      swap_components(way_members(&query, rman, rels, attic_rels, 0, &ids), nodes, attic_nodes);
    else
      swap_components(way_members(&query, rman, rels, attic_rels, 0, &ids, invert_ids),
                      nodes, attic_nodes);
  }
  else
  {
    if (ids.empty())
      swap_components(way_members(&query, rman, rels, attic_rels, &ranges), nodes, attic_nodes);
    else if (!invert_ids)
      swap_components(way_members(&query, rman, rels, attic_rels, &ranges, &ids),
                      nodes, attic_nodes);
    else
      swap_components(way_members(&query, rman, rels, attic_rels, 0, &ids, invert_ids),
                      nodes, attic_nodes);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
		  const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
		  const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
		  const std::vector< Relation::Id_Type >& ids, bool invert_ids,
		  std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations)
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
                  const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
                  const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
                  const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
                  const std::vector< Relation::Id_Type >& ids, bool invert_ids,
                  std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
                  std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, 0, &ids, invert_ids), relations, attic_relations);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, &ranges), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, &ranges, &ids, invert_ids), relations, attic_relations);
  }
}


void collect_relations(const Statement& query, Resource_Manager& rman,
                  const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
                  const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
                  const std::vector< Relation::Id_Type >& ids, bool invert_ids,
                  std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
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
                  const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& rels,
                  const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_rels,
                  const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
                  const std::vector< Relation::Id_Type >& ids, bool invert_ids,
                  std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
                  std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_relations,
                  uint32 role_id)
{
  if (ranges.empty())
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, 0, 0, false, &role_id), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, 0, &ids, invert_ids, &role_id),
                      relations, attic_relations);
  }
  else
  {
    if (ids.empty())
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, &ranges, 0, false, &role_id), relations, attic_relations);
    else
      swap_components(relation_relation_members
          (query, rman, rels, attic_rels, &ranges, &ids, invert_ids, &role_id),
                      relations, attic_relations);
  }
}


void relations_loop(const Statement& query, Resource_Manager& rman,
		    std::map< Uint31_Index, std::vector< Relation_Skeleton > > source,
		    std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result)
{
  uint old_rel_count = count(source);
  while (true)
  {
    result = relation_relation_members(query, rman, source);
    sort_second(source);
    sort_second(result);
    indexed_set_union(result, source);
    uint new_rel_count = count(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}


void relations_loop(const Statement& query, Resource_Manager& rman,
                    std::map< Uint31_Index, std::vector< Relation_Skeleton > > source,
                    std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_source,
                    std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
                    std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result)
{
  uint old_rel_count = count(source) + count(attic_source);
  while (true)
  {
    std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
        std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > result_pair
        = relation_relation_members(query, rman, source, attic_source);
    sort_second(source);
    sort_second(attic_source);
    sort_second(result_pair.first);
    sort_second(result_pair.second);
    indexed_set_union(result_pair.first, source);
    indexed_set_union(result_pair.second, attic_source);
    keep_matching_skeletons(result_pair.first, result_pair.second, rman.get_desired_timestamp());
    uint new_rel_count = count(result_pair.first) + count(result_pair.second);
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
		    std::map< Uint31_Index, std::vector< Relation_Skeleton > > source,
		    std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result)
{
  uint old_rel_count = count(source);
  while (true)
  {
    result.clear();
    collect_relations(query, rman, source, result);
    sort_second(source);
    sort_second(result);
    indexed_set_union(result, source);
    uint new_rel_count = count(result);
    if (new_rel_count == old_rel_count)
      return;
    old_rel_count = new_rel_count;
    source.swap(result);
  }
}


void relations_up_loop(const Statement& query, Resource_Manager& rman,
                    std::map< Uint31_Index, std::vector< Relation_Skeleton > > source,
                    std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_source,
                    std::map< Uint31_Index, std::vector< Relation_Skeleton > >& result,
                    std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_result)
{
  uint old_rel_count = count(source) + count(attic_source);
  while (true)
  {
    result.clear();
    attic_result.clear();
    collect_relations(query, rman, source, attic_source, result, attic_result);
    sort_second(source);
    sort_second(result);
    indexed_set_union(result, source);
    sort_second(attic_source);
    sort_second(attic_result);
    indexed_set_union(attic_result, attic_source);
    keep_matching_skeletons(result, attic_result, rman.get_desired_timestamp());
    uint new_rel_count = count(result) + count(attic_result);
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

    virtual bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);
    virtual bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges);

    Query_Filter_Strategy delivers_data(Resource_Manager& rman) { return prefer_ranges; }

    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
                          const std::vector< Node::Id_Type >& ids,
                          bool invert_ids);
    virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                          const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
                          int type,
                          const std::vector< Uint32_Index >& ids,
                          bool invert_ids);
    void filter(Resource_Manager& rman, Set& into);
    void filter(const Statement& query, Resource_Manager& rman, Set& into);
    virtual ~Recurse_Constraint() {}

  private:
    Recurse_Statement* stmt;
};


bool Recurse_Constraint::get_ranges(Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges.clear();

  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return true;

  if (rman.get_desired_timestamp() == NOW)
  {
    if (stmt->get_type() == RECURSE_RELATION_NODE)
    {
      relation_node_member_indices< Relation_Skeleton >(
          stmt, rman, input->relations.begin(), input->relations.end()).swap(ranges);

      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_NODE)
    {
      way_nd_indices(stmt, rman, input->ways.begin(), input->ways.end()).swap(ranges);

      return true;
    }
    else if (stmt->get_type() == RECURSE_DOWN)
      return false;
    else if (stmt->get_type() == RECURSE_DOWN_REL)
      return false;
  }
  else
  {
    if (stmt->get_type() == RECURSE_RELATION_NODE)
    {
      relation_node_member_indices< Relation_Skeleton >(
          stmt, rman, input->relations.begin(), input->relations.end(),
          input->attic_relations.begin(), input->attic_relations.end()).swap(ranges);

      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_NODE)
    {
      way_nd_indices(stmt, rman, input->ways.begin(), input->ways.end(),
          input->attic_ways.begin(), input->attic_ways.end()).swap(ranges);

      return true;
    }
    else if (stmt->get_type() == RECURSE_DOWN)
      return false;
    else if (stmt->get_type() == RECURSE_DOWN_REL)
      return false;
  }

  return false;
}


bool Recurse_Constraint::get_ranges(Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  ranges.clear();

  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return true;

  if (rman.get_desired_timestamp() == NOW)
  {
    if (stmt->get_type() == RECURSE_RELATION_WAY)
    {
      std::vector< Uint31_Index > req = relation_way_member_indices< Relation_Skeleton >(
          stmt, rman, input->relations.begin(), input->relations.end());
      for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));

      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
      return false;
    else if (stmt->get_type() == RECURSE_DOWN)
      return false;
    else if (stmt->get_type() == RECURSE_DOWN_REL)
      return false;
    else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_NODE_RELATION)
    {
      std::set< Uint31_Index > req = extract_parent_indices(input->nodes);
      for (std::set< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));

      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      std::set< Uint31_Index > req = extract_parent_indices(input->ways);
      for (std::set< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));

      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      return false;
    else if (stmt->get_type() == RECURSE_UP)
      return false;
    else if (stmt->get_type() == RECURSE_UP_REL)
      return false;
    else
      return false;
  }
  else
  {
    if (stmt->get_type() == RECURSE_RELATION_WAY)
    {
      std::vector< Uint31_Index > req = relation_way_member_indices< Relation_Skeleton >(
          stmt, rman, input->relations.begin(), input->relations.end(),
          input->attic_relations.begin(), input->attic_relations.end());
      for (std::vector< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));

      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
      return false;
    else if (stmt->get_type() == RECURSE_DOWN)
      return false;
    else if (stmt->get_type() == RECURSE_DOWN_REL)
      return false;
    else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_NODE_RELATION)
    {
      std::set< Uint31_Index > req = extract_parent_indices(input->nodes);
      for (std::set< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));
      std::set< Uint31_Index > attic_req = extract_parent_indices(input->attic_nodes);
      for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));

      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      std::set< Uint31_Index > req = extract_parent_indices(input->ways);
      for (std::set< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));
      std::set< Uint31_Index > attic_req = extract_parent_indices(input->attic_ways);
      for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
        ranges.insert(std::make_pair(*it, inc(*it)));

      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      return false;
    else if (stmt->get_type() == RECURSE_UP)
      return false;
    else if (stmt->get_type() == RECURSE_UP_REL)
      return false;
    else
      return false;
  }

  return false;
}



bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges,
     const std::vector< Node::Id_Type >& ids,
     bool invert_ids)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return true;

  if (rman.get_desired_timestamp() == NOW)
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == std::numeric_limits< uint32 >::max())
        return true;

      if (stmt->get_type() == RECURSE_RELATION_NODE)
        ::collect_nodes(query, rman, input->relations, ranges, ids, invert_ids, into.nodes, role_id);
      return true;
    }

    if (stmt->get_type() == RECURSE_RELATION_NODE)
      ::collect_nodes(query, rman, input->relations, ranges, ids, invert_ids, into.nodes);
    else if (stmt->get_type() == RECURSE_WAY_NODE)
      ::collect_nodes(query, rman, input->ways, ranges, ids, invert_ids, into.nodes);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      std::map< Uint32_Index, std::vector< Node_Skeleton > > rel_nodes;
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways;
      ::collect_nodes(query, rman, input->relations, ranges, ids, invert_ids, rel_nodes);
      rel_ways = relation_way_members(&query, rman, input->relations);
      ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
      sort_second(into.nodes);
      sort_second(rel_nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, input->relations, rel_rels);
      std::map< Uint32_Index, std::vector< Node_Skeleton > > rel_nodes;
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways;
      ::collect_nodes(query, rman, rel_rels, ranges, ids, invert_ids, rel_nodes);
      rel_ways = relation_way_members(&query, rman, rel_rels);
      ::collect_nodes(query, rman, rel_ways, ranges, ids, invert_ids, into.nodes);
      sort_second(into.nodes);
      sort_second(rel_nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
  }
  else
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == std::numeric_limits< uint32 >::max())
        return true;

      if (stmt->get_type() == RECURSE_RELATION_NODE)
        ::collect_nodes(query, rman, input->relations, input->attic_relations,
                        ranges, ids, invert_ids, into.nodes, into.attic_nodes, role_id);
      return true;
    }

    if (stmt->get_type() == RECURSE_RELATION_NODE)
      ::collect_nodes(query, rman, input->relations, input->attic_relations,
                      ranges, ids, invert_ids, into.nodes, into.attic_nodes);
    else if (stmt->get_type() == RECURSE_WAY_NODE)
      ::collect_nodes(query, rman, input->ways, input->attic_ways,
                      ranges, ids, invert_ids, into.nodes, into.attic_nodes);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      std::map< Uint32_Index, std::vector< Node_Skeleton > > rel_nodes;
      std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > rel_attic_nodes;
      ::collect_nodes(query, rman, input->relations, input->attic_relations,
                      ranges, ids, invert_ids, rel_nodes, rel_attic_nodes);

      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > rel_ways
          = relation_way_members(&query, rman, input->relations, input->attic_relations);
      ::collect_nodes(query, rman, rel_ways.first, rel_ways.second,
                      ranges, ids, invert_ids, into.nodes, into.attic_nodes);
      sort_second(into.nodes);
      sort_second(rel_nodes);
      indexed_set_union(into.nodes, rel_nodes);
      sort_second(into.attic_nodes);
      sort_second(rel_attic_nodes);
      indexed_set_union(into.attic_nodes, rel_attic_nodes);
      keep_matching_skeletons(into.nodes, into.attic_nodes, rman.get_desired_timestamp());
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_rel_rels;
      relations_loop(query, rman,
                     input->relations, input->attic_relations,
                     rel_rels, attic_rel_rels);

      std::map< Uint32_Index, std::vector< Node_Skeleton > > rel_nodes;
      std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > rel_attic_nodes;
      ::collect_nodes(query, rman, rel_rels, attic_rel_rels,
                      ranges, ids, invert_ids, rel_nodes, rel_attic_nodes);

      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > rel_ways
          = relation_way_members(&query, rman, rel_rels, attic_rel_rels);
      ::collect_nodes(query, rman, rel_ways.first, rel_ways.second,
                      ranges, ids, invert_ids, into.nodes, into.attic_nodes);

      sort_second(into.nodes);
      sort_second(rel_nodes);
      indexed_set_union(into.nodes, rel_nodes);
      sort_second(into.attic_nodes);
      sort_second(rel_attic_nodes);
      indexed_set_union(into.attic_nodes, rel_attic_nodes);
      keep_matching_skeletons(into.nodes, into.attic_nodes, rman.get_desired_timestamp());
    }
  }
  return true;
}

bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges,
     int type,
     const std::vector< Uint32_Index >& ids,
     bool invert_ids)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return true;

  if (rman.get_desired_timestamp() == NOW)
  {
    if (stmt->get_role())
    {
      uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
      if (role_id == std::numeric_limits< uint32 >::max())
        return true;

      if (stmt->get_type() == RECURSE_RELATION_WAY)
        collect_ways(query, rman, input->relations, ranges, ids, invert_ids, into.ways, role_id);
      else if (stmt->get_type() == RECURSE_RELATION_RELATION)
        collect_relations(query, rman, input->relations, ranges,
                        ids, invert_ids, into.relations, role_id);
      else if (stmt->get_type() == RECURSE_NODE_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, input->nodes, Relation_Entry::NODE, into.relations, role_id);
        else
          collect_relations(query, rman, input->nodes, Relation_Entry::NODE, into.relations, ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, input->ways, Relation_Entry::WAY, into.relations, role_id);
        else
          collect_relations(query, rman, input->ways, Relation_Entry::WAY,
                        into.relations, ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      {
        if (ids.empty())
          collect_relations(query, rman, input->relations, into.relations, role_id);
        else
          collect_relations(query, rman, input->relations, into.relations,
                        ids, invert_ids, role_id);
      }
      else
        return false;

      return true;
    }

    if (stmt->get_type() == RECURSE_RELATION_WAY)
      collect_ways(query, rman, input->relations, ranges, ids, invert_ids, into.ways);
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
      collect_relations(query, rman, input->relations, ranges,
		      ids, invert_ids, into.relations);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      if (type != QUERY_WAY)
        return true;
      collect_ways(query, rman, input->relations, ranges, ids, invert_ids, into.ways);
      return true;
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, input->relations, rel_rels);
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
        collect_ways(query, rman, input->nodes, into.ways);
      else
        collect_ways(query, rman, input->nodes, into.ways, ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, input->nodes, Relation_Entry::NODE, into.relations);
      else
        collect_relations(query, rman, input->nodes, Relation_Entry::NODE, into.relations, ids, invert_ids);
      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, input->ways, Relation_Entry::WAY, into.relations);
      else
        collect_relations(query, rman, input->ways, Relation_Entry::WAY,
			  into.relations, ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
      if (ids.empty())
        collect_relations(query, rman, input->relations, into.relations);
      else
        collect_relations(query, rman, input->relations, into.relations,
			  ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_UP)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
	  collect_ways(query, rman, input->nodes, into.ways);
        else
	  collect_ways(query, rman, input->nodes, into.ways,
		       ids, invert_ids);
      }
      else
      {
        std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
        std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
        collect_ways(query, rman, input->nodes, node_ways);
        sort_second(rel_ways);
        sort_second(node_ways);
        indexed_set_union(rel_ways, node_ways);
        if (ids.empty())
	  collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations);
        else
	  collect_relations(query, rman, rel_ways, Relation_Entry::WAY, into.relations,
			  ids, invert_ids);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
        if (ids.empty())
	  collect_relations(query, rman, input->nodes, Relation_Entry::NODE, node_rels);
        else
	  collect_relations(query, rman, input->nodes, Relation_Entry::NODE, node_rels,
			  ids, invert_ids);
        sort_second(into.relations);
        sort_second(node_rels);
        indexed_set_union(into.relations, node_rels);
      }
    }
    else if (stmt->get_type() == RECURSE_UP_REL)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
	  collect_ways(query, rman, input->nodes, into.ways);
        else
	  collect_ways(query, rman, input->nodes, into.ways, ids, invert_ids);
      }
      else
      {
        std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
        std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
        collect_ways(query, rman, input->nodes, node_ways);
        sort_second(rel_ways);
        sort_second(node_ways);
        indexed_set_union(rel_ways, node_ways);
        std::map< Uint31_Index, std::vector< Relation_Skeleton > > way_rels;
        collect_relations(query, rman, rel_ways, Relation_Entry::WAY, way_rels);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels = input->relations;
        sort_second(rel_rels);
        sort_second(way_rels);
        indexed_set_union(rel_rels, way_rels);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
        collect_relations(query, rman, input->nodes, Relation_Entry::NODE, node_rels);
        sort_second(rel_rels);
        sort_second(node_rels);
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
      if (role_id == std::numeric_limits< uint32 >::max())
        return true;

      if (stmt->get_type() == RECURSE_RELATION_WAY)
        collect_ways(query, rman, input->relations, input->attic_relations,
                     ranges, ids, invert_ids, into.ways, into.attic_ways, role_id);
      else if (stmt->get_type() == RECURSE_RELATION_RELATION)
        collect_relations(query, rman, input->relations, input->attic_relations,
                     ranges, ids, invert_ids, into.relations, into.attic_relations, role_id);
      else if (stmt->get_type() == RECURSE_NODE_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, input->nodes, input->attic_nodes,
                            Relation_Entry::NODE, into.relations, into.attic_relations, role_id);
        else
          collect_relations(query, rman, input->nodes, input->attic_nodes,
                            Relation_Entry::NODE, into.relations, into.attic_relations,
                            ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
      {
        if (ids.empty())
          collect_relations(query, rman, input->ways, input->attic_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations, role_id);
        else
          collect_relations(query, rman, input->ways, input->attic_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations,
                            ids, invert_ids, role_id);
      }
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      {
        if (ids.empty())
          collect_relations(query, rman, input->relations, input->attic_relations,
                            into.relations, into.attic_relations, role_id);
        else
          collect_relations(query, rman, input->relations, input->attic_relations,
                            into.relations, into.attic_relations, ids, invert_ids, role_id);
      }
      else
        return false;

      return true;
    }

    if (stmt->get_type() == RECURSE_RELATION_WAY)
      collect_ways(query, rman, input->relations, input->attic_relations,
                   ranges, ids, invert_ids, into.ways, into.attic_ways);
    else if (stmt->get_type() == RECURSE_RELATION_RELATION)
      collect_relations(query, rman, input->relations, input->attic_relations,
                     ranges, ids, invert_ids, into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_DOWN)
    {
      if (type != QUERY_WAY)
        return true;
      collect_ways(query, rman, input->relations, input->attic_relations,
                   ranges, ids, invert_ids, into.ways, into.attic_ways);
    }
    else if (stmt->get_type() == RECURSE_DOWN_REL)
    {
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_rel_rels;
      relations_loop(query, rman,
                     input->relations, input->attic_relations,
                     rel_rels, attic_rel_rels);
      if (type == QUERY_WAY)
        collect_ways(query, rman, rel_rels, attic_rel_rels,
                     ranges, ids, invert_ids, into.ways, into.attic_ways);
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

        keep_matching_skeletons(into.relations, into.attic_relations, rman.get_desired_timestamp());
      }
    }
    else if (stmt->get_type() == RECURSE_NODE_WAY)
    {
      if (ids.empty())
        collect_ways(query, rman,
                     input->nodes, input->attic_nodes, into.ways, into.attic_ways);
      else
        collect_ways(query, rman,
                     input->nodes, input->attic_nodes, into.ways, into.attic_ways,
                     ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, input->nodes, input->attic_nodes,
                          Relation_Entry::NODE, into.relations, into.attic_relations);
      else
        collect_relations(query, rman, input->nodes, input->attic_nodes,
                          Relation_Entry::NODE, into.relations, into.attic_relations,
                          ids, invert_ids);
      return true;
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      if (ids.empty())
        collect_relations(query, rman, input->ways, input->attic_ways,
                          Relation_Entry::WAY, into.relations, into.attic_relations);
      else
        collect_relations(query, rman, input->ways, input->attic_ways,
                          Relation_Entry::WAY, into.relations, into.attic_relations,
                          ids, invert_ids);
      return true;
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
        if (ids.empty())
          collect_relations(query, rman, input->relations, input->attic_relations,
                            into.relations, into.attic_relations);
        else
          collect_relations(query, rman, input->relations, input->attic_relations,
                            into.relations, into.attic_relations, ids, invert_ids);
    }
    else if (stmt->get_type() == RECURSE_UP)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
          collect_ways(query, rman,
                       input->nodes, input->attic_nodes, into.ways, into.attic_ways);
        else
          collect_ways(query, rman,
                       input->nodes, input->attic_nodes, into.ways, into.attic_ways,
                       ids, invert_ids);
      }
      else
      {
        std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
        std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_rel_ways = input->attic_ways;
        std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
        std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_node_ways;
        collect_ways(query, rman,
                     input->nodes, input->attic_nodes, node_ways, attic_node_ways);
        sort_second(rel_ways);
        sort_second(node_ways);
        indexed_set_union(rel_ways, node_ways);
        sort_second(attic_rel_ways);
        sort_second(attic_node_ways);
        indexed_set_union(attic_rel_ways, attic_node_ways);
        if (ids.empty())
          collect_relations(query, rman, rel_ways, attic_rel_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations);
        else
          collect_relations(query, rman, rel_ways, attic_rel_ways,
                            Relation_Entry::WAY, into.relations, into.attic_relations,
                            ids, invert_ids);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
        std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_node_rels;
        if (ids.empty())
          collect_relations(query, rman, input->nodes, input->attic_nodes,
                            Relation_Entry::NODE, node_rels, attic_node_rels);
        else
          collect_relations(query, rman, input->nodes, input->attic_nodes,
                            Relation_Entry::NODE, node_rels, attic_node_rels,
                            ids, invert_ids);
        sort_second(into.relations);
        sort_second(node_rels);
        indexed_set_union(into.relations, node_rels);
        sort_second(into.attic_relations);
        sort_second(attic_node_rels);
        indexed_set_union(into.attic_relations, attic_node_rels);
      }
    }
    else if (stmt->get_type() == RECURSE_UP_REL)
    {
      if (type == QUERY_WAY)
      {
        if (ids.empty())
          collect_ways(query, rman,
                       input->nodes, input->attic_nodes, into.ways, into.attic_ways);
        else
          collect_ways(query, rman,
                       input->nodes, input->attic_nodes, into.ways, into.attic_ways,
                       ids, invert_ids);
      }
      else
      {
        std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
        std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_rel_ways = input->attic_ways;
        std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
        std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_node_ways;
        collect_ways(query, rman,
                     input->nodes, input->attic_nodes, node_ways, attic_node_ways);
        sort_second(rel_ways);
        sort_second(node_ways);
        indexed_set_union(rel_ways, node_ways);
        sort_second(attic_rel_ways);
        sort_second(attic_node_ways);
        indexed_set_union(attic_rel_ways, attic_node_ways);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > way_rels;
        std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_way_rels;
        collect_relations(query, rman, rel_ways, attic_rel_ways,
                          Relation_Entry::WAY, way_rels, attic_way_rels);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels = input->relations;
        std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_rel_rels = input->attic_relations;
        sort_second(rel_rels);
        sort_second(way_rels);
        indexed_set_union(rel_rels, way_rels);
        sort_second(attic_rel_rels);
        sort_second(attic_way_rels);
        indexed_set_union(attic_rel_rels, attic_way_rels);

        std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
        std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_node_rels;
        collect_relations(query, rman, input->nodes, input->attic_nodes,
                          Relation_Entry::NODE, node_rels, attic_node_rels);
        sort_second(rel_rels);
        sort_second(node_rels);
        indexed_set_union(rel_rels, node_rels);
        sort_second(attic_rel_rels);
        sort_second(attic_node_rels);
        indexed_set_union(attic_rel_rels, attic_node_rels);

        relations_up_loop(query, rman, rel_rels, attic_rel_rels, into.relations, into.attic_relations);

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


void Recurse_Constraint::filter(Resource_Manager& rman, Set& into)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
  {
    into.clear();
    return;
  }

  if (stmt->get_type() == RECURSE_DOWN || stmt->get_type() == RECURSE_DOWN_REL)
    return;

  if (stmt->get_role())
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
    if (role_id == std::numeric_limits< uint32 >::max())
      return;

    std::vector< Node::Id_Type > ids;
    if (stmt->get_type() == RECURSE_RELATION_NODE)
    {
      if (rman.get_desired_timestamp() == NOW)
        ids = relation_node_member_ids(rman, input->relations, &role_id);
      else
        ids = relation_node_member_ids(rman, input->relations, input->attic_relations, &role_id);
    }

    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.attic_nodes);

    if (stmt->get_type() == RECURSE_RELATION_WAY)
    {
      std::vector< Way::Id_Type > ids;
      if (rman.get_desired_timestamp() == NOW)
        relation_way_member_ids(rman, input->relations, &role_id).swap(ids);
      else
        relation_way_member_ids(rman, input->relations, input->attic_relations, &role_id).swap(ids);
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
      std::vector< Relation::Id_Type > ids;
      if (rman.get_desired_timestamp() == NOW)
        relation_relation_member_ids(rman, input->relations, &role_id).swap(ids);
      else
        relation_relation_member_ids(rman, input->relations, input->attic_relations, &role_id)
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
      else
        source_type = Relation_Entry::RELATION;

      std::vector< Relation_Entry::Ref_Type > ids;
      if (stmt->get_type() == RECURSE_NODE_RELATION)
      {
        std::vector< Node::Id_Type > current_ids = extract_children_ids
            < Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(input->nodes);
        std::vector< Node::Id_Type > attic_ids = extract_children_ids
            < Uint32_Index, Attic< Node_Skeleton >, Relation_Entry::Ref_Type >(input->attic_nodes);
        ids.clear();
        std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
      }
      else if (stmt->get_type() == RECURSE_WAY_RELATION)
      {
        std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
            < Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(input->ways);
        std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
            < Uint31_Index, Attic< Way_Skeleton >, Relation_Entry::Ref_Type >(input->attic_ways);
        ids.clear();
        std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
                 std::back_inserter(ids));
      }
      else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
        ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(input->relations);

      filter_items(Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), into.relations);
      filter_items(Get_Parent_Rels_Role_Predicate(ids, source_type, role_id), into.attic_relations);
    }
    else
      into.relations.clear();

    return;
  }

  std::vector< Node::Id_Type > ids;
  if (stmt->get_type() == RECURSE_WAY_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      ids = way_nd_ids(input->ways);
    else
      ids = way_nd_ids(input->ways, input->attic_ways);
    rman.health_check(*stmt);
  }
  else if (stmt->get_type() == RECURSE_RELATION_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      ids = relation_node_member_ids(rman, input->relations);
    else
      ids = relation_node_member_ids(rman, input->relations, input->attic_relations);
    rman.health_check(*stmt);
  }

  filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
  filter_items(Id_Predicate< Node_Skeleton >(ids), into.attic_nodes);

  if (stmt->get_type() == RECURSE_RELATION_WAY)
  {
    std::vector< Way::Id_Type > ids;
    if (rman.get_desired_timestamp() == NOW)
      relation_way_member_ids(rman, input->relations).swap(ids);
    else
      relation_way_member_ids(rman, input->relations, input->attic_relations).swap(ids);
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.ways);
    filter_items(Id_Predicate< Way_Skeleton >(ids), into.attic_ways);
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_UP
      || stmt->get_type() == RECURSE_UP_REL)
  {
    std::vector< Node::Id_Type > current_ids = extract_children_ids
        < Uint32_Index, Node_Skeleton, Node::Id_Type >(input->nodes);
    std::vector< Node::Id_Type > attic_ids = extract_children_ids
        < Uint32_Index, Attic< Node_Skeleton >, Node::Id_Type >(input->attic_nodes);
    std::vector< Node::Id_Type > ids;
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
    std::vector< Relation::Id_Type > ids;
    if (rman.get_desired_timestamp() == NOW)
      relation_relation_member_ids(rman, input->relations).swap(ids);
    else
      relation_relation_member_ids(rman, input->relations, input->attic_relations).swap(ids);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
    filter_items(Id_Predicate< Relation_Skeleton >(ids), into.attic_relations);
  }
  else if (stmt->get_type() == RECURSE_NODE_RELATION
      || stmt->get_type() == RECURSE_WAY_RELATION
      || stmt->get_type() == RECURSE_RELATION_BACKWARDS)
  {
    uint32 source_type = Relation_Entry::RELATION;
    if (stmt->get_type() == RECURSE_NODE_RELATION)
      source_type = Relation_Entry::NODE;
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      source_type = Relation_Entry::WAY;

    std::vector< Relation_Entry::Ref_Type > ids;
    if (stmt->get_type() == RECURSE_NODE_RELATION)
    {
      std::vector< Node::Id_Type > current_ids = extract_children_ids
          < Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(input->nodes);
      std::vector< Node::Id_Type > attic_ids = extract_children_ids
          < Uint32_Index, Attic< Node_Skeleton >, Relation_Entry::Ref_Type >(input->attic_nodes);
      ids.clear();
      std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
               std::back_inserter(ids));
    }
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
    {
      std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
          < Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(input->ways);
      std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
          < Uint31_Index, Attic< Way_Skeleton >, Relation_Entry::Ref_Type >(input->attic_ways);
      ids.clear();
      std::set_union(current_ids.begin(), current_ids.end(), attic_ids.begin(), attic_ids.end(),
               std::back_inserter(ids));
    }
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    {
      std::vector< Relation_Entry::Ref_Type > current_ids = extract_children_ids
          < Uint31_Index, Relation_Skeleton, Relation_Entry::Ref_Type >(input->relations);
      std::vector< Relation_Entry::Ref_Type > attic_ids = extract_children_ids
          < Uint31_Index, Attic< Relation_Skeleton >, Relation_Entry::Ref_Type >(input->attic_relations);
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


void Recurse_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return;
  if (stmt->get_type() != RECURSE_DOWN && stmt->get_type() != RECURSE_DOWN_REL
      && stmt->get_type() != RECURSE_UP && stmt->get_type() != RECURSE_UP_REL)
    return;

  if (stmt->get_type() == RECURSE_DOWN)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      std::vector< Node::Id_Type > rel_ids
          = relation_node_member_ids(rman, input->relations);
      std::map< Uint31_Index, std::vector< Way_Skeleton > > intermediate_ways;
      collect_ways(query, rman, input->relations, std::set< std::pair< Uint31_Index, Uint31_Index > >(),
	  std::vector< Way::Id_Type >(), false, intermediate_ways);
      std::vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways);
      rman.health_check(*stmt);

      std::vector< Node::Id_Type > ids;
      set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));

      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

      filter_items(Id_Predicate< Way_Skeleton >(relation_way_member_ids(rman, input->relations)),
		   into.ways);
    }
    else
    {
      std::vector< Node::Id_Type > rel_ids
          = relation_node_member_ids(rman, input->relations, input->attic_relations);
      std::map< Uint31_Index, std::vector< Way_Skeleton > > intermediate_ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > intermediate_attic_ways;
      collect_ways(query, rman, input->relations, input->attic_relations,
          std::set< std::pair< Uint31_Index, Uint31_Index > >(), std::vector< Way::Id_Type >(), false,
          intermediate_ways, intermediate_attic_ways);
      std::vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways, intermediate_attic_ways);
      rman.health_check(*stmt);

      std::vector< Node::Id_Type > ids;
      set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));

      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
      filter_items(Id_Predicate< Attic< Node_Skeleton > >(ids), into.attic_nodes);

      std::vector< Way::Id_Type > rel_way_ids
          = relation_way_member_ids(rman, input->relations, input->attic_relations);
      filter_items(Id_Predicate< Way_Skeleton >(rel_way_ids), into.ways);
      filter_items(Id_Predicate< Attic< Way_Skeleton > >(rel_way_ids), into.attic_ways);
    }

    into.attic_relations.clear();
    into.relations.clear();
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels;
      relations_loop(query, rman, input->relations, rel_rels);
      std::vector< Node::Id_Type > rel_ids
          = relation_node_member_ids(rman, rel_rels);
      std::map< Uint31_Index, std::vector< Way_Skeleton > > intermediate_ways;
      collect_ways(query, rman, rel_rels, std::set< std::pair< Uint31_Index, Uint31_Index > >(),
		   std::vector< Way::Id_Type >(), false, intermediate_ways);
      std::vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways);
      rman.health_check(*stmt);

      std::vector< Node::Id_Type > ids;
      set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));

      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);

      filter_items(Id_Predicate< Way_Skeleton >(relation_way_member_ids(rman, rel_rels)), into.ways);

      filter_items(Id_Predicate< Relation_Skeleton >(filter_for_ids(rel_rels)), into.relations);
    }
    else
    {
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_rel_rels;
      relations_loop(query, rman, input->relations, input->attic_relations, rel_rels, attic_rel_rels);

      std::vector< Node::Id_Type > rel_node_ids
          = relation_node_member_ids(rman, rel_rels, attic_rel_rels);
      std::map< Uint31_Index, std::vector< Way_Skeleton > > intermediate_ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > intermediate_attic_ways;
      collect_ways(query, rman, rel_rels, attic_rel_rels,
          std::set< std::pair< Uint31_Index, Uint31_Index > >(), std::vector< Way::Id_Type >(), false,
          intermediate_ways, intermediate_attic_ways);
      std::vector< Node::Id_Type > way_node_ids = way_nd_ids(intermediate_ways, intermediate_attic_ways);
      rman.health_check(*stmt);

      std::vector< Node::Id_Type > ids;
      set_union(way_node_ids.begin(), way_node_ids.end(), rel_node_ids.begin(), rel_node_ids.end(),
                back_inserter(ids));

      filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
      filter_items(Id_Predicate< Attic< Node_Skeleton > >(ids), into.attic_nodes);

      std::vector< Way::Id_Type > rel_way_ids
          = relation_way_member_ids(rman, input->relations, input->attic_relations);
      filter_items(Id_Predicate< Way_Skeleton >(rel_way_ids), into.ways);
      filter_items(Id_Predicate< Attic< Way_Skeleton > >(rel_way_ids), into.attic_ways);

      item_filter_map(into.relations, rel_rels);
      item_filter_map(into.attic_relations, attic_rel_rels);
    }
  }
  else if (stmt->get_type() == RECURSE_UP && !into.relations.empty())
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
      std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, input->nodes, node_ways);
      sort_second(rel_ways);
      sort_second(node_ways);
      indexed_set_union(rel_ways, node_ways);

      std::vector< Relation_Entry::Ref_Type > node_ids = extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(input->nodes);
      std::vector< Relation_Entry::Ref_Type > way_ids = extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(rel_ways);

      filter_items(
          Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
          (Get_Parent_Rels_Predicate(node_ids, Relation_Entry::NODE),
	   Get_Parent_Rels_Predicate(way_ids, Relation_Entry::WAY)), into.relations);
    }
    else
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_rel_ways = input->attic_ways;
      std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_node_ways;
      collect_ways(query, rman, input->nodes, input->attic_nodes, node_ways, attic_node_ways);
      sort_second(rel_ways);
      sort_second(node_ways);
      indexed_set_union(rel_ways, node_ways);
      sort_second(attic_rel_ways);
      sort_second(attic_node_ways);
      indexed_set_union(attic_rel_ways, attic_node_ways);

      std::vector< Relation_Entry::Ref_Type > current_node_ids =
          extract_children_ids< Uint32_Index, Node_Skeleton, Relation_Entry::Ref_Type >(input->nodes);
      std::vector< Relation_Entry::Ref_Type > attic_node_ids =
          extract_children_ids< Uint32_Index, Attic< Node_Skeleton >, Relation_Entry::Ref_Type >(input->attic_nodes);
      std::vector< Relation_Entry::Ref_Type > node_ids;
      std::set_union(current_node_ids.begin(), current_node_ids.end(),
                     attic_node_ids.begin(), attic_node_ids.end(),
                     std::back_inserter(node_ids));

      std::vector< Relation_Entry::Ref_Type > current_way_ids =
          extract_children_ids< Uint31_Index, Way_Skeleton, Relation_Entry::Ref_Type >(rel_ways);
      std::vector< Relation_Entry::Ref_Type > attic_way_ids =
          extract_children_ids< Uint31_Index, Attic< Way_Skeleton >, Relation_Entry::Ref_Type >(attic_rel_ways);
      std::vector< Relation_Entry::Ref_Type > way_ids;
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
    if (rman.get_desired_timestamp() == NOW)
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
      std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
      collect_ways(query, rman, input->nodes, node_ways);
      sort_second(rel_ways);
      sort_second(node_ways);
      indexed_set_union(rel_ways, node_ways);
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > way_rels;
      collect_relations(query, rman, rel_ways, Relation_Entry::WAY, way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels = input->relations;
      sort_second(rel_rels);
      sort_second(way_rels);
      indexed_set_union(rel_rels, way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
      collect_relations(query, rman, input->nodes, Relation_Entry::NODE, node_rels);
      sort_second(rel_rels);
      sort_second(node_rels);
      indexed_set_union(rel_rels, node_rels);

      relations_up_loop(query, rman, rel_rels, rel_rels);

      std::vector< Relation::Id_Type > ids = extract_children_ids< Uint31_Index, Relation_Skeleton, Relation::Id_Type >(rel_rels);
      filter_items(Id_Predicate< Relation_Skeleton >(ids), into.relations);
    }
    else
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input->ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_rel_ways = input->attic_ways;
      std::map< Uint31_Index, std::vector< Way_Skeleton > > node_ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_node_ways;
      collect_ways(query, rman,
                   input->nodes, input->attic_nodes, node_ways, attic_node_ways);
      sort_second(rel_ways);
      sort_second(node_ways);
      indexed_set_union(rel_ways, node_ways);
      sort_second(attic_rel_ways);
      sort_second(attic_node_ways);
      indexed_set_union(attic_rel_ways, attic_node_ways);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > way_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_way_rels;
      collect_relations(query, rman, rel_ways, attic_rel_ways,
                        Relation_Entry::WAY, way_rels, attic_way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels = input->relations;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_rel_rels = input->attic_relations;
      sort_second(rel_rels);
      sort_second(way_rels);
      indexed_set_union(rel_rels, way_rels);
      sort_second(attic_rel_rels);
      sort_second(attic_way_rels);
      indexed_set_union(attic_rel_rels, attic_way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_node_rels;
      collect_relations(query, rman, input->nodes, input->attic_nodes,
                        Relation_Entry::NODE, node_rels, attic_node_rels);
      sort_second(rel_rels);
      sort_second(node_rels);
      indexed_set_union(rel_rels, node_rels);
      sort_second(attic_rel_rels);
      sort_second(attic_node_rels);
      indexed_set_union(attic_rel_rels, attic_node_rels);

      relations_up_loop(query, rman, rel_rels, attic_rel_rels, rel_rels, attic_rel_rels);

      std::vector< Relation::Id_Type > current_ids = extract_children_ids
          < Uint31_Index, Relation_Skeleton, Relation::Id_Type >(rel_rels);
      std::vector< Relation::Id_Type > attic_ids = extract_children_ids
          < Uint31_Index, Attic< Relation_Skeleton >, Relation::Id_Type >(attic_rel_rels);

      std::vector< Relation::Id_Type > ids;
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
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), restrict_to_role(false)
{
  std::map< std::string, std::string > attributes;

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
    std::ostringstream temp;
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
      std::ostringstream temp;
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


std::string Recurse_Statement::to_target_type(int type)
{
  if (type == RECURSE_RELATION_RELATION || type == RECURSE_RELATION_BACKWARDS
      || type == RECURSE_WAY_RELATION || type == RECURSE_NODE_RELATION)
    return "relation";
  else if (type == RECURSE_RELATION_WAY || type == RECURSE_NODE_WAY)
    return "way";
  else if (type == RECURSE_RELATION_NODE || type == RECURSE_WAY_NODE)
    return "node";

  return "";
}


std::string Recurse_Statement::to_xml_representation(int type)
{
  if (type == RECURSE_RELATION_RELATION)
    return "relation-relation";
  else if (type == RECURSE_RELATION_BACKWARDS)
    return "relation-backwards";
  else if (type == RECURSE_RELATION_WAY)
    return "relation-way";
  else if (type == RECURSE_RELATION_NODE)
    return "relation-node";
  else if (type == RECURSE_WAY_NODE)
    return "way-node";
  else if (type == RECURSE_WAY_RELATION)
    return "way-relation";
  else if (type == RECURSE_NODE_RELATION)
    return "node-relation";
  else if (type == RECURSE_NODE_WAY)
    return "node-way";
  else if (type == RECURSE_DOWN)
    return "down";
  else if (type == RECURSE_DOWN_REL)
    return "down-rel";
  else if (type == RECURSE_UP)
    return "up";
  else if (type == RECURSE_UP_REL)
    return "up-rel";

  return "void";
}


std::string Recurse_Statement::to_ql_representation(int type)
{
  if (type == RECURSE_RELATION_RELATION)
    return "r";
  else if (type == RECURSE_RELATION_BACKWARDS)
    return "br";
  else if (type == RECURSE_RELATION_WAY)
    return "r";
  else if (type == RECURSE_RELATION_NODE)
    return "r";
  else if (type == RECURSE_WAY_NODE)
    return "w";
  else if (type == RECURSE_WAY_RELATION)
    return "bw";
  else if (type == RECURSE_NODE_RELATION)
    return "bn";
  else if (type == RECURSE_NODE_WAY)
    return "bn";
  else if (type == RECURSE_DOWN)
    return ">";
  else if (type == RECURSE_DOWN_REL)
    return ">>";
  else if (type == RECURSE_UP)
    return "<";
  else if (type == RECURSE_UP_REL)
    return "<<";

  return "";
}


void Recurse_Statement::execute(Resource_Manager& rman)
{
  Set into;

  const Set* input_set = rman.get_set(input);
  if (!input_set)
  {
    transfer_output(rman, into);
    return;
  }

  if (restrict_to_role)
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), role);
    if (role_id == std::numeric_limits< uint32 >::max())
    {
      transfer_output(rman, into);
      return;
    }

    if (type == RECURSE_RELATION_RELATION)
    {
      if (rman.get_desired_timestamp() == NOW)
        into.relations = relation_relation_members(*this, rman, input_set->relations,
                                                 0, 0, false, &role_id);
      else
      {
        std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
            std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > all_relations
            = relation_relation_members(*this, rman, input_set->relations, input_set->attic_relations,
                                        0, 0, false, &role_id);
        into.relations.swap(all_relations.first);
        into.attic_relations.swap(all_relations.second);
      }
    }
    else if (type == RECURSE_RELATION_WAY)
    {
      if (rman.get_desired_timestamp() == NOW)
        into.ways = relation_way_members(this, rman, input_set->relations,
                                         0, 0, false, &role_id);
      else
      {
        std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
            std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
            = relation_way_members(this, rman, input_set->relations, input_set->attic_relations,
                                   0, 0, false, &role_id);
        into.ways.swap(all_ways.first);
        into.attic_ways.swap(all_ways.second);
      }
    }
    else if (type == RECURSE_RELATION_NODE)
    {
      if (rman.get_desired_timestamp() == NOW)
        into.nodes = relation_node_members(this, rman, input_set->relations,
                                           0, 0, false, &role_id);
      else
      {
        std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
            std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
            = relation_node_members(this, rman, input_set->relations, input_set->attic_relations,
                                    0, 0, false, &role_id);
        into.nodes.swap(all_nodes.first);
        into.attic_nodes.swap(all_nodes.second);
      }
    }
    else if (type == RECURSE_RELATION_BACKWARDS)
    {
      if (rman.get_desired_timestamp() == NOW)
        collect_relations(*this, rman, input_set->relations, into.relations, role_id);
      else
        collect_relations(*this, rman, input_set->relations, input_set->attic_relations,
                          into.relations, into.attic_relations, role_id);
    }
    else if (type == RECURSE_NODE_RELATION)
    {
      if (rman.get_desired_timestamp() == NOW)
        collect_relations(*this, rman, input_set->nodes, Relation_Entry::NODE, into.relations, role_id);
      else
        collect_relations(*this, rman, input_set->nodes, input_set->attic_nodes, Relation_Entry::NODE,
                          into.relations, into.attic_relations, role_id);
    }
    else if (type == RECURSE_WAY_RELATION)
    {
      if (rman.get_desired_timestamp() == NOW)
        collect_relations(*this, rman, input_set->ways, Relation_Entry::WAY, into.relations, role_id);
      else
        collect_relations(*this, rman, input_set->ways, input_set->attic_ways, Relation_Entry::WAY,
                          into.relations, into.attic_relations, role_id);
    }
  }
  else if (type == RECURSE_RELATION_RELATION)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.relations = relation_relation_members(*this, rman, input_set->relations);
    else
    {
      std::pair< std::map< Uint31_Index, std::vector< Relation_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > > all_relations
          = relation_relation_members(*this, rman, input_set->relations, input_set->attic_relations);
      into.relations.swap(all_relations.first);
      into.attic_relations.swap(all_relations.second);
    }
  }
  else if (type == RECURSE_RELATION_BACKWARDS)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_relations(*this, rman, input_set->relations, into.relations);
    else
      collect_relations(*this, rman, input_set->relations, input_set->attic_relations,
                        into.relations, into.attic_relations);
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.ways = relation_way_members(this, rman, input_set->relations);
    else
    {
      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
          = relation_way_members(this, rman, input_set->relations, input_set->attic_relations);
      into.ways.swap(all_ways.first);
      into.attic_ways.swap(all_ways.second);
    }
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.nodes = relation_node_members(this, rman, input_set->relations);
    else
    {
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
          = relation_node_members(this, rman, input_set->relations, input_set->attic_relations);
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
    }
  }
  else if (type == RECURSE_WAY_NODE)
  {
    if (rman.get_desired_timestamp() == NOW)
      into.nodes = way_members(this, rman, input_set->ways);
    else
    {
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
         std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
         = way_members(this, rman, input_set->ways, input_set->attic_ways);
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);
    }
  }
  else if (type == RECURSE_DOWN)
    add_nw_member_objects(rman, this, *input_set, into);
  else if (type == RECURSE_DOWN_REL)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      relations_loop(*this, rman, input_set->relations, into.relations);
      std::map< Uint32_Index, std::vector< Node_Skeleton > > rel_nodes
          = relation_node_members(this, rman, into.relations);
      into.ways = relation_way_members(this, rman, into.relations);
      std::map< Uint31_Index, std::vector< Way_Skeleton > > source_ways = input_set->ways;
      sort_second(source_ways);
      sort_second(into.ways);
      indexed_set_union(source_ways, into.ways);
      into.nodes = way_members(this, rman, source_ways);
      sort_second(into.nodes);
      sort_second(rel_nodes);
      indexed_set_union(into.nodes, rel_nodes);
    }
    else
    {
      relations_loop(*this, rman, input_set->relations, input_set->attic_relations,
                     into.relations, into.attic_relations);
      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > all_nodes
          = relation_node_members(this, rman, into.relations, into.attic_relations);
      into.nodes.swap(all_nodes.first);
      into.attic_nodes.swap(all_nodes.second);

      std::pair< std::map< Uint31_Index, std::vector< Way_Skeleton > >,
          std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > > all_ways
          = relation_way_members(this, rman, into.relations, into.attic_relations);
      into.ways = all_ways.first;
      into.attic_ways = all_ways.second;

      std::map< Uint31_Index, std::vector< Way_Skeleton > > source_ways = input_set->ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > source_attic_ways = input_set->attic_ways;
      sort_second(all_ways.first);
      sort_second(source_ways);
      indexed_set_union(all_ways.first, source_ways);
      sort_second(all_ways.second);
      sort_second(source_attic_ways);
      indexed_set_union(all_ways.second, source_attic_ways);

      std::pair< std::map< Uint32_Index, std::vector< Node_Skeleton > >,
          std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > > more_nodes
          = way_members(this, rman, all_ways.first, all_ways.second);
      sort_second(into.nodes);
      sort_second(more_nodes.first);
      indexed_set_union(into.nodes, more_nodes.first);
      sort_second(into.attic_nodes);
      sort_second(more_nodes.second);
      indexed_set_union(into.attic_nodes, more_nodes.second);
      keep_matching_skeletons(into.nodes, into.attic_nodes, rman.get_desired_timestamp());
    }
  }
  else if (type == RECURSE_NODE_WAY)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_ways(*this, rman, input_set->nodes, into.ways);
    else
      collect_ways(*this, rman,
                   input_set->nodes, input_set->attic_nodes, into.ways, into.attic_ways);
  }
  else if (type == RECURSE_NODE_RELATION)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_relations(*this, rman, input_set->nodes, Relation_Entry::NODE, into.relations);
    else
      collect_relations(*this, rman, input_set->nodes, input_set->attic_nodes, Relation_Entry::NODE,
                        into.relations, into.attic_relations);
  }
  else if (type == RECURSE_WAY_RELATION)
  {
    if (rman.get_desired_timestamp() == NOW)
      collect_relations(*this, rman, input_set->ways, Relation_Entry::WAY, into.relations);
    else
      collect_relations(*this, rman, input_set->ways, input_set->attic_ways, Relation_Entry::WAY,
                        into.relations, into.attic_relations);
  }
  else if (type == RECURSE_UP)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input_set->ways;
      collect_ways(*this, rman, input_set->nodes, into.ways);

      sort_second(rel_ways);
      sort_second(into.ways);
      indexed_set_union(rel_ways, into.ways);
      collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, into.relations);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
      collect_relations(*this, rman, input_set->nodes, Relation_Entry::NODE, node_rels);
      sort_second(into.relations);
      sort_second(node_rels);
      indexed_set_union(into.relations, node_rels);
    }
    else
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input_set->ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_rel_ways = input_set->attic_ways;
      collect_ways(*this, rman, input_set->nodes, input_set->attic_nodes, into.ways, into.attic_ways);

      sort_second(rel_ways);
      sort_second(into.ways);
      indexed_set_union(rel_ways, into.ways);
      sort_second(attic_rel_ways);
      sort_second(into.attic_ways);
      indexed_set_union(attic_rel_ways, into.attic_ways);
      collect_relations(*this, rman, rel_ways, attic_rel_ways,
                        Relation_Entry::WAY, into.relations, into.attic_relations);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_node_rels;
      collect_relations(*this, rman, input_set->nodes, input_set->attic_nodes,
                        Relation_Entry::NODE, node_rels, attic_node_rels);
      sort_second(into.relations);
      sort_second(node_rels);
      indexed_set_union(into.relations, node_rels);
      sort_second(into.attic_relations);
      sort_second(attic_node_rels);
      indexed_set_union(into.attic_relations, attic_node_rels);
    }
  }
  else if (type == RECURSE_UP_REL)
  {
    if (rman.get_desired_timestamp() == NOW)
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input_set->ways;
      collect_ways(*this, rman, input_set->nodes, into.ways);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels = input_set->relations;
      std::map< Uint31_Index, std::vector< Relation_Skeleton > > way_rels;
      sort_second(rel_ways);
      sort_second(into.ways);
      indexed_set_union(rel_ways, into.ways);
      collect_relations(*this, rman, rel_ways, Relation_Entry::WAY, way_rels);
      sort_second(rel_rels);
      sort_second(way_rels);
      indexed_set_union(rel_rels, way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
      collect_relations(*this, rman, input_set->nodes, Relation_Entry::NODE, node_rels);
      sort_second(rel_rels);
      sort_second(node_rels);
      indexed_set_union(rel_rels, node_rels);

      relations_up_loop(*this, rman, rel_rels, into.relations);
    }
    else
    {
      std::map< Uint31_Index, std::vector< Way_Skeleton > > rel_ways = input_set->ways;
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_rel_ways = input_set->attic_ways;
      collect_ways(*this, rman,
                   input_set->nodes, input_set->attic_nodes, into.ways, into.attic_ways);
      sort_second(rel_ways);
      sort_second(into.ways);
      indexed_set_union(rel_ways, into.ways);
      sort_second(attic_rel_ways);
      sort_second(into.attic_ways);
      indexed_set_union(attic_rel_ways, into.attic_ways);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > way_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_way_rels;
      collect_relations(*this, rman, rel_ways, attic_rel_ways,
                        Relation_Entry::WAY, way_rels, attic_way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > rel_rels = input_set->relations;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_rel_rels = input_set->attic_relations;
      sort_second(rel_rels);
      sort_second(way_rels);
      indexed_set_union(rel_rels, way_rels);
      sort_second(attic_rel_rels);
      sort_second(attic_way_rels);
      indexed_set_union(attic_rel_rels, attic_way_rels);

      std::map< Uint31_Index, std::vector< Relation_Skeleton > > node_rels;
      std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > > attic_node_rels;
      collect_relations(*this, rman, input_set->nodes, input_set->attic_nodes,
                        Relation_Entry::NODE, node_rels, attic_node_rels);
      sort_second(rel_rels);
      sort_second(node_rels);
      indexed_set_union(rel_rels, node_rels);
      sort_second(attic_rel_rels);
      sort_second(attic_node_rels);
      indexed_set_union(attic_rel_rels, attic_node_rels);

      relations_up_loop(*this, rman, rel_rels, attic_rel_rels, into.relations, into.attic_relations);
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
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


Query_Constraint* Recurse_Statement::get_query_constraint()
{
  constraints.push_back(new Recurse_Constraint(*this));
  return constraints.back();
}
