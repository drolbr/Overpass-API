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

#include "escape_xml.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

string xapi_unescape(string input)
{
  string result;
  string::size_type pos = 0;
  while (pos < input.length())
  {
    if (input[pos] != '\\')
      result += input[pos];
    else
    {
      ++pos;
      if (pos >= input.length())
	break;
      result += input[pos];
    }
    ++pos;
  }
  return result;
}


string::size_type find_unescaped(const string& input, char c)
{
  if (input[0] == c)
    return 0;
  string::size_type pos = input.find(c);
  while (pos != string::npos)
  {
    if (input[pos-1] != '\\')
      return pos;
    int backslash_pos = pos-1;
    while (backslash_pos >= 0 && input[backslash_pos] == '\\')
      --backslash_pos;
    if ((pos - backslash_pos) % 2 == 1)
      return pos;
    pos = input.find(c, pos+1);
  }
  return string::npos;
}


struct InputAnalizer
{
  InputAnalizer(const string& input, bool force_meta = false);
  
  string south, north, east, west;
  bool bbox_found;
  vector< pair< string, string > > key_value;
  string user;
  string uid;
  string newer;
  string timeout;
  bool meta_found;
};

InputAnalizer::InputAnalizer(const string& input_, bool force_meta)
    : bbox_found(false), meta_found(force_meta)
{
#ifdef HOURLY_TIMEOUT
  timeout = "3600";
#endif
    
  string input = input_;
  while (!input.empty())
  {
    if (input[0] != '[')
    {
      cout<<"Error: Text before '[' found.\n";
      throw string();
    }
    if (input.substr(0, 6) == "[bbox=")
    {
      if (bbox_found)
      {
	cout<<"Error: At most one bbox allowed.\n";
	throw string();
      }
      bbox_found = true;
      input = input.substr(6);
      west = input.substr(0, input.find(','));
      input = input.substr(input.find(',')+1);
      south = input.substr(0, input.find(','));
      input = input.substr(input.find(',')+1);
      east = input.substr(0, input.find(','));
      input = input.substr(input.find(',')+1);
      north = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 7) == "[@meta]")
    {
      meta_found = true;
      input = input.substr(7);
    }
    else if (input.substr(0, 7) == "[@user=")
    {
      input = input.substr(7);
      user = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 6) == "[@uid=")
    {
      input = input.substr(6);
      uid = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 8) == "[@newer=")
    {
      input = input.substr(8);
      newer = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else if (input.substr(0, 10) == "[@timeout=")
    {
      input = input.substr(10);
      timeout = input.substr(0, input.find(']'));
      input = input.substr(input.find(']')+1);
    }
    else
    {
      key_value.push_back(make_pair("", ""));
      input = input.substr(1);
      if (find_unescaped(input, '=') != string::npos && find_unescaped(input, ']') != string::npos &&
	 find_unescaped(input, '=') < find_unescaped(input, ']'))
      {
        key_value.back().first = input.substr(0, find_unescaped(input, '='));
        input = input.substr(find_unescaped(input, '=')+1);
        key_value.back().second = input.substr(0, find_unescaped(input, ']'));
      }
      else
      {
	if (find_unescaped(input, ']') == string::npos)
	  cout<<"Error: Expected ']' after value.\n";
	else
	  cout<<"Error: Expected '=' after key.\n";
	throw string();
      }
      if (key_value.back().second == "*")
	key_value.back().second="";
      input = input.substr(find_unescaped(input, ']')+1);
    }
  }
}

void print_meta(const InputAnalizer& analizer, string prefix)
{
  if (analizer.user != "")
    cout<<prefix<<"<user name=\""<<escape_xml(analizer.user)<<"\"/>\n";
  if (analizer.uid != "")
    cout<<prefix<<"<user uid=\""<<escape_xml(analizer.uid)<<"\"/>\n";
  if (analizer.newer != "")
    cout<<prefix<<"<newer than=\""<<escape_xml(analizer.newer)<<"\"/>\n";
}

void print_meta(const InputAnalizer& analizer, string prefix, string type)
{
  if (analizer.user != "")
    cout<<prefix<<"<user type=\""<<escape_xml(type)<<"\" name=\""<<escape_xml(analizer.user)<<"\"/>\n";
  else if (analizer.uid != "")
    cout<<prefix<<"<user type=\""<<escape_xml(type)<<"\" uid=\""<<escape_xml(analizer.uid)<<"\"/>\n";
  if (analizer.newer != "")
    cout<<prefix<<"<query type=\""<<escape_xml(type)<<"\">\n"
                  "  <item/>\n"
		  "  <newer than=\""<<escape_xml(analizer.newer)<<"\"/>\n"
		  "</query>\n";
}

void print_bbox(const InputAnalizer& analizer, string prefix)
{
  cout<<prefix<<
      "<bbox-query s=\""<<escape_xml(analizer.south)<<"\" n=\""<<escape_xml(analizer.north)
      <<"\" w=\""<<escape_xml(analizer.west)<<"\" e=\""<<escape_xml(analizer.east)<<"\"/>\n";
}


