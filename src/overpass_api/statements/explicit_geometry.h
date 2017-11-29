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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__EXPLICIT_GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__EXPLICIT_GEOMETRY_H


#include "evaluator.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


/* === Point evaluator ===

This operator always returns a geometry.
The geometry is a point at the latitude and longitude from the given values.

Its syntax is

  pt(<Value>, <Value>)
*/


struct Eval_Point_Geometry_Task : Eval_Geometry_Task
{
  virtual ~Eval_Point_Geometry_Task() {}

  virtual Opaque_Geometry* eval() const
  {
    Compound_Geometry* geom = new Compound_Geometry();
    geom->add_component(new Point_Geometry(51.35, 7.05));
    std::vector< Point_Double > points;
    points.push_back(Point_Double(50.87, 6.98));
    points.push_back(Point_Double(50.88, 7.01));
    points.push_back(Point_Double(50.89, 7.07));
    geom->add_component(new Linestring_Geometry(points));
    return geom;
  }
};


class Evaluator_Point : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Point >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Point >("eval-point") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_func_name()["pt"].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

  virtual std::string dump_xml(const std::string& indent) const
  { return indent + "<eval-point/>\n"; }
  virtual std::string dump_compact_ql(const std::string&) const
  { return "pt(...)"; }

  Evaluator_Point(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-point"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Point() {}

  virtual Requested_Context request_context() const { return Requested_Context(); }

  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task("[51.25, 7.15]"); }
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context)
  { return new Eval_Point_Geometry_Task(); }

private:
};


#endif
