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
    : Output_Statement(line_number_), multi_evaluator(0)
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
  {
    if (set_tag->get_key())
    {
      for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      {
        if ((*it)->get_key() && *(*it)->get_key() == *set_tag->get_key())
          add_static_error(std::string("A key cannot be added twice to an element: \"") + *set_tag->get_key() + '\"');
      }
    }
    else if (!multi_evaluator)
      multi_evaluator = set_tag;
    else
      add_static_error("A make statement can have at most one any-key set-tag statement.");
    evaluators.push_back(set_tag);
  }
  else
    substatement_error(get_name(), statement);
}


template< typename Index, typename Object >
void notify_tags(Transaction& transaction, const std::string& set_name,
    const std::map< Index, std::vector< Object > >& items,
    std::vector< Set_Tag_Statement* >& evaluators, std::vector< std::string >* found_keys)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);
  
  for (typename std::map< Index, std::vector< Object > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Object >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const std::vector< std::pair< std::string, std::string > >* tags =
          tag_store.get(it_idx->first, *it_elem);
      if (!tags)
        continue;
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if ((*it_evals)->get_tag_value() && (*it_evals)->get_tag_value()->needs_tags(set_name))
          (*it_evals)->get_tag_value()->tag_notice(set_name, *it_elem, tags);
      }
      
      if (found_keys)
      {
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = tags->begin();
            it_keys != tags->end(); ++it_keys)
          found_keys->push_back(it_keys->first);
      }
    }
    
    if (found_keys)
    {
      std::sort(found_keys->begin(), found_keys->end());
      found_keys->erase(std::unique(found_keys->begin(), found_keys->end()), found_keys->end());
    }
  } 
}


template< typename Index, typename Object >
void notify_tags(Transaction& transaction, const std::string& set_name,
    const std::map< Index, std::vector< Attic< Object > > >& items,
    std::vector< Set_Tag_Statement* >& evaluators, std::vector< std::string >* found_keys)
{
  Tag_Store< Index, Object > tag_store(transaction);
  tag_store.prefetch_all(items);
  
  for (typename std::map< Index, std::vector< Attic< Object > > >::const_iterator it_idx = items.begin();
      it_idx != items.end(); ++it_idx)
  {
    for (typename std::vector< Attic< Object > >::const_iterator it_elem = it_idx->second.begin();
        it_elem != it_idx->second.end(); ++it_elem)
    {
      const std::vector< std::pair< std::string, std::string > >* tags =
          tag_store.get(it_idx->first, *it_elem);
      if (!tags)
        continue;
      for (std::vector< Set_Tag_Statement* >::const_iterator it_evals = evaluators.begin();
          it_evals != evaluators.end(); ++it_evals)
      {
        if ((*it_evals)->get_tag_value() && (*it_evals)->get_tag_value()->needs_tags(set_name))
          (*it_evals)->get_tag_value()->tag_notice(set_name, *it_elem, tags);
      }
      
      if (found_keys)
      {
        for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = tags->begin();
            it_keys != tags->end(); ++it_keys)
          found_keys->push_back(it_keys->first);
      }
    }
    
    if (found_keys)
    {
      std::sort(found_keys->begin(), found_keys->end());
      found_keys->erase(std::unique(found_keys->begin(), found_keys->end()), found_keys->end());
    }
  } 
}


