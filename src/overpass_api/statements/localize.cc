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
#include "localize.h"
#include "set_prop.h"


Generic_Statement_Maker< Localize_Statement > Localize_Statement::statement_maker("localize");


Localize_Statement::Localize_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), type(data)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "l";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
  input = attributes["from"];
  
  if (attributes["type"] == "ll")
    type = also_loose;
  else if (attributes["type"] == "llb")
    type = all;
  else if (attributes["type"] != "l")
    add_static_error("Localize must have one of the values \"l\", \"ll\", or \"llb\". \"l\" is the default value.");
}


void Localize_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  transfer_output(rman, into);
  rman.health_check(*this);
}
