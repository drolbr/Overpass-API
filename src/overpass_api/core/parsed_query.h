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

#ifndef DE__OSM3S___OVERPASS_API__CORE__PARSED_QUERY_H
#define DE__OSM3S___OVERPASS_API__CORE__PARSED_QUERY_H


#include "../frontend/output_handler.h"
#include "../frontend/output_handler_parser.h"
#include "geometry.h"


/* This class collects all the information gathered during parsing.
   Thus it models a query immediately before execution. */
class Parsed_Query
{
public:
  Parsed_Query() : output_handler(0), global_bbox_limitation(Bbox_Double::invalid), last_dispensed_id(0ull) {}
  ~Parsed_Query() { delete output_handler; }

  Output_Handler* get_output_handler() const { return output_handler; }
  void set_output_handler(Output_Handler_Parser* parser,
			  Tokenizer_Wrapper* token, Error_Output* error_output);
  void set_global_bbox(const Bbox_Double& bbox) { global_bbox_limitation = bbox; }
  void set_api_key(const std::string& api_key_) { api_key = api_key_; }

  const std::map< std::string, std::string >& get_input_params() const { return input_params; }
  void set_input_params(const std::map< std::string, std::string >& input_params_) { input_params = input_params_; }

  void trigger_print_bounds() const;
  const Bbox_Double& get_global_bbox_limitation() const { return global_bbox_limitation; }
  std::string get_api_key() const { return api_key; }

  Derived_Skeleton::Id_Type dispense_derived_id() { return ++last_dispensed_id; }

private:
  // The class has ownership of objects - hence no assignment or copies are allowed
  Parsed_Query(const Parsed_Query&);
  Parsed_Query& operator=(const Parsed_Query&);

  Output_Handler* output_handler;
  Bbox_Double global_bbox_limitation;
  std::map< std::string, std::string > input_params;
  Derived_Skeleton::Id_Type last_dispensed_id;
  std::string api_key;
};


inline void Parsed_Query::set_output_handler(Output_Handler_Parser* parser,
					     Tokenizer_Wrapper* token, Error_Output* error_output)
{
  delete output_handler;
  output_handler = parser->new_output_handler(input_params, token, error_output);
}


inline void Parsed_Query::trigger_print_bounds() const
{
  if (global_bbox_limitation.valid() && output_handler)
    output_handler->print_global_bbox(global_bbox_limitation);
}


#endif
