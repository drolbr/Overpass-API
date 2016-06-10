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
#include "make.h"


Generic_Statement_Maker< Make_Statement > Make_Statement::statement_maker("make");

Make_Statement::Make_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  
  if (attributes["type"] == "")
    add_static_error("The attribute type must be set to a nonempty string.");

  type = attributes["type"];
}


Make_Statement::~Make_Statement()
{
}


void Make_Statement::add_statement(Statement* statement, std::string text)
{
  Set_Tag_Statement* set_tag = dynamic_cast< Set_Tag_Statement* >(statement);
  if (set_tag)
    evaluators.push_back(set_tag);
  else
    substatement_error(get_name(), statement);
}


template< typename Index, typename Object >
void notify_tags(Transaction& transaction, const std::string& set_name,
    const std::map< Index, std::vector< Object > >& items, std::vector< Set_Tag_Statement* >& evaluators)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);
  
  for (typename std::map< Index, std::vector< Object > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if ((*it_evals)->get_tag_value()->needs_tags(set_name))
          (*it_evals)->get_tag_value()->tag_notice(set_name, *it_elem, tag_store.get(it_idx->first, *it_elem));
      }
    }
  } 
}


template< typename Index, typename Object >
void notify_tags(Transaction& transaction, const std::string& set_name,
    const std::map< Index, std::vector< Attic< Object > > >& items, std::vector< Set_Tag_Statement* >& evaluators)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);
  
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if ((*it_evals)->get_tag_value()->needs_tags(set_name))
          (*it_evals)->get_tag_value()->tag_notice(set_name, *it_elem, tag_store.get(it_idx->first, *it_elem));
      }
    }
  } 
}


void Make_Statement::execute(Resource_Manager& rman)
{
  for (std::map< std::string, Set >::const_iterator it_set = rman.sets().begin(); it_set != rman.sets().end();
      ++it_set)
  {
    bool needs_tags = false;
    for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
        needs_tags |= (*it)->get_tag_value()->needs_tags(it_set->first);
    if (needs_tags)
    {
      std::map< std::string, Set >::const_iterator mit(rman.sets().find(it_set->first));
      notify_tags< Uint32_Index, Node_Skeleton >(
          *rman.get_transaction(), it_set->first, mit->second.nodes, evaluators);
      if (rman.get_desired_timestamp() != NOW)
        notify_tags< Uint32_Index, Node_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.attic_nodes, evaluators);
      notify_tags< Uint31_Index, Way_Skeleton >(
          *rman.get_transaction(), it_set->first, mit->second.ways, evaluators);
      if (rman.get_desired_timestamp() != NOW)
        notify_tags< Uint31_Index, Way_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.attic_ways, evaluators);
      notify_tags< Uint31_Index, Relation_Skeleton >(
          *rman.get_transaction(), it_set->first, mit->second.relations, evaluators);
      if (rman.get_desired_timestamp() != NOW)
        notify_tags< Uint31_Index, Relation_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.attic_relations, evaluators);
      if (!mit->second.areas.empty())
        notify_tags< Uint31_Index, Area_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.areas, evaluators);
      notify_tags< Uint31_Index, Derived_Structure >(
          *rman.get_transaction(), it_set->first, mit->second.deriveds, evaluators);
    }
  }
  
  Set into;
  
  std::vector< std::pair< std::string, std::string > > tags;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    tags.push_back(std::make_pair((*it)->get_key(), (*it)->eval(rman.sets())));

  into.deriveds[Uint31_Index(0u)].push_back(Derived_Structure(
      type, rman.get_global_settings().dispense_derived_id(), tags));
  
  transfer_output(rman, into);
  rman.health_check(*this);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Set_Tag_Statement > Set_Tag_Statement::statement_maker("set-tag");


