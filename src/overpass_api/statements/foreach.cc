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

#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "foreach.h"

using namespace std;

Generic_Statement_Maker< Foreach_Statement > Foreach_Statement::statement_maker("foreach");

Foreach_Statement::Foreach_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  output = attributes["into"];
}

void Foreach_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  substatements.push_back(statement);
}


void Foreach_Statement::execute(Resource_Manager& rman)
{
  Set base_set(rman.sets()[input]);
  rman.push_reference(base_set);
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(base_set.nodes.begin()); it != base_set.nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      
      rman.sets()[output].attic_nodes.clear();
      rman.sets()[output].attic_ways.clear();
      rman.sets()[output].attic_relations.clear();
      
      rman.sets()[output].areas.clear();
      
      rman.sets()[output].nodes[it->first].push_back(*it2);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
	(*it)->execute(rman);
    }
  }
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(base_set.ways.begin()); it != base_set.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      
      rman.sets()[output].attic_nodes.clear();
      rman.sets()[output].attic_ways.clear();
      rman.sets()[output].attic_relations.clear();
      
      rman.sets()[output].areas.clear();

      rman.sets()[output].ways[it->first].push_back(*it2);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
	(*it)->execute(rman);
    }
  }
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
    it(base_set.relations.begin()); it != base_set.relations.end(); ++it)
  {
    for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      
      rman.sets()[output].attic_nodes.clear();
      rman.sets()[output].attic_ways.clear();
      rman.sets()[output].attic_relations.clear();
      
      rman.sets()[output].areas.clear();

      rman.sets()[output].relations[it->first].push_back(*it2);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
	(*it)->execute(rman);
    }
  }
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
    it(base_set.areas.begin()); it != base_set.areas.end(); ++it)
  {
    for (vector< Area_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      
      rman.sets()[output].attic_nodes.clear();
      rman.sets()[output].attic_ways.clear();
      rman.sets()[output].attic_relations.clear();
            
      rman.sets()[output].areas.clear();
      
      rman.sets()[output].areas[it->first].push_back(*it2);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
	(*it)->execute(rman);
    }
  }
  
  if (input == output)
    rman.sets()[output] = base_set;
  
  rman.pop_reference();
  rman.health_check(*this);
}
