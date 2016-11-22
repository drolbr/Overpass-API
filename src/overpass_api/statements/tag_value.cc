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


void Set_With_Context::prefetch(const Set_Usage& usage, const Set& set, Transaction& transaction)
{
  base = &set;
  
  if (usage.usage & Set_Usage::TAGS)
  {
    tag_store_nodes = new Tag_Store< Uint32_Index, Node_Skeleton >(transaction);
    tag_store_nodes->prefetch_all(set.nodes);
    
    if (!set.attic_nodes.empty())
    {
      tag_store_attic_nodes = new Tag_Store< Uint32_Index, Node_Skeleton >(transaction);
      tag_store_attic_nodes->prefetch_all(set.attic_nodes);
    }
    
    tag_store_ways = new Tag_Store< Uint31_Index, Way_Skeleton >(transaction);
    tag_store_ways->prefetch_all(set.ways);
    
    if (!set.attic_ways.empty())
    {
      tag_store_attic_ways = new Tag_Store< Uint31_Index, Way_Skeleton >(transaction);
      tag_store_attic_ways->prefetch_all(set.attic_ways);
    }
    
    tag_store_relations = new Tag_Store< Uint31_Index, Relation_Skeleton >(transaction);
    tag_store_relations->prefetch_all(set.relations);
    
    if (!set.attic_relations.empty())
    {
      tag_store_attic_relations = new Tag_Store< Uint31_Index, Relation_Skeleton >(transaction);
      tag_store_attic_relations->prefetch_all(set.attic_relations);
    }
    
    if (!base->areas.empty())
    {
      tag_store_areas = new Tag_Store< Uint31_Index, Area_Skeleton >(transaction);
      tag_store_areas->prefetch_all(set.areas);
    }
    
    tag_store_deriveds = new Tag_Store< Uint31_Index, Derived_Structure >(transaction);
    tag_store_deriveds->prefetch_all(set.deriveds);
  }
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Set_Tag_Statement > Set_Tag_Statement::statement_maker("set-tag");


Set_Tag_Statement::Set_Tag_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), set_id(false), tag_value(0)
{
  std::map< std::string, std::string > attributes;
  
  attributes["k"] = "";
  attributes["keytype"] = "tag";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  if (attributes["keytype"] == "tag")
  {
    if (attributes["k"] != "")
      keys.push_back(attributes["k"]);
    else
      add_static_error("For the statement \"set-tag\" in mode \"keytype\"=\"tag\", "
          "the attribute \"k\" must be nonempty.");
  }
  else if (attributes["keytype"] == "id")
    set_id = true;
  else if (attributes["keytype"] != "generic")
    add_static_error("For the attribute \"keytype\" of the element \"set-tag\""
        " the only allowed values are \"tag\", \"id\", or \"generic\".");
}


void Set_Tag_Statement::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (tag_value_ && !tag_value)
    tag_value = tag_value_;
  else if (tag_value)
    add_static_error("set-tag must have exactly one evaluator substatement.");
  else
    substatement_error(get_name(), statement);
}


std::string Set_Tag_Statement::eval(const std::string* key)
{
  return tag_value ? tag_value->eval(key) : "";
}


std::string Set_Tag_Statement::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


std::string Set_Tag_Statement::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return tag_value ? tag_value->eval(elem, tags, key) : "";
}


void Set_Tag_Statement::prefetch(const Set_With_Context& set)
{
  if (tag_value)
    tag_value->prefetch(set);
}


std::pair< std::vector< Set_Usage >, uint > Set_Tag_Statement::used_sets() const
{
  if (tag_value)
  {
    if (input == "")
      return tag_value->used_sets();
    
    std::pair< std::vector< Set_Usage >, uint > result = tag_value->used_sets();
    std::vector< Set_Usage >::iterator it =
        std::lower_bound(result.first.begin(), result.first.end(), Set_Usage(input, 0u));
    if (it == result.first.end() || it->set_name != input)
      result.first.insert(it, Set_Usage(input, Set_Usage::TAGS));
    else
      it->usage |= Set_Usage::TAGS;
    return result;
  }
  
  std::vector< Set_Usage > result;
  if (input != "")
    result.push_back(Set_Usage(input, Set_Usage::TAGS));
  return std::make_pair(result, 0u);
}


void Set_Tag_Statement::clear()
{
  if (tag_value)
    tag_value->clear();
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Evaluator_Fixed > Evaluator_Fixed::statement_maker("eval-fixed");


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


Generic_Statement_Maker< Evaluator_Generic > Evaluator_Generic::statement_maker("eval-generic");


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
    : Evaluator(line_number_), counter(0)
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


std::string Evaluator_Count::eval(const std::string* key)
{
  if (to_count == Evaluator_Count::members || to_count == Evaluator_Count::tags)
    return "0";
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return "0";
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return "0";
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->nds.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->nds.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return to_string(elem->members.size());
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return "0";
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Evaluator_Count::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Evaluator_Count::members)
    return "0";
  else if (to_count == Evaluator_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


void Evaluator_Count::prefetch(const Set_With_Context& set)
{
  if (set.name == input)
  {
    if (to_count == Evaluator_Count::nodes)
      counter = count(set.base->nodes) + count(set.base->attic_nodes);
    else if (to_count == Evaluator_Count::ways)
      counter = count(set.base->ways) + count(set.base->attic_ways);
    else if (to_count == Evaluator_Count::relations)
      counter = count(set.base->relations) + count(set.base->attic_relations);
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
  

void Evaluator_Count::clear()
{
  counter = 0;
}
