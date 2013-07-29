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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
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
  if ((method) && (!strncmp(method, "GET", 4)))
    return getenv("QUERY_STRING");
  if ((method) && (!strncmp(method, "OPTIONS", 8)))
    return getenv("QUERY_STRING");
  if ((method) && (!strncmp(method, "HEAD", 5)))
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

string replace_cgi(const string& raw)
{
  string result;
  string::size_type pos = 0;
  
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

string decode_cgi_to_plain(const string& raw, int& error,
			   string& jsonp, string& url, bool& redirect, string& template_name)
{
  error = 0;
  string result;
  string::size_type pos = raw.find("data=");
  
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      endpos = raw.size();
    while (endpos > 0 && isspace(raw[endpos-1]))
      --endpos;
    
    result = replace_cgi(raw.substr(pos + 5, endpos - pos - 5));
  }
  else
    return raw;
  
  pos = raw.find("jsonp=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      endpos = raw.size();
    while (endpos > 0 && isspace(raw[endpos-1]))
      --endpos;
    
    jsonp = replace_cgi(raw.substr(pos + 6, endpos - pos - 6));
  }
  
  pos = raw.find("url=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      endpos = raw.size();
    while (endpos > 0 && isspace(raw[endpos-1]))
      --endpos;
    
    url = replace_cgi(raw.substr(pos + 4, endpos - pos - 4));
  }
  
  pos = raw.find("template=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      endpos = raw.size();
    while (endpos > 0 && isspace(raw[endpos-1]))
      --endpos;
    
    template_name = replace_cgi(raw.substr(pos + 9, endpos - pos - 9));
  }
  
  pos = raw.find("redirect=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      endpos = raw.size();
    while (endpos > 0 && isspace(raw[endpos-1]))
      --endpos;
    
    redirect = !(raw.substr(pos + 9, endpos - pos - 9) == "no");
  }

  pos = raw.find("bbox=");
  if (pos != string::npos)
  {
    string::size_type endpos = raw.find('&', pos);
    if (endpos == string::npos)
      endpos = raw.size();
    while (endpos > 0 && isspace(raw[endpos-1]))
      --endpos;
    
    string lonlat = replace_cgi(raw.substr(pos + 5, endpos - pos - 5));
    
    vector< string > coords;
    pos = 0;
    string::size_type newpos = lonlat.find(",");
    while (newpos != string::npos)
    {
      coords.push_back(lonlat.substr(pos, newpos - pos));
      pos = newpos + 1;
      newpos = lonlat.find(",", pos);
    }
    coords.push_back(lonlat.substr(pos));

    if (coords.size() == 4)
    {
      string latlon = coords[1] + "," + coords[0] + "," + coords[3] + "," + coords[2];
      
      pos = result.find("(bbox)");
      while (pos != string::npos)
      {
        result = result.substr(0, pos) + "(" + latlon + ")" + result.substr(pos + 6);
        pos = result.find("(bbox)");
      }
      
      pos = result.find("[bbox]");
      if (pos != string::npos)
        result = result.substr(0, pos) + "[bbox:" + latlon + "]" + result.substr(pos + 6);
    }
  }
  
  return result;
}
