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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__SET_LIST_OPERATORS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__SET_LIST_OPERATORS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"
#include "unary_functions.h"

#include <map>
#include <string>
#include <vector>


/* == List Represented Set Operators ==

Sometimes there is a need to represent multiple values in a tag's value.
Although by no means mandated by the data format,
the de-facto solution is
to represent a set of values by a a semi-colon separated list of those values.

This section offers some functions to make handling of these lists easier.
Currently, the delimiter is hard-coded to a semi-colon.
For the sake of simplicity,
leading or trailing white space at each list entry is ignored.

Note also that the lists are understood as sets.
This means that the order of list elements does no matter.
*/

template< typename Evaluator_ >
struct Unary_Set_List_Operator_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  virtual Statement* create_statement(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
        || !tree_it.assert_has_arguments(error_output, true))
      return 0;
  
    std::map< std::string, std::string > attributes;
    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    if (result)
    {
      Statement* rhs = stmt_factory.create_evaluator(tree_it.rhs(), tree_context);
      if (rhs)
        result->add_statement(rhs, "");
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
    }
    return result;
  }

  Unary_Set_List_Operator_Statement_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name())
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


template< typename Evaluator_ >
struct Evaluator_Unary_Set_List_Operator_Syntax : public Evaluator_Unary_Function
{
  Evaluator_Unary_Set_List_Operator_Syntax(int line_number_, const std::map< std::string, std::string >& input_attributes)
    : Evaluator_Unary_Function(line_number_)
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
    return Evaluator_::stmt_func_name() + "(" + (rhs ? rhs->dump_compact_ql("") : "") + ")";
  }

  virtual std::string get_name() const { return Evaluator_::stmt_name(); }
};


template< typename Evaluator_ >
struct Binary_Set_List_Operator_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  virtual Statement* create_statement(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
        || !tree_it.assert_has_arguments(error_output, true))
      return 0;
  
    std::map< std::string, std::string > attributes;
    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    if (result)
    {
      if (tree_it.rhs()->token == "," && tree_it.rhs()->lhs && tree_it.rhs()->rhs)
      {
        Statement* first = stmt_factory.create_evaluator(tree_it.rhs().lhs(), tree_context);
        if (first)
          result->add_statement(first, "");
        else if (error_output)
          error_output->add_parse_error("First argument of " + Evaluator_::stmt_func_name()
              + "(...) must be an evualator", tree_it->line_col.first);

        Statement* second = stmt_factory.create_evaluator(tree_it.rhs().rhs(), tree_context);
        if (second)
          result->add_statement(second, "");
        else if (error_output)
          error_output->add_parse_error("Second argument of " + Evaluator_::stmt_func_name()
              + "(...) must be an evualator", tree_it->line_col.second);
      }
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs two arguments",
            tree_it->line_col.first);
    }
    return result;
  }

  Binary_Set_List_Operator_Statement_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name())
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


template< typename Evaluator_ >
struct Evaluator_Binary_Set_List_Operator_Syntax : public Evaluator_Binary_Function
{
  Evaluator_Binary_Set_List_Operator_Syntax(int line_number_, const std::map< std::string, std::string >& input_attributes)
    : Evaluator_Binary_Function(line_number_)
  {
    std::map< std::string, std::string > attributes;
    eval_attributes_array(Evaluator_::stmt_name(), attributes, input_attributes);
  }

  virtual std::string dump_xml(const std::string& indent) const
  {
    return indent + "<" + Evaluator_::stmt_name() + ">\n"
        + (first ? first->dump_xml(indent + "  ") : "")
        + (second ? second->dump_xml(indent + "  ") : "")
        + indent + "</" + Evaluator_::stmt_name() + ">\n";
  }

  virtual std::string dump_compact_ql(const std::string&) const
  {
    return Evaluator_::stmt_func_name() + "(" + (first ? first->dump_compact_ql("") : "")
         + (second ? second->dump_compact_ql("") : "") + ")";
  }

  virtual std::string get_name() const { return Evaluator_::stmt_name(); }
};


/* === List Represented Set Theoretic Operators ===

The function <em>lrs_in</em> returns "1"
if its first argument is contained in the second argument treated as set.
It returns "0" otherwise.

The function's syntax is

  lrs_in(<Evaluator>, <Evaluator>)

The function <em>lrs_isect</em> returns the intersection of its two arguments treated as sets.
If the arguments have no values in common then the empty string is returned.

The function's syntax is

  lrs_isect(<Evaluator>, <Evaluator>)

The function <em>lrs_union</em> returns the union of its two arguments treated as sets.

The function's syntax is

  lrs_union(<Evaluator>, <Evaluator>)

*/

class Evaluator_Lrs_In : public Evaluator_Binary_Set_List_Operator_Syntax< Evaluator_Lrs_In >
{
public:
  static Binary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_In > statement_maker;
  static std::string stmt_func_name() { return "lrs_in"; }
  static std::string stmt_name() { return "eval-lrs-in"; }

  Evaluator_Lrs_In(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Binary_Set_List_Operator_Syntax< Evaluator_Lrs_In >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& first_result, const std::string& second_result) const;
};


class Evaluator_Lrs_Isect : public Evaluator_Binary_Set_List_Operator_Syntax< Evaluator_Lrs_Isect >
{
public:
  static Binary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Isect > statement_maker;
  static std::string stmt_func_name() { return "lrs_isect"; }
  static std::string stmt_name() { return "eval-lrs-isect"; }

  Evaluator_Lrs_Isect(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Binary_Set_List_Operator_Syntax< Evaluator_Lrs_Isect >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& first_result, const std::string& second_result) const;
};


class Evaluator_Lrs_Union : public Evaluator_Binary_Set_List_Operator_Syntax< Evaluator_Lrs_Union >
{
public:
  static Binary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Union > statement_maker;
  static std::string stmt_func_name() { return "lrs_union"; }
  static std::string stmt_name() { return "eval-lrs-union"; }

  Evaluator_Lrs_Union(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Binary_Set_List_Operator_Syntax< Evaluator_Lrs_Union >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& first_result, const std::string& second_result) const;
};


/* === List Represented Set Statistic Operators ===

The function <em>lrs_min</em> returns the minimum of the elements in its argument treated as set.
If all entries are numbers then the comparsion is numerical.

The function's syntax is

  lrs_min(<Evaluator>)


The function <em>lrs_min</em> returns the minimum of the elements in its argument treated as set.
If all entries are numbers then the comparsion is numerical.

The function's syntax is

  lrs_max(<Evaluator>)
*/

class Evaluator_Lrs_Max : public Evaluator_Unary_Set_List_Operator_Syntax< Evaluator_Lrs_Max >
{
public:
  static Unary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Max > statement_maker;
  static std::string stmt_func_name() { return "lrs_max"; }
  static std::string stmt_name() { return "eval-lrs-max"; }

  Evaluator_Lrs_Max(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Unary_Set_List_Operator_Syntax< Evaluator_Lrs_Max >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


class Evaluator_Lrs_Min : public Evaluator_Unary_Set_List_Operator_Syntax< Evaluator_Lrs_Min >
{
public:
  static Unary_Set_List_Operator_Statement_Maker< Evaluator_Lrs_Min > statement_maker;
  static std::string stmt_func_name() { return "lrs_min"; }
  static std::string stmt_name() { return "eval-lrs-min"; }

  Evaluator_Lrs_Min(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Unary_Set_List_Operator_Syntax< Evaluator_Lrs_Min >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


#endif
