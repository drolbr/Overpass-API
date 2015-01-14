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


#include "tokenizer_utils.h"


string decode_to_utf8(const string& token, string::size_type& pos, Error_Output* error_output)
{
  uint val = 0;
  pos += 2;
  string::size_type max_pos = pos + 4;
  if (token.size() < max_pos)
    max_pos = token.size();
  while (pos < max_pos &&
      ((token[pos] >= '0' && token[pos] <= '9')
      || (token[pos] >= 'a' && token[pos] <= 'f')
      || (token[pos] >= 'A' && token[pos] <= 'F')))
  {
    val *= 16;
    if (token[pos] >= '0' && token[pos] <= '9')
      val += (token[pos] - 0x30);
    else if (token[pos] >= 'a' && token[pos] <= 'f')
      val += (token[pos] - 87);
    else if (token[pos] >= 'A' && token[pos] <= 'F')
      val += (token[pos] - 55);
    ++pos;
  }
  if (val < 0x20)
  {
    if (error_output)
      error_output->add_parse_error("Invalid UTF-8 character (value below 32) in escape sequence.", 0);
  }
  else if (val < 0x80)
  {
    string buf = " ";
    buf[0] = val;
    return buf;
  }
  else if (val < 0x800)
  {
    string buf = "  ";
    buf[0] = (0xc0 | (val>>6));
    buf[1] = (0x80 | (val & 0x3f));
    return buf;
  }
  else
  {
    string buf = "   ";
    buf[0] = (0xe0 | (val>>12));
    buf[1] = (0x80 | ((val>>6) & 0x3f));
    buf[2] = (0x80 | (val & 0x3f));
    return buf;
  }
  return "";
}


string get_text_token(Tokenizer_Wrapper& token, Error_Output* error_output,
		      string type_of_token)
{
  string result = "";
  bool result_valid = true;

  if (!token.good() || (*token).size() == 0)
    result_valid = false;
  else if ((*token)[0] == '"' || (*token)[0] == '\'')
  {
    string::size_type start = 1;
    string::size_type pos = (*token).find('\\');
    while (pos != string::npos)
    {
      result += (*token).substr(start, pos - start);
      if ((*token)[pos + 1] == 'n')
        result += '\n';
      else if ((*token)[pos + 1] == 't')
        result += '\t';
      else if ((*token)[pos + 1] == 'u')
      {
        result += decode_to_utf8(*token, pos, error_output);
        pos -= 2;
      }
      else
        result += (*token)[pos + 1];
      start = pos + 2;
      pos = (*token).find('\\', start);
    }
    result += (*token).substr(start, (*token).size() - start - 1);
  }
  else if (isalpha((*token)[0]) || isdigit((*token)[0]) || (*token)[0] == '_')
    result = *token;
  else if ((*token)[0] == '-' && (*token).size() > 1 && isdigit((*token)[1]))
    result = *token;
  else
    result_valid = false;
  
  if (result_valid)
    ++token;
  else
  {
    if (error_output)
      error_output->add_parse_error(type_of_token + " expected - '" + *token + "' found.", token.line_col().first);
  }
  
  return result;
}


void process_after(Tokenizer_Wrapper& token, Error_Output* error_output, bool after)
{
  if (!token.good())
  {
    if (error_output)
      error_output->add_parse_error("Unexpected end of input.", token.line_col().first);
  }
  else if (after)
    ++token;
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       string target_1, bool after)
{
  if (*token != target_1)
  {
    if (error_output)
      error_output->add_parse_error(string("'") + target_1 + "' expected - '"
          + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       string target_1, string target_2, bool after)
{
  if (*token != target_1 && *token != target_2)
  {
    if (error_output)
      error_output->add_parse_error
          (string("'") + target_1 + "' or '" + target_2 + "' expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       string target_1, string target_2, string target_3, bool after)
{
  if (*token != target_1 && *token != target_2 && *token != target_3)
  {
    if (error_output)
      error_output->add_parse_error
      (string("'") + target_1 + "', '" + target_2 + "', or '" + target_3 + "'  expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2 && *token != target_3)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       string target_1, string target_2, string target_3, string target_4,
		       bool after)
{
  if (*token != target_1 && *token != target_2 && *token != target_3 && *token != target_4)
  {
    if (error_output)
      error_output->add_parse_error
          (string("'") + target_1 + "', '" + target_2 + "', '" + target_3 + "', or '"
              + target_4 + "'  expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2 && *token != target_3
      && *token != target_4)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       string target_1, string target_2, string target_3, string target_4,
		       string target_5, bool after)
{
  if (*token != target_1 && *token != target_2 && *token != target_3
      && *token != target_4 && *token != target_5)
  {
    if (error_output)
      error_output->add_parse_error
          (string("'") + target_1 + "', '" + target_2 + "', '" + target_3+ "', '" + target_4
	      + "', or '" + target_5 + "'  expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2 && *token != target_3
      && *token != target_4 && *token != target_5)
    ++token;
  process_after(token, error_output, after);
}
