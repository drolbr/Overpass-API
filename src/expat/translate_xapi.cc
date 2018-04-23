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

#include "escape_xml.h"

#include <iostream>
#include <string>
#include <vector>


std::string xapi_unescape(std::string input)
{
  std::string result;
  std::string::size_type pos = 0;
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


std::string::size_type find_unescaped(const std::string& input, char c)
{
  if (input[0] == c)
    return 0;
  std::string::size_type pos = input.find(c);
  while (pos != std::string::npos)
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
  return std::string::npos;
}


struct InputAnalizer
{
  InputAnalizer(const std::string& input, bool force_meta = false);

  std::string south, north, east, west;
  bool bbox_found;
  std::vector< std::pair< std::string, std::string > > key_value;
  std::string user;
  std::string uid;
  std::string newer;
  std::string timeout;
  bool meta_found;
};

InputAnalizer::InputAnalizer(const std::string& input_, bool force_meta)
    : bbox_found(false), meta_found(force_meta)
{
#ifdef HOURLY_TIMEOUT
  timeout = "3600";
#endif

  std::string input = input_;
  while (!input.empty())
  {
    if (input[0] != '[')
    {
      std::cout<<"Error: Text before '[' found.\n";
      throw std::string();
    }
    if (input.substr(0, 6) == "[bbox=")
    {
      if (bbox_found)
      {
	std::cout<<"Error: At most one bbox allowed.\n";
	throw std::string();
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
      key_value.push_back(std::make_pair("", ""));
      input = input.substr(1);
      if (find_unescaped(input, '=') != std::string::npos && find_unescaped(input, ']') != std::string::npos &&
	 find_unescaped(input, '=') < find_unescaped(input, ']'))
      {
        key_value.back().first = input.substr(0, find_unescaped(input, '='));
        input = input.substr(find_unescaped(input, '=')+1);
        key_value.back().second = input.substr(0, find_unescaped(input, ']'));
      }
      else
      {
	if (find_unescaped(input, ']') == std::string::npos)
	  std::cout<<"Error: Expected ']' after value.\n";
	else
	  std::cout<<"Error: Expected '=' after key.\n";
	throw std::string();
      }
      if (key_value.back().second == "*")
	key_value.back().second = "";
      input = input.substr(find_unescaped(input, ']')+1);
    }
  }
}

void print_meta(const InputAnalizer& analizer, std::string prefix)
{
  if (analizer.user != "")
    std::cout<<prefix<<"<user name=\""<<escape_xml(analizer.user)<<"\"/>\n";
  if (analizer.uid != "")
    std::cout<<prefix<<"<user uid=\""<<escape_xml(analizer.uid)<<"\"/>\n";
  if (analizer.newer != "")
    std::cout<<prefix<<"<newer than=\""<<escape_xml(analizer.newer)<<"\"/>\n";
}

void print_meta(const InputAnalizer& analizer, std::string prefix, std::string type)
{
  if (analizer.user != "")
    std::cout<<prefix<<"<user type=\""<<escape_xml(type)<<"\" name=\""<<escape_xml(analizer.user)<<"\"/>\n";
  else if (analizer.uid != "")
    std::cout<<prefix<<"<user type=\""<<escape_xml(type)<<"\" uid=\""<<escape_xml(analizer.uid)<<"\"/>\n";
  if (analizer.newer != "")
    std::cout<<prefix<<"<query type=\""<<escape_xml(type)<<"\">\n"
                  "  <item/>\n"
		  "  <newer than=\""<<escape_xml(analizer.newer)<<"\"/>\n"
		  "</query>\n";
}

void print_bbox(const InputAnalizer& analizer, std::string prefix)
{
  std::cout<<prefix<<
      "<bbox-query s=\""<<escape_xml(analizer.south)<<"\" n=\""<<escape_xml(analizer.north)
      <<"\" w=\""<<escape_xml(analizer.west)<<"\" e=\""<<escape_xml(analizer.east)<<"\"/>\n";
}


void print_key_values(const InputAnalizer& analizer, const std::string& space)
{
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = analizer.key_value.begin();
      it != analizer.key_value.end(); ++it)
  {
    if (it->second == "*")
      std::cout<<space<<"<has-kv k=\""<<escape_xml(xapi_unescape(it->first))<<"\"/>\n";
    else if (find_unescaped(it->second, '|') != std::string::npos)
      std::cout<<space<<"<has-kv k=\""<<escape_xml(xapi_unescape(it->first))
          <<"\" regv=\"^("<<escape_xml(xapi_unescape(it->second))<<")$\"/>\n";
    else
      std::cout<<space<<"<has-kv k=\""<<escape_xml(xapi_unescape(it->first))
          <<"\" v=\""<<escape_xml(xapi_unescape(it->second))<<"\"/>\n";
  }
}


void print_print(const InputAnalizer& analizer)
{
  if (analizer.meta_found)
    std::cout<<"<print mode=\"meta\"/>\n";
  else
    std::cout<<"<print/>\n";
}

void process_nodes(std::string input, bool is_star = false, bool force_meta = false)
{
  InputAnalizer analizer(input, force_meta);
  if (analizer.timeout != "")
    std::cout<<"<osm-script timeout=\""<<escape_xml(analizer.timeout)<<"\">\n\n";

  std::cout<<"<query type=\"node\">\n";
  if (analizer.bbox_found)
    print_bbox(analizer, "  ");
  print_key_values(analizer, "  ");
  print_meta(analizer, "  ");
  std::cout<<"</query>\n";

  if (!is_star)
  {
    print_print(analizer);
    if (analizer.timeout != "")
      std::cout<<"\n</osm-script>\n";
  }
}

void process_ways(std::string input, bool is_star = false, bool force_meta = false)
{
  InputAnalizer analizer(input, force_meta);
  if (!is_star && analizer.timeout != "")
    std::cout<<"<osm-script timeout=\""<<escape_xml(analizer.timeout)<<"\">\n\n";
  std::cout<<"<union>\n";
  if (is_star)
    std::cout<<"  <item/>\n";

  std::cout<<"  <query type=\"way\">\n";
  if (analizer.bbox_found)
    print_bbox(analizer, "    ");
  print_key_values(analizer, "    ");
  print_meta(analizer, "    ");
  std::cout<<"  </query>\n";

  std::cout<<"  <recurse type=\"way-node\"/>\n"
        "</union>\n";
  print_print(analizer);
  if (!is_star && analizer.timeout != "")
    std::cout<<"\n</osm-script>\n";
}

void process_relations(std::string input, bool is_star = false, bool force_meta = false)
{
  InputAnalizer analizer(input, force_meta);
  if (!is_star && analizer.timeout != "")
    std::cout<<"<osm-script timeout=\""<<escape_xml(analizer.timeout)<<"\">\n\n";

  std::cout<<"<query type=\"relation\">\n";
  if (analizer.bbox_found)
    print_bbox(analizer, "  ");
  print_key_values(analizer, "  ");
  print_meta(analizer, "  ");
  std::cout<<"</query>\n";

  print_print(analizer);
  if (analizer.timeout != "")
    std::cout<<"\n</osm-script>\n";
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    return 1;

  std::string input;
  bool force_meta = false;
  if (argc == 2)
    input = argv[1];
  else if (argc == 3 && std::string(argv[1]) == std::string("--force-meta"))
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
    catch (std::string& s)
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
    catch (std::string& s)
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
    catch (std::string& s)
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
    catch (std::string& s)
    {
      return 1;
    }
  }
  else if (input.substr(0, 9) == "map?bbox=")
  {
    std::string south, north, east, west;
    input = input.substr(9);
    west = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    south = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    east = input.substr(0, input.find(','));
    input = input.substr(input.find(',')+1);
    north = input;
    std::cout<<"<union>\n"
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
    std::cout<<"Error: Query must start with 'node', 'way', 'relation', or '*'\n";
    return 1;
  }

  return 0;
}
