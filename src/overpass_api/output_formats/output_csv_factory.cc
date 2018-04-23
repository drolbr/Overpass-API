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
#include "../frontend/tokenizer_utils.h"
#include "../statements/osm_script.h"
#include "output_csv.h"


class Output_CSV_Generator : public Output_Handler_Parser
{
public:
  Output_CSV_Generator() : Output_Handler_Parser("csv") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_CSV_Generator singleton;
};


Output_CSV_Generator Output_CSV_Generator::singleton;


Output_Handler* Output_CSV_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							 Tokenizer_Wrapper* token, Error_Output* error_output)
{
  if (token)
  {
    Csv_Settings csv_settings;

    std::string csv_format_string_field;
    std::string csv_headerline;
    std::string csv_separator("\t");

    clear_until_after(*token, error_output, "(", false);

    if (**token == "(")
    {
      do
      {
        ++(*token);
	bool placeholder = (**token == "::");
	if (placeholder)
	  ++(*token);
        csv_format_string_field = get_text_token(*token, error_output, "CSV format specification");
        csv_settings.keyfields.push_back(std::make_pair(csv_format_string_field, placeholder));
        clear_until_after(*token, error_output, ",", ";", ")", false);
      } while (token->good() && **token == ",");

      if (**token == ";")
      {
        ++(*token);
        csv_headerline = get_text_token(*token, error_output, "CSV output header line (true or false)");
        clear_until_after(*token, error_output, ";", ")", false);
      }
      if (**token == ";")
      {
        ++(*token);
        csv_separator = get_text_token(*token, error_output, "CSV separator character");
      }
      clear_until_after(*token, error_output, ")");
    }

    csv_settings.with_headerline = (csv_headerline == "false" ? false : true);
    csv_settings.separator = csv_separator;

    return new Output_CSV(csv_settings);
  }

  return 0;
}
