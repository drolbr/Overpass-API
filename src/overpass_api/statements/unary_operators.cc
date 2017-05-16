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
#include "unary_operators.h"


Evaluator_Prefix_Operator::Evaluator_Prefix_Operator(int line_number_) : Evaluator(line_number_), rhs(0) {}


void Evaluator_Prefix_Operator::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one evaluator substatement.");
}


void Evaluator_Prefix_Operator::add_substatements(Statement* result, const std::string& operator_name,
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Error_Output* error_output)
{    
  if (result)
  {
    Statement* rhs = stmt_factory.create_statement(tree_it.rhs(), tree_context);
    if (rhs)
      result->add_statement(rhs, "");
    else if (error_output)
      error_output->add_parse_error(std::string("Operator \"") + operator_name
          + "\" needs a right-hand-side argument", tree_it->line_col.first);
  }  
}


Eval_Task* Evaluator_Prefix_Operator::get_task(const Prepare_Task_Context& context)
{
  Eval_Task* rhs_task = rhs ? rhs->get_task(context) : 0;
  return new Unary_Eval_Task(rhs_task, this);
}


std::string Unary_Eval_Task::eval(const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}


std::string Unary_Eval_Task::eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(rhs ? rhs->eval(data, key) : "");
}
  

Requested_Context Evaluator_Prefix_Operator::request_context() const
{
  if (rhs)
    return rhs->request_context();
  return Requested_Context();
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Not > Evaluator_Not::statement_maker;


std::string Evaluator_Not::process(const std::string& rhs_s) const
{
  double rhs_d = 0;  
  if (try_double(rhs_s, rhs_d))
    return !rhs_d ? "1" : "0";
  
  return rhs_s == "" ? "1" : "0";
}


//-----------------------------------------------------------------------------


Operator_Stmt_Maker< Evaluator_Negate > Evaluator_Negate::statement_maker;


std::string Evaluator_Negate::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;  
  if (try_int64(rhs_s, rhs_l))
    return to_string(-rhs_l);
  
  double rhs_d = 0;  
  if (try_double(rhs_s, rhs_d))
    return to_string(-rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Number > Evaluator_Number::statement_maker;


std::string Evaluator_Number::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;
  if (try_int64(rhs_s, rhs_l))
    return to_string(rhs_l);
  
  double rhs_d = 0;  
  if (try_starts_with_double(rhs_s, rhs_d))
    return to_string(rhs_d);
  
  return "NaN";
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Is_Num > Evaluator_Is_Num::statement_maker;


std::string Evaluator_Is_Num::process(const std::string& rhs_s) const
{
  int64 rhs_l = 0;
  if (try_int64(rhs_s, rhs_l))
    return "1";
  
  double rhs_d = 0;  
  if (try_starts_with_double(rhs_s, rhs_d))
    return "1";
  
  return "0";
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Suffix > Evaluator_Suffix::statement_maker;


std::string Evaluator_Suffix::process(const std::string& rhs_s) const
{
  return double_suffix(rhs_s);
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Date > Evaluator_Date::statement_maker;


std::string Evaluator_Date::process(const std::string& rhs_s) const
{
  //First run: try for year, month, day, hour, minute, second
  std::string::size_type pos = 0;
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int year = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    year = 10*year + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int month = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    month = 10*month + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int day = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    day = 10*day + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int hour = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    hour = 10*hour + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int minute = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    minute = 10*minute + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int second = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    second = 10*second + (rhs_s[pos] - '0');
    ++pos;
  }
  
  if (year < 1000 || month > 12 || day > 31 || hour > 24 || minute > 60 || second > 60)
    return "NaD";
  
  return to_string(year + month/16. + day/(16.*32)
      + hour/(16.*32*32) + minute/(16.*32*32*64) + second/(16.*32*32*64*64));
}


//-----------------------------------------------------------------------------


String_Endom_Statement_Maker< Evaluator_Is_Date > Evaluator_Is_Date::statement_maker;


std::string Evaluator_Is_Date::process(const std::string& rhs_s) const
{
  //First run: try for year, month, day, hour, minute, second
  std::string::size_type pos = 0;
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int year = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    year = 10*year + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int month = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    month = 10*month + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int day = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    day = 10*day + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int hour = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    hour = 10*hour + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int minute = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    minute = 10*minute + (rhs_s[pos] - '0');
    ++pos;
  }
  
  while (pos < rhs_s.size() && !isdigit(rhs_s[pos]))
    ++pos;
  unsigned int second = 0;
  while (pos < rhs_s.size() && isdigit(rhs_s[pos]))
  {
    second = 10*second + (rhs_s[pos] - '0');
    ++pos;
  }
  
  if (year < 1000 || month > 12 || day > 31 || hour > 24 || minute > 60 || second > 60)
    return "0";
  
  return "1";
}
