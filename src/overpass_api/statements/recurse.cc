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
const unsigned int RECURSE_RELATION_NWR = 5;
const unsigned int RECURSE_RELATION_NW = 6;
const unsigned int RECURSE_RELATION_WR = 7;
const unsigned int RECURSE_RELATION_NR = 8;
const unsigned int RECURSE_WAY_NODE = 9;
const unsigned int RECURSE_WAY_LINK = 10;
const unsigned int RECURSE_WAY_COUNT = 11;
const unsigned int RECURSE_WAY_RELATION = 12;
const unsigned int RECURSE_NODE_RELATION = 13;
const unsigned int RECURSE_NODE_WAY = 14;
const unsigned int RECURSE_NODE_WR = 15;
const unsigned int RECURSE_DOWN = 16;
const unsigned int RECURSE_DOWN_REL = 17;
const unsigned int RECURSE_UP = 18;
const unsigned int RECURSE_UP_REL = 19;


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
  std::vector< std::string > roles;
  std::string lower, upper;
  bool role_found = false;

  while (tree_it->token == "," && tree_it->rhs && tree_it->lhs)
  {
    roles.push_back(tree_it.rhs()->token);
    tree_it = tree_it.lhs();
  }

  if (tree_it->token == ":" && tree_it->rhs)
  {
    role_found = true;
    if (tree_it.rhs()->token == "-")
    {
      if (tree_it.rhs()->lhs)
        lower = tree_it.rhs().lhs()->token;
      if (tree_it.rhs()->rhs)
        upper = tree_it.rhs().rhs()->token;
    }
    else if (!tree_it.rhs()->lhs && !tree_it.rhs()->rhs)
      roles.push_back(decode_json(tree_it.rhs()->token, error_output));
    else
      error_output->add_parse_error("Simple role token expected, structured syntax tree found.", line_nr);
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
    else if (result_type == "nwr")
      attributes["type"] = "relation-nwr";
    else if (result_type == "nw")
      attributes["type"] = "relation-nw";
    else if (result_type == "wr")
      attributes["type"] = "relation-wr";
    else if (result_type == "nr")
      attributes["type"] = "relation-nr";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'r' produces nodes, ways, or relations.", line_nr);
  }
  else if (type == "w")
  {
    if (result_type == "node")
      attributes["type"] = "way-node";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'w' produces nodes.", line_nr);
  }
  else if (type == "way_link")
  {
    if (result_type == "node")
      attributes["type"] = "way-link";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'way_link' produces nodes.", line_nr);
  }
  else if (type == "way_cnt")
  {
    if (result_type == "node")
      attributes["type"] = "way-count";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'way_cnt' produces nodes.", line_nr);
  }
  else if (type == "br")
  {
    if (result_type == "relation")
      attributes["type"] = "relation-backwards";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'br' produces relations.", line_nr);
  }
  else if (type == "bw")
  {
    if (result_type == "relation")
      attributes["type"] = "way-relation";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'bw' produces relations.", line_nr);
  }
  else if (type == "bn")
  {
    if (result_type == "way")
      attributes["type"] = "node-way";
    else if (result_type == "relation")
      attributes["type"] = "node-relation";
    else if (result_type == "wr")
      attributes["type"] = "node-wr";
    else if (error_output)
      error_output->add_parse_error("A recursion of type 'bn' produces ways or relations.", line_nr);
  }
  else
    return 0;

  if (role_found)
  {
    if (type == "r" || result_type == "relation")
    {
      attributes["role"] = roles.back();
      attributes["role-restricted"] = "yes";
    }
    else if (type == "w" || result_type == "way")
    {
      attributes["pos"] = roles[0];
      for (uint i = 1; i < roles.size(); ++i)
        attributes["pos"] += "," + roles[i];
    }
    else if (type == "way_cnt" || type == "way_link")
    {
      if (role_found)
      {
        if (roles.empty())
        {
          attributes["lower"] = lower;
          attributes["upper"] = upper;
        }
        else
        {
          attributes["lower"] = roles.back();
          attributes["upper"] = roles.back();
        }
      }
      else if (error_output)
        error_output->add_parse_error("A recursion of type 'way_cnt' or 'way_link' must have a parameter.", line_nr);
    }
    else if (error_output)
      error_output->add_parse_error("A recursion of type '" + type + "' cannot have restrictions.", line_nr);
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


template< typename Index, typename Object >
std::set< Uint31_Index > extract_parent_indices_set(
    const std::map< Index, std::vector< Object > >& cur_elems,
    const std::map< Index, std::vector< Attic< Object > > >& attic_elems)
{
  std::set< Uint31_Index > req = extract_parent_indices(cur_elems);
  if (!attic_elems.empty())
  {
    std::set< Uint31_Index > attic_req = extract_parent_indices(attic_elems);
    for (std::set< Uint31_Index >::const_iterator it = attic_req.begin(); it != attic_req.end(); ++it)
      req.insert(*it);
  }
  
  return req;
}


template< typename Index, typename Object >
Ranges< Uint31_Index > extract_parent_indices(
    const std::map< Index, std::vector< Object > >& cur_elems,
    const std::map< Index, std::vector< Attic< Object > > >& attic_elems)
{
  Ranges< Uint31_Index > result;

  std::set< Uint31_Index > req = extract_parent_indices(cur_elems);
  for (auto i : req)
    result.push_back(i, inc(i));

  if (!attic_elems.empty())
  {
    std::set< Uint31_Index > attic_req = extract_parent_indices(attic_elems);
    for (auto i : attic_req)
      result.push_back(i, inc(i));
  }

  result.sort();
  return result;
}


template< class TSourceIndex, class TSourceObject >
Timeless< Uint31_Index, Relation_Skeleton > collect_relations
    (Request_Context& context,
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources,
     const std::map< TSourceIndex, std::vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids)
{
  std::vector< Relation_Entry::Ref_Type > parent_ids = extract_ids(sources, attic_sources);
  std::set< Uint31_Index > req = extract_parent_indices_set(sources, attic_sources);
  context.get_health_guard().check(0, parent_ids.size() * sizeof(Relation_Entry::Ref_Type) + req.size()*64);

  if (!invert_ids)
    return collect_items_discrete< Uint31_Index, Relation_Skeleton >(context, req,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(parent_ids, source_type)));
  else if (ids.empty())
    return collect_items_discrete< Uint31_Index, Relation_Skeleton >(context, req,
        Get_Parent_Rels_Predicate(parent_ids, source_type));

  return collect_items_discrete< Uint31_Index, Relation_Skeleton >(context, req,
      And_Predicate< Relation_Skeleton,
          Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
          Get_Parent_Rels_Predicate >
          (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
            (Id_Predicate< Relation_Skeleton >(ids)),
          Get_Parent_Rels_Predicate(parent_ids, source_type)));
}


