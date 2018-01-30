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


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "aggregators.h"


Evaluator_Aggregator::Evaluator_Aggregator
    (const std::string& func_name, int line_number_,
     const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
    : Evaluator(line_number_), rhs(0)
{
  std::map< std::string, std::string > attributes;
  attributes["from"] = "_";
  eval_attributes_array(func_name, attributes, input_attributes);
  input = attributes["from"];
}


void Evaluator_Aggregator::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one evaluator substatements.");
}


template< typename Index, typename Maybe_Attic >
void eval_elems(Value_Aggregator& aggregator, Eval_Task& task,
    const std::map< Index, std::vector< Maybe_Attic > >& elems, Set_With_Context& input_set,
    const std::string* key)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator idx_it = elems.begin();
      idx_it != elems.end(); ++idx_it)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator elem_it = idx_it->second.begin();
        elem_it != idx_it->second.end(); ++elem_it)
      aggregator.update_value(task.eval(input_set.get_context(idx_it->first, *elem_it), key));
  }
}


template< typename Index, typename Maybe_Attic >
void eval_elems(Geometry_Aggregator& aggregator, Eval_Geometry_Task& task,
    const std::map< Index, std::vector< Maybe_Attic > >& elems, Set_With_Context& input_set)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator idx_it = elems.begin();
      idx_it != elems.end(); ++idx_it)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator elem_it = idx_it->second.begin();
        elem_it != idx_it->second.end(); ++elem_it)
      aggregator.consume_value(task.eval(input_set.get_context(idx_it->first, *elem_it)));
  }
}


Eval_Task* Evaluator_Aggregator::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  if (!rhs)
    return 0;

  Owner< Eval_Task > rhs_task(rhs->get_string_task(context, key));
  if (!rhs_task)
    return 0;

  Set_With_Context* input_set = context.get_set(input);
  if (!input_set || !input_set->base)
    return 0;

  Owner< Value_Aggregator > value_agg(get_aggregator());
  if (!value_agg)
    return 0;

  eval_elems(*value_agg, *rhs_task, input_set->base->nodes, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->attic_nodes, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->ways, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->attic_ways, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->relations, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->attic_relations, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->areas, *input_set, key);
  eval_elems(*value_agg, *rhs_task, input_set->base->deriveds, *input_set, key);

  return new Const_Eval_Task((*value_agg).get_value());
}


Eval_Geometry_Task* Evaluator_Aggregator::get_geometry_task(Prepare_Task_Context& context)
{
  if (!rhs)
    return 0;

  Owner< Eval_Geometry_Task > rhs_task(rhs->get_geometry_task(context));
  if (!rhs_task)
    return 0;

  Set_With_Context* input_set = context.get_set(input);
  if (!input_set || !input_set->base)
    return 0;

  Owner< Geometry_Aggregator > value_agg(get_geometry_aggregator());
  if (!value_agg)
    return 0;

  eval_elems(*value_agg, *rhs_task, input_set->base->nodes, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->attic_nodes, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->ways, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->attic_ways, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->relations, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->attic_relations, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->areas, *input_set);
  eval_elems(*value_agg, *rhs_task, input_set->base->deriveds, *input_set);

  return new Const_Eval_Geometry_Task((*value_agg).move_value());
}


Requested_Context Evaluator_Aggregator::request_context() const
{
  if (rhs)
  {
    Requested_Context result = rhs->request_context();
    result.bind(input);
    return result;
  }
  
  return Requested_Context();
}


bool try_parse_input_set(const Token_Node_Ptr& tree_it, Error_Output* error_output, const std::string& message,
    std::string& input_set, bool& explicit_input_set)
{
  if (tree_it->token == "(")
  {
    if (!tree_it->lhs)
      return false;
    if (!tree_it->rhs)
    {
      if (error_output)
        error_output->add_parse_error(message, tree_it->line_col.first);
      return false;
    }

    input_set = "_";
    explicit_input_set = false;
  }
  else
  {
    if (!tree_it->lhs)
      return false;
    if (!tree_it->rhs || !tree_it.rhs()->rhs)
    {
      if (error_output)
        error_output->add_parse_error(message, tree_it->line_col.first);
      return false;
    }
    if (!tree_it.rhs()->lhs)
    {
      if (error_output)
        error_output->add_parse_error("Input set required if dot is present", tree_it->line_col.first);
      return false;
    }

    input_set = tree_it.lhs()->token;
    explicit_input_set = true;
  }
  return true;
}


