/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "ternary_operator.h"


void Ternary_Evaluator::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!condition)
    condition = tag_value_;
  else if (!lhs)
    lhs = tag_value_;
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly three evaluator substatements.");
}


void Ternary_Evaluator::add_substatements(Statement* result, const std::string& operator_name,
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Error_Output* error_output)
{
  if (result)
  {
    if (tree_it.rhs()->token != ":")
    {
      if (error_output)
        error_output->add_parse_error(
            "Operator \"?\" must have a \":\" operator as right-hand-side argument", tree_it->line_col.first);
      return;
    }

    Statement* condition = stmt_factory.create_evaluator(
        tree_it.lhs(), tree_context, Statement::Single_Return_Type_Checker(Statement::string));
    if (condition)
      result->add_statement(condition, "");
    else if (error_output)
      error_output->add_parse_error(
          "Operator \"?\" needs a left-hand-side argument", tree_it->line_col.first);
      
    Statement* lhs = stmt_factory.create_evaluator(
        tree_it.rhs().lhs(), tree_context, Statement::Single_Return_Type_Checker(Statement::string));
    if (lhs)
      result->add_statement(lhs, "");
    else if (error_output)
      error_output->add_parse_error(
          "Operator \":\" of operator \"?\" needs a left-hand-side argument", tree_it->line_col.first);

    Statement* rhs = stmt_factory.create_evaluator(
        tree_it.rhs().rhs(), tree_context, Statement::Single_Return_Type_Checker(Statement::string));
    if (rhs)
      result->add_statement(rhs, "");
    else if (error_output)
      error_output->add_parse_error(
          "Operator \":\" of operator \"?\" needs a right-hand-side argument", tree_it->line_col.first);
  }
}


Eval_Task* Ternary_Evaluator::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  Eval_Task* cond_task = condition ? condition->get_string_task(context, key) : 0;
  Eval_Task* lhs_task = lhs ? lhs->get_string_task(context, key) : 0;
  Eval_Task* rhs_task = rhs ? rhs->get_string_task(context, key) : 0;
  return new Ternary_Eval_Task(cond_task, lhs_task, rhs_task);
}


std::string Ternary_Eval_Task::eval(const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(key)))
    return lhs ? lhs->eval(key) : "";
  return rhs ? rhs->eval(key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


std::string Ternary_Eval_Task::eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
{
  if (!condition)
    return "0";
  if (string_represents_boolean_true(condition->eval(data, key)))
    return lhs ? lhs->eval(data, key) : "";
  return rhs ? rhs->eval(data, key) : "";
}


Requested_Context Ternary_Evaluator::request_context() const
{
  if (condition)
  {
    Requested_Context result = condition->request_context();
    if (lhs)
      result.add(lhs->request_context());
    if (rhs)
      result.add(rhs->request_context());
    return result;
  }
  
  return Requested_Context();
}


Operator_Stmt_Maker< Ternary_Evaluator > Ternary_Evaluator::statement_maker;
Operator_Eval_Maker< Ternary_Evaluator > Ternary_Evaluator::evaluator_maker;
