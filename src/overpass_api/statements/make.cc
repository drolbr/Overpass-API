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

#include "make.h"


Generic_Statement_Maker< Make_Statement > Make_Statement::statement_maker("make");

Make_Statement::Make_Statement
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
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
  Set_Tag_Statement* set_tag = dynamic_cast<Set_Tag_Statement*>(statement);
  if (set_tag)
    evaluators.push_back(set_tag);
  else
    substatement_error(get_name(), statement);
}


void Make_Statement::execute(Resource_Manager& rman)
{
  Set from;
  Set into;
  
  std::vector< std::pair< std::string, std::string > > tags;
  for (std::vector< Set_Tag_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
    tags.push_back(std::make_pair((*it)->get_key(), (*it)->eval(from)));

  into.deriveds[Uint31_Index(0u)].push_back(Derived_Structure(type, Uint64(0ull), tags));
  
  transfer_output(rman, into);
  rman.health_check(*this);
}


//-----------------------------------------------------------------------------


Generic_Statement_Maker< Set_Tag_Statement > Set_Tag_Statement::statement_maker("set-tag");


Set_Tag_Statement::Set_Tag_Statement
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["v"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  key = attributes["k"];
  value = attributes["v"];
  
  if (key == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"set-tag\""
        <<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
}
