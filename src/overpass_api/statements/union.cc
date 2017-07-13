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
#include "../data/utils.h"
#include "union.h"


Generic_Statement_Maker< Union_Statement > Union_Statement::statement_maker("union");


Union_Statement::Union_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);
}


void Union_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());


  if (statement)
  {
    if (statement->get_name() == "newer")
      add_static_error("\"newer\" can appear only inside \"query\" statements.");
    else if (statement->get_result_name() == "")
      substatement_error(get_name(), statement);
    else
      substatements.push_back(statement);
  }
}


void Union_Statement::execute(Resource_Manager& rman)
{
  rman.push_stack_frame();
  rman.move_outward(get_result_name(), get_result_name());
  
  for (std::vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->execute(rman);
    rman.union_inward((*it)->get_result_name(), get_result_name());
  }
  
  rman.move_all_inward_except(get_result_name());
  rman.pop_stack_frame();
  
  rman.health_check(*this);
}