//-----------------------------------------------------------------------------


Aggregator_Statement_Maker< Evaluator_Union_Value > Evaluator_Union_Value::statement_maker;
Aggregator_Evaluator_Maker< Evaluator_Union_Value > Evaluator_Union_Value::evaluator_maker;


void Evaluator_Union_Value::Aggregator::update_value(const std::string& value)
{
  if (value != "" && value != agg_value)
    agg_value = (agg_value == "" ? value : "< multiple values found >");
}


//-----------------------------------------------------------------------------


Aggregator_Statement_Maker< Evaluator_Min_Value > Evaluator_Min_Value::statement_maker;
Aggregator_Evaluator_Maker< Evaluator_Min_Value > Evaluator_Min_Value::evaluator_maker;


void Evaluator_Min_Value::Aggregator::update_value(const std::string& value)
{
  if (relevant_type == type_void)
    relevant_type = type_int64;

  if (relevant_type <= type_int64)
  {
    int64 rhs_l = 0;
    if (try_int64(value, rhs_l))
      result_l = std::min(result_l, rhs_l);
    else
      relevant_type = type_double;
  }

  if (relevant_type <= type_double)
  {
    double rhs_d = 0;
    if (try_double(value, rhs_d))
      result_d = std::min(result_d, rhs_d);
    else
      relevant_type = type_string;
  }

  if (value != "")
    result_s = (result_s != "" ? std::min(result_s, value) : value);
}


std::string Evaluator_Min_Value::Aggregator::get_value()
{
  if (relevant_type == type_void)
    return "";
  else if (relevant_type == type_int64)
    return to_string(result_l);
  else if (relevant_type == type_double)
    return to_string(result_d);

  return result_s;
}


//-----------------------------------------------------------------------------


Aggregator_Statement_Maker< Evaluator_Max_Value > Evaluator_Max_Value::statement_maker;
Aggregator_Evaluator_Maker< Evaluator_Max_Value > Evaluator_Max_Value::evaluator_maker;


void Evaluator_Max_Value::Aggregator::update_value(const std::string& value)
{
  if (relevant_type == type_void)
    relevant_type = type_int64;

  if (relevant_type <= type_int64)
  {
    int64 rhs_l = 0;
    if (try_int64(value, rhs_l))
      result_l = std::max(result_l, rhs_l);
    else
      relevant_type = type_double;
  }

  if (relevant_type <= type_double)
  {
    double rhs_d = 0;
    if (try_double(value, rhs_d))
      result_d = std::max(result_d, rhs_d);
    else
      relevant_type = type_string;
  }

  if (value != "")
    result_s = (result_s != "" ? std::max(result_s, value) : value);
}


std::string Evaluator_Max_Value::Aggregator::get_value()
{
  if (relevant_type == type_void)
    return "";
  else if (relevant_type == type_int64)
    return to_string(result_l);
  else if (relevant_type == type_double)
    return to_string(result_d);

  return result_s;
}


//-----------------------------------------------------------------------------


Aggregator_Statement_Maker< Evaluator_Sum_Value > Evaluator_Sum_Value::statement_maker;
Aggregator_Evaluator_Maker< Evaluator_Sum_Value > Evaluator_Sum_Value::evaluator_maker;


void Evaluator_Sum_Value::Aggregator::update_value(const std::string& value)
{
  if (relevant_type == type_int64)
  {
    int64 rhs_l = 0;
    if (try_int64(value, rhs_l))
      result_l += rhs_l;
    else
      relevant_type = type_double;
  }

  if (relevant_type == type_int64 || relevant_type == type_double)
  {
    double rhs_d = 0;
    if (try_double(value, rhs_d))
      result_d += rhs_d;
    else
      relevant_type = type_string;
  }
}


std::string Evaluator_Sum_Value::Aggregator::get_value()
{
  if (relevant_type == type_int64)
    return to_string(result_l);
  else if (relevant_type == type_double)
    return to_string(result_d);

  return "NaN";
}


//-----------------------------------------------------------------------------


Aggregator_Statement_Maker< Evaluator_Set_Value > Evaluator_Set_Value::statement_maker;
Aggregator_Evaluator_Maker< Evaluator_Set_Value > Evaluator_Set_Value::evaluator_maker;


void Evaluator_Set_Value::Aggregator::update_value(const std::string& value)
{
  if (value != "")
    values.insert(value);
}


