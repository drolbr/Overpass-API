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

#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "foreach.h"


Generic_Statement_Maker< Foreach_Statement > Foreach_Statement::statement_maker("foreach");


Foreach_Statement::Foreach_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  output = attributes["into"];
}


void Foreach_Statement::add_statement(Statement* statement, std::string text)
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


void add_to_set(Set& target, Uint32_Index idx, const Node_Skeleton& skel)
{ target.nodes[idx].push_back(skel); }
void add_to_set(Set& target, Uint32_Index idx, const Attic< Node_Skeleton >& skel)
{ target.attic_nodes[idx].push_back(skel); }
void add_to_set(Set& target, Uint31_Index idx, const Way_Skeleton& skel)
{ target.ways[idx].push_back(skel); }
void add_to_set(Set& target, Uint31_Index idx, const Attic< Way_Skeleton >& skel)
{ target.attic_ways[idx].push_back(skel); }
void add_to_set(Set& target, Uint31_Index idx, const Relation_Skeleton& skel)
{ target.relations[idx].push_back(skel); }
void add_to_set(Set& target, Uint31_Index idx, const Attic< Relation_Skeleton >& skel)
{ target.attic_relations[idx].push_back(skel); }
void add_to_set(Set& target, Uint31_Index idx, const Area_Skeleton& skel)
{ target.areas[idx].push_back(skel); }
void add_to_set(Set& target, Uint31_Index idx, const Derived_Structure& skel)
{ target.deriveds[idx].push_back(skel); }


template< typename Index, typename Object >
void loop_over_elements(const std::map< Index, std::vector< Object > >& source, Resource_Manager& rman,
    std::vector< Statement* >& substatements, const std::string& input, const std::string& result_name)
{
  for (typename std::map< Index, std::vector< Object > >::const_iterator
      it = source.begin(); it != source.end(); ++it)
  {
    for (typename std::vector< Object >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      Set empty;
      add_to_set(empty, it->first, *it2);
      rman.swap_set(result_name, empty);

      for (std::vector< Statement* >::iterator it = substatements.begin();
          it != substatements.end(); ++it)
	(*it)->execute(rman);
    }
  }
}


void Foreach_Statement::execute(Resource_Manager& rman)
{
  Set base_result_set;
  rman.swap_set(get_result_name(), base_result_set);

  rman.push_stack_frame();

  const Set* base_set = (input == get_result_name() ? &base_result_set : rman.get_set(input));
  if (!base_set)
    base_set = &base_result_set;

  loop_over_elements(base_set->nodes, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->attic_nodes, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->ways, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->attic_ways, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->relations, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->attic_relations, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->areas, rman, substatements, input, get_result_name());
  loop_over_elements(base_set->deriveds, rman, substatements, input, get_result_name());

  rman.move_all_inward_except(get_result_name());
  rman.pop_stack_frame();

  rman.health_check(*this);
}
