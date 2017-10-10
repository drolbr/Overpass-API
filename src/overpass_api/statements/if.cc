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
#include "if.h"


If_Statement::Statement_Maker If_Statement::statement_maker;


Statement* If_Statement::Statement_Maker::create_criterion(const Token_Node_Ptr& tree_it,
    const std::string& type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  Statement* filter = 0;
  uint line_nr = tree_it->line_col.first;
  
  if (tree_it->token == ":" && tree_it->rhs)
  {
    Statement* criterion = stmt_factory.create_evaluator(tree_it.rhs(), Statement::elem_eval_possible);
    if (criterion)
    {
      std::map< std::string, std::string > attributes;
      filter = new If_Statement(line_nr, attributes, global_settings);
      if (filter)
        filter->add_statement(criterion, "");
    }
  }

  return filter;
}


If_Statement::If_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Statement(line_number_), criterion(0)
{
  std::map< std::string, std::string > attributes;
  
  eval_attributes_array(get_name(), attributes, input_attributes);
}


void If_Statement::add_statement(Statement* statement, std::string text)
{
  assure_no_text(text, this->get_name());
  
  if (!criterion)
  {
    Evaluator* tag_value = dynamic_cast< Evaluator* >(statement);
    if (tag_value)
      criterion = tag_value;
    else
      add_static_error("An If statement must have an Evaluator as first sub-statement.");
  }
  else if (statement)
  {
    if (statement->get_name() == "newer")
      add_static_error("\"newer\" can appear only inside \"query\" statements.");
    
    substatements.push_back(statement);
  }
}


bool evals_to_true(Evaluator& criterion, const Statement& stmt, Resource_Manager& rman)
{
  Prepare_Task_Context context(criterion.request_context(), stmt, rman);
  Owner< Eval_Task > task(criterion.get_task(context));  
  std::string valuation = (*task).eval(0);
  double val_d = 0;
  return valuation != "" && (!try_double(valuation, val_d) || val_d != 0);
}


void If_Statement::execute(Resource_Manager& rman)
{
  if (criterion && evals_to_true(*criterion, *this, rman))
  {
    for (std::vector< Statement* >::iterator it = substatements.begin(); it != substatements.end(); ++it)
      (*it)->execute(rman);
  }

  rman.health_check(*this);
}
