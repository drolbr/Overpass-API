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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "cgi-helper.h"


char hex_digit(char c)
{
  if (c <= 57)
  {
    if (c >= 48)
      return c - 48;
    return 16;
  }
  if (c <= 70)
  {
    if (c >= 65)
      return c - 55;
    return 16;
  }
  if (c <= 102)
  {
    if (c >= 97)
      return c - 87;
    return 16;
  }
  return 16;
}

std::string cgi_get_to_text()
{
  char* method;
  method = getenv("REQUEST_METHOD");
  if ((method) && (!strncmp(method, "GET", 4)))
    return getenv("QUERY_STRING");
  if ((method) && (!strncmp(method, "OPTIONS", 8)))
    return getenv("QUERY_STRING");
  if ((method) && (!strncmp(method, "HEAD", 5)))
    return getenv("QUERY_STRING");

  return "";
}

std::string cgi_post_to_text()
{
  std::string raw, buf;
  while (!std::cin.eof())
  {
    getline(std::cin, buf);
    raw += buf + '\n';
  }
  return raw;
}

std::string replace_cgi(const std::string& raw)
{
  std::string result;
  std::string::size_type pos = 0;

  while (pos < raw.size())
  {
    if (raw[pos] == '%')
    {
      if (pos >= raw.size()+2)
	return (result + raw.substr(0, pos));
      char a(hex_digit(raw[pos+1])), b(hex_digit(raw[pos+2]));
      if ((a < 16) && (b < 16))
      {
	result += (char)(a*16 + b);
	pos += 3;
      }
      else
	result += raw[pos++];
    }
    else if (raw[pos] == '+')
    {
      result += ' ';
      ++pos;
    }
    else
      result += raw[pos++];
  }

  return result;
}

std::map< std::string, std::string > decode_cgi_to_plain(const std::string& raw)
{
  std::map< std::string, std::string > result;

  std::string::size_type pos = 0;
  do
  {
    std::string::size_type delim_pos = raw.find('&', pos);
    if (delim_pos == std::string::npos)
      delim_pos = raw.size();

    std::string::size_type middle_pos = raw.find('=', pos);
    if (middle_pos != std::string::npos && middle_pos < delim_pos)
    {
      std::string::size_type end_pos = delim_pos;
      while (end_pos > 0 && isspace(raw[end_pos-1]))
	--end_pos;
      result[raw.substr(pos, middle_pos - pos)] = replace_cgi(raw.substr(middle_pos + 1, end_pos - middle_pos - 1));
    }

    pos = delim_pos + 1;
  }
  while (pos < raw.size());

  if (result["data"] == "")
    result["data"] = raw;

  return result;
}
