/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht, 2014 mmd
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

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include <iomanip>

#include "../../template_db/block_backend.h"
#include "map_to_area.h"

using namespace std;

bool Map_To_Area_Statement::is_used_ = false;

Generic_Statement_Maker< Map_To_Area_Statement > Map_To_Area_Statement::statement_maker("map-to-area");

Map_To_Area_Statement::Map_To_Area_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  is_used_ = true;

  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  set_output(attributes["into"]);
 
}

void Map_To_Area_Statement::execute(Resource_Manager& rman)
{ 
  Set into;

  transfer_output(rman, into);
  rman.health_check(*this);
}