template< class TSourceIndex, class TSourceObject >
Timeless< Uint31_Index, Relation_Skeleton > collect_relations
    (Request_Context& context,
     const std::map< TSourceIndex, std::vector< TSourceObject > >& sources,
     const std::map< TSourceIndex, std::vector< Attic< TSourceObject > > >& attic_sources,
     uint32 source_type,
     const std::vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > parent_ids = extract_ids(sources, attic_sources);
  std::set< Uint31_Index > req = extract_parent_indices_set(sources, attic_sources);
  context.get_health_guard().check(0, parent_ids.size() * sizeof(Relation_Entry::Ref_Type) + req.size()*64);

  if (!invert_ids)
    return collect_items_discrete< Uint31_Index, Relation_Skeleton >(context, req,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(parent_ids, source_type, role_id)));
  else if (ids.empty())
    return collect_items_discrete< Uint31_Index, Relation_Skeleton >(context, req,
        Get_Parent_Rels_Role_Predicate(parent_ids, source_type, role_id));

  return collect_items_discrete< Uint31_Index, Relation_Skeleton >(context, req,
      And_Predicate< Relation_Skeleton,
          Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
          Get_Parent_Rels_Role_Predicate >
          (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
            (Id_Predicate< Relation_Skeleton >(ids)),
          Get_Parent_Rels_Role_Predicate(parent_ids, source_type, role_id)));
}


Timeless< Uint31_Index, Relation_Skeleton > collect_relations(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_sources,
    const std::vector< Relation::Id_Type >& ids, bool invert_ids)
{
  std::vector< Relation_Entry::Ref_Type > parent_ids = extract_ids(sources, attic_sources);
  context.get_health_guard().check(0, parent_ids.size() * sizeof(Relation_Entry::Ref_Type));

  if (!invert_ids)
    return collect_items_flat< Uint31_Index, Relation_Skeleton >(context,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Predicate(parent_ids, Relation_Entry::RELATION)));
  else if (ids.empty())
    return collect_items_flat< Uint31_Index, Relation_Skeleton >(context,
        Get_Parent_Rels_Predicate(parent_ids, Relation_Entry::RELATION));

  return collect_items_flat< Uint31_Index, Relation_Skeleton >(context,
      And_Predicate< Relation_Skeleton,
          Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
          Get_Parent_Rels_Predicate >
          (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
            (Id_Predicate< Relation_Skeleton >(ids)),
          Get_Parent_Rels_Predicate(parent_ids, Relation_Entry::RELATION)));
}


Timeless< Uint31_Index, Relation_Skeleton > collect_relations(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& sources,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_sources,
    const std::vector< Relation::Id_Type >& ids, bool invert_ids, uint32 role_id)
{
  std::vector< Relation_Entry::Ref_Type > parent_ids = extract_ids(sources, attic_sources);
  context.get_health_guard().check(0, parent_ids.size() * sizeof(Relation_Entry::Ref_Type));

  if (!invert_ids)
    return collect_items_flat< Uint31_Index, Relation_Skeleton >(context,
        And_Predicate< Relation_Skeleton,
            Id_Predicate< Relation_Skeleton >, Get_Parent_Rels_Role_Predicate >
            (Id_Predicate< Relation_Skeleton >(ids),
            Get_Parent_Rels_Role_Predicate(parent_ids, Relation_Entry::RELATION, role_id)));
  else if (ids.empty())
    return collect_items_flat< Uint31_Index, Relation_Skeleton >(context,
        Get_Parent_Rels_Role_Predicate(parent_ids, Relation_Entry::RELATION, role_id));

  return collect_items_flat< Uint31_Index, Relation_Skeleton >(context,
      And_Predicate< Relation_Skeleton,
          Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >,
          Get_Parent_Rels_Role_Predicate >
          (Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >
            (Id_Predicate< Relation_Skeleton >(ids)),
          Get_Parent_Rels_Role_Predicate(parent_ids, Relation_Entry::RELATION, role_id)));
}


Timeless< Uint31_Index, Relation_Skeleton > relations_loop(
    Request_Context& context,
    const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& source,
    const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& attic_source)
{
  auto working_set = Timeless< Uint31_Index, Relation_Skeleton >{ source, attic_source };
  working_set.sort();
  uint old_rel_count = 0;
  uint new_rel_count = count(working_set.get_current()) + count(working_set.get_attic());
  while (old_rel_count < new_rel_count)
  {
    old_rel_count = new_rel_count;
    auto new_results = relation_relation_members(
        context, working_set.get_current(), working_set.get_attic(), Ranges< Uint31_Index >::global(), {}, true);
    working_set.set_union(new_results.sort());
    working_set.keep_matching_skeletons(context.get_desired_timestamp());
    new_rel_count = count(working_set.get_current()) + count(working_set.get_attic());
  }
  return working_set;
}


Timeless< Uint31_Index, Relation_Skeleton > relations_up_loop(
    Request_Context& context,
    Timeless< Uint31_Index, Relation_Skeleton >&& working_set)
{
  working_set.sort();
  uint old_rel_count = 0;
  uint new_rel_count = count(working_set.get_current()) + count(working_set.get_attic());
  while (old_rel_count < new_rel_count)
  {
    old_rel_count = new_rel_count;
    auto new_results = collect_relations(context, working_set.get_current(), working_set.get_attic(), {}, true);
    working_set.set_union(new_results.sort());
    working_set.keep_matching_skeletons(context.get_desired_timestamp());
    new_rel_count = count(working_set.get_current()) + count(working_set.get_attic());
  }
  return std::move(working_set);
}


