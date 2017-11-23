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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__UNARY_FUNCTIONS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__UNARY_FUNCTIONS_H


#include "evaluator.h"
#include "statement.h"

#include <string>


class Evaluator_Unary_Function : public Evaluator
{
public:
  Evaluator_Unary_Function(int line_number_);
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual std::string get_result_name() const { return ""; }

  virtual Requested_Context request_context() const;
  virtual Eval_Task* get_task(Prepare_Task_Context& context, const std::string* key);

  virtual std::string process(const std::string& rhs_result) const = 0;

protected:
  Evaluator* rhs;
};


struct Unary_Eval_Task : public Eval_Task
{
  Unary_Eval_Task(Eval_Task* rhs_, Evaluator_Unary_Function* evaluator_) : rhs(rhs_), evaluator(evaluator_) {}
  ~Unary_Eval_Task() { delete rhs; }

  virtual std::string eval(const std::string* key) const;

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const;

private:
  Eval_Task* rhs;
  Evaluator_Unary_Function* evaluator;
};


class Evaluator_Binary_Function : public Evaluator
{
public:
  Evaluator_Binary_Function(int line_number_);
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual std::string get_result_name() const { return ""; }

  virtual Requested_Context request_context() const;
  virtual Eval_Task* get_task(Prepare_Task_Context& context, const std::string* key);

  virtual std::string process(const std::string& first_result, const std::string& second_result) const = 0;
  static bool needs_an_element_to_eval() { return false; }

protected:
  Evaluator* first;
  Evaluator* second;
};


struct Binary_Func_Eval_Task : public Eval_Task
{
  Binary_Func_Eval_Task(Eval_Task* first_, Eval_Task* second_, Evaluator_Binary_Function* evaluator_)
      : first(first_), second(second_), evaluator(evaluator_) {}
  ~Binary_Func_Eval_Task()
  {
    delete first;
    delete second;
  }

  virtual std::string eval(const std::string* key) const;

  virtual std::string eval(const Element_With_Context< Node_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Node_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Way_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Way_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Relation_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Attic< Relation_Skeleton > >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Area_Skeleton >& data, const std::string* key) const;
  virtual std::string eval(const Element_With_Context< Derived_Skeleton >& data, const std::string* key) const;

private:
  Eval_Task* first;
  Eval_Task* second;
  Evaluator_Binary_Function* evaluator;
};


#endif
