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


Opaque_Geometry* Eval_Point_Geometry_Task::make_point(const std::string& lat_s, const std::string& lon_s) const
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
    Statement* first = stmt_factory.create_evaluator(tree_it.rhs().lhs(), tree_context);
    if (first)
      result->add_statement(first, "");
    else if (error_output)
      error_output->add_parse_error("First argument of pt(...) must be an evualator", tree_it->line_col.first);

    Statement* second = stmt_factory.create_evaluator(tree_it.rhs().rhs(), tree_context);
    if (second)
      result->add_statement(second, "");
    else if (error_output)
      error_output->add_parse_error("Second argument of pt(...) must be an evualator", tree_it->line_col.second);
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
  Evaluator* tag_value_ = dynamic_cast< Evaluator* >(statement);
  if (!tag_value_)
    substatement_error(get_name(), statement);
  else if (!lat)
    lat = tag_value_;
  else if (!lon)
    lon = tag_value_;
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
