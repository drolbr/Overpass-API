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


#include "../data/utils.h"
#include "make.h"


Generic_Statement_Maker< Make_Statement > Make_Statement::statement_maker("make");

Make_Statement::Make_Statement
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
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


void Make_Statement::add_statement(Statement* statement, string text)
{
  Set_Tag_Statement* set_tag = dynamic_cast< Set_Tag_Statement* >(statement);
  if (set_tag)
    evaluators.push_back(set_tag);
  else
    substatement_error(get_name(), statement);
}


void Make_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  std::vector< std::pair< std::string, std::string > > tags;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    tags.push_back(std::make_pair((*it)->get_key(), (*it)->eval(rman.sets())));

  into.deriveds[Uint31_Index(0u)].push_back(Derived_Structure(type, Uint64(0ull), tags));
  
  transfer_output(rman, into);
  rman.health_check(*this);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Set_Tag_Statement > Set_Tag_Statement::statement_maker("set-tag");


Set_Tag_Statement::Set_Tag_Statement
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), tag_value(0)
{
  map< string, string > attributes;
  
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


void Set_Tag_Statement::add_statement(Statement* statement, string text)
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
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  map< string, string > attributes;
  
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
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_)
{
  map< string, string > attributes;
  
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


Generic_Statement_Maker< Tag_Value_Plus > Tag_Value_Plus::statement_maker("value-plus");


Tag_Value_Plus::Tag_Value_Plus
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_), lhs(0), rhs(0)
{
  map< string, string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


void Tag_Value_Plus::add_statement(Statement* statement, string text)
{
  Tag_Value* tag_value_ = dynamic_cast< Tag_Value* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!lhs)
    lhs = tag_value_;
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error("value-plus must have exactly two tag-value substatements.");
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


Generic_Statement_Maker< Tag_Value_Times > Tag_Value_Times::statement_maker("value-times");


Tag_Value_Times::Tag_Value_Times
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Tag_Value(line_number_), lhs(0), rhs(0)
{
  map< string, string > attributes;  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


void Tag_Value_Times::add_statement(Statement* statement, string text)
{
  Tag_Value* tag_value_ = dynamic_cast< Tag_Value* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!lhs)
    lhs = tag_value_;
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error("value-times must have exactly two tag-value substatements.");
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
