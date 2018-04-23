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

#ifndef DE__OSM3S___OVERPASS_API__CORE__OUTPUT_HANDLER_PARSER_H
#define DE__OSM3S___OVERPASS_API__CORE__OUTPUT_HANDLER_PARSER_H


#include "../../expat/map_ql_input.h"
#include "../core/datatypes.h"
#include "output_handler.h"


#include <map>
#include <string>


class Output_Handler_Parser
{
public:
  // You can inherit from this class
  virtual ~Output_Handler_Parser() {}

  // You need to provide a name token for each subclass
  Output_Handler_Parser(const std::string& format_name);

  // The class to parse the arguments (from ql notation) if any
  // Return null if and only if a fatal error has happened
  virtual Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
					     Tokenizer_Wrapper* token, Error_Output* error_output) = 0;

  static Output_Handler_Parser* get_format_parser(const std::string& format_name);

private:
  // Of each subclass only a single instance should exist - hence no assignment or copies are allowed
  Output_Handler_Parser(const Output_Handler_Parser&);
  Output_Handler_Parser& operator=(const Output_Handler_Parser&);

  static std::map< std::string, Output_Handler_Parser* >& registry();
};


#endif
