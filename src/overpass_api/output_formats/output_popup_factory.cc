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
#include "output_popup.h"


class Output_Popup_Generator : public Output_Handler_Parser
{
public:
  Output_Popup_Generator() : Output_Handler_Parser("popup") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_Popup_Generator singleton;
};


Output_Popup_Generator Output_Popup_Generator::singleton;


Output_Handler* Output_Popup_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							   Tokenizer_Wrapper* token, Error_Output* error_output)
{
  if (token)
  {
    std::vector< Category_Filter* > categories;

    clear_until_after(*token, error_output, "(", "]", false);
    while (token->good() && **token == "(")
    {
      ++(*token);

      Category_Filter* category = new Category_Filter();

      category->set_title(get_text_token(*token, error_output, "title"));
      clear_until_after(*token, error_output, ";", ")", true);

      while (token->good() && **token == "[")
      {
	std::vector< Tag_Filter* > filter_conjunction;

        while (token->good() && **token == "[")
	{
	  ++(*token);

          std::string key = get_text_token(*token, error_output, "Key");
	  std::string value = ".";
          bool straight = true;

          clear_until_after(*token, error_output, "!", "~", "=", "!=", "]", false);

          if (**token == "!")
          {
	    straight = false;
	    ++(*token);
	    clear_until_after(*token, error_output, "~", "=", "]", false);
          }

          if (**token == "=" || **token == "!=")
          {
	    straight = (**token == "=");
	    ++(*token);
	    value = "^" + get_text_token(*token, error_output, "Value") + "$";
          }
          else if (**token == "~")
          {
	    ++(*token);
	    value = get_text_token(*token, error_output, "Value");
          }
	  clear_until_after(*token, error_output, "]");

	  filter_conjunction.push_back(new Tag_Filter(key, value, straight));
	}

        clear_until_after(*token, error_output, ";", true);

	category->add_filter(filter_conjunction);
      }

      if (**token != ")")
      {
	category->set_title_key(get_text_token(*token, error_output, "title key"));
        clear_until_after(*token, error_output, ";", true);
      }
      clear_until_after(*token, error_output, ")", true);

      categories.push_back(category);
    }

    return new Output_Popup(categories);
  }
  else
    return 0;
}
