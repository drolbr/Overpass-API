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


Generic_Statement_Maker< Set_Tag_Statement > Set_Tag_Statement::statement_maker("set-tag");


Set_Tag_Statement::Set_Tag_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), set_id(false), tag_value(0)
{
  std::map< std::string, std::string > attributes;
  
  attributes["k"] = "";
  attributes["from"] = "_";
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
  else if (attributes["keytype"] == "generic")
  {
    if (attributes["from"] != "")
      input = attributes["from"];
    else
      add_static_error("For the statement \"set-tag\" in mode \"keytype\"=\"tag\", "
          "the attribute \"from\" must be nonempty. Default is \"_\".");
  }
  else if (attributes["keytype"] == "id")
    set_id = true;
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


std::string Tag_Value_Fixed::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  return value;
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


uint Tag_Value_Count::needs_tags(const std::string& set_name) const
{
  if (set_name != input)
    return 0;
  
  if (to_count == tags)
    return TAGS;
  else if (to_count == members)
    return SKELETON;
  return 0;
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
  else if (to_count == Tag_Value_Count::members)
    counter += elem.nds.size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
  else if (to_count == Tag_Value_Count::members)
    counter += elem.nds.size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
  else if (to_count == Tag_Value_Count::members)
    counter += elem.members.size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
  else if (to_count == Tag_Value_Count::members)
    counter += elem.members.size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
}


void Tag_Value_Count::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (to_count == Tag_Value_Count::tags && tags)
    counter += tags->size();
}


std::string Tag_Value_Count::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::map< std::string, Set >::const_iterator mit(sets.find(input));
  if (mit == sets.end())
    return "";
  const Set& from = mit->second;
    
  if (to_count == nodes)
    return to_string(count(from.nodes) + count(from.attic_nodes));
  else if (to_count == ways)
    return to_string(count(from.ways) + count(from.attic_ways));
  else if (to_count == relations)
    return to_string(count(from.relations) + count(from.attic_relations));
  else if (to_count == deriveds)
    return to_string(count(from.areas) + count(from.deriveds));
  else if (to_count == tags || to_count == members)
    return to_string(counter);
  return "0";
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


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (lhs)
    lhs->tag_notice(set_name, elem, tags);
  if (rhs)
    rhs->tag_notice(set_name, elem, tags);
}


void Tag_Value_Pair_Operator::clear()
{
  if (lhs)
    lhs->clear();
  if (rhs)
    rhs->clear();
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


std::string Tag_Value_Plus::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";
  
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


Generic_Statement_Maker< Tag_Value_Minus > Tag_Value_Minus::statement_maker("value-minus");


Tag_Value_Minus::Tag_Value_Minus
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Minus::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";
  
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


std::string Tag_Value_Times::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";
  
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


std::string Tag_Value_Divided::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";

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
    : Tag_Value(line_number_), key_type(tag), value_set(false)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["k"] = "";
  attributes["keytype"] = "tag";
  
  eval_attributes_array(func_name, attributes, input_attributes);
  
  input = attributes["from"];
  key = attributes["k"];
  
  if (attributes["keytype"] == "tag")
    ;
  else if (attributes["keytype"] == "generic")
    key_type = generic;
  else if (attributes["keytype"] == "id")
    key_type = id;
  else if (attributes["keytype"] == "type")
    key_type = type;
  else
    add_static_error(std::string("In statement \"") + func_name + "\" the attribute \"keytype\" must have the value "
        "\"tag\", \"generic\", \"id\", or \"type\". \"tag\" will be taken as default.");
  
  if (key_type != tag)
  {
    if (key != "")
      add_static_error(std::string("In statement \"") + func_name + "\" the attribute \"generic\" must have the value "
          "\"no\" if the attribute \"k\" is a non-empty string.");      
  }  
}


void Tag_Value_Aggregator::update_value(const std::string& id, const std::string& type,
    const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (key_type == Tag_Value_Aggregator::id)
  {
    if (value_set)
      value = update_value(value, id);
    else
    {
      value_set = true;
      value = id;
    }
    return;
  }
  else if (key_type == Tag_Value_Aggregator::type)
  {
    if (value_set)
      value = update_value(value, type);
    else
    {
      value_set = true;
      value = type;
    }
    return;
  }
  
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
    {
      if (value_set)
        value = update_value(value, it->second);
      else
      {
        value_set = true;
        value = it->second;
      }
    }
    else if (key_type == generic)
    {
      std::map< std::string, std::string >::iterator it_tag = value_per_key.find(it->first);
      
      if (it_tag == value_per_key.end())
        value_per_key.insert(*it);
      else if (it_tag->second != it->second)
        it_tag->second = update_value(it_tag->second, it->second);
    }
  }
}


void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "node", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "node", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "way", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "way", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "relation", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "relation", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), "area", tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(to_string(elem.id.val()), elem.type_name, tags); }


std::string Tag_Value_Aggregator::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  if (key && key_type == Tag_Value_Aggregator::generic)
    return value_per_key[*key];
  else
    return value;
}


void Tag_Value_Aggregator::clear()
{
  value_set = false;
  value = "";
  value_per_key.clear();
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


Generic_Statement_Maker< Tag_Value_Set_Value > Tag_Value_Set_Value::statement_maker("value-set-value");


Tag_Value_Set_Value::Tag_Value_Set_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-set-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Set_Value::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::vector< std::string >* values_ = 0;
  
  if (key && key_type == Tag_Value_Aggregator::generic)
  {
    std::map< std::string, std::vector< std::string > >::iterator it = values_per_key.find(*key);
    if (it == values_per_key.end())
      return "";
    values_ = &it->second;
  }
  else
    values_ = &values;
  
  std::sort(values_->begin(), values_->end());
  values_->erase(std::unique(values_->begin(), values_->end()), values_->end());
  
  std::string result;
  
  std::vector< std::string >::const_iterator it = values_->begin();
  if (it != values_->end())
  {
    result = *it;
    ++it;
  }
  for (; it != values_->end(); ++it)
    result += ";" + *it;
  
  return result;
}


void Tag_Value_Set_Value::update_value(const std::string& id, const std::string& type,
    const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (key_type == Tag_Value_Aggregator::id)
  {
    values.push_back(id);
    return;
  }
  else if (key_type == Tag_Value_Aggregator::type)
  {
    values.push_back(type);
    return;
  }
  
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      values.push_back(it->second);
    else if (key_type == Tag_Value_Aggregator::generic)
      values_per_key[it->first].push_back(it->second);
  }
}


void Tag_Value_Set_Value::clear()
{
  values.clear();
  values_per_key.clear();
}
