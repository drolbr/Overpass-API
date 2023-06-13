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

#include "hash_request.h"


std::string sanitize_string(const std::string& input, bool anonymize)
{
  std::string result = input;
  std::string::size_type src_pos = 0;
  std::string::size_type target_pos = 0;
  while (src_pos < input.size())
  {
    if (isspace(input[src_pos]))
    {
      ++src_pos;
      while (src_pos < input.size() && isspace(input[src_pos]))
        ++src_pos;
      if (target_pos > 0 && src_pos < input.size()
          && (isalpha(result[target_pos-1]) || isdigit(result[target_pos-1]) || result[target_pos-1] == '_')
          && (isalpha(input[src_pos]) || isdigit(input[src_pos]) || input[src_pos] == '_' || input[src_pos] == '-'))
        result[target_pos++] = ' ';
    }
    else if (anonymize && (isdigit(input[src_pos])
        || (input[src_pos] == '-' && src_pos+1 < input.size() && isdigit(input[src_pos+1]))))
    {
      result[target_pos++] = '0';
      ++src_pos;
      while (src_pos < input.size() && (isdigit(input[src_pos]) || input[src_pos] == '.'))
        ++src_pos;
    }
    else if (anonymize && input[src_pos] == '\"' && target_pos >= 5 && result.substr(target_pos-5,5) == "poly:")
    {
      ++src_pos;
      while (src_pos < input.size() && input[src_pos] != '\"')
        ++src_pos;
      result[target_pos++] = '\"';
      result[target_pos++] = '\"';
      ++src_pos;
    }
    else if (anonymize && input[src_pos] == '\'' && target_pos >= 5 && result.substr(target_pos-5,5) == "poly:")
    {
      ++src_pos;
      while (src_pos < input.size() && input[src_pos] != '\'')
        ++src_pos;
      result[target_pos++] = '\'';
      result[target_pos++] = '\'';
      ++src_pos;
    }
    else if (input[src_pos] == '/' && src_pos+1 < input.size())
    {
      if (input[src_pos+1] == '/')
      {
        src_pos += 2;
        while (src_pos < input.size() && input[src_pos] != '\n')
          ++src_pos;
      }
      else if (input[src_pos+1] == '*')
      {
        src_pos += 2;
        while (src_pos < input.size() && (input[src_pos] != '/' || input[src_pos-1] != '*'))
          ++src_pos;
        if (src_pos < input.size())
          ++src_pos;
      }
      else
        result[target_pos++] = input[src_pos++];
    }
    else if ((input[src_pos] & 0xe0) == 0)
      ++src_pos;
    else
      result[target_pos++] = input[src_pos++];
  }
  result.resize(target_pos);
  return result;
}


uint64_t hash(const std::string& input)
{
  uint64_t result = (input.size()<<32) ^ (input.size()<<48);
  int pos = 0;
  for (decltype(input.size()) i = 0; i < input.size(); ++i)
  {
    int spread0 = ((5*i)%17)+8;
    int spread1 = ((5*i)%23)+33;
    uint64_t val = (unsigned char)input[i];
    val = val ^ (val<<spread0);
    val = val ^ (val<<spread1);
    result ^= ((val << pos)|(val >> (64-pos)));
    pos = (pos + 19) & 63;
  }
  return result;
}
