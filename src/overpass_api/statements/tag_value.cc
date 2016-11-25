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
#include "tag_value.h"


Evaluator_Fixed::Statement_Maker Evaluator_Fixed::statement_maker;


Statement* Evaluator_Fixed::Statement_Maker::create_statement
    (const Token_Node_Ptr& tree_it, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_it->lhs || tree_it->rhs)
    return 0;
  map< string, string > attributes;
  attributes["v"] = decode_json(tree_it->token, error_output);
  return new Evaluator_Fixed(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Fixed::Evaluator_Fixed
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["v"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  value = attributes["v"];
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Id > Evaluator_Id::statement_maker("eval-id");


Evaluator_Id::Evaluator_Id
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Type > Evaluator_Type::statement_maker("eval-type");


Evaluator_Type::Evaluator_Type
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


std::string find_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key)
{
  if (!tags)
    return "";
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      return it->second;
  }
  
  return "";
}


Generic_Statement_Maker< Evaluator_Value > Evaluator_Value::statement_maker("eval-value");


Evaluator_Value::Evaluator_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
}


//-----------------------------------------------------------------------------


std::string exists_value(const std::vector< std::pair< std::string, std::string > >* tags, const std::string& key)
{
  if (!tags)
    return "0";
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      return "1";
  }
  
  return "0";
}


Generic_Statement_Maker< Evaluator_Is_Tag > Evaluator_Is_Tag::statement_maker("eval-is-tag");


Evaluator_Is_Tag::Evaluator_Is_Tag
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
}


//-----------------------------------------------------------------------------


Evaluator_Generic::Statement_Maker Evaluator_Generic::statement_maker;


Statement* Evaluator_Generic::Statement_Maker::create_statement
    (const Token_Node_Ptr& tree_it, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_it->lhs || tree_it->rhs)
    return 0;
  map< string, string > attributes;
  return new Evaluator_Generic(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Generic::Evaluator_Generic
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Count > Evaluator_Count::statement_maker("eval-count");


Evaluator_Count::Evaluator_Count
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  
  if (attributes["type"] == "nodes")
    to_count = nodes;
  else if (attributes["type"] == "ways")
    to_count = ways;
  else if (attributes["type"] == "relations")
    to_count = relations;
  else if (attributes["type"] == "deriveds")
    to_count = deriveds;
  else if (attributes["type"] == "tags")
    to_count = tags;
  else if (attributes["type"] == "members")
    to_count = members;
  else
  {
    ostringstream temp("");
    temp<<"For the attribute \"type\" of the element \"eval-count\""
        <<" the only allowed values are \"nodes\", \"ways\", \"relations\", \"deriveds\", \"tags\", "
          "or \"members\" strings.";
    add_static_error(temp.str());
  }
}


std::pair< std::vector< Set_Usage >, uint > Evaluator_Count::used_sets() const
{
  std::vector< Set_Usage > result;
  if (to_count == Evaluator_Count::nodes || to_count == Evaluator_Count::ways || to_count == Evaluator_Count::relations)
    result.push_back(Set_Usage(input, 1u));
  if (to_count == Evaluator_Count::tags)
    return std::make_pair(result, 2u);
  else if (to_count == Evaluator_Count::members)
    return std::make_pair(result, 1u);
  return std::make_pair(result, 0u);
}

  
std::vector< std::string > Evaluator_Count::used_tags() const
{
  std::vector< std::string > result;
  return result;
}


Eval_Task* Evaluator_Count::get_task(const Prepare_Task_Context& context)
{
  if (to_count == tags || to_count == members)
    return new Count_Eval_Task(to_count);
  
  const Set_With_Context* set = context.get_set(input);
  
  unsigned int counter = 0;
  if (set)
  {
    if (to_count == nodes)
      counter = count(set->base->nodes) + count(set->base->attic_nodes);
    if (to_count == ways)
      counter = count(set->base->ways) + count(set->base->attic_ways);
    if (to_count == relations)
      counter = count(set->base->relations) + count(set->base->attic_relations);
  }
  
  return new Const_Eval_Task(to_string(counter));
}


std::string Count_Eval_Task::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->nds.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->nds.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}


std::string Count_Eval_Task::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key) const
{
  if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return "0";
}
