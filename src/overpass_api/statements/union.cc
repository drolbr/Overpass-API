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

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "union.h"

using namespace std;

Generic_Statement_Maker< Union_Statement > Union_Statement::statement_maker("union");

Union_Statement::Union_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["into"];
}

void Union_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if (statement->get_result_name() != "")
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Union_Statement::forecast()
{
}

void Union_Statement::execute(Resource_Manager& rman)
{
  Set base_set;
  rman.push_reference(base_set);
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(base_set.nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways(base_set.ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(base_set.relations);
  map< Uint31_Index, vector< Area_Skeleton > >& areas(base_set.areas);
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->execute(rman);
    
    Set& summand(rman.sets()[(*it)->get_result_name()]);

    indexed_set_union(nodes, summand.nodes);
    indexed_set_union(ways, summand.ways);
    indexed_set_union(relations, summand.relations);
    indexed_set_union(areas, summand.areas);
  }
  
  rman.sets()[output] = base_set;
  
  rman.pop_reference();
  rman.health_check(*this);
}
