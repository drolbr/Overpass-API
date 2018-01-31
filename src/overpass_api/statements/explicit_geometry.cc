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


#include "explicit_geometry.h"


Opaque_Geometry* Eval_Point_Geometry_Task::make_point(const std::string& lat_s, const std::string& lon_s)
{
  double lat_d = 0;
  double lon_d = 0;
  if (try_double(lat_s, lat_d) && lat_d >= -90. && lat_d <= 90.
      && try_double(lon_s, lon_d) && lon_d >= -180. && lon_d <= 180.)
    return new Point_Geometry(lat_d, lon_d);
  
  return new Null_Geometry();
}


Evaluator_Point::Statement_Maker Evaluator_Point::statement_maker;
Evaluator_Point::Evaluator_Maker Evaluator_Point::evaluator_maker;


Statement* Evaluator_Point::Evaluator_Maker::create_evaluator(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, true))
    return 0;

  std::map< std::string, std::string > attributes;
  Evaluator_Point* result = new Evaluator_Point(tree_it->line_col.first, attributes, global_settings);
  if (tree_it.rhs()->token == "," && tree_it.rhs()->lhs && tree_it.rhs()->rhs)
  {
    Statement* first = stmt_factory.create_evaluator(
        tree_it.rhs().lhs(), tree_context, Statement::Single_Return_Type_Checker(Statement::string));
    if (first)
      result->add_statement(first, "");
    else if (error_output)
      error_output->add_parse_error("First argument of pt(...) must be an evaluator", tree_it->line_col.first);

    Statement* second = stmt_factory.create_evaluator(
        tree_it.rhs().rhs(), tree_context, Statement::Single_Return_Type_Checker(Statement::string));
    if (second)
      result->add_statement(second, "");
    else if (error_output)
      error_output->add_parse_error("Second argument of pt(...) must be an evaluator", tree_it->line_col.first);
  }
  else if (error_output)
    error_output->add_parse_error("pt(...) needs two arguments", tree_it->line_col.first);

  return result;
}


Evaluator_Point::Evaluator_Point
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_), lat(0), lon(0)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


void Evaluator_Point::add_statement(Statement* statement, std::string text)
{
  Evaluator* eval = dynamic_cast< Evaluator* >(statement);
  if (!eval)
    substatement_error(get_name(), statement);
  else if (!lat)
    lat = eval;
  else if (!lon)
    lon = eval;
  else
    add_static_error(get_name() + " must have exactly two evaluator substatements.");
}


Requested_Context Evaluator_Point::request_context() const
{
  if (lat && lon)
  {
    Requested_Context result = lat->request_context();
    result.add(lon->request_context());
    return result;
  }
  else if (lat)
    return lat->request_context();
  else if (lon)
    return lon->request_context();
  
  return Requested_Context();
}


//-----------------------------------------------------------------------------


Opaque_Geometry* Eval_Linestring_Geometry_Task::make_linestring(const std::vector< Eval_Geometry_Task* >& tasks)
{
  std::vector< Point_Double > points;
  for (std::vector< Eval_Geometry_Task* >::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
  {
    Owner< Opaque_Geometry > geom((*it)->eval());
    if (geom && geom->has_center())
      points.push_back(Point_Double(geom->center_lat(), geom->center_lon()));
  }
  return new Linestring_Geometry(points);
}


template< typename Context >
Opaque_Geometry* Eval_Linestring_Geometry_Task::make_linestring(
      const std::vector< Eval_Geometry_Task* >& tasks, const Context& data)
{
  std::vector< Point_Double > points;
  for (std::vector< Eval_Geometry_Task* >::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
  {
    Owner< Opaque_Geometry > geom((*it)->eval(data));
    if (geom && geom->has_center())
      points.push_back(Point_Double(geom->center_lat(), geom->center_lon()));
  }
  return new Linestring_Geometry(points);
}


Evaluator_Linestring::Statement_Maker Evaluator_Linestring::statement_maker;
Evaluator_Linestring::Evaluator_Maker Evaluator_Linestring::evaluator_maker;


Statement* Evaluator_Linestring::Evaluator_Maker::create_evaluator(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, true))
    return 0;

  std::map< std::string, std::string > attributes;
  Evaluator_Linestring* result = new Evaluator_Linestring(tree_it->line_col.first, attributes, global_settings);
  
  std::vector< Token_Node_Ptr > args;
  Token_Node_Ptr args_tree = tree_it.rhs();
  while (args_tree->token == ",")
  {
    args.push_back(args_tree.rhs());
    args_tree = args_tree.lhs();
  }
  args.push_back(args_tree);
  std::reverse(args.begin(), args.end());
  
  for (std::vector< Token_Node_Ptr >::const_iterator it = args.begin(); it != args.end(); ++it)
  {
    Evaluator* sub = dynamic_cast< Evaluator* >(
        stmt_factory.create_evaluator(
            *it, tree_context, Statement::Single_Return_Type_Checker(Statement::geometry)));
    if (sub)
    {
      if (sub->return_type() == Statement::geometry)
        result->add_statement(sub, "");
      else if (error_output)
        error_output->add_parse_error(
            "Every argument of lstr(...) must be a geometry evaluator", (*it)->line_col.first);
    }
    else if (error_output)
      error_output->add_parse_error("Every argument of lstr(...) must be an evaluator", (*it)->line_col.first);
  }

  return result;
}


