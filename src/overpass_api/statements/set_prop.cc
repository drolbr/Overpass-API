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
#include "set_prop.h"


Generic_Statement_Maker< Set_Prop_Statement > Set_Prop_Statement::statement_maker("set-prop");


Set_Prop_Statement::Set_Prop_Statement
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
      add_static_error("For the statement \"set-prop\" in mode \"keytype\"=\"tag\", "
          "the attribute \"k\" must be nonempty.");
  }
  else if (attributes["keytype"] == "id")
    set_id = true;
  else if (attributes["keytype"] != "generic")
    add_static_error("For the attribute \"keytype\" of the element \"set-prop\""
        " the only allowed values are \"tag\", \"id\", or \"generic\".");
}


void Set_Prop_Statement::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (tag_value_ && !tag_value)
    tag_value = tag_value_;
  else if (tag_value)
    add_static_error("set-prop must have exactly one evaluator substatement.");
  else
    substatement_error(get_name(), statement);
}


std::pair< std::vector< Set_Usage >, uint > Set_Prop_Statement::used_sets() const
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


Set_Prop_Task* Set_Prop_Statement::get_task(const Prepare_Task_Context& context)
{
  Eval_Task* rhs_task = tag_value ? tag_value->get_task(context) : 0;
  return new Set_Prop_Task(rhs_task, keys.empty() ? "" : keys.front(),
      set_id ? Set_Prop_Task::set_id : keys.empty() ? Set_Prop_Task::generic : Set_Prop_Task::single_key);
}


void Set_Prop_Task::process(Derived_Structure& result, bool& id_set) const
{
  if (!rhs)
    return;
  
  if (mode == single_key)
    result.tags.push_back(std::make_pair(key, rhs->eval(0)));
  else if (mode == set_id && !id_set)
  {
    int64 id = 0;
    id_set |= try_int64(rhs->eval(0), id);
    if (id_set)
      result.id = Uint64(id);
  }
}


template< typename Object >
void process(const std::string& key, Set_Prop_Task::Mode mode, Eval_Task* rhs, const Object* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set)
{
  if (!rhs)
    return;
  
  if (mode == Set_Prop_Task::single_key)
    result.tags.push_back(std::make_pair(key, rhs->eval(elem, tags, 0)));
  else if (mode == Set_Prop_Task::generic)
  {
    if (tags)
    {
      std::vector< std::string > found_keys;
      for (std::vector< std::pair< std::string, std::string > >::const_iterator it_keys = tags->begin();
          it_keys != tags->end(); ++it_keys)
        found_keys.push_back(it_keys->first);
      std::sort(found_keys.begin(), found_keys.end());
      found_keys.erase(std::unique(found_keys.begin(), found_keys.end()), found_keys.end());
      found_keys.erase(std::set_difference(found_keys.begin(), found_keys.end(),
          declared_keys.begin(), declared_keys.end(), found_keys.begin()), found_keys.end());
        
      for (std::vector< std::string >::const_iterator it_keys = found_keys.begin();
          it_keys != found_keys.end(); ++it_keys)
        result.tags.push_back(std::make_pair(*it_keys, rhs->eval(elem, tags, &*it_keys)));
    }
  }
  else if (!id_set)
  {
    int64 id = 0;
    id_set |= try_int64(rhs->eval(0), id);
    if (id_set)
      result.id = Uint64(id);
  }
}

  
void Set_Prop_Task::process(const Node_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Attic< Node_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Way_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Attic< Way_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Relation_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Attic< Relation_Skeleton >* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Area_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}


void Set_Prop_Task::process(const Derived_Skeleton* elem,
    const std::vector< std::pair< std::string, std::string > >* tags, const std::vector< std::string >& declared_keys,
    Derived_Structure& result, bool& id_set) const
{
  ::process(key, mode, rhs, elem, tags, declared_keys, result, id_set);
}