Set_Tag_Statement::Set_Tag_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), tag_value(0)
{
  std::map< std::string, std::string > attributes;
  
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
  
  if (key == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"set-tag\""
        <<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
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
  
  if (value == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"v\" of the element \"value-fixed\""
        <<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
}


std::string Tag_Value_Fixed::eval(const std::map< std::string, Set >& sets) const
{
  return value;
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Count > Tag_Value_Count::statement_maker("value-count");


Tag_Value_Count::Tag_Value_Count
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
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
  else
  {
    ostringstream temp("");
    temp<<"For the attribute \"type\" of the element \"value-count\""
        <<" the only allowed values are \"nodes\", \"ways\", \"relations\", or \"deriveds\" strings.";
    add_static_error(temp.str());
  }
}


std::string Tag_Value_Count::eval(const std::map< std::string, Set >& sets) const
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


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Plus > Tag_Value_Plus::statement_maker("value-plus");


Tag_Value_Plus::Tag_Value_Plus
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Pair_Operator(line_number_)
{
  std::map< std::string, std::string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


std::string Tag_Value_Plus::eval(const std::map< std::string, Set >& sets) const
{
  std::string lhs_s = lhs ? lhs->eval(sets) : "";
  std::string rhs_s = rhs ? rhs->eval(sets) : "";
  double lhs_d = 0;
  double rhs_d = 0;
  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d + rhs_d);
  else
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


std::string Tag_Value_Minus::eval(const std::map< std::string, Set >& sets) const
{
  std::string lhs_s = lhs ? lhs->eval(sets) : "";
  std::string rhs_s = rhs ? rhs->eval(sets) : "";
  double lhs_d = 0;
  double rhs_d = 0;
  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d - rhs_d);
  else
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


std::string Tag_Value_Times::eval(const std::map< std::string, Set >& sets) const
{
  std::string lhs_s = lhs ? lhs->eval(sets) : "";
  std::string rhs_s = rhs ? rhs->eval(sets) : "";
  double lhs_d = 0;
  double rhs_d = 0;
  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d * rhs_d);
  else
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


std::string Tag_Value_Divided::eval(const std::map< std::string, Set >& sets) const
{
  std::string lhs_s = lhs ? lhs->eval(sets) : "";
  std::string rhs_s = rhs ? rhs->eval(sets) : "";
  double lhs_d = 0;
  double rhs_d = 0;
  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d / rhs_d);
  else
    return "NaN";
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Union_Value > Tag_Value_Union_Value::statement_maker("value-union-value");


Tag_Value_Union_Value::Tag_Value_Union_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_), unique(true)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  key = attributes["k"];
}


std::string Tag_Value_Union_Value::eval(const std::map< std::string, Set >& sets) const
{
  return value;
}


void update_value(const std::vector< std::pair< std::string, std::string > >* tags,
    const std::string& key, std::string& value, bool& unique)
{
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
    {
      if (it->second == value)
        ;
      else if (value == "")
        value = it->second;
      else
      {
        unique = false;
        value = "< multiple values found >";
      }
    }
  }
}


void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }

void Tag_Value_Union_Value::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value(tags, key, value, unique); }


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Min_Value > Tag_Value_Min_Value::statement_maker("value-min-value");


Tag_Value_Min_Value::Tag_Value_Min_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_), value_set(false)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  key = attributes["k"];
}


std::string Tag_Value_Min_Value::eval(const std::map< std::string, Set >& sets) const
{
  return value;
}


void update_value_min(const std::vector< std::pair< std::string, std::string > >* tags,
    const std::string& key, std::string& value, bool& value_set)
{
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
    {
      if (value_set)
        value = std::min(value, it->second);
      else
      {
        value_set = true;
        value = it->second;
      }
    }
  }
}


void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }

void Tag_Value_Min_Value::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_min(tags, key, value, value_set); }


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Max_Value > Tag_Value_Max_Value::statement_maker("value-max-value");


Tag_Value_Max_Value::Tag_Value_Max_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_), value_set(false)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  key = attributes["k"];
}


std::string Tag_Value_Max_Value::eval(const std::map< std::string, Set >& sets) const
{
  return value;
}


void update_value_max(const std::vector< std::pair< std::string, std::string > >* tags,
    const std::string& key, std::string& value, bool& value_set)
{
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
    {
      if (value_set)
        value = std::max(value, it->second);
      else
      {
        value_set = true;
        value = it->second;
      }
    }
  }
}


void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }

void Tag_Value_Max_Value::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_max(tags, key, value, value_set); }


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Set_Value > Tag_Value_Set_Value::statement_maker("value-set-value");


Tag_Value_Set_Value::Tag_Value_Set_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["k"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  key = attributes["k"];
}


std::string Tag_Value_Set_Value::eval(const std::map< std::string, Set >& sets) const
{
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


void update_value_set(const std::vector< std::pair< std::string, std::string > >* tags,
    const std::string& key, std::vector< std::string >& values)
{
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      values.push_back(it->second);
  }
}


void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Node_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }

void Tag_Value_Set_Value::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags)
{ update_value_set(tags, key, values); }
