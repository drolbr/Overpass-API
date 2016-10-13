/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#include "../data/abstract_processing.h"
#include "difference.h"


Generic_Statement_Maker< Difference_Statement > Difference_Statement::statement_maker("difference");


Difference_Statement::Difference_Statement
    (int line_number_, const map< string, string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);
}


void Difference_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());

  if (substatements.size() >= 2)
  {
    ostringstream temp;
    temp<<"A set difference always requires exactly two substatements: "
          "the set of elements to copy to the result minus "
          "the set of elements to leave out in the result.";
    add_static_error(temp.str());
  }
  
  if (statement->get_result_name() != "")
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}


void Difference_Statement::execute(Resource_Manager& rman)
{
  Set base_set;
  
  if (substatements.empty())
  {
    transfer_output(rman, base_set);
    return;
  }
  vector< Statement* >::iterator it = substatements.begin();
  
  rman.push_reference(base_set);
  (*it)->execute(rman);
  rman.pop_reference();
  
  base_set = rman.sets()[(*it)->get_result_name()];
  
  ++it;
  if (it != substatements.end())
  {
    rman.push_reference(base_set);
    (*it)->execute(rman);
    rman.pop_reference();
    
    Set& summand = rman.sets()[(*it)->get_result_name()];
    
    indexed_set_difference(base_set.nodes, summand.nodes);
    indexed_set_difference(base_set.ways, summand.ways);
    indexed_set_difference(base_set.relations, summand.relations);
    
    indexed_set_difference(base_set.attic_nodes, summand.attic_nodes);
    indexed_set_difference(base_set.attic_ways, summand.attic_ways);
    indexed_set_difference(base_set.attic_relations, summand.attic_relations);
    
    indexed_set_difference(base_set.areas, summand.areas);    
  }

  transfer_output(rman, base_set);
  rman.health_check(*this);
}
