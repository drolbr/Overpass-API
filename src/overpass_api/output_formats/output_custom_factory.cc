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
#include "output_custom.h"


class Output_Custom_Generator : public Output_Handler_Parser
{
public:
  Output_Custom_Generator() : Output_Handler_Parser("custom") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_Custom_Generator singleton;
};


Output_Custom_Generator Output_Custom_Generator::singleton;


Output_Handler* Output_Custom_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							    Tokenizer_Wrapper* token, Error_Output* error_output)
{
  std::map< std::string, std::string >::const_iterator redirect_it = input_params.find("redirect");

  std::map< std::string, std::string >::const_iterator url_it = input_params.find("url");
  std::string url = (url_it == input_params.end() ?
      "https://www.openstreetmap.org/browse/{{{type}}}/{{{id}}}" : url_it->second);

  std::map< std::string, std::string >::const_iterator template_it = input_params.find("template");
  std::string template_name = (template_it == input_params.end() ? "default.wiki" : template_it->second);
  // sanity check for template_name
  if (template_name.find("/") != std::string::npos)
  {
    if (error_output)
      error_output->add_encoding_error("Parameter \"template\" must not contain slashes.");
    template_name = "";
  }

  return new Output_Custom(redirect_it == input_params.end() || redirect_it->second != "no",
      template_name, url);
}
