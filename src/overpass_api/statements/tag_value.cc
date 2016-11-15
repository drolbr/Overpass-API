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
  Tag_Value* tag_value_ = dynamic_cast< Tag_Value* >(statement);
  if (tag_value_ && !tag_value)
    tag_value = tag_value_;
  else if (tag_value)
    add_static_error("set-tag must have exactly one tag-value substatement.");
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


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Fixed > Tag_Value_Fixed::statement_maker("value-fixed");


Tag_Value_Fixed::Tag_Value_Fixed
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["v"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  value = attributes["v"];
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Id > Tag_Value_Id::statement_maker("value-id");


Tag_Value_Id::Tag_Value_Id
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Type > Tag_Value_Type::statement_maker("value-type");


Tag_Value_Type::Tag_Value_Type
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
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


Generic_Statement_Maker< Tag_Value_Value > Tag_Value_Value::statement_maker("value-value");


Tag_Value_Value::Tag_Value_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Generic > Tag_Value_Generic::statement_maker("value-generic");


Tag_Value_Generic::Tag_Value_Generic
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Count > Tag_Value_Count::statement_maker("value-count");


Tag_Value_Count::Tag_Value_Count
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_), counter(0)
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
    temp<<"For the attribute \"type\" of the element \"value-count\""
        <<" the only allowed values are \"nodes\", \"ways\", \"relations\", \"deriveds\", \"tags\", "
          "or \"members\" strings.";
    add_static_error(temp.str());
  }
}


std::string Tag_Value_Count::eval(const std::string* key)
{
  if (to_count == Tag_Value_Count::members || to_count == Tag_Value_Count::tags)
    return "0";
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return "0";
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return "0";
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return to_string(elem->nds.size());
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return to_string(elem->nds.size());
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return to_string(elem->members.size());
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return to_string(elem->members.size());
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return "0";
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


std::string Tag_Value_Count::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  if (to_count == Tag_Value_Count::members)
    return "0";
  else if (to_count == Tag_Value_Count::tags)
    return to_string(tags->size());
  return to_string(counter);
}


void Tag_Value_Count::prefetch(const Set_With_Context& set)
{
  if (set.name == input)
  {
    if (to_count == Tag_Value_Count::nodes)
      counter = count(set.base->nodes) + count(set.base->attic_nodes);
    else if (to_count == Tag_Value_Count::ways)
      counter = count(set.base->ways) + count(set.base->attic_ways);
    else if (to_count == Tag_Value_Count::relations)
      counter = count(set.base->relations) + count(set.base->attic_relations);
  }
}


std::pair< std::vector< Set_Usage >, uint > Tag_Value_Count::used_sets() const
{
  std::vector< Set_Usage > result;
  if (to_count == Tag_Value_Count::nodes || to_count == Tag_Value_Count::ways || to_count == Tag_Value_Count::relations)
    result.push_back(Set_Usage(input, 1u));
  if (to_count == Tag_Value_Count::tags)
    return std::make_pair(result, 2u);
  else if (to_count == Tag_Value_Count::members)
    return std::make_pair(result, 1u);
  return std::make_pair(result, 0u);
}

  
std::vector< std::string > Tag_Value_Count::used_tags() const
{
  std::vector< std::string > result;
  return result;
}
  

void Tag_Value_Count::clear()
{
  counter = 0;
}


//-----------------------------------------------------------------------------


Tag_Value_Pair_Operator::Tag_Value_Pair_Operator(int line_number_) : Tag_Value(line_number_), lhs(0), rhs(0) {}


void Tag_Value_Pair_Operator::add_statement(Statement* statement, std::string text)
{
  Tag_Value* tag_value_ = dynamic_cast< Tag_Value* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!lhs)
    lhs = tag_value_;
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly two tag-value substatements.");
}


