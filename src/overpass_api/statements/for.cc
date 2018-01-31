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

#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "for.h"


Generic_Statement_Maker< For_Statement > For_Statement::statement_maker("for");


For_Statement::For_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), evaluator(0)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  output = attributes["into"];
}


void For_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());

  if (!evaluator)
  {
    evaluator = dynamic_cast< Evaluator* >(statement);
    if (!evaluator)
      add_static_error("A For statement must have an Evaluator as first sub-statement.");
  }
  else if (statement)
  {
    if (statement->get_name() != "newer")
      substatements.push_back(statement);
    else
      add_static_error("\"newer\" can appear only inside \"query\" statements.");
  }
}


template< typename Index, typename Object, typename Valuation_Target >
void collect_for_targets_by_string(
    const std::map< Index, std::vector< Object > >& container,
    const Valuation_Target& target,
    const Eval_Task& task, Set_With_Context& context_from)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it_idx = container.begin(); it_idx != container.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      std::string valuation = task.eval(context_from.get_context(it_idx->first, *it_elem), 0);
      target(valuation)[it_idx->first].push_back(*it_elem);
    }
  }
}


template< typename Index, typename Object, typename Valuation_Target >
void collect_for_targets_by_container(
    const std::map< Index, std::vector< Object > >& container,
    const Valuation_Target& target,
    const Eval_Container_Task& task, Set_With_Context& context_from)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it_idx = container.begin(); it_idx != container.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      std::vector< std::string > valuation = task.eval(context_from.get_context(it_idx->first, *it_elem), 0);
      for (std::vector< std::string >::const_iterator it_val = valuation.begin();
          it_val != valuation.end(); ++it_val)
        target(*it_val)[it_idx->first].push_back(*it_elem);
    }
  }
}


struct Base_Valuation_Target
{
  Base_Valuation_Target(std::map< std::string, Set >& element_groups_) : element_groups(&element_groups_) {}
protected:
  std::map< std::string, Set >* element_groups;
};


struct Node_Valuation_Target : Base_Valuation_Target
{
  Node_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint32_Index, std::vector< Node_Skeleton > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].nodes; }
};


struct Attic_Node_Valuation_Target : Base_Valuation_Target
{
  Attic_Node_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].attic_nodes; }
};


struct Way_Valuation_Target : Base_Valuation_Target
{
  Way_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint31_Index, std::vector< Way_Skeleton > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].ways; }
};


struct Attic_Way_Valuation_Target : Base_Valuation_Target
{
  Attic_Way_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].attic_ways; }
};


struct Relation_Valuation_Target : Base_Valuation_Target
{
  Relation_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint31_Index, std::vector< Relation_Skeleton > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].relations; }
};


struct Attic_Relation_Valuation_Target : Base_Valuation_Target
{
  Attic_Relation_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].attic_relations; }
};


struct Area_Valuation_Target : Base_Valuation_Target
{
  Area_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint31_Index, std::vector< Area_Skeleton > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].areas; }
};


struct Derived_Valuation_Target : Base_Valuation_Target
{
  Derived_Valuation_Target(std::map< std::string, Set >& rhs) : Base_Valuation_Target(rhs) {}
  std::map< Uint31_Index, std::vector< Derived_Structure > >& operator()(const std::string& rhs) const
  { return (*element_groups)[rhs].deriveds; }
};


void For_Statement::execute(Resource_Manager& rman)
{
  if (!evaluator)
    return;
  
  Requested_Context requested_context;
  requested_context.add(evaluator->request_context());
  requested_context.bind(input);
  
  Prepare_Task_Context context(requested_context, *this, rman);
  
  Set_With_Context* context_from = context.get_set(input);
  if (!context_from)
    return;

  Set base_result_set;
  rman.swap_set(get_result_name(), base_result_set);
  
  rman.push_stack_frame();
  
  const Set* base_set = (input == get_result_name() ? &base_result_set : rman.get_set(input));
  if (!base_set)
    base_set = &base_result_set;
  
  std::map< std::string, Set > element_groups;
  
  if (evaluator->return_type() == Statement::string)
  {
    Owner< Eval_Task > task(evaluator->get_string_task(context, 0));
  
    collect_for_targets_by_string(base_set->nodes, Node_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->attic_nodes, Attic_Node_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->ways, Way_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->attic_ways, Attic_Way_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->relations, Relation_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->attic_relations, Attic_Relation_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->areas, Area_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_string(base_set->deriveds, Derived_Valuation_Target(element_groups),
        *task, *context_from);
  }
  else
  {
    Owner< Eval_Container_Task > task(evaluator->get_container_task(context, 0));
  
    collect_for_targets_by_container(base_set->nodes, Node_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->attic_nodes, Attic_Node_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->ways, Way_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->attic_ways, Attic_Way_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->relations, Relation_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->attic_relations, Attic_Relation_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->areas, Area_Valuation_Target(element_groups),
        *task, *context_from);
    collect_for_targets_by_container(base_set->deriveds, Derived_Valuation_Target(element_groups),
        *task, *context_from);
  }
  
  for (std::map< std::string, Set >::iterator it = element_groups.begin(); it != element_groups.end(); ++it)
  {
    rman.count_loop();
    rman.swap_set(get_result_name(), it->second);
    rman.set_value(get_result_name(), "val", it->first);

    for (std::vector< Statement* >::iterator it = substatements.begin();
        it != substatements.end(); ++it)
      (*it)->execute(rman);
    
    rman.union_inward(input, get_result_name());
  }

  rman.erase_set(input);
  rman.move_all_inward_except(get_result_name());
  rman.pop_stack_frame();

  rman.health_check(*this);
}
