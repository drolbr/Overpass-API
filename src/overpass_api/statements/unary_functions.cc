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


#include "unary_functions.h"


Evaluator_Unary_Function::Evaluator_Unary_Function(int line_number_) : Evaluator(line_number_), rhs(0) {}


void Evaluator_Unary_Function::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one evaluator substatement.");
}


Eval_Task* Evaluator_Unary_Function::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  Eval_Task* rhs_task = rhs ? rhs->get_string_task(context, key) : 0;
  return new Unary_Eval_Task(rhs_task, this);
}


Requested_Context Evaluator_Unary_Function::request_context() const
{
  if (rhs)
    return rhs->request_context();
  return Requested_Context();
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


//-----------------------------------------------------------------------------


Evaluator_Geometry_Unary_Function::Evaluator_Geometry_Unary_Function(int line_number_)
    : Evaluator(line_number_), rhs(0) {}


void Evaluator_Geometry_Unary_Function::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!rhs)
    rhs = tag_value_;
  else
    add_static_error(get_name() + " must have exactly one evaluator substatement.");
}


Eval_Geometry_Task* Evaluator_Geometry_Unary_Function::get_geometry_task(Prepare_Task_Context& context)
{
  Eval_Geometry_Task* rhs_task = rhs ? rhs->get_geometry_task(context) : 0;
  return new Unary_Geometry_Eval_Task(rhs_task, this);
}


Requested_Context Evaluator_Geometry_Unary_Function::request_context() const
{
  if (rhs)
    return rhs->request_context();
  return Requested_Context();
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval() const
{
  return evaluator->process(rhs ? rhs->eval() : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Node_Skeleton >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Attic< Node_Skeleton > >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Area_Skeleton >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


Opaque_Geometry* Unary_Geometry_Eval_Task::eval(const Element_With_Context< Derived_Skeleton >& data) const
{
  return evaluator->process(rhs ? rhs->eval(data) : 0);
}


//-----------------------------------------------------------------------------


Evaluator_Binary_Function::Evaluator_Binary_Function(int line_number_) : Evaluator(line_number_),
    first(0), second(0) {}


void Evaluator_Binary_Function::add_statement(Statement* statement, std::string text)
{
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!first)
    first = tag_value_;
  else if (!second)
    second = tag_value_;
  else
    add_static_error(get_name() + " must have exactly two evaluator substatements.");
}


Eval_Task* Evaluator_Binary_Function::get_string_task(Prepare_Task_Context& context, const std::string* key)
{
  Eval_Task* first_task = first ? first->get_string_task(context, key) : 0;
  Eval_Task* second_task = second ? second->get_string_task(context, key) : 0;
  return new Binary_Func_Eval_Task(first_task, second_task, this);
}


Requested_Context Evaluator_Binary_Function::request_context() const
{
  if (first && second)
  {
    Requested_Context result = first->request_context();
    result.add(second->request_context());
    return result;
  }
  else if (first)
    return first->request_context();
  else if (second)
    return second->request_context();

  return Requested_Context();
}


std::string Binary_Func_Eval_Task::eval(const std::string* key) const
{
  return evaluator->process(first ? first->eval(key) : "", second ? second->eval(key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}


std::string Binary_Func_Eval_Task::eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const
{
  return evaluator->process(first ? first->eval(data, key) : "", second ? second->eval(data, key) : "");
}