Timeless< Uint32_Index, Node_Skeleton > recurse_down_nodes(
    Request_Context& context,
    const Ranges< Uint32_Index >& ranges, const std::vector< Node::Id_Type >& ids, bool invert_ids,
    const Timeless< Uint31_Index, Relation_Skeleton >& rels)
{
  Timeless< Uint32_Index, Node_Skeleton > rel_nodes =
      relation_node_members(context, rels.get_current(), rels.get_attic(), ranges, ids, invert_ids);

  auto rel_ways =
      relation_way_members(context, rels.get_current(), rels.get_attic(), Ranges< Uint31_Index >::global(), {}, true);

  Timeless< Uint32_Index, Node_Skeleton > way_nodes =
      way_members(context, rel_ways.get_current(), rel_ways.get_attic(), 0, ranges, ids, invert_ids);
  way_nodes.sort().set_union(rel_nodes.sort());
  way_nodes.keep_matching_skeletons(context.get_desired_timestamp());

  return way_nodes;
}


//-----------------------------------------------------------------------------

class Recurse_Constraint : public Query_Constraint
{
public:
  Recurse_Constraint(Recurse_Statement& stmt_) : stmt(&stmt_) {}

  Ranges< Uint32_Index > get_node_ranges(Resource_Manager& rman) override;
  Ranges< Uint31_Index > get_way_ranges(Resource_Manager& rman) override;
  Ranges< Uint31_Index > get_relation_ranges(Resource_Manager& rman) override;

  Query_Filter_Strategy delivers_data(Resource_Manager& rman) override { return prefer_ranges; }

  virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                        const Ranges< Uint32_Index >& ranges,
                        const std::vector< Node::Id_Type >& ids,
                        bool invert_ids) override;
  virtual bool get_data(const Statement& query, Resource_Manager& rman, Set& into,
                        const Ranges< Uint31_Index >& ranges,
                        int type,
                        const std::vector< Uint32_Index >& ids,
                        bool invert_ids) override;
  void filter(Resource_Manager& rman, Set& into) override;
  void filter(const Statement& query, Resource_Manager& rman, Set& into) override;
  virtual ~Recurse_Constraint() {}

private:
  Recurse_Statement* stmt;
};


Ranges< Uint32_Index > Recurse_Constraint::get_node_ranges(Resource_Manager& rman)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return Ranges< Uint32_Index >();

  if (stmt->get_type() == RECURSE_RELATION_NODE || stmt->get_type() == RECURSE_RELATION_NWR
      || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_NR)
    return relation_node_member_indices(input->relations, input->attic_relations);
  else if (stmt->get_type() == RECURSE_WAY_NODE || stmt->get_type() == RECURSE_WAY_LINK
      || stmt->get_type() == RECURSE_WAY_COUNT)
    return way_nd_indices(input->ways, input->attic_ways);

  return Ranges< Uint32_Index >::global();
}


Ranges< Uint31_Index > Recurse_Constraint::get_way_ranges(Resource_Manager& rman)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return Ranges< Uint31_Index >();

  if (stmt->get_type() == RECURSE_RELATION_WAY || stmt->get_type() == RECURSE_RELATION_NWR
      || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_WR)
    return relation_way_member_indices(input->relations, input->attic_relations);
  else if (stmt->get_type() == RECURSE_NODE_WAY)
    return extract_parent_indices(input->nodes, input->attic_nodes);

  return Ranges< Uint31_Index >::global();
}


Ranges< Uint31_Index > Recurse_Constraint::get_relation_ranges(Resource_Manager& rman)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return Ranges< Uint31_Index >();

  if (stmt->get_type() == RECURSE_NODE_RELATION)
    return extract_parent_indices(input->nodes, input->attic_nodes);
  else if (stmt->get_type() == RECURSE_WAY_RELATION)
    return extract_parent_indices(input->ways, input->attic_ways);

  return Ranges< Uint31_Index >::global();
}


bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const Ranges< Uint32_Index >& ranges,
     const std::vector< Node::Id_Type >& ids,
     bool invert_ids)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return true;

  Request_Context context(&query, rman);
  if (stmt->get_role())
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
    if (role_id != std::numeric_limits< uint32 >::max()
        && (stmt->get_type() == RECURSE_RELATION_NODE || stmt->get_type() == RECURSE_RELATION_NWR
          || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_NR))
      relation_node_members(
          context, input->relations, input->attic_relations, ranges, ids, invert_ids, &role_id)
          .swap(into.nodes, into.attic_nodes);
  }
  else if (stmt->get_type() == RECURSE_RELATION_NODE || stmt->get_type() == RECURSE_RELATION_NWR
      || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_NR)
    relation_node_members(
        context, input->relations, input->attic_relations, ranges, ids, invert_ids)
        .swap(into.nodes, into.attic_nodes);
  else if (stmt->get_type() == RECURSE_WAY_NODE)
    way_members(
        context, input->ways, input->attic_ways, stmt->get_pos(), ranges, ids, invert_ids)
        .swap(into.nodes, into.attic_nodes);
  else if (stmt->get_type() == RECURSE_WAY_COUNT)
    way_cnt_members(
        context, input->ways, input->attic_ways, stmt->get_lower(), stmt->get_upper(),
        ranges, ids, invert_ids)
        .swap(into.nodes, into.attic_nodes);
  else if (stmt->get_type() == RECURSE_WAY_LINK)
    way_link_members(
        context, input->ways, input->attic_ways, stmt->get_lower(), stmt->get_upper(),
        ranges, ids, invert_ids)
        .swap(into.nodes, into.attic_nodes);
  else if (stmt->get_type() == RECURSE_DOWN)
    recurse_down_nodes(context, ranges, ids, invert_ids, {input->relations, input->attic_relations})
        .swap(into.nodes, into.attic_nodes);
  else if (stmt->get_type() == RECURSE_DOWN_REL)
    recurse_down_nodes(
        context, ranges, ids, invert_ids,
        relations_loop(context, input->relations, input->attic_relations))
        .swap(into.nodes, into.attic_nodes);

  return true;
}