void print_key_values(const InputAnalizer& analizer)
{
  for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
      it != analizer.key_value.end(); ++it)
  {
    if (it->second == "*")
      cout<<"  <has-kv k=\""<<escape_xml(xapi_unescape(it->first))<<"\"/>\n";
    else if (find_unescaped(it->second, '|') != string::npos)
      cout<<"  <has-kv k=\""<<escape_xml(xapi_unescape(it->first))
          <<"\" regv=\"^("<<escape_xml(xapi_unescape(it->second))<<")$\"/>\n";
    else
      cout<<"  <has-kv k=\""<<escape_xml(xapi_unescape(it->first))
          <<"\" v=\""<<escape_xml(xapi_unescape(it->second))<<"\"/>\n";
  }
}


void print_print(const InputAnalizer& analizer)
{
  if (analizer.meta_found)
    cout<<"<print mode=\"meta\"/>\n";
  else
    cout<<"<print/>\n";
}

void process_nodes(string input, bool is_star = false, bool force_meta = false)
{
  InputAnalizer analizer(input, force_meta);
  if (analizer.timeout != "")
    cout<<"<osm-script timeout=\""<<escape_xml(analizer.timeout)<<"\">\n\n";
  
  cout<<"<query type=\"node\">\n";
  if (analizer.bbox_found)
    print_bbox(analizer, "  ");
  print_key_values(analizer);
  print_meta(analizer, "  ");
  cout<<"</query>\n";
    
  if (!is_star)
  {
    print_print(analizer);
    if (analizer.timeout != "")
      cout<<"\n</osm-script>\n";
  }
}

void process_ways(string input, bool is_star = false, bool force_meta = false)
{
  InputAnalizer analizer(input, force_meta);
  if (!is_star && analizer.timeout != "")
    cout<<"<osm-script timeout=\""<<escape_xml(analizer.timeout)<<"\">\n\n";
  cout<<"<union>\n";
  if (is_star)
    cout<<"  <item/>\n";
  
  cout<<"  <query type=\"way\">\n";
  if (analizer.bbox_found)
    print_bbox(analizer, "    ");
  for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
      it != analizer.key_value.end(); ++it)
  {
    if (it->second == "*")
      cout<<"    <has-kv k=\""<<escape_xml(it->first)<<"\"/>\n";
    else if (it->second.find('|') != string::npos)
      cout<<"    <has-kv k=\""<<escape_xml(it->first)<<"\" regv=\"^("<<escape_xml(it->second)<<")$\"/>\n";
    else
      cout<<"    <has-kv k=\""<<escape_xml(it->first)<<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
  }
  print_meta(analizer, "    ");
  cout<<"  </query>\n";

  cout<<"  <recurse type=\"way-node\"/>\n"
        "</union>\n";
  print_print(analizer);
  if (!is_star && analizer.timeout != "")
    cout<<"\n</osm-script>\n";
}

void process_relations(string input, bool is_star = false, bool force_meta = false)
{
  InputAnalizer analizer(input, force_meta);
  if (!is_star && analizer.timeout != "")
    cout<<"<osm-script timeout=\""<<escape_xml(analizer.timeout)<<"\">\n\n";
  
  cout<<"<query type=\"relation\">\n";
  if (analizer.bbox_found)
    print_bbox(analizer, "  ");
  for (vector< pair< string, string > >::const_iterator it = analizer.key_value.begin();
      it != analizer.key_value.end(); ++it)
  {
    if (it->second == "*")
      cout<<"  <has-kv k=\""<<escape_xml(it->first)<<"\"/>\n";
    else if (it->second.find('|') != string::npos)
      cout<<"  <has-kv k=\""<<escape_xml(it->first)<<"\" regv=\"^("<<escape_xml(it->second)<<")$\"/>\n";
    else
      cout<<"  <has-kv k=\""<<escape_xml(it->first)<<"\" v=\""<<escape_xml(it->second)<<"\"/>\n";
  }
  print_meta(analizer, "  ");
  cout<<"</query>\n";
    
  print_print(analizer);
  if (analizer.timeout != "")
    cout<<"\n</osm-script>\n";
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    return 1;
  
  string input;
  bool force_meta = false;
  if (argc == 2)
    input = argv[1];
  else if (argc == 3 && string(argv[1]) == string("--force-meta"))
  {
    force_meta = true;
    input = argv[2];
  }
  
  if (input.substr(0, 4) == "node")
  {
    try
    {
      process_nodes(input.substr(4), false, force_meta);
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 3) == "way")
  {
    try
    {
      process_ways(input.substr(3), false, force_meta);
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 8) == "relation")
  {
    try
    {
      process_relations(input.substr(8), false, force_meta);
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 1) == "*")
  {
    try
    {
      process_nodes(input.substr(1), true, force_meta);
      process_ways(input.substr(1), true, force_meta);
      process_relations(input.substr(1), true, force_meta);
    }
    catch (string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 9) == "map?bbox=")
  {
    string south, north, east, west;
    input = input.substr(9);
    west = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    south = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    east = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    north = input;
    cout<<"<union>\n"
          "  <bbox-query s=\""<<escape_xml(south)<<"\" n=\""<<escape_xml(north)<<"\" w=\""
            <<escape_xml(west)<<"\" e=\""<<escape_xml(east)<<"\"/>\n"
          "  <recurse type=\"node-relation\" into=\"foo\"/>\n"
          "  <recurse type=\"node-way\"/>\n"
	  "  <recurse type=\"way-node\" into=\"foo\"/>\n"
	  "  <recurse type=\"way-relation\"/>\n"
          "</union>\n"
          "<print mode=\"meta\"/>\n";
  }
  else
  {
    cout<<"Error: Query must start with 'node', 'way', 'relation', or '*'\n";
    return 1;
  }

  return 0;
}
