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

#include "../data/tag_store.h"
#include "../data/utils.h"
#include "item_geometry.h"


Evaluator_Is_Closed::Statement_Maker Evaluator_Is_Closed::statement_maker;
Element_Function_Maker< Evaluator_Is_Closed > Evaluator_Is_Closed::evaluator_maker;


Evaluator_Is_Closed::Evaluator_Is_Closed
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;

  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Evaluator_Geometry::Statement_Maker Evaluator_Geometry::statement_maker;
Element_Function_Maker< Evaluator_Geometry > Evaluator_Geometry::evaluator_maker;


Evaluator_Geometry::Evaluator_Geometry
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Evaluator_Length::Statement_Maker Evaluator_Length::statement_maker;
Element_Function_Maker< Evaluator_Length > Evaluator_Length::evaluator_maker;


Evaluator_Length::Evaluator_Length
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


//-----------------------------------------------------------------------------


Evaluator_Latitude::Statement_Maker Evaluator_Latitude::statement_maker;
Element_Function_Maker< Evaluator_Latitude > Evaluator_Latitude::evaluator_maker;


Evaluator_Latitude::Evaluator_Latitude
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}


Evaluator_Longitude::Statement_Maker Evaluator_Longitude::statement_maker;
Element_Function_Maker< Evaluator_Longitude > Evaluator_Longitude::evaluator_maker;


Evaluator_Longitude::Evaluator_Longitude
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Evaluator(line_number_)
{
  std::map< std::string, std::string > attributes;
  eval_attributes_array(get_name(), attributes, input_attributes);
}
