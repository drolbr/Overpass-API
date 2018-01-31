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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__GEOMETRY_ENDOMORPHISMS_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__GEOMETRY_ENDOMORPHISMS_H


#include "../data/tag_store.h"
#include "../data/utils.h"
#include "evaluator.h"
#include "statement.h"
#include "unary_functions.h"

#include <map>
#include <string>


/* == Geometry Endomorphisms ==

Geometry endomorphisms are functions with one argument.
Many of them help to normalize or check the value of its argument.
They always first let their argument be evaluated.
Their syntax is always

  <Function Name>(<Evaluator>)
*/

template< typename Evaluator_ >
struct Geometry_Endom_Statement_Maker : public Generic_Statement_Maker< Evaluator_ >
{
  Geometry_Endom_Statement_Maker() : Generic_Statement_Maker< Evaluator_ >(Evaluator_::stmt_name()) {}
};


template< typename Evaluator_ >
struct Geometry_Endom_Evaluator_Maker : public Statement::Evaluator_Maker
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
      Statement* rhs = stmt_factory.create_evaluator(
          tree_it.rhs(), tree_context, Statement::Single_Return_Type_Checker(Statement::geometry));
      if (rhs)
        result->add_statement(rhs, "");
      else if (error_output)
        error_output->add_parse_error(Evaluator_::stmt_func_name() + "(...) needs an argument",
            tree_it->line_col.first);
    }
    return result;
  }

  Geometry_Endom_Evaluator_Maker()
  {
    Statement::maker_by_func_name()[Evaluator_::stmt_func_name()].push_back(this);
  }
};


template< typename Evaluator_ >
struct Evaluator_Geometry_Endom_Syntax : public Evaluator_Geometry_Unary_Function
{
  Evaluator_Geometry_Endom_Syntax(int line_number_, const std::map< std::string, std::string >& input_attributes)
    : Evaluator_Geometry_Unary_Function(line_number_)
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


/* === Center ===

The function <em>center</em> returns the center of its argument.
It expects a function that evaluates to a geometry.
It then delivers the point that is at the center of the bounding box of the geometry.

Its syntax is

  center(<Evaluator>)

*/

class Evaluator_Center : public Evaluator_Geometry_Endom_Syntax< Evaluator_Center >
{
public:
  static Geometry_Endom_Statement_Maker< Evaluator_Center > statement_maker;
  static Geometry_Endom_Evaluator_Maker< Evaluator_Center > evaluator_maker;
  static std::string stmt_func_name() { return "center"; }
  static std::string stmt_name() { return "eval-center"; }

  Evaluator_Center(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Geometry_Endom_Syntax< Evaluator_Center >(line_number_, input_attributes) {}

  virtual Opaque_Geometry* process(Opaque_Geometry* geom) const;
};


/* === Trace ===

The function <em>trace</em> returns the trace of its argument.
It expects a function that evaluates to a geometry.
It then delivers a collection of all segments and nodes that appear in its input.
Ways are split at points that are explicitly in the set.
Every node and segments is contained at most once.

Its syntax is

  trace(<Evaluator>)

*/

class Evaluator_Trace : public Evaluator_Geometry_Endom_Syntax< Evaluator_Trace >
{
public:
  static Geometry_Endom_Statement_Maker< Evaluator_Trace > statement_maker;
  static Geometry_Endom_Evaluator_Maker< Evaluator_Trace > evaluator_maker;
  static std::string stmt_func_name() { return "trace"; }
  static std::string stmt_name() { return "eval-trace"; }

  Evaluator_Trace(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Geometry_Endom_Syntax< Evaluator_Trace >(line_number_, input_attributes) {}

  virtual Opaque_Geometry* process(Opaque_Geometry* geom) const;
};


/* === Hull ===

The function <em>hull</em> returns the convex hull of its argument.
It expects a function that evaluates to a geometry
It then delivers a polygon without holes that contains all of its arguments.

Its syntax is

  hull(<Evaluator>)

*/

class Evaluator_Hull : public Evaluator_Geometry_Endom_Syntax< Evaluator_Hull >
{
public:
  static Geometry_Endom_Statement_Maker< Evaluator_Hull > statement_maker;
  static Geometry_Endom_Evaluator_Maker< Evaluator_Hull > evaluator_maker;
  static std::string stmt_func_name() { return "hull"; }
  static std::string stmt_name() { return "eval-hull"; }

  Evaluator_Hull(int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
      : Evaluator_Geometry_Endom_Syntax< Evaluator_Hull >(line_number_, input_attributes) {}

  virtual Opaque_Geometry* process(Opaque_Geometry* geom) const;
};


#endif