bool Recurse_Constraint::get_data
    (const Statement& query, Resource_Manager& rman, Set& into,
     const Ranges< Uint31_Index >& ranges,
     int type,
     const std::vector< Uint32_Index >& ids,
     bool invert_ids)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return true;

  Request_Context context(&query, rman);
  if (stmt->get_role())
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), *stmt->get_role());
    if (role_id == std::numeric_limits< uint32 >::max())
      return true;

    if (stmt->get_type() == RECURSE_RELATION_WAY
        || (stmt->get_type() == RECURSE_RELATION_NWR && type == QUERY_WAY)
        || (stmt->get_type() == RECURSE_RELATION_NW && type == QUERY_WAY)
        || (stmt->get_type() == RECURSE_RELATION_WR && type == QUERY_WAY))
      collect_ways(context, input->relations, input->attic_relations, ranges, ids, invert_ids, &role_id)
          .swap(into.ways, into.attic_ways);
    else if (stmt->get_type() == RECURSE_RELATION_RELATION
        || (stmt->get_type() == RECURSE_RELATION_NWR && type == QUERY_RELATION)
        || (stmt->get_type() == RECURSE_RELATION_WR && type == QUERY_RELATION)
        || (stmt->get_type() == RECURSE_RELATION_NR && type == QUERY_RELATION))
      relation_relation_members(
          context, input->relations, input->attic_relations, ranges, ids, invert_ids, &role_id)
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
      collect_relations(
          context, input->nodes, input->attic_nodes, Relation_Entry::NODE, ids, invert_ids, role_id)
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      collect_relations(context, input->ways, input->attic_ways, Relation_Entry::WAY, ids, invert_ids, role_id)
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      collect_relations(context, input->relations, input->attic_relations, ids, invert_ids, role_id)
          .swap(into.relations, into.attic_relations);
    else
      return false;
  }
  else if (stmt->get_type() == RECURSE_RELATION_WAY
          || (stmt->get_type() == RECURSE_RELATION_NWR && type == QUERY_WAY)
          || (stmt->get_type() == RECURSE_RELATION_NW && type == QUERY_WAY)
          || (stmt->get_type() == RECURSE_RELATION_WR && type == QUERY_WAY))
    collect_ways(context, input->relations, input->attic_relations, ranges, ids, invert_ids)
        .swap(into.ways, into.attic_ways);
  else if (stmt->get_type() == RECURSE_RELATION_RELATION
        || (stmt->get_type() == RECURSE_RELATION_NWR && type == QUERY_RELATION)
        || (stmt->get_type() == RECURSE_RELATION_NR && type == QUERY_RELATION)
        || (stmt->get_type() == RECURSE_RELATION_WR && type == QUERY_RELATION))
    relation_relation_members(
        context, input->relations, input->attic_relations, ranges, ids, invert_ids, 0)
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_DOWN)
  {
    if (type != QUERY_WAY)
      return true;
    collect_ways(context, input->relations, input->attic_relations, ranges, ids, invert_ids)
        .swap(into.ways, into.attic_ways);
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    Timeless< Uint31_Index, Relation_Skeleton > rel_rels =
        relations_loop(context, input->relations, input->attic_relations);

    if (type == QUERY_WAY)
      collect_ways(context, rel_rels.get_current(), rel_rels.get_attic(), ranges, ids, invert_ids)
          .swap(into.ways, into.attic_ways);
    else
    {
      if (!ids.empty())
      {
        if (!invert_ids)
          rel_rels.filter_items(Id_Predicate< Relation_Skeleton >(ids));
        else
          rel_rels.filter_items(
              Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >(
                  Id_Predicate< Relation_Skeleton >(ids)));
      }
      rel_rels.keep_matching_skeletons(context.get_desired_timestamp());
      rel_rels.swap(into.relations, into.attic_relations);
    }
  }
  else if (stmt->get_type() == RECURSE_NODE_WAY)
    collect_ways(context, input->nodes, input->attic_nodes, stmt->get_pos(), ids, invert_ids)
        .swap(into.ways, into.attic_ways);
  else if (stmt->get_type() == RECURSE_NODE_RELATION)
    collect_relations(context, input->nodes, input->attic_nodes, Relation_Entry::NODE, ids, invert_ids)
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_WAY_RELATION)
    collect_relations(context, input->ways, input->attic_ways, Relation_Entry::WAY, ids, invert_ids)
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    collect_relations(context, input->relations, input->attic_relations, ids, invert_ids)
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_UP)
  {
    if (type == QUERY_WAY)
      collect_ways(context, input->nodes, input->attic_nodes, 0, ids, invert_ids)
          .swap(into.ways, into.attic_ways);
    else
    {
      auto rel_ways = Timeless< Uint31_Index, Way_Skeleton >{ input->ways, input->attic_ways };
      rel_ways.sort();
      rel_ways.set_union(collect_ways(context, input->nodes, input->attic_nodes, 0, {}, true).sort());

      auto result = collect_relations(
          context, rel_ways.get_current(), rel_ways.get_attic(), Relation_Entry::WAY, ids, invert_ids);
      result.sort();
      result.set_union(
          collect_relations(context, input->nodes, input->attic_nodes, Relation_Entry::NODE, ids, invert_ids)
          .sort());
      result.swap(into.relations, into.attic_relations);
    }
  }
  else if (stmt->get_type() == RECURSE_UP_REL)
  {
    if (type == QUERY_WAY)
      collect_ways(context, input->nodes, input->attic_nodes, 0, ids, invert_ids)
          .swap(into.ways, into.attic_ways);
    else
    {
      auto rel_ways = Timeless< Uint31_Index, Way_Skeleton >{ input->ways, input->attic_ways };
      rel_ways.sort();
      rel_ways.set_union(collect_ways(context, input->nodes, input->attic_nodes, 0, {}, true).sort());

      auto rel_rels = Timeless< Uint31_Index, Relation_Skeleton >{ input->relations, input->attic_relations };
      rel_rels.sort();
      rel_rels.set_union(collect_relations(
          context, rel_ways.get_current(), rel_ways.get_attic(), Relation_Entry::WAY, {}, true).sort());
      rel_rels.set_union(collect_relations(
          context, input->nodes, input->attic_nodes, Relation_Entry::NODE, {}, true).sort());

      Timeless< Uint31_Index, Relation_Skeleton > result = relations_up_loop(context, std::move(rel_rels));

      if (!ids.empty())
      {
        if (!invert_ids)
          result.filter_items(Id_Predicate< Relation_Skeleton >(ids));
        else
          result.filter_items(
              Not_Predicate< Relation_Skeleton, Id_Predicate< Relation_Skeleton > >(
                  Id_Predicate< Relation_Skeleton >(ids)));
      }
      result.swap(into.relations, into.attic_relations);
    }
  }
  else
    return false;

  return true;
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

    if (stmt->get_type() == RECURSE_RELATION_NODE || stmt->get_type() == RECURSE_RELATION_NWR
        || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_NR)
      Timeless< Uint32_Index, Node_Skeleton >{}.swap(into.nodes, into.attic_nodes)
          .filter_items(Id_Predicate< Node_Skeleton >(
              relation_node_member_ids(input->relations, input->attic_relations, &role_id)))
          .swap(into.nodes, into.attic_nodes);
    else
      Timeless< Uint32_Index, Node_Skeleton >{}.swap(into.nodes, into.attic_nodes);

    if (stmt->get_type() == RECURSE_RELATION_WAY || stmt->get_type() == RECURSE_RELATION_NWR
        || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_WR)
      Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways)
          .filter_items(Id_Predicate< Way_Skeleton >(
                relation_way_member_ids(input->relations, input->attic_relations, &role_id)))
          .swap(into.ways, into.attic_ways);
    else
      Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways);

    if (stmt->get_type() == RECURSE_RELATION_RELATION || stmt->get_type() == RECURSE_RELATION_NWR
        || stmt->get_type() == RECURSE_RELATION_WR || stmt->get_type() == RECURSE_RELATION_NR)
      Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
          .filter_items(Id_Predicate< Relation_Skeleton >(
              relation_relation_member_ids(input->relations, input->attic_relations, &role_id)))
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_NODE_RELATION)
      Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
          .filter_items(Get_Parent_Rels_Role_Predicate(
              extract_ids(input->nodes, input->attic_nodes), Relation_Entry::NODE, role_id))
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_WAY_RELATION)
      Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
          .filter_items(Get_Parent_Rels_Role_Predicate(
              extract_ids(input->ways, input->attic_ways), Relation_Entry::WAY, role_id))
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
      Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
          .filter_items(Get_Parent_Rels_Role_Predicate(
              extract_ids(input->relations), Relation_Entry::RELATION, role_id))
          .swap(into.relations, into.attic_relations);
    else if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
      return;
    else
      Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations);

    return;
  }

  {
    std::vector< Node::Id_Type > ids;
    if (stmt->get_type() == RECURSE_WAY_NODE)
    {
      ids = way_nd_ids(input->ways, input->attic_ways, stmt->get_pos());
      rman.health_check(*stmt);
    }
    else if (stmt->get_type() == RECURSE_WAY_COUNT)
    {
      ids = way_cnt_nd_ids(input->ways, input->attic_ways, stmt->get_lower(), stmt->get_upper());
      rman.health_check(*stmt);
    }
    else if (stmt->get_type() == RECURSE_WAY_LINK)
    {
      ids = way_link_nd_ids(input->ways, input->attic_ways, stmt->get_lower(), stmt->get_upper());
      rman.health_check(*stmt);
    }
    else if (stmt->get_type() == RECURSE_RELATION_NODE || stmt->get_type() == RECURSE_RELATION_NWR
        || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_NR)
    {
      ids = relation_node_member_ids(input->relations, input->attic_relations);
      rman.health_check(*stmt);
    }

    filter_items(Id_Predicate< Node_Skeleton >(ids), into.nodes);
    filter_items(Id_Predicate< Node_Skeleton >(ids), into.attic_nodes);
  }

  if (stmt->get_type() == RECURSE_RELATION_WAY || stmt->get_type() == RECURSE_RELATION_NWR
      || stmt->get_type() == RECURSE_RELATION_NW || stmt->get_type() == RECURSE_RELATION_WR)
    Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways)
        .filter_items(Id_Predicate< Way_Skeleton >(
            relation_way_member_ids(input->relations, input->attic_relations)))
        .swap(into.ways, into.attic_ways);
  else if (stmt->get_type() == RECURSE_NODE_WAY || stmt->get_type() == RECURSE_UP
      || stmt->get_type() == RECURSE_UP_REL)
    Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways)
        .filter_items(Get_Parent_Ways_Predicate(extract_ids(input->nodes, input->attic_nodes), stmt->get_pos()))
        .swap(into.ways, into.attic_ways);
  else
    Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways);

  if (stmt->get_type() == RECURSE_RELATION_RELATION || stmt->get_type() == RECURSE_RELATION_NWR
      || stmt->get_type() == RECURSE_RELATION_WR || stmt->get_type() == RECURSE_RELATION_NR)
    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
        .filter_items(Id_Predicate< Relation_Skeleton >(
            relation_relation_member_ids(input->relations, input->attic_relations)))
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_NODE_RELATION)
    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
        .filter_items(Get_Parent_Rels_Predicate(
            extract_ids(input->nodes, input->attic_nodes), Relation_Entry::NODE))
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_WAY_RELATION)
    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
        .filter_items(Get_Parent_Rels_Predicate(
            extract_ids(input->ways, input->attic_ways), Relation_Entry::WAY))
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_RELATION_BACKWARDS)
    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
        .filter_items(Get_Parent_Rels_Predicate(
            extract_ids(input->relations, input->attic_relations), Relation_Entry::RELATION))
        .swap(into.relations, into.attic_relations);
  else if (stmt->get_type() == RECURSE_UP || stmt->get_type() == RECURSE_UP_REL)
    return;
  else
    into.relations.clear();
}


