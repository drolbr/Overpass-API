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
#include "set_tag.h"


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
