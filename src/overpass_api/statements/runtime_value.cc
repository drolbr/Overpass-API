/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#include "../../expat/escape_xml.h"
#include "runtime_value.h"


Evaluator_Set_Key::Statement_Maker Evaluator_Set_Key::statement_maker;
Evaluator_Set_Key::Evaluator_Maker Evaluator_Set_Key::evaluator_maker;


Statement* Evaluator_Set_Key::Evaluator_Maker::create_evaluator(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (tree_context != Statement::evaluator_expected && tree_context != Statement::elem_eval_possible
      && tree_context != Statement::member_eval_possible)
    return 0;
  if (!tree_it->lhs || !tree_it->rhs || tree_it.lhs()->token.empty() || tree_it.rhs()->token.empty())
    return 0;

  std::map< std::string, std::string > attributes;
  attributes["from"] = tree_it.lhs()->token;
  attributes["key"] = tree_it.rhs()->token;
  return new Evaluator_Set_Key(tree_it->line_col.first, attributes, global_settings);
}


Evaluator_Set_Key::Evaluator_Set_Key(int line_number_, const std::map< std::string, std::string >& input_attributes,
    Parsed_Query& global_settings) : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  attributes["from"] = "_";
  attributes["key"] = "";
  eval_attributes_array("eval-set-key", attributes, input_attributes);
  input = attributes["from"];
  if (attributes["key"].empty())
    add_static_error("\"eval-set-key\" needs a nonempty key in attribute \"key\" to evaluate.");
  else
    key = attributes["key"];
}


Eval_Task* Evaluator_Set_Key::get_string_task(Prepare_Task_Context& context, const std::string*)
{
  const Set_With_Context* set_ = context.get_set(input);
  std::string result;
  if (set_ && set_->set_key_values)
  {
    std::map< std::string, std::string >::const_iterator it = set_->set_key_values->find(key);
    result = (it != set_->set_key_values->end() ? it->second : "");
  }
  return new Const_Eval_Task(result);
}
