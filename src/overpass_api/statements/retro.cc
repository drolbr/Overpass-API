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


#include "evaluator.h"
#include "retro.h"


Generic_Statement_Maker< Retro_Statement > Retro_Statement::statement_maker("retro");


Retro_Statement::Retro_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), timestamp(0)
{
  std::map< std::string, std::string > attributes;

  eval_attributes_array(get_name(), attributes, input_attributes);
}


void Retro_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());

  if (!timestamp)
  {
    Evaluator* tag_value = dynamic_cast< Evaluator* >(statement);
    if (tag_value)
      timestamp = tag_value;
    else
      add_static_error("A Retro statement must have an Evaluator as first sub-statement.");
  }
  else if (statement)
  {
    if (statement->get_name() == "newer")
      add_static_error("\"newer\" can appear only inside \"query\" statements.");

    substatements.push_back(statement);
  }
}


uint64 eval_timestamp(Evaluator& criterion, const Statement& stmt, Resource_Manager& rman)
{
  Prepare_Task_Context context(criterion.request_context(), stmt, rman);
  Owner< Eval_Task > task(criterion.get_string_task(context, 0));
  std::string valuation = (*task).eval(0);

  return Timestamp(valuation).timestamp;
}


void Retro_Statement::execute(Resource_Manager& rman)
{
  if (!timestamp)
    return;

  uint64 retro_timestamp = eval_timestamp(*timestamp, *this, rman);
  if (!retro_timestamp)
    return;

  rman.push_stack_frame();
  rman.set_desired_timestamp(retro_timestamp);

  for (std::vector< Statement* >::iterator it = substatements.begin(); it != substatements.end(); ++it)
    (*it)->execute(rman);

  rman.pop_stack_frame();
  rman.health_check(*this);
}
