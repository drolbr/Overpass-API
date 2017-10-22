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
#include "binary_operators.h"


Evaluator_Pair_Operator::Evaluator_Pair_Operator(int line_number_) : Evaluator(line_number_), lhs(0), rhs(0) {}


void Evaluator_Pair_Operator::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!lhs)
    lhs = tag_value_;
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly two evaluator substatements.");
}


void Evaluator_Pair_Operator::add_substatements(Statement* result, const std::string& operator_name,
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Error_Output* error_output)
{    
  if (result)
  {
    Statement* lhs = stmt_factory.create_evaluator(tree_it.lhs(), tree_context);
    if (lhs)
      result->add_statement(lhs, "");
    else if (error_output)
      error_output->add_parse_error(std::string("Operator \"") + operator_name
          + "\" needs a left-hand-side argument", tree_it->line_col.first);
    
    Statement* rhs = stmt_factory.create_evaluator(tree_it.rhs(), tree_context);
    if (rhs)
      result->add_statement(rhs, "");
    else if (error_output)
      error_output->add_parse_error(std::string("Operator \"") + operator_name
          + "\" needs a right-hand-side argument", tree_it->line_col.first);
  }  
}


Eval_Task* Evaluator_Pair_Operator::get_task(Prepare_Task_Context& context)
{
  Eval_Task* lhs_task = lhs ? lhs->get_task(context) : 0;
  Eval_Task* rhs_task = rhs ? rhs->get_task(context) : 0;
  return new Binary_Eval_Task(lhs_task, rhs_task, this);
}


std::string Binary_Eval_Task::eval(const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(key) : "", rhs ? rhs->eval(key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}


std::string Binary_Eval_Task::eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(lhs ? lhs->eval(data, key) : "", rhs ? rhs->eval(data, key) : "");
}

    
Requested_Context Evaluator_Pair_Operator::request_context() const
{
  if (lhs && rhs)
  {
    Requested_Context result = lhs->request_context();
    result.add(rhs->request_context());
    return result;
  }
  else if (lhs)
    return lhs->request_context();
  else if (rhs)
    return rhs->request_context();
  
  return Requested_Context();
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_And > Evaluator_And::statement_maker;
Operator_Eval_Maker< Evaluator_And > Evaluator_And::evaluator_maker;


std::string Evaluator_And::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d && rhs_d ? "1" : "0";
  
  return (lhs_s != "") && (rhs_s != "") ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Or > Evaluator_Or::statement_maker;
Operator_Eval_Maker< Evaluator_Or > Evaluator_Or::evaluator_maker;


std::string Evaluator_Or::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d || rhs_d ? "1" : "0";
  
  return (lhs_s != "") || (rhs_s != "") ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Equal > Evaluator_Equal::statement_maker;
Operator_Eval_Maker< Evaluator_Equal > Evaluator_Equal::evaluator_maker;


std::string Evaluator_Equal::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l == rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d == rhs_d ? "1" : "0";
  
  return lhs_s == rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Not_Equal > Evaluator_Not_Equal::statement_maker;
Operator_Eval_Maker< Evaluator_Not_Equal > Evaluator_Not_Equal::evaluator_maker;


std::string Evaluator_Not_Equal::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l == rhs_l ? "0" : "1";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d == rhs_d ? "0" : "1";
  
  return lhs_s == rhs_s ? "0" : "1";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Less > Evaluator_Less::statement_maker;
Operator_Eval_Maker< Evaluator_Less > Evaluator_Less::evaluator_maker;


std::string Evaluator_Less::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l < rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d < rhs_d ? "1" : "0";
  
  return lhs_s < rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Less_Equal > Evaluator_Less_Equal::statement_maker;
Operator_Eval_Maker< Evaluator_Less_Equal > Evaluator_Less_Equal::evaluator_maker;


std::string Evaluator_Less_Equal::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l <= rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d <= rhs_d ? "1" : "0";
  
  return lhs_s <= rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Greater > Evaluator_Greater::statement_maker;
Operator_Eval_Maker< Evaluator_Greater > Evaluator_Greater::evaluator_maker;


std::string Evaluator_Greater::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l > rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d > rhs_d ? "1" : "0";
  
  return lhs_s > rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Greater_Equal > Evaluator_Greater_Equal::statement_maker;
Operator_Eval_Maker< Evaluator_Greater_Equal > Evaluator_Greater_Equal::evaluator_maker;


std::string Evaluator_Greater_Equal::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return lhs_l >= rhs_l ? "1" : "0";
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return lhs_d >= rhs_d ? "1" : "0";
  
  return lhs_s >= rhs_s ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Plus > Evaluator_Plus::statement_maker;
Operator_Eval_Maker< Evaluator_Plus > Evaluator_Plus::evaluator_maker;


std::string Evaluator_Plus::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return to_string(lhs_l + rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d + rhs_d);
  
  return lhs_s + rhs_s;
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Minus > Evaluator_Minus::statement_maker;
Operator_Eval_Maker< Evaluator_Minus > Evaluator_Minus::evaluator_maker;


std::string Evaluator_Minus::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return to_string(lhs_l - rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d - rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Times > Evaluator_Times::statement_maker;
Operator_Eval_Maker< Evaluator_Times > Evaluator_Times::evaluator_maker;


std::string Evaluator_Times::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  int64 lhs_l = 0;
  int64 rhs_l = 0;  
  if (try_int64(lhs_s, lhs_l) && try_int64(rhs_s, rhs_l))
    return to_string(lhs_l * rhs_l);
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d * rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Divided > Evaluator_Divided::statement_maker;
Operator_Eval_Maker< Evaluator_Divided > Evaluator_Divided::evaluator_maker;


std::string Evaluator_Divided::process(const std::string& lhs_s, const std::string& rhs_s) const
{
  // On purpose no int64 detection  
  
  double lhs_d = 0;
  double rhs_d = 0;  
  if (try_double(lhs_s, lhs_d) && try_double(rhs_s, rhs_d))
    return to_string(lhs_d / rhs_d);
  
  return "NaN";
}