Evaluator_Linestring::Evaluator_Linestring
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


void Evaluator_Linestring::add_statement(Statement* statement, std::string text)
{
  Evaluator* eval = dynamic_cast< Evaluator* >(statement);
  if (!eval)
    substatement_error(get_name(), statement);
  else
    points.push_back(eval);
}


Requested_Context Evaluator_Linestring::request_context() const
{
  Requested_Context result;
  for (std::vector< Evaluator* >::const_iterator it = points.begin(); it != points.end(); ++it)
    result.add((*it)->request_context());
  return result;
}


//-----------------------------------------------------------------------------


Opaque_Geometry* Eval_Polygon_Geometry_Task::make_polygon(const std::vector< Eval_Geometry_Task* >& tasks)
{
  Free_Polygon_Geometry polygon;
  for (std::vector< Eval_Geometry_Task* >::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
  {
    Owner< Opaque_Geometry > geom((*it)->eval());
    if (geom && geom->has_line_geometry())
      polygon.add_linestring(*geom->get_line_geometry());
  }
  return new RHR_Polygon_Geometry(polygon);
}


template< typename Context >
Opaque_Geometry* Eval_Polygon_Geometry_Task::make_polygon(
      const std::vector< Eval_Geometry_Task* >& tasks, const Context& data)
{
  Free_Polygon_Geometry polygon;
  for (std::vector< Eval_Geometry_Task* >::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
  {
    Owner< Opaque_Geometry > geom((*it)->eval(data));
    if (geom && geom->has_line_geometry())
      polygon.add_linestring(*geom->get_line_geometry());
  }
  return new RHR_Polygon_Geometry(polygon);
}


Evaluator_Polygon::Statement_Maker Evaluator_Polygon::statement_maker;
Evaluator_Polygon::Evaluator_Maker Evaluator_Polygon::evaluator_maker;


Statement* Evaluator_Polygon::Evaluator_Maker::create_evaluator(
    const Token_Node_Ptr& tree_it, Statement::QL_Context tree_context,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  if (!tree_it.assert_is_function(error_output) || !tree_it.assert_has_input_set(error_output, false)
      || !tree_it.assert_has_arguments(error_output, true))
    return 0;

  std::map< std::string, std::string > attributes;
  Evaluator_Polygon* result = new Evaluator_Polygon(tree_it->line_col.first, attributes, global_settings);
  
  std::vector< Token_Node_Ptr > args;
  Token_Node_Ptr args_tree = tree_it.rhs();
  while (args_tree->token == ",")
  {
    args.push_back(args_tree.rhs());
    args_tree = args_tree.lhs();
  }
  args.push_back(args_tree);
  std::reverse(args.begin(), args.end());
  
  for (std::vector< Token_Node_Ptr >::const_iterator it = args.begin(); it != args.end(); ++it)
  {
    Evaluator* sub = dynamic_cast< Evaluator* >(
        stmt_factory.create_evaluator(
            *it, tree_context, Statement::Single_Return_Type_Checker(Statement::geometry)));
    if (sub)
    {
      if (sub->return_type() == Statement::geometry)
        result->add_statement(sub, "");
      else if (error_output)
        error_output->add_parse_error(
            "Every argument of poly(...) must be a geometry evaluator", (*it)->line_col.first);
    }
    else if (error_output)
      error_output->add_parse_error("Every argument of poly(...) must be an evaluator", (*it)->line_col.first);
  }

  return result;
}


Evaluator_Polygon::Evaluator_Polygon
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


void Evaluator_Polygon::add_statement(Statement* statement, std::string text)
{
  Evaluator* eval = dynamic_cast< Evaluator* >(statement);
  if (!eval)
    substatement_error(get_name(), statement);
  else
    linestrings.push_back(eval);
}


Requested_Context Evaluator_Polygon::request_context() const
{
  Requested_Context result;
  for (std::vector< Evaluator* >::const_iterator it = linestrings.begin(); it != linestrings.end(); ++it)
    result.add((*it)->request_context());
  return result;
}
