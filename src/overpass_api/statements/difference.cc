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
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);
}


void Difference_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());

  if (substatements.size() >= 2)
  {
    std::ostringstream temp;
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
  if (substatements.empty())
    return;

  rman.push_stack_frame();

  std::vector< Statement* >::iterator it = substatements.begin();
  (*it)->execute(rman);

  rman.copy_inward((*it)->get_result_name(), get_result_name());

  ++it;
  if (it != substatements.end())
  {
    (*it)->execute(rman);
    rman.substract_from_inward((*it)->get_result_name(), get_result_name());
  }

  rman.move_all_inward_except(get_result_name());
  rman.pop_stack_frame();

  rman.health_check(*this);
}