void Recurse_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  const Set* input = rman.get_set(stmt->get_input());
  if (!input)
    return;
  if (stmt->get_type() != RECURSE_DOWN && stmt->get_type() != RECURSE_DOWN_REL
      && stmt->get_type() != RECURSE_UP && stmt->get_type() != RECURSE_UP_REL)
    return;

  Request_Context context(&query, rman);
  if (stmt->get_type() == RECURSE_DOWN)
  {
    std::vector< Node::Id_Type > rel_ids
        = relation_node_member_ids(input->relations, input->attic_relations);
    Timeless< Uint31_Index, Way_Skeleton > intermediate_ways =
        collect_ways(context, input->relations, input->attic_relations,
            Ranges< Uint31_Index >::global(), std::vector< Way::Id_Type >{}, true);

    std::vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways.get_current(), intermediate_ways.get_attic(), 0);
    rman.health_check(*stmt);

    std::vector< Node::Id_Type > ids;
    set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
    Timeless< Uint32_Index, Node_Skeleton >{}.swap(into.nodes, into.attic_nodes)
        .filter_items(Id_Predicate< Node_Skeleton >(ids)).swap(into.nodes, into.attic_nodes);

    Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways)
        .filter_items(Id_Predicate< Way_Skeleton >(
            relation_way_member_ids(input->relations, input->attic_relations)))
        .swap(into.ways, into.attic_ways);

    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations);
  }
  else if (stmt->get_type() == RECURSE_DOWN_REL)
  {
    Timeless< Uint31_Index, Relation_Skeleton > rel_rels =
        relations_loop(context, input->relations, input->attic_relations);

    std::vector< Node::Id_Type > rel_ids = relation_node_member_ids(rel_rels.get_current(), rel_rels.get_attic());

    Timeless< Uint31_Index, Way_Skeleton > intermediate_ways =
        collect_ways(context, rel_rels.get_current(), rel_rels.get_attic(),
            Ranges< Uint31_Index >::global(), std::vector< Way::Id_Type >{}, true);
    std::vector< Node::Id_Type > way_ids = way_nd_ids(intermediate_ways.get_current(), intermediate_ways.get_attic(), 0);
    rman.health_check(*stmt);

    std::vector< Node::Id_Type > ids;
    set_union(way_ids.begin(), way_ids.end(), rel_ids.begin(), rel_ids.end(), back_inserter(ids));
    Timeless< Uint32_Index, Node_Skeleton >{}.swap(into.nodes, into.attic_nodes)
        .filter_items(Id_Predicate< Node_Skeleton >(ids)).swap(into.nodes, into.attic_nodes);

    Timeless< Uint31_Index, Way_Skeleton >{}.swap(into.ways, into.attic_ways)
        .filter_items(Id_Predicate< Way_Skeleton >(
            relation_way_member_ids(rel_rels.get_current(), rel_rels.get_attic())))
        .swap(into.ways, into.attic_ways);

    if (context.get_desired_timestamp() == NOW)
      filter_items(Id_Predicate< Relation_Skeleton >(filter_for_ids(rel_rels.get_current())), into.relations);
    else
    {
      item_filter_map(into.relations, rel_rels.get_current());
      item_filter_map(into.attic_relations, rel_rels.get_attic());
    }
  }
  else if (stmt->get_type() == RECURSE_UP && !into.relations.empty())
  {
    auto rel_ways = Timeless< Uint31_Index, Way_Skeleton >{ input->ways, input->attic_ways };
    rel_ways.sort();
    rel_ways.set_union(collect_ways(context, input->nodes, input->attic_nodes, 0, {}, true).sort());

    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
        .filter_items(Or_Predicate< Relation_Skeleton, Get_Parent_Rels_Predicate, Get_Parent_Rels_Predicate >
          (Get_Parent_Rels_Predicate(extract_ids(input->nodes, input->attic_nodes), Relation_Entry::NODE),
          Get_Parent_Rels_Predicate(extract_ids(rel_ways.get_current(), rel_ways.get_attic()), Relation_Entry::WAY)))
        .swap(into.relations, into.attic_relations);
  }
  else if (stmt->get_type() == RECURSE_UP_REL && !into.relations.empty())
  {
    auto rel_ways = Timeless< Uint31_Index, Way_Skeleton >{ input->ways, input->attic_ways };
    rel_ways.sort();
    rel_ways.set_union(collect_ways(context, input->nodes, input->attic_nodes, 0, {}, true).sort());

    auto rel_rels = Timeless< Uint31_Index, Relation_Skeleton >{ input->relations, input->attic_relations };
    rel_rels.sort();
    rel_rels.set_union(
        collect_relations(context, rel_ways.get_current(), rel_ways.get_attic(), Relation_Entry::WAY, {}, true).sort());
    rel_rels.set_union(
        collect_relations(context, input->nodes, input->attic_nodes, Relation_Entry::NODE, {}, true).sort());

    rel_rels = relations_up_loop(context, std::move(rel_rels));

    Timeless< Uint31_Index, Relation_Skeleton >{}.swap(into.relations, into.attic_relations)
        .filter_items(Id_Predicate< Relation_Skeleton >(
            extract_ids< Uint31_Index, Relation_Skeleton, Relation::Id_Type >(rel_rels.get_current(), rel_rels.get_attic())))
        .swap(into.relations, into.attic_relations);
  }
}

