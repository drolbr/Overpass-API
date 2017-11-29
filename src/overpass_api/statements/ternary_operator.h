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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__TERNARY_OPERATORS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__TERNARY_OPERATORS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


/* == the Ternary Operator ==

The ternary operator needs for execution three operands.
If the first operand evaluates to boolean true
then it evaluates to what the second operand evaluates.
Otherwise it evaluates to what the third operand evaluates.

The two variants

  <Evaulator>?<Evaluator>:<Evaluator>

and

  (<Evaulator>?<Evaluator>:<Evaluator>)

have as standalone expressions precisely the same semantics.
The parenthesis variant exists to override operator precedence.
For the precendence, see the binary operators.
*/

struct Ternary_Eval_Task : public Eval_Task
{
  Ternary_Eval_Task(Eval_Task* condition_, Eval_Task* lhs_, Eval_Task* rhs_)
      : condition(condition_), lhs(lhs_), rhs(rhs_) {}
  ~Ternary_Eval_Task()
  {
    delete condition;
    delete lhs;
    delete rhs;
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
  Eval_Task* condition;
  Eval_Task* lhs;
  Eval_Task* rhs;
};


struct Ternary_Evaluator : public Evaluator
{
  static Operator_Stmt_Maker< Ternary_Evaluator > statement_maker;
  static Operator_Eval_Maker< Ternary_Evaluator > evaluator_maker;
  static std::string stmt_operator() { return "?"; }
  static std::string stmt_name() { return "eval-ternary"; }

  Ternary_Evaluator(int line_number_, const std::map< std::string, std::string >& input_attributes,
      Parsed_Query& global_settings) : Evaluator(line_number_), condition(0), lhs(0), rhs(0)
  {
    std::map< std::string, std::string > attributes;
    eval_attributes_array(stmt_name(), attributes, input_attributes);
  }

  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual std::string get_result_name() const { return ""; }

  virtual Requested_Context request_context() const;

  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key);

  static bool applicable_by_subtree_structure(const Token_Node_Ptr& tree_it)
  { return tree_it->lhs && tree_it->rhs; }
  static void add_substatements(Statement* result, const std::string& operator_name, const Token_Node_Ptr& tree_it,
      Statement::QL_Context tree_context, Statement::Factory& stmt_factory, Error_Output* error_output);

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + stmt_name() + ">\n"
        + (condition ? condition->dump_xml(indent + "  ") : "")
        + (lhs ? lhs->dump_xml(indent + "  ") : "")
        + (rhs ? rhs->dump_xml(indent + "  ") : "")
        + indent + "</" + stmt_name() + ">\n";
  }

  virtual std::string dump_compact_ql(const std::string&) const
  {
    return (condition ?
            (condition->get_operator_priority() < get_operator_priority() ?
                std::string("(") + condition->dump_compact_ql("") + ")"
                : condition->dump_compact_ql("")) : "")
        + "?"
        + (lhs ?
            (lhs->get_operator_priority() < get_operator_priority() ?
                std::string("(") + lhs->dump_compact_ql("") + ")"
                : lhs->dump_compact_ql("")) : "")
        + ":"
        + (rhs ?
            (rhs->get_operator_priority() < get_operator_priority() ?
                std::string("(") + rhs->dump_compact_ql("") + ")"
                : rhs->dump_compact_ql("")) : "");
  }

  virtual std::string get_name() const { return stmt_name(); }
  virtual int get_operator_priority() const { return operator_priority(stmt_operator(), false); }

private:
  Evaluator* condition;
  Evaluator* lhs;
  Evaluator* rhs;
};


#endif
