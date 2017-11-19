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


#include "../data/set_comparison.h"
#include "evaluator.h"
#include "compare.h"


Generic_Statement_Maker< Compare_Statement > Compare_Statement::statement_maker("compare");


Compare_Statement::Compare_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), criterion(0), set_comparison(0)
{
  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);
}


void Compare_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());
  
  if (!criterion)
  {
    Evaluator* tag_value = dynamic_cast< Evaluator* >(statement);
    if (tag_value)
    {
      criterion = tag_value;
      return;
    }
  }

  if (statement->get_name() == "newer")
    add_static_error("\"newer\" can appear only inside \"query\" statements.");

  substatements.push_back(statement);
}


void Compare_Statement::execute(Resource_Manager& rman)
{
  Diff_Action::_ action = rman.get_desired_action();
  
  const Set* input_set = rman.get_set(input);
  if (!input_set)
    return;

  if (action == Diff_Action::collect_lhs)
  {
    delete set_comparison;
    set_comparison = new Set_Comparison(
        *rman.get_transaction(), *input_set, rman.get_desired_timestamp());
    
    Set into;
    transfer_output(rman, into);
    rman.health_check(*this);
  }
  else if (action == Diff_Action::collect_rhs_no_del || action == Diff_Action::collect_rhs_with_del)
  {
    if (criterion)
    {
      Diff_Set into = set_comparison->compare_to_lhs(rman, *this, *input_set,
          criterion, action == Diff_Action::collect_rhs_with_del);
      transfer_output(rman, into);
    }
    else
    {
      Diff_Set into = set_comparison->compare_to_lhs(rman, *this, *input_set,
          1., 0., 0., 0., action == Diff_Action::collect_rhs_with_del);
      transfer_output(rman, into);
    }

    if (!substatements.empty())
    {
      rman.push_stack_frame();
      rman.switch_diff_show_from(get_result_name());
  
      for (std::vector< Statement* >::iterator it = substatements.begin(); it != substatements.end(); ++it)
        (*it)->execute(rman);

      rman.pop_stack_frame();

      rman.push_stack_frame();
      rman.switch_diff_show_to(get_result_name());
  
      for (std::vector< Statement* >::iterator it = substatements.begin(); it != substatements.end(); ++it)
        (*it)->execute(rman);

      rman.pop_stack_frame();

      rman.health_check(*this);
    }
  }
  else
  {
    Set into;
    transfer_output(rman, into);
    rman.health_check(*this);
  }
}


Compare_Statement::~Compare_Statement()
{
  delete set_comparison;
}