std::string Tag_Value_Pair_Operator::eval(const std::string* key)
{
  return process(lhs ? lhs->eval(key) : "", rhs ? rhs->eval(key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Pair_Operator::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(lhs ? lhs->eval(elem, tags, key) : "", rhs ? rhs->eval(elem, tags, key) : "");
}

    
void Tag_Value_Pair_Operator::prefetch(const Set_With_Context& set)
{
  if (lhs)
    lhs->prefetch(set);
  if (rhs)
    rhs->prefetch(set);
}


std::pair< std::vector< Set_Usage >, uint > union_usage(const std::pair< std::vector< Set_Usage >, uint >& lhs,
    const std::pair< std::vector< Set_Usage >, uint >& rhs)
{
  std::vector< Set_Usage > result(lhs.first.size() + rhs.first.size(), Set_Usage("", 0u));
    
  std::vector< Set_Usage >::const_iterator it_lhs = lhs.first.begin();
  std::vector< Set_Usage >::const_iterator it_rhs = rhs.first.begin();
  std::vector< Set_Usage >::iterator it_res = result.begin();
    
  while (it_lhs != lhs.first.end() && it_rhs != rhs.first.end())
  {
    if (it_lhs->set_name < it_rhs->set_name)
    {
      *it_res = *it_lhs;
      ++it_res;
      ++it_lhs;        
    }
    else if (it_rhs->set_name < it_lhs->set_name)
    {
      *it_res = *it_rhs;
      ++it_res;
      ++it_rhs;
    }
    else
    {
      *it_res = *it_lhs;
      it_res->usage |= it_rhs->usage;
      ++it_res;
      ++it_lhs;
      ++it_rhs;
    }
  }
  
  it_res = std::copy(it_lhs, lhs.first.end(), it_res);
  it_res = std::copy(it_rhs, rhs.first.end(), it_res);  
  result.erase(it_res, result.end());
  
  return std::make_pair(result, lhs.second | rhs.second);
}


std::pair< std::vector< Set_Usage >, uint > Tag_Value_Pair_Operator::used_sets() const
{
  if (lhs && rhs)
    return union_usage(lhs->used_sets(), rhs->used_sets());
  else if (lhs)
    return lhs->used_sets();
  else if (rhs)
    return rhs->used_sets();
  return std::make_pair(std::vector< Set_Usage >(), 0u);
}


std::vector< std::string > Tag_Value_Pair_Operator::used_tags() const
{
  std::vector< std::string > lhs_result;
  if (lhs)
    lhs->used_tags().swap(lhs_result);
  std::vector< std::string > rhs_result;
  if (rhs)
    rhs->used_tags().swap(rhs_result);
  
  std::vector< std::string > result(lhs_result.size() + rhs_result.size());
  result.erase(std::set_union(lhs_result.begin(), lhs_result.end(), rhs_result.begin(), rhs_result.end(),
      result.begin()), result.end());
  return result;
}
  

void Tag_Value_Pair_Operator::clear()
{
  if (lhs)
    lhs->clear();
  if (rhs)
    rhs->clear();
}


//-----------------------------------------------------------------------------


Tag_Value_Prefix_Operator::Tag_Value_Prefix_Operator(int line_number_) : Tag_Value(line_number_), rhs(0) {}


void Tag_Value_Prefix_Operator::add_statement(Statement* statement, std::string text)
{
  Tag_Value* tag_value_ = dynamic_cast< Tag_Value* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one tag-value substatements.");
}


std::string Tag_Value_Prefix_Operator::eval(const std::string* key)
{
  return process(rhs ? rhs->eval(key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}


std::string Tag_Value_Prefix_Operator::eval(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::string* key)
{
  return process(rhs ? rhs->eval(elem, tags, key) : "");
}

    
void Tag_Value_Prefix_Operator::prefetch(const Set_With_Context& set)
{
  if (rhs)
    rhs->prefetch(set);
}
  

std::pair< std::vector< Set_Usage >, uint > Tag_Value_Prefix_Operator::used_sets() const
{
  if (rhs)
    return rhs->used_sets();
  return std::make_pair(std::vector< Set_Usage >(), 0u);
}


std::vector< std::string > Tag_Value_Prefix_Operator::used_tags() const
{
  if (rhs)
    return rhs->used_tags();
  
  return std::vector< std::string >();
}
  

void Tag_Value_Prefix_Operator::clear()
{
  if (rhs)
    rhs->clear();
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_And > Tag_Value_And::statement_maker("value-and");


Tag_Value_And::Tag_Value_And
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_And::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d && rhs_d ? "1" : "0";
  
  return (lhs_s != "") && (rhs_s != "") ? "1" : "0";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Or > Tag_Value_Or::statement_maker("value-or");


Tag_Value_Or::Tag_Value_Or
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Or::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d || rhs_d ? "1" : "0";
  
  return (lhs_s != "") || (rhs_s != "") ? "1" : "0";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Not > Tag_Value_Not::statement_maker("value-not");


Tag_Value_Not::Tag_Value_Not
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Prefix_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Not::process(const std::string& rhs_s) const
{
  double rhs_d = 0;  
  if (try_double(rhs_s, rhs_d))
    return !rhs_d ? "1" : "0";
  
  return rhs_s == "" ? "1" : "0";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Equal > Tag_Value_Equal::statement_maker("value-equal");


Tag_Value_Equal::Tag_Value_Equal
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Equal::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l == rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d == rhs_d ? "1" : "0";
  
  return lhs_s == rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Less > Tag_Value_Less::statement_maker("value-less");


Tag_Value_Less::Tag_Value_Less
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Less::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l < rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d < rhs_d ? "1" : "0";
  
  return lhs_s < rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Plus > Tag_Value_Plus::statement_maker("value-plus");


Tag_Value_Plus::Tag_Value_Plus
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Plus::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return to_string(lhs_l + rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d + rhs_d);
  
  return lhs_s + rhs_s;
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Negate > Tag_Value_Negate::statement_maker("value-negate");


Tag_Value_Negate::Tag_Value_Negate
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Prefix_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Negate::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;  
  if (try_int64(rhs_s, rhs_l))
    return to_string(-rhs_l);
  
  double rhs_d = 0;  
  if (try_double(rhs_s, rhs_d))
    return to_string(-rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Minus > Tag_Value_Minus::statement_maker("value-minus");


Tag_Value_Minus::Tag_Value_Minus
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Minus::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return to_string(lhs_l - rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d - rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Times > Tag_Value_Times::statement_maker("value-times");


Tag_Value_Times::Tag_Value_Times
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Times::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return to_string(lhs_l * rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d * rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Divided > Tag_Value_Divided::statement_maker("value-divided");


Tag_Value_Divided::Tag_Value_Divided
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Divided::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  // On purpose no int64 detection  
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d / rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Tag_Value_Aggregator::Tag_Value_Aggregator
    (const string& func_name, int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
    : Tag_Value(line_number_), rhs(0), input_set(0), value_set(false)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  
  eval_attributes_array(func_name, attributes, input_attributes);
  
  input = attributes["from"];  
}


void Tag_Value_Aggregator::add_statement(Statement* statement, std::string text)
{
  Tag_Value* tag_value_ = dynamic_cast< Tag_Value* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one tag-value substatements.");
}


template< typename Index, typename Maybe_Attic, typename Object >
void eval_elems(Tag_Value_Aggregator* aggregator, const std::map< Index, std::vector< Maybe_Attic > >& elems,
    Tag_Store< Index, Object >* tag_store, const std::string* key)
{
  for (typename std::map< Index, std::vector< Maybe_Attic > >::const_iterator idx_it = elems.begin();
      idx_it != elems.end(); ++idx_it)
  {
    for (typename std::vector< Maybe_Attic >::const_iterator elem_it = idx_it->second.begin();
        elem_it != idx_it->second.end(); ++elem_it)
    {
      std::string value = aggregator->rhs->eval(&*elem_it, tag_store->get(idx_it->first, *elem_it), key);
      
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

  
std::string Tag_Value_Aggregator::eval_input(const std::string* key)
{
  if (!input_set || !rhs)
    return "";
  
  value_set = false;
  eval_elems(this, input_set->base->nodes, input_set->tag_store_nodes, key);
  eval_elems(this, input_set->base->attic_nodes, input_set->tag_store_attic_nodes, key);
  eval_elems(this, input_set->base->ways, input_set->tag_store_ways, key);
  eval_elems(this, input_set->base->attic_ways, input_set->tag_store_attic_ways, key);
  eval_elems(this, input_set->base->relations, input_set->tag_store_relations, key);
  eval_elems(this, input_set->base->attic_relations, input_set->tag_store_attic_relations, key);
  eval_elems(this, input_set->base->areas, input_set->tag_store_areas, key);
  eval_elems(this, input_set->base->deriveds, input_set->tag_store_deriveds, key);
  
  return value;
}


void Tag_Value_Aggregator::prefetch(const Set_With_Context& set)
{
  if (set.name == input)
    input_set = &set;
}

  
std::pair< std::vector< Set_Usage >, uint > Tag_Value_Aggregator::used_sets() const
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


Generic_Statement_Maker< Tag_Value_Union_Value > Tag_Value_Union_Value::statement_maker("value-union-value");


Tag_Value_Union_Value::Tag_Value_Union_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-union-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Union_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  if (agg_value == new_value)
    return agg_value;
  else
    return "< multiple values found >";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Min_Value > Tag_Value_Min_Value::statement_maker("value-min-value");


Tag_Value_Min_Value::Tag_Value_Min_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-min-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Min_Value::update_value(const std::string& agg_value, const std::string& new_value)
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


Generic_Statement_Maker< Tag_Value_Max_Value > Tag_Value_Max_Value::statement_maker("value-max-value");


Tag_Value_Max_Value::Tag_Value_Max_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-max-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Max_Value::update_value(const std::string& agg_value, const std::string& new_value)
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


Generic_Statement_Maker< Tag_Value_Sum_Value > Tag_Value_Sum_Value::statement_maker("value-sum-value");


Tag_Value_Sum_Value::Tag_Value_Sum_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-sum-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Sum_Value::update_value(const std::string& agg_value, const std::string& new_value)
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


Generic_Statement_Maker< Tag_Value_Set_Value > Tag_Value_Set_Value::statement_maker("value-set-value");


Tag_Value_Set_Value::Tag_Value_Set_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-set-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Set_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  if (values.empty())
    values.push_back(agg_value);
  values.push_back(new_value);
  
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  
  std::string result;
  std::vector< std::string >::const_iterator it = values.begin();
  if (it != values.end())
  {
    result = *it;
    ++it;
  }
  for (; it != values.end(); ++it)
    result += ";" + *it;
  return result;
}


void Tag_Value_Set_Value::clear()
{
  values.clear();
}
