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
#include <cctype>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "../data/abstract_processing.h"
#include "complete.h"


Generic_Statement_Maker< Complete_Statement > Complete_Statement::statement_maker("complete");


Complete_Statement::Complete_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  set_output(attributes["into"]);
}


void Complete_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());
  
  if (statement)
  {
    if (statement->get_name() == "newer")
      add_static_error("\"newer\" can appear only inside \"query\" statements.");
    
    substatements.push_back(statement);
  }
}


void Complete_Statement::execute(Resource_Manager& rman)
{
  bool new_elements_found = false;
  const Set* result_ref = rman.get_set(input);
  Set result_set = result_ref ? *result_ref : Set();

  do
  {
    rman.swap_set(get_result_name(), result_set);
    result_set = *rman.get_set(get_result_name());
    
    for (std::vector< Statement* >::iterator it = substatements.begin(); it != substatements.end(); ++it)
    {
      rman.push_reference(result_set);
      (*it)->execute(rman);
      rman.pop_reference();
    }

    const Set& growth_set = *rman.get_set(input);
    
    new_elements_found = indexed_set_union(result_set.nodes, growth_set.nodes);
    new_elements_found |= indexed_set_union(result_set.attic_nodes, growth_set.attic_nodes);

    new_elements_found |= indexed_set_union(result_set.ways, growth_set.ways);
    new_elements_found |= indexed_set_union(result_set.attic_ways, growth_set.attic_ways);

    new_elements_found |= indexed_set_union(result_set.relations, growth_set.relations);
    new_elements_found |= indexed_set_union(result_set.attic_relations, growth_set.attic_relations);

    new_elements_found |= indexed_set_union(result_set.areas, growth_set.areas);
    new_elements_found |= indexed_set_union(result_set.deriveds, growth_set.deriveds);
  }
  while (new_elements_found);

  transfer_output(rman, result_set);

  rman.health_check(*this);
}
