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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__STRING_ENDOMORPHISMS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__STRING_ENDOMORPHISMS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"
#include "unary_functions.h"

#include <map>
#include <string>


/* == String Endomorphisms ==

String endomorphisms are functions with one argument.
Many of them help to normalize or check the value of its argument.
They always first let their argument be evaluated.
Their syntax is always

  <Function Name>(<Evaluator>)
*/

template< typename Evaluator_ >
struct String_Endom_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  String_Endom_Statement_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name()) {}
};


template< typename Evaluator_ >
struct String_Endom_Evaluator_Maker : public Statement::Evaluator_Maker
{
  virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
      Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
  {
    if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
        || !tree_it.assert_has_arguments(error_output, true))
      return 0;
  
    std::map< std::string, std::string > attributes;
    Statement* result = new Evaluator_(tree_it->line_col.first, attributes, global_settings);
    if (result)
    {
      Statement* rhs = stmt_factory.create_evaluator(tree_it.rhs(), tree_context, Statement::string);
      if (rhs)
        result->add_statement(rhs, "");
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
    }
    return result;
  }

  String_Endom_Evaluator_Maker()
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


template< typename Evaluator_ >
struct Evaluator_String_Endom_Syntax : public Evaluator_Unary_Function
{
  Evaluator_String_Endom_Syntax(int line_number_, const std::map< std::string, std::string >& input_attributes)
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


/* === Number Check, Normalizer and Suffix ===

The function <em>number</em> turns its argument into a number.
If its argument starts with a number then <em>number</em> returns that number in a normalized format.
Otherwise it returns "NaN".
The function <em>is_number</em> checks whether its argument starts with a number.
It returns "1" if its argument can be parsed as a number and "0" otherwise.
The function <em>suffix</em> returns the suffix if any after the number in its argument.
If the argument does not start with a number then it returns the empty string.

Their syntaxes are

  number(<Evaluator>)

resp.

  is_number(<Evaluator>)
  
resp.

  suffix(<Evaluator>)

*/

class Evaluator_Number : public Evaluator_String_Endom_Syntax< Evaluator_Number >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Number > statement_maker;
  static String_Endom_Evaluator_Maker< Evaluator_Number > evaluator_maker;
  static std::string stmt_func_name() { return "number"; }
  static std::string stmt_name() { return "eval-number"; }

  Evaluator_Number(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Number >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


class Evaluator_Is_Num : public Evaluator_String_Endom_Syntax< Evaluator_Is_Num >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Is_Num > statement_maker;
  static String_Endom_Evaluator_Maker< Evaluator_Is_Num > evaluator_maker;
  static std::string stmt_func_name() { return "is_number"; }
  static std::string stmt_name() { return "eval-is-number"; }

  Evaluator_Is_Num(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Is_Num >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


class Evaluator_Suffix : public Evaluator_String_Endom_Syntax< Evaluator_Suffix >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Suffix > statement_maker;
  static String_Endom_Evaluator_Maker< Evaluator_Suffix > evaluator_maker;
  static std::string stmt_func_name() { return "suffix"; }
  static std::string stmt_name() { return "eval-suffix"; }

  Evaluator_Suffix(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Suffix >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


/* === Date Check and Normalizer ===

The function <em>date</em> turns its argument into a number representing a date.
If its argument is a date then <em>date</em> returns the number representing its argument's value.
Otherwise it returns "NaD".
The function <em>is_date</em> checks whether its argument represents a date.
It returns "1" if its argument can be parsed as a date and "0" otherwise.

A string is parsed for a date as follows:
* the first group of digits is understood as year
* the next group of digits if present is understood as month
* then the next group if present is understood as day
* if more groups of digits are present then they are understood as hour, minute, second
To be a date the year must be bigger than 1000,
the month if present less or equal 12,
the day if present less or equal 31,
the hour if present less or equal 24,
and the minute and second if present less or equal 60.

The date parser may get more liberal in future versions and accept more representations of dates.

The functions' syntaxes are

  date(<Evaluator>)

resp.

  is_date(<Evaluator>)
*/

class Evaluator_Date : public Evaluator_String_Endom_Syntax< Evaluator_Date >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Date > statement_maker;
  static String_Endom_Evaluator_Maker< Evaluator_Date > evaluator_maker;
  static std::string stmt_func_name() { return "date"; }
  static std::string stmt_name() { return "eval-date"; }

  Evaluator_Date(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Date >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


class Evaluator_Is_Date : public Evaluator_String_Endom_Syntax< Evaluator_Is_Date >
{
public:
  static String_Endom_Statement_Maker< Evaluator_Is_Date > statement_maker;
  static String_Endom_Evaluator_Maker< Evaluator_Is_Date > evaluator_maker;
  static std::string stmt_func_name() { return "is_date"; }
  static std::string stmt_name() { return "eval-is-date"; }

  Evaluator_Is_Date(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_String_Endom_Syntax< Evaluator_Is_Date >(line_number_, input_attributes) {}

  virtual std::string process(const std::string& rhs_result) const;
};


#endif