void Make_Statement::execute(Resource_Manager& rman)
{
  std::vector< std::string > declared_keys;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
  {
    if ((*it)->get_key())
      declared_keys.push_back(*(*it)->get_key());
  }
  std::sort(declared_keys.begin(), declared_keys.end());
  declared_keys.erase(std::unique(declared_keys.begin(), declared_keys.end()), declared_keys.end());

  for (std::map< std::string, Set >::const_iterator it_set = rman.sets().begin(); it_set != rman.sets().end();
      ++it_set)
  {
    bool needs_tags = multi_evaluator;
    for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
        needs_tags |= (*it)->get_tag_value() && (*it)->get_tag_value()->needs_tags(it_set->first);
    
    if (needs_tags)
    {
      std::map< std::string, Set >::const_iterator mit(rman.sets().find(it_set->first));
      
      bool multi_evaluator_ = multi_evaluator && multi_evaluator->get_input_name() == it_set->first;
      std::vector< std::string > found_keys;
      
      notify_tags< Uint32_Index, Node_Skeleton >(
          *rman.get_transaction(), it_set->first, mit->second.nodes, evaluators,
          multi_evaluator_ ? &found_keys : 0);
      if (rman.get_desired_timestamp() != NOW)
        notify_tags< Uint32_Index, Node_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.attic_nodes, evaluators,
            multi_evaluator_ ? &found_keys : 0);
      notify_tags< Uint31_Index, Way_Skeleton >(
          *rman.get_transaction(), it_set->first, mit->second.ways, evaluators,
          multi_evaluator_ ? &found_keys : 0);
      if (rman.get_desired_timestamp() != NOW)
        notify_tags< Uint31_Index, Way_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.attic_ways, evaluators,
            multi_evaluator_ ? &found_keys : 0);
      notify_tags< Uint31_Index, Relation_Skeleton >(
          *rman.get_transaction(), it_set->first, mit->second.relations, evaluators,
          multi_evaluator_ ? &found_keys : 0);
      if (rman.get_desired_timestamp() != NOW)
        notify_tags< Uint31_Index, Relation_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.attic_relations, evaluators,
            multi_evaluator_ ? &found_keys : 0);
      if (!mit->second.areas.empty())
        notify_tags< Uint31_Index, Area_Skeleton >(
            *rman.get_transaction(), it_set->first, mit->second.areas, evaluators,
            multi_evaluator_ ? &found_keys : 0);
      notify_tags< Uint31_Index, Derived_Structure >(
          *rman.get_transaction(), it_set->first, mit->second.deriveds, evaluators,
          multi_evaluator_ ? &found_keys : 0);
      
      if (multi_evaluator_)
      {
        found_keys.erase(std::set_difference(found_keys.begin(), found_keys.end(),
            declared_keys.begin(), declared_keys.end(), found_keys.begin()), found_keys.end());
        multi_evaluator->set_keys(found_keys);
      }
    }
  }
  
  Set into;
  
  std::vector< std::pair< std::string, std::string > > tags;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
  {
    if (!(*it)->get_tag_value())
        continue;
    
    if ((*it)->get_key())
      tags.push_back(std::make_pair(*(*it)->get_key(), (*it)->eval(rman.sets(), 0)));
    else
    {
      const std::vector< std::string >& keys = *(*it)->get_keys();
      for (std::vector< std::string >::const_iterator it_keys = keys.begin(); it_keys != keys.end(); ++it_keys)
        tags.push_back(std::make_pair(*it_keys, (*it)->eval(rman.sets(), &*it_keys)));
    }
    (*it)->get_tag_value()->clear();
  }

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
  attributes["from"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  if (attributes["k"] != "")
    keys.push_back(attributes["k"]);
  else
    input = attributes["from"];
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


std::string Tag_Value_Minus::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";
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


std::string Tag_Value_Times::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";
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


std::string Tag_Value_Divided::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  std::string lhs_s = lhs ? lhs->eval(sets, key) : "";
  std::string rhs_s = rhs ? rhs->eval(sets, key) : "";
  double lhs_d = 0;
  double rhs_d = 0;
  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d / rhs_d);
  else
    return "NaN";
}


//-----------------------------------------------------------------------------


Tag_Value_Aggregator::Tag_Value_Aggregator
    (const string& func_name, int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings)
    : Tag_Value(line_number_), value_set(false)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["k"] = "";
  attributes["generic"] = "no";
  
  eval_attributes_array(func_name, attributes, input_attributes);
  
  input = attributes["from"];
  key = attributes["k"];
  generic = (attributes["generic"] == "yes");
  
  if (generic)
  {
    if (key != "")
      add_static_error(std::string("In statement \"") + func_name + "\" the attribute \"generic\" must have the value "
          "\"no\" if the attribute \"k\" is a non-empty string.");      
  }
  else if (!(attributes["generic"] == "no"))
    add_static_error(std::string("In statement \"") + func_name + "\" the attribute \"generic\" must have the value "
        "\"yes\" or the value \"no\". \"no\" would be taken as default.");
}


void Tag_Value_Aggregator::update_value(const std::vector< std::pair< std::string, std::string > >* tags)
{
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
    else if (generic)
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
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Attic< Node_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Way_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Attic< Way_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Relation_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Attic< Relation_Skeleton >& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Area_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }

void Tag_Value_Aggregator::tag_notice(const std::string& set_name, const Derived_Skeleton& elem,
      const std::vector< std::pair< std::string, std::string > >* tags) { update_value(tags); }


std::string Tag_Value_Aggregator::eval(const std::map< std::string, Set >& sets, const std::string* key) const
{
  if (key)
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
  double lhs_d = 0;
  double rhs_d = 0;
  if (try_double(agg_value, lhs_d) && try_double(new_value, rhs_d))
    return lhs_d < rhs_d ? agg_value : new_value;
  
  return std::min(agg_value, new_value);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Tag_Value_Max_Value > Tag_Value_Max_Value::statement_maker("value-max-value");


Tag_Value_Max_Value::Tag_Value_Max_Value
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value_Aggregator("value-max-value", line_number_, input_attributes, global_settings) {}


std::string Tag_Value_Max_Value::update_value(const std::string& agg_value, const std::string& new_value)
{
  double lhs_d = 0;
  double rhs_d = 0;
  if (try_double(agg_value, lhs_d) && try_double(new_value, rhs_d))
    return lhs_d > rhs_d ? agg_value : new_value;
  
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
  
  if (key)
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


void Tag_Value_Set_Value::update_value(const std::vector< std::pair< std::string, std::string > >* tags)
{
  if (!tags)
    return;
  
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = tags->begin();
      it != tags->end(); ++it)
  {
    if (it->first == key)
      values.push_back(it->second);
    else if (generic)
      values_per_key[it->first].push_back(it->second);
  }
}


void Tag_Value_Set_Value::clear()
{
  values.clear();
  values_per_key.clear();
}