//-----------------------------------------------------------------------------

Recurse_Statement::Recurse_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), restrict_to_role(false), lower(1), upper(0)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["pos"] = "";
  attributes["lower"] = "";
  attributes["upper"] = "";
  attributes["role"] = "";
  attributes["role-restricted"] = "no";
  attributes["type"] = "";

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
  else if (attributes["type"] == "relation-nwr")
    type = RECURSE_RELATION_NWR;
  else if (attributes["type"] == "relation-nw")
    type = RECURSE_RELATION_NW;
  else if (attributes["type"] == "relation-wr")
    type = RECURSE_RELATION_WR;
  else if (attributes["type"] == "relation-nr")
    type = RECURSE_RELATION_NR;
  else if (attributes["type"] == "way-node")
    type = RECURSE_WAY_NODE;
  else if (attributes["type"] == "way-link")
    type = RECURSE_WAY_LINK;
  else if (attributes["type"] == "way-count")
    type = RECURSE_WAY_COUNT;
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
  else if (attributes["type"] == "node-wr")
    type = RECURSE_NODE_WR;
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

  if (!attributes["role"].empty() || attributes["role-restricted"] == "yes")
  {
    if (type != RECURSE_RELATION_RELATION && type != RECURSE_RELATION_BACKWARDS
        && type != RECURSE_RELATION_WAY && type != RECURSE_RELATION_NODE
        && type != RECURSE_RELATION_NWR && type != RECURSE_RELATION_NW && type != RECURSE_RELATION_WR
        && type != RECURSE_RELATION_NR
        && type != RECURSE_NODE_RELATION && type != RECURSE_WAY_RELATION)
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

  if (!attributes["pos"].empty())
  {
    if (type != RECURSE_WAY_NODE && type != RECURSE_NODE_WAY)
      add_static_error("A role can only be specified for values \"way-node\" or \"node-way\",");
    else
    {
      std::string::size_type startpos = 0;
      std::string::size_type endpos = attributes["pos"].find(",");
      while (endpos != std::string::npos)
      {
        pos.push_back(atoll(&attributes["pos"][startpos]));
        if (pos.back() == 0)
          add_static_error("Invalid pos '"
              + (endpos != std::string::npos ?
                  attributes["pos"].substr(startpos,endpos-startpos) : attributes["pos"].substr(startpos))
              + "' in positions list.");
        startpos = endpos + 1;
        endpos = attributes["pos"].find(",", startpos);
      }
      pos.push_back(atoll(&attributes["pos"][startpos]));
      if (pos.back() == 0)
        add_static_error("Invalid pos '" + attributes["pos"].substr(startpos) + "' in positions list.");
    }

    std::sort(pos.begin(), pos.end());
  }

  if (!attributes["lower"].empty() || !attributes["upper"].empty())
  {
    int64_t lower_i = atoll(attributes["lower"].c_str());
    int64_t upper_i = atoll(attributes["upper"].c_str());
    if (lower_i <= 0)
      add_static_error("lower must be an integer greater or equal 1");
    if (attributes["upper"].empty())
      upper_i = std::numeric_limits< unsigned int >::max();
    else if (upper_i < lower_i)
      add_static_error("upper must be an integer greater or equal lower");
    lower = lower_i;
    upper = upper_i;
  }
}


