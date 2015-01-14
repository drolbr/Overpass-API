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


#include "../core/datatypes.h"
#include "../../expat/map_ql_input.h"


#include <string>


std::string decode_to_utf8(const std::string& token, std::string::size_type& pos, Error_Output* error_output);

std::string get_text_token(Tokenizer_Wrapper& token, Error_Output* error_output,
		      std::string type_of_token);

void process_after(Tokenizer_Wrapper& token, Error_Output* error_output, bool after);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, std::string target_4,
		       bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, std::string target_4,
		       std::string target_5, bool after = true);
