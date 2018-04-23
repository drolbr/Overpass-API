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

#include "../frontend/output_handler_parser.h"
#include "output_xml.h"


class Output_XML_Generator : public Output_Handler_Parser
{
public:
  Output_XML_Generator() : Output_Handler_Parser("xml") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_XML_Generator singleton;
};


Output_XML_Generator Output_XML_Generator::singleton;


Output_Handler* Output_XML_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							 Tokenizer_Wrapper* token, Error_Output* error_output)
{
  return new Output_XML();
}
