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

using namespace std;

Generic_Statement_Maker< Complete_Statement > Complete_Statement::statement_maker("complete");

Complete_Statement::Complete_Statement
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["into_complete"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = "_";
  output_iteration = attributes["into"];
  output_complete = attributes["into_complete"];
}

void Complete_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if (statement)
  {
    if (statement->get_name() != "newer")
      substatements.push_back(statement);
    else
      add_static_error("\"newer\" can appear only inside \"query\" statements.");
  }
}

uint count_itemset_entries(const Set& set_)
{
  uint size(0);
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(set_.nodes.begin()); it != set_.nodes.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(set_.ways.begin()); it != set_.ways.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(set_.relations.begin()); it != set_.relations.end(); ++it)
    size += it->second.size();

  for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::const_iterator
      it(set_.attic_nodes.begin()); it != set_.attic_nodes.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Attic< Way_Skeleton > > >::const_iterator
      it(set_.attic_ways.begin()); it != set_.attic_ways.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Attic< Relation_Skeleton > > >::const_iterator
      it(set_.attic_relations.begin()); it != set_.attic_relations.end(); ++it)
    size += it->second.size();

  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
      it(set_.areas.begin()); it != set_.areas.end(); ++it)
    size += it->second.size();

  return size;
}

void Complete_Statement::execute(Resource_Manager& rman)
{
  bool new_elements_found;

  Set result_set;
  Set iteration_base_set = rman.sets()[input];

  do
  {
    rman.sets()[output_iteration] = iteration_base_set;

    rman.push_reference(iteration_base_set);
    for (vector< Statement* >::iterator it(substatements.begin());
        it != substatements.end(); ++it)
    {
      (*it)->execute(rman);
    }
    rman.pop_reference();

    Set& iteration_new_set = rman.sets()[input];

    indexed_set_difference(iteration_new_set.nodes, result_set.nodes);
    indexed_set_difference(iteration_new_set.attic_nodes, result_set.attic_nodes);

    indexed_set_difference(iteration_new_set.ways, result_set. ways);
    indexed_set_difference(iteration_new_set.attic_ways, result_set.attic_ways);

    indexed_set_difference(iteration_new_set.relations, result_set. relations);
    indexed_set_difference(iteration_new_set.attic_relations, result_set.attic_relations);

    indexed_set_difference(iteration_new_set.areas, result_set.areas);

    new_elements_found = ::count_itemset_entries(iteration_new_set) > 0;

    iteration_base_set = iteration_new_set;

    indexed_set_union(result_set.nodes, iteration_new_set.nodes);
    indexed_set_union(result_set.attic_nodes, iteration_new_set.attic_nodes);

    indexed_set_union(result_set.ways, iteration_new_set.ways);
    indexed_set_union(result_set.attic_ways, iteration_new_set.attic_ways);

    indexed_set_union(result_set.relations, iteration_new_set.relations);
    indexed_set_union(result_set.attic_relations, iteration_new_set.attic_relations);

    indexed_set_union(result_set.areas, iteration_new_set.areas);

  }  while (new_elements_found);

  rman.sets()[output_complete].swap(result_set);
  result_set.clear();

  rman.health_check(*this);
}

