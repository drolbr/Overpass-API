/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include "cgi-helper.h"

using namespace std;

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

string cgi_get_to_text()
{
  char* method;
  method = getenv("REQUEST_METHOD");
  if ((method) && (!strcmp(method, "GET")))
    return getenv("QUERY_STRING");
  
  return "";
}

string cgi_post_to_text()
{
  string raw, buf;
  while (!cin.eof())
  {
    getline(cin, buf);
    raw += buf + '\n';
  }
  return raw;
}

string decode_cgi_to_plain(const string& raw, int& error,
			   string& jsonp,
			   string& url, bool& redirect,
			   string& node_template_name,
			   string& way_template_name,
			   string& relation_template_name)
{
  error = 0;
  string result;
  string::size_type pos(raw.find("data="));
  if ((pos >= raw.size()) || (pos == string::npos))
  {
    error = 1;
    return "";
  }
  pos += 5;
  
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
    else if (raw[pos] == '&')
      pos = raw.size();
    else
      result += raw[pos++];
  }
  
  pos = raw.find("jsonp=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      jsonp = raw.substr(pos + 6);
    else
      jsonp = raw.substr(pos + 6, endpos - pos - 6);
  }
  
  pos = raw.find("url=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      url = raw.substr(pos + 4);
    else
      url = raw.substr(pos + 4, endpos - pos - 4);
  }
  
  pos = raw.find("node_template=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      node_template_name = raw.substr(pos + 14);
    else
      node_template_name = raw.substr(pos + 14, endpos - pos - 14);
  }
  
  pos = raw.find("way_template=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      way_template_name = raw.substr(pos + 13);
    else
      way_template_name = raw.substr(pos + 13, endpos - pos - 13);
  }
  
  pos = raw.find("relation_template=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      relation_template_name = raw.substr(pos + 18);
    else
      relation_template_name = raw.substr(pos + 18, endpos - pos - 18);
  }
  
  pos = raw.find("redirect=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    string redirect_s;
    if (endpos == string::npos)
      redirect_s = raw.substr(pos + 9);
    else
      redirect_s = raw.substr(pos + 9, endpos - pos - 9);
    redirect = !(redirect_s == "no");
  }

  return result;
}
