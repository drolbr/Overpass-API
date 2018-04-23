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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__UNARY_OPERATORS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__UNARY_OPERATORS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"
#include "unary_functions.h"

#include <map>
#include <string>
#include <vector>


/* == Unary Operators ==

Unary operators need for execution an operand.
They are always written in prefix notation.
The operators can be grouped with parentheses:
The two variants

  <Operator><Evaluator>

and

  (<Operator><Evaluator>)

have as standalone expressions precisely the same semantics.
The parenthesis variant exists to override operator precedence.

The order of precedence is as follows, ordered weak to strong binding:
* logical disjunction
* logical conjunction
* equality, inequality
* less, less-equal, greater, greater-equal
* plus, binary minus
* times, divided
* logical negation
* unary minus

In the following, the operators are ordered by precedence, stronger binding last.
*/

class Evaluator_Prefix_Operator : public Evaluator_Unary_Function
{
public:
  Evaluator_Prefix_Operator(int line_number_);

  static bool applicable_by_subtree_structure(const Token_Node_Ptr& tree_it) { return !tree_it->lhs && tree_it->rhs; }
  static void add_substatements(Statement* result, const std::string& operator_name, const Token_Node_Ptr& tree_it,
      Statement::QL_Context tree_context, Statement::Factory& stmt_factory, Error_Output* error_output);
};


template< typename Evaluator_ >
struct Evaluator_Prefix_Operator_Syntax : public Evaluator_Prefix_Operator
{
  Evaluator_Prefix_Operator_Syntax(int line_number_, const std::map< std::string, std::string >& input_attributes)
    : Evaluator_Prefix_Operator(line_number_)
  {
    std::map< std::string, std::string > attributes;
    eval_attributes_array(Evaluator_::stmt_name(), attributes, input_attributes);
  }

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + ">\n"
        + (rhs ? rhs->dump_xml(indent + "  ") : "")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }

  virtual std::string dump_compact_ql(const std::string&) const
  {
    if (!rhs)
      return Evaluator_::stmt_operator();
    if (rhs->get_operator_priority() < get_operator_priority())
      return Evaluator_::stmt_operator() + "(" + rhs->dump_compact_ql("") + ")";
    return Evaluator_::stmt_operator() + rhs->dump_compact_ql("");
  }

  virtual std::string get_name() const { return Evaluator_::stmt_name(); }
  virtual int get_operator_priority() const { return operator_priority(Evaluator_::stmt_operator(), true); }
};


/* === Boolean Negation ===

The boolean negation evaluates to "1" if its argument evaluates to a representation of boolean false.
Otherwise it evaluates to "1".
Representations of boolean false are the empty std::string and every std::string that is a numerical representation of zero.
Every other std::string represents boolean true.

Its syntax is

  ! <Evaluator>

The whitespace is optional.
*/

class Evaluator_Not : public Evaluator_Prefix_Operator_Syntax< Evaluator_Not >
{
public:
  static Operator_Stmt_Maker< Evaluator_Not > statement_maker;
  static Operator_Eval_Maker< Evaluator_Not > evaluator_maker;
  static std::string stmt_operator() { return "!"; }
  static std::string stmt_name() { return "eval-not"; }

  Evaluator_Not(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Prefix_Operator_Syntax< Evaluator_Not >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


/* === Unary Minus ===

The unary minus operator negates its argument.
If the argument is an integer or a floating point number then it is negated as integer resp. floating point number.
Otherwise the unary minus operator returns "NaN".

The syntax for unary minus is:

  - <Evaluator>

The whitespace is optional.
*/

class Evaluator_Negate : public Evaluator_Prefix_Operator_Syntax< Evaluator_Negate >
{
public:
  static Operator_Stmt_Maker< Evaluator_Negate > statement_maker;
  static Operator_Eval_Maker< Evaluator_Negate > evaluator_maker;
  static std::string stmt_operator() { return "-"; }
  static std::string stmt_name() { return "eval-negate"; }

  Evaluator_Negate(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Prefix_Operator_Syntax< Evaluator_Negate >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


#endif
