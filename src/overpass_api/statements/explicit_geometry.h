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
  Eval_Point_Geometry_Task(Eval_Task* lat_, Eval_Task* lon_) : lat(lat_), lon(lon_) {}
  
  virtual ~Eval_Point_Geometry_Task()
  {
    delete lat;
    delete lon;
  }

  virtual Opaque_Geometry* eval() const { return make_point(lat->eval(0), lon->eval(0)); }

  virtual Opaque_Geometry* eval(const Element_With_Context< Node_Skeleton >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Node_Skeleton > >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Way_Skeleton >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Way_Skeleton > >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Relation_Skeleton >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Relation_Skeleton > >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Area_Skeleton >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Derived_Skeleton >& data) const
      { return make_point(lat->eval(data, 0), lon->eval(data, 0)); }
  
  static Opaque_Geometry* make_point(const std::string& lat, const std::string& lon);
  
private:
  Eval_Task* lat;
  Eval_Task* lon;
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
  {
    return indent + "<eval-point>\n"
        + (lat ? lat->dump_xml(indent + "  ") : "")
        + (lon ? lon->dump_xml(indent + "  ") : "")
        + indent + "</eval-point>\n";
  }
  virtual std::string dump_compact_ql(const std::string&) const
  {
    return std::string("pt(") + (lat ? lat->dump_compact_ql("") : "") + ","
        + (lon ? lon->dump_compact_ql("") : "") + ")";
  }

  Evaluator_Point(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-point"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Point() {}

  virtual Requested_Context request_context() const;

  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task("<Point_Geometry>"); }
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context)
  { return new Eval_Point_Geometry_Task(lat->get_string_task(context, 0), lon->get_string_task(context, 0)); }
  virtual bool returns_geometry() const { return true; }

private:
  Evaluator* lat;
  Evaluator* lon;
};


/* === Linestring evaluator ===

This operator always returns a geometry.
The geometry is a line made of the points that are supplied as arguments.

Its syntax is

  lstr(<Evaluator>, <Evaluator>[, ...])
*/


struct Eval_Linestring_Geometry_Task : Eval_Geometry_Task
{
  Eval_Linestring_Geometry_Task(std::vector< Eval_Geometry_Task* >& points_) : points(points_) {}
  
  virtual ~Eval_Linestring_Geometry_Task()
  {
    for (std::vector< Eval_Geometry_Task* >::iterator it = points.begin(); it != points.end(); ++it)
      delete *it;
  }

  virtual Opaque_Geometry* eval() const { return make_linestring(points); }

  virtual Opaque_Geometry* eval(const Element_With_Context< Node_Skeleton >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Node_Skeleton > >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Way_Skeleton >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Way_Skeleton > >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Relation_Skeleton >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Relation_Skeleton > >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Area_Skeleton >& data) const
      { return make_linestring(points, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Derived_Skeleton >& data) const
      { return make_linestring(points, data); }
  
  static Opaque_Geometry* make_linestring(const std::vector< Eval_Geometry_Task* >& points);
  template< typename Context >
  static Opaque_Geometry* make_linestring(
      const std::vector< Eval_Geometry_Task* >& points, const Context& data);
  
private:
  std::vector< Eval_Geometry_Task* > points;
};


class Evaluator_Linestring : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Linestring >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Linestring >("eval-linestring") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_func_name()["lstr"].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<eval-linestring>\n";
    for (std::vector< Evaluator* >::const_iterator it = points.begin(); it != points.end(); ++it)
      result += (*it)->dump_xml(indent + "  ");
    return result + indent + "</eval-linestring>\n";
  }
  virtual std::string dump_compact_ql(const std::string&) const
  {
    std::string result = std::string("lstr(");
    for (std::vector< Evaluator* >::const_iterator it = points.begin(); it != points.end(); ++it)
      result += (*it)->dump_compact_ql("");
    return result + ")";
  }

  Evaluator_Linestring(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-linestring"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Linestring() {}

  virtual Requested_Context request_context() const;

  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task("<Linestring_Geometry>"); }
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context)
  {
    std::vector< Eval_Geometry_Task* > tasks;
    for (std::vector< Evaluator* >::const_iterator it = points.begin(); it != points.end(); ++it)
    {
      tasks.push_back((*it)->get_geometry_task(context));
      if (!tasks.back())
        tasks.pop_back();
    }
    return new Eval_Linestring_Geometry_Task(tasks);
  }
  virtual bool returns_geometry() const { return true; }

