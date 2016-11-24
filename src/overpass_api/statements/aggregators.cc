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
    (const string& func_name, int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
    : Evaluator(line_number_), rhs(0), input_set(0), value_set(false)
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


template< typename Index, typename Maybe_Attic, typename Object >
void eval_elems(Evaluator_Aggregator* aggregator, Eval_Task& task,
    const std::map< Index, std::vector< Maybe_Attic > >& elems, Tag_Store< Index, Object >* tag_store)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator idx_it = elems.begin();
      idx_it != elems.end(); ++idx_it)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator elem_it = idx_it->second.begin();
        elem_it != idx_it->second.end(); ++elem_it)
    {
      std::string value = task.eval(&*elem_it, tag_store ? tag_store->get(idx_it->first, *elem_it) : 0, 0);
      
      if (aggregator->value_set)
        aggregator->value = aggregator->update_value(aggregator->value, value);
      else
      {
        aggregator->value = value;
        aggregator->value_set = true;
      }
    }
  }
}

  
Eval_Task* Evaluator_Aggregator::get_task(const Prepare_Task_Context& context)
{
  if (!rhs)
    return 0;
  
  Owner < Eval_Task > rhs_task(rhs->get_task(context));
  if (!rhs_task)
    return 0;
  
  const Set_With_Context* input_set = context.get_set(input);
  if (!input_set)
    return 0;
  
  value_set = false;
  eval_elems(this, *rhs_task, input_set->base->nodes, input_set->tag_store_nodes);
  eval_elems(this, *rhs_task, input_set->base->attic_nodes, input_set->tag_store_attic_nodes);
  eval_elems(this, *rhs_task, input_set->base->ways, input_set->tag_store_ways);
  eval_elems(this, *rhs_task, input_set->base->attic_ways, input_set->tag_store_attic_ways);
  eval_elems(this, *rhs_task, input_set->base->relations, input_set->tag_store_relations);
  eval_elems(this, *rhs_task, input_set->base->attic_relations, input_set->tag_store_attic_relations);
  eval_elems(this, *rhs_task, input_set->base->areas, input_set->tag_store_areas);
  eval_elems(this, *rhs_task, input_set->base->deriveds, input_set->tag_store_deriveds);

  return new Const_Eval_Task(value);
}

  
std::pair< std::vector< Set_Usage >, uint > Evaluator_Aggregator::used_sets() const
{
  if (rhs)
  {
    std::pair< std::vector< Set_Usage >, uint > result = rhs->used_sets();
    std::vector< Set_Usage >::iterator it =
        std::lower_bound(result.first.begin(), result.first.end(), Set_Usage(input, 0u));
    if (it == result.first.end() || it->set_name != input)
      result.first.insert(it, Set_Usage(input, result.second));
    else
      it->usage |= result.second;
    return result;
  }
  
  std::vector< Set_Usage > result;
  result.push_back(Set_Usage(input, 0u));
  return std::make_pair(result, 0u);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Union_Value > Evaluator_Union_Value::statement_maker("eval-union-value");


Evaluator_Union_Value::Evaluator_Union_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator_Aggregator("eval-union-value", line_number_, input_attributes, global_settings) {}


std::string Evaluator_Union_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  if (new_value == "" || agg_value == new_value)
    return agg_value;
  else if (agg_value == "")
    return new_value;
  else
    return "< multiple values found >";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Min_Value > Evaluator_Min_Value::statement_maker("eval-min-value");


Evaluator_Min_Value::Evaluator_Min_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator_Aggregator("eval-min-value", line_number_, input_attributes, global_settings) {}


std::string Evaluator_Min_Value::update_value(const std::string& agg_value, const std::string& new_value)
{  
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(agg_value, lhs_l) && try_int64(new_value, rhs_l))
    return rhs_l < lhs_l ? new_value : agg_value;
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(agg_value, lhs_d) && try_double(new_value, rhs_d))
    return rhs_d < lhs_d ? new_value : agg_value;
  
  if (new_value == "")
    return agg_value;
  if (agg_value == "")
    return new_value;
    
  return std::min(agg_value, new_value);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Max_Value > Evaluator_Max_Value::statement_maker("eval-max-value");


Evaluator_Max_Value::Evaluator_Max_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator_Aggregator("eval-max-value", line_number_, input_attributes, global_settings) {}


std::string Evaluator_Max_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(agg_value, lhs_l) && try_int64(new_value, rhs_l))
    return rhs_l > lhs_l ? new_value : agg_value;
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(agg_value, lhs_d) && try_double(new_value, rhs_d))
    return rhs_d > lhs_d ? new_value : agg_value;
  
  return std::max(agg_value, new_value);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Sum_Value > Evaluator_Sum_Value::statement_maker("eval-sum-value");


Evaluator_Sum_Value::Evaluator_Sum_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator_Aggregator("eval-sum-value", line_number_, input_attributes, global_settings) {}


std::string Evaluator_Sum_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(agg_value, lhs_l) && try_int64(new_value, rhs_l))
    return to_string(lhs_l + rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(agg_value, lhs_d) && try_double(new_value, rhs_d))
    return to_string(lhs_d + rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Set_Value > Evaluator_Set_Value::statement_maker("eval-set-value");


Evaluator_Set_Value::Evaluator_Set_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator_Aggregator("eval-set-value", line_number_, input_attributes, global_settings) {}


std::string Evaluator_Set_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  if (values.empty())
    values.insert(agg_value);
  
  values.insert(new_value);
  
  std::string result;
  std::set< std::string >::const_iterator it = values.begin();
  if (it != values.end() && *it == "")
    ++it;
  if (it != values.end())
  {
    result = *it;
    ++it;
  }
  for (; it != values.end(); ++it)
    result += ";" + *it;
  return result;
}