std::string Recurse_Statement::to_target_type(int type)
{
  if (type == RECURSE_RELATION_RELATION || type == RECURSE_RELATION_BACKWARDS
      || type == RECURSE_WAY_RELATION || type == RECURSE_NODE_RELATION)
    return "relation";
  else if (type == RECURSE_RELATION_WAY || type == RECURSE_NODE_WAY)
    return "way";
  else if (type == RECURSE_RELATION_NODE || type == RECURSE_WAY_NODE
      || type == RECURSE_WAY_LINK || type == RECURSE_WAY_COUNT)
    return "node";
  else if (type == RECURSE_RELATION_NWR)
    return "nwr";
  else if (type == RECURSE_RELATION_NW)
    return "nw";
  else if (type == RECURSE_RELATION_WR || type == RECURSE_NODE_WR)
    return "wr";
  else if (type == RECURSE_RELATION_NR)
    return "nr";

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
  else if (type == RECURSE_RELATION_NWR)
    return "relation-nwr";
  else if (type == RECURSE_RELATION_NW)
    return "relation-nw";
  else if (type == RECURSE_RELATION_WR)
    return "relation-wr";
  else if (type == RECURSE_RELATION_NR)
    return "relation-nr";
  else if (type == RECURSE_WAY_NODE)
    return "way-node";
  else if (type == RECURSE_WAY_LINK)
    return "way-link";
  else if (type == RECURSE_WAY_COUNT)
    return "way-count";
  else if (type == RECURSE_WAY_RELATION)
    return "way-relation";
  else if (type == RECURSE_NODE_RELATION)
    return "node-relation";
  else if (type == RECURSE_NODE_WR)
    return "node-wr";
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
  else if (type == RECURSE_RELATION_WAY || type == RECURSE_RELATION_NODE || type == RECURSE_RELATION_NWR
      || type == RECURSE_RELATION_WR || type == RECURSE_RELATION_NW || type == RECURSE_RELATION_NR)
    return "r";
  else if (type == RECURSE_WAY_NODE)
    return "w";
  else if (type == RECURSE_WAY_LINK)
    return "way_link";
  else if (type == RECURSE_WAY_COUNT)
    return "way_cnt";
  else if (type == RECURSE_WAY_RELATION)
    return "bw";
  else if (type == RECURSE_NODE_RELATION || type == RECURSE_NODE_WAY || type == RECURSE_NODE_WR)
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

  Request_Context context(this, rman);
  if (restrict_to_role)
  {
    uint32 role_id = determine_role_id(*rman.get_transaction(), role);
    if (role_id == std::numeric_limits< uint32 >::max())
    {
      transfer_output(rman, into);
      return;
    }

    if (type == RECURSE_RELATION_RELATION || type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_WR
        || type == RECURSE_RELATION_NR)
      relation_relation_members(
          context, input_set->relations, input_set->attic_relations, Ranges< Uint31_Index >::global(), {}, true, &role_id)
          .swap(into.relations, into.attic_relations);
    if (type == RECURSE_RELATION_WAY || type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_NW
        || type == RECURSE_RELATION_WR)
      relation_way_members(
          context, input_set->relations, input_set->attic_relations, Ranges< Uint31_Index >::global(), {}, true, &role_id)
          .swap(into.ways, into.attic_ways);
    if (type == RECURSE_RELATION_NODE || type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_NW
        || type == RECURSE_RELATION_NR)
      relation_node_members(
          context, input_set->relations, input_set->attic_relations,
          Ranges< Uint32_Index >::global(), {}, true, &role_id)
          .swap(into.nodes, into.attic_nodes);
    else if (type == RECURSE_RELATION_BACKWARDS)
      collect_relations(context, input_set->relations, input_set->attic_relations, {}, true, role_id)
          .swap(into.relations, into.attic_relations);
    else if (type == RECURSE_NODE_RELATION)
      collect_relations(
          context, input_set->nodes, input_set->attic_nodes, Relation_Entry::NODE, {}, true, role_id)
          .swap(into.relations, into.attic_relations);
    else if (type == RECURSE_WAY_RELATION)
      collect_relations(
          context, input_set->ways, input_set->attic_ways, Relation_Entry::WAY, {}, true, role_id)
          .swap(into.relations, into.attic_relations);
  }
  else if (type == RECURSE_RELATION_RELATION)
    relation_relation_members(
        context, input_set->relations, input_set->attic_relations, Ranges< Uint31_Index >::global(), {}, true)
        .swap(into.relations, into.attic_relations);
  else if (type == RECURSE_RELATION_BACKWARDS)
    collect_relations(context, input_set->relations, input_set->attic_relations, {}, true)
        .swap(into.relations, into.attic_relations);
  else if (type == RECURSE_RELATION_WAY)
    relation_way_members(
        context, input_set->relations, input_set->attic_relations, Ranges< Uint31_Index >::global(), {}, true)
        .swap(into.ways, into.attic_ways);
  else if (type == RECURSE_RELATION_NODE)
    relation_node_members(
        context, input_set->relations, input_set->attic_relations,
        Ranges< Uint32_Index >::global(), {}, true)
        .swap(into.nodes, into.attic_nodes);
  else if (type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_NW || type == RECURSE_RELATION_WR
      || type == RECURSE_RELATION_NR)
  {
    if (type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_NR || type == RECURSE_RELATION_WR)
      relation_relation_members(
          context, input_set->relations, input_set->attic_relations, Ranges< Uint31_Index >::global(), {}, true)
          .swap(into.relations, into.attic_relations);

    if (type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_NW || type == RECURSE_RELATION_WR)
      relation_way_members(
          context, input_set->relations, input_set->attic_relations, Ranges< Uint31_Index >::global(), {}, true)
          .swap(into.ways, into.attic_ways);

    if (type == RECURSE_RELATION_NWR || type == RECURSE_RELATION_NW || type == RECURSE_RELATION_NR)
      relation_node_members(
              context, input_set->relations, input_set->attic_relations,
              Ranges< Uint32_Index >::global(), {}, true)
          .swap(into.nodes, into.attic_nodes);
  }
  else if (type == RECURSE_WAY_NODE)
    way_members(context, input_set->ways, input_set->attic_ways, get_pos(),
        Ranges< Uint32_Index >::global(), {}, true)
        .swap(into.nodes, into.attic_nodes);
  else if (type == RECURSE_WAY_COUNT)
    way_cnt_members(context, input_set->ways, input_set->attic_ways, lower, upper,
        Ranges< Uint32_Index >::global(), {}, true)
        .swap(into.nodes, into.attic_nodes);
  else if (type == RECURSE_WAY_LINK)
    way_link_members(context, input_set->ways, input_set->attic_ways, lower, upper,
        Ranges< Uint32_Index >::global(), {}, true)
        .swap(into.nodes, into.attic_nodes);
  else if (type == RECURSE_DOWN)
    add_nw_member_objects(context, *input_set, into);
  else if (type == RECURSE_DOWN_REL)
  {
    relations_loop(context, input_set->relations, input_set->attic_relations)
        .swap(into.relations, into.attic_relations);
    relation_way_members(
        context, into.relations, into.attic_relations, Ranges< Uint31_Index >::global(), {}, true)
        .swap(into.ways, into.attic_ways);
    auto source_ways = Timeless< Uint31_Index, Way_Skeleton >{ input_set->ways, input_set->attic_ways };
    source_ways.sort();
    {
      Timeless< Uint31_Index, Way_Skeleton > result_ways;
      source_ways.set_union(result_ways.swap(into.ways, into.attic_ways).sort());
      result_ways.swap(into.ways, into.attic_ways);
    }
    Timeless< Uint32_Index, Node_Skeleton > rel_nodes = relation_node_members(
        context, into.relations, into.attic_relations, Ranges< Uint32_Index >::global(), {}, true);
    rel_nodes.sort();
    {
      Timeless< Uint32_Index, Node_Skeleton > more_nodes
          = way_members(context, source_ways.get_current(), source_ways.get_attic(), 0,
                        Ranges< Uint32_Index >::global(), {}, true);
      rel_nodes.set_union(more_nodes.sort());
    }
    rel_nodes.keep_matching_skeletons(context.get_desired_timestamp());
    rel_nodes.swap(into.nodes, into.attic_nodes);
  }
  else if (type == RECURSE_NODE_WAY || type == RECURSE_NODE_RELATION || type == RECURSE_NODE_WR)
  {
    if (type == RECURSE_NODE_WAY || type == RECURSE_NODE_WR)
      collect_ways(context, input_set->nodes, input_set->attic_nodes, get_pos(), {}, true)
          .swap(into.ways, into.attic_ways);
    if (type == RECURSE_NODE_RELATION || type == RECURSE_NODE_WR)
      collect_relations(context, input_set->nodes, input_set->attic_nodes, Relation_Entry::NODE, {}, true)
          .swap(into.relations, into.attic_relations);
  }
  else if (type == RECURSE_WAY_RELATION)
    collect_relations(context, input_set->ways, input_set->attic_ways, Relation_Entry::WAY, {}, true)
        .swap(into.relations, into.attic_relations);
  else if (type == RECURSE_UP || type == RECURSE_UP_REL)
  {
    auto rel_ways = Timeless< Uint31_Index, Way_Skeleton >{ input_set->ways, input_set->attic_ways };
    rel_ways.sort();
    auto node_ways = collect_ways(context, input_set->nodes, input_set->attic_nodes, 0, {}, true);
    rel_ways.set_union(node_ways.sort());
    node_ways.swap(into.ways, into.attic_ways);

    auto rel_rels = Timeless< Uint31_Index, Relation_Skeleton >{ input_set->relations, input_set->attic_relations };
    rel_rels.sort();
    rel_rels.set_union(
        collect_relations(context, rel_ways.get_current(), rel_ways.get_attic(), Relation_Entry::WAY, {}, true).sort());
    rel_rels.set_union(
        collect_relations(context, input_set->nodes, input_set->attic_nodes, Relation_Entry::NODE, {}, true)
        .sort());

    if (type == RECURSE_UP)
      rel_rels.swap(into.relations, into.attic_relations);
    else
      relations_up_loop(context, std::move(rel_rels)).swap(into.relations, into.attic_relations);
  }

  if (context.get_desired_timestamp() != NOW)
  {
    filter_attic_elements(context, context.get_desired_timestamp(), into.nodes, into.attic_nodes);
    filter_attic_elements(context, context.get_desired_timestamp(), into.ways, into.attic_ways);
    filter_attic_elements(context, context.get_desired_timestamp(), into.relations, into.attic_relations);
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