private:
  std::vector< Evaluator* > points;
};


/* === Polygon evaluator ===

This operator always returns a geometry.
The geometry is a polygon made of the linestrings that are supplied as arguments.
The polygon adheres to the right hand rule and has no self-intersections.

Its syntax is

  poly(<Evaluator>, <Evaluator>[, ...])
*/


struct Eval_Polygon_Geometry_Task : Eval_Geometry_Task
{
  Eval_Polygon_Geometry_Task(std::vector< Eval_Geometry_Task* >& linestrings_) : linestrings(linestrings_) {}
  
  virtual ~Eval_Polygon_Geometry_Task()
  {
    for (std::vector< Eval_Geometry_Task* >::iterator it = linestrings.begin(); it != linestrings.end(); ++it)
      delete *it;
  }

  virtual Opaque_Geometry* eval() const { return make_polygon(linestrings); }

  virtual Opaque_Geometry* eval(const Element_With_Context< Node_Skeleton >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Node_Skeleton > >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Way_Skeleton >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Way_Skeleton > >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Relation_Skeleton >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Attic< Relation_Skeleton > >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Area_Skeleton >& data) const
      { return make_polygon(linestrings, data); }
  virtual Opaque_Geometry* eval(const Element_With_Context< Derived_Skeleton >& data) const
      { return make_polygon(linestrings, data); }
  
  static Opaque_Geometry* make_polygon(const std::vector< Eval_Geometry_Task* >& linestrings);
  template< typename Context >
  static Opaque_Geometry* make_polygon(
      const std::vector< Eval_Geometry_Task* >& linestrings, const Context& data);
  
private:
  std::vector< Eval_Geometry_Task* > linestrings;
};


class Evaluator_Polygon : public Evaluator
{
public:
  struct Statement_Maker : public Generic_Statement_Maker< Evaluator_Polygon >
  {
    Statement_Maker() : Generic_Statement_Maker< Evaluator_Polygon >("eval-polygon") {}
  };
  static Statement_Maker statement_maker;

  struct Evaluator_Maker : public Statement::Evaluator_Maker
  {
    virtual Statement* create_evaluator(const Token_Node_Ptr& tree_it, QL_Context tree_context,
        Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
    Evaluator_Maker() { Statement::maker_by_func_name()["poly"].push_back(this); }
  };
  static Evaluator_Maker evaluator_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<eval-polygon>\n";
    for (std::vector< Evaluator* >::const_iterator it = linestrings.begin(); it != linestrings.end(); ++it)
      result += (*it)->dump_xml(indent + "  ");
    return result + indent + "</eval-polygon>\n";
  }
  virtual std::string dump_compact_ql(const std::string&) const
  {
    std::string result = std::string("poly(");
    for (std::vector< Evaluator* >::const_iterator it = linestrings.begin(); it != linestrings.end(); ++it)
      result += (*it)->dump_compact_ql("");
    return result + ")";
  }

  Evaluator_Polygon(int line_number_, const std::map< std::string, std::string >& input_attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "eval-polygon"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman) {}
  virtual ~Evaluator_Polygon() {}

  virtual Requested_Context request_context() const;

  virtual Eval_Task* get_string_task(Prepare_Task_Context& context, const std::string* key)
  { return new Const_Eval_Task("<Polygon_Geometry>"); }
  virtual Eval_Geometry_Task* get_geometry_task(Prepare_Task_Context& context)
  {
    std::vector< Eval_Geometry_Task* > tasks;
    for (std::vector< Evaluator* >::const_iterator it = linestrings.begin(); it != linestrings.end(); ++it)
    {
      tasks.push_back((*it)->get_geometry_task(context));
      if (!tasks.back())
        tasks.pop_back();
    }
    return new Eval_Polygon_Geometry_Task(tasks);
  }
  virtual bool returns_geometry() const { return true; }

private:
  std::vector< Evaluator* > linestrings;
};


#endif