std::string Evaluator_Set_Value::Aggregator::get_value()
{
  std::string result;
  std::set< std::string >::const_iterator it = values.begin();
  if (it != values.end())
  {
    result = *it;
    ++it;
  }
  for (; it != values.end(); ++it)
    result += ";" + *it;
  return result;
}


//-----------------------------------------------------------------------------


Evaluator_Set_Count::Statement_Maker Evaluator_Set_Count::statement_maker;
Evaluator_Set_Count::Evaluator_Maker Evaluator_Set_Count::evaluator_maker;


bool Evaluator_Set_Count::try_parse_object_type(const std::string& input, Evaluator_Set_Count::Objects& result)
{
  if (input == "nodes")
    result = Evaluator_Set_Count::nodes;
  else if (input == "ways")
    result = Evaluator_Set_Count::ways;
  else if (input == "relations")
    result = Evaluator_Set_Count::relations;
  else if (input == "deriveds")
    result = Evaluator_Set_Count::deriveds;
  else
    return false;
  return true;
}


Statement* Evaluator_Set_Count::Evaluator_Maker::create_evaluator(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (!tree_it.assert_is_function(error_output)
      || !tree_it.assert_has_arguments(error_output, true))
    return 0;
  
  std::map< std::string, std::string > attributes;

  if (tree_it->token == "(")
  {
    attributes["from"] = "_";
    attributes["type"] = tree_it.rhs()->token;
  }
  else
  {
    attributes["from"] = tree_it.lhs()->token;
    attributes["type"] = tree_it.rhs().rhs()->token;
  }
  
  Objects to_count;
  if (try_parse_object_type(attributes["type"], to_count))
    return new Evaluator_Set_Count(tree_it->line_col.first, attributes, global_settings);
  else if (error_output)
    error_output->add_parse_error(
        "\"count\" accepts only one of \"nodes\", \"ways\", \"relations\", or \"deriveds\" as type argument.",
        tree_it->line_col.first);

  return 0;
}


std::string Evaluator_Set_Count::to_string(Evaluator_Set_Count::Objects objects)
{
  if (objects == nodes)
    return "node";
  if (objects == ways)
    return "ways";
  if (objects == relations)
    return "relations";
  if (objects == deriveds)
    return "deriveds";
  return "nothing";
}


Evaluator_Set_Count::Evaluator_Set_Count
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["type"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];

  if (!try_parse_object_type(attributes["type"], to_count))
  {
    std::ostringstream temp("");
    temp<<"For the attribute \"type\" of the element \"eval-set-count\""
        <<" the only allowed values are \"nodes\", \"ways\", \"relations\", or \"deriveds\" strings.";
    add_static_error(temp.str());
  }
}


Requested_Context Evaluator_Set_Count::request_context() const
{
  Requested_Context result;
  if (to_count == Evaluator_Set_Count::nodes || to_count == Evaluator_Set_Count::ways
        || to_count == Evaluator_Set_Count::relations || to_count == Evaluator_Set_Count::deriveds)
    result.add_usage(input, 1u);
  return result;
}


Eval_Task* Evaluator_Set_Count::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  const Set_With_Context* set = context.get_set(input);

  unsigned int counter = 0;
  if (set && set->base)
  {
    if (to_count == nodes)
      counter = count(set->base->nodes) + count(set->base->attic_nodes);
    if (to_count == ways)
      counter = count(set->base->ways) + count(set->base->attic_ways);
    if (to_count == relations)
      counter = count(set->base->relations) + count(set->base->attic_relations);
    if (to_count == deriveds)
      counter = count(set->base->areas) + count(set->base->deriveds);
  }

  return new Const_Eval_Task(::to_string(counter));
}


//-----------------------------------------------------------------------------


Aggregator_Statement_Maker< Evaluator_Geom_Concat_Value > Evaluator_Geom_Concat_Value::statement_maker;
Aggregator_Evaluator_Maker< Evaluator_Geom_Concat_Value > Evaluator_Geom_Concat_Value::evaluator_maker;


void Evaluator_Geom_Concat_Value::Aggregator::consume_value(Opaque_Geometry* geom)
{
  if (!geom)
    return;
  
  if (!result)
    result = new Compound_Geometry();
  
  result->add_component(geom);
}


Opaque_Geometry* Evaluator_Geom_Concat_Value::Aggregator::move_value()
{
  Opaque_Geometry* result_ = result;
  result = 0;
  return result_;
}
