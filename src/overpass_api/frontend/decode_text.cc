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


#include "decode_text.h"


void decode_to_utf8(const std::string& input, std::string& result,
                    std::string::size_type& from, std::string::size_type& to,
                    Error_Output* error_output)
{
  std::string::size_type limit = from + 4;
  if (result.size() < limit)
    limit = result.size();
  
  uint val = 0;
  while (from < limit)
  {
    val *= 16;
    if (input[from] <= '9' && input[from] >= '0')
      val += (input[from] - '0');
    else if (input[from] >= 'a' && input[from] <= 'f')
      val += (input[from] - ('a' - 10));
    else if (input[from] >= 'A' && input[from] <= 'F')
      val += (input[from] - ('A' - 10));
    else
      break;
    ++from;
  }
  if (val < 0x20)
  {
    if (error_output)
      error_output->add_parse_error("Invalid UTF-8 character (value below 32) in escape sequence.", 0);
  }
  else if (val < 0x80)
    result[to++] = val;
  else if (val < 0x800)
  {
    result[to++] = 0xc0 | (val>>6);
    result[to++] = 0x80 | (val & 0x3f);
  }
  else
  {
    result[to++] = 0xe0 | (val>>12);
    result[to++] = 0x80 | ((val>>6) & 0x3f);
    result[to++] = 0x80 | (val & 0x3f);
  }
}


std::string decode_json(const std::string& input, Error_Output* error_output, uint frame_size)
{
  if (frame_size > 0 && (input.empty() || input[0] != '\"' && input[0] != '\''))
    return input;
  
  std::string::size_type j = 0;
  std::string result(input.size(), '\x0');
  for (std::string::size_type i = frame_size; i < input.size()-frame_size; ++i)
  {
    if (input[i] == '\\')
    {
      ++i;
      if (input[i] == 'n')
        result[j++] = '\n';
      else if (input[i] == 't')
        result[j++] = '\t';
      else if (input[i] == 'u')
      {
        decode_to_utf8(input, result, ++i, j, error_output);
        --i;
      }
      else
        result[j++] = input[i];
    }
    else
      result[j++] = input[i];
  }
  result.resize(j);
    
  return result;
}


std::string decode_to_utf8(const std::string& token, std::string::size_type& pos, Error_Output* error_output)
{
  uint val = 0;
  pos += 2;
  std::string::size_type max_pos = pos + 4;
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
    std::string buf = " ";
    buf[0] = val;
    return buf;
  }
  else if (val < 0x800)
  {
    std::string buf = "  ";
    buf[0] = (0xc0 | (val>>6));
    buf[1] = (0x80 | (val & 0x3f));
    return buf;
  }
  else
  {
    std::string buf = "   ";
    buf[0] = (0xe0 | (val>>12));
    buf[1] = (0x80 | ((val>>6) & 0x3f));
    buf[2] = (0x80 | (val & 0x3f));
    return buf;
  }
  return "";
}
