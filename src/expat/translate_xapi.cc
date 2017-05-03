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

#include <stdlib.h>


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


struct InputAnalyzer
{
  InputAnalyzer(const std::string& input, bool force_meta = false);

  std::string south, north, east, west;
  bool bbox_found;
  std::vector< std::pair< std::string, std::string > > key_value;
  std::string user;
  unsigned long uid;
  std::string newer;
  unsigned int timeout;
  bool meta_found;
};

InputAnalyzer::InputAnalyzer(const std::string& input_, bool force_meta)
    : bbox_found(false), meta_found(force_meta), timeout(0), uid(0),
      south(""), north(""), east(""), west(""), user(""), newer("")
{
#ifdef HOURLY_TIMEOUT
  timeout = 3600;
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
      uid = atol(input.substr(0, input.find(']')).c_str());
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
      timeout = atoi(input.substr(0, input.find(']')).c_str());
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
      input = input.substr(find_unescaped(input, ']')+1);
    }
  }
}

void print_meta_filter(const InputAnalyzer& analyzer)
{
  if (analyzer.user != "")
    std::cout<<"(user:\""<<escape_xml(analyzer.user)<<"\")";
  if (analyzer.uid != 0)
    std::cout<<"(uid:"<<analyzer.uid<<")";
  if (analyzer.newer != "")
    std::cout<<"(newer:\""<<escape_xml(analyzer.newer)<<"\")";
}


void print_bbox(const InputAnalyzer& analyzer)
{
  std::cout<< "(" << escape_xml(analyzer.south) << ","
                  << escape_xml(analyzer.west)  << ","
                  << escape_xml(analyzer.north) << ","
                  << escape_xml(analyzer.east)  << ")";
}


void print_key_values(const InputAnalyzer& analyzer)
{
  for (std::vector< std::pair< std::string, std::string > >::const_iterator it = analyzer.key_value.begin();
      it != analyzer.key_value.end(); ++it)
  {
    if (it->second == "*")
      std::cout<<"[\""<<escape_xml(xapi_unescape(it->first))<<"\"]";
    else if (find_unescaped(it->second, '|') != std::string::npos)
      std::cout<<"[\""<<escape_xml(xapi_unescape(it->first))<<"\""
               <<"~\"^("<<escape_xml(xapi_unescape(it->second))<<")$\"]";
    else
      std::cout<<"[\""<<escape_xml(xapi_unescape(it->first))<<"\""
               <<"=\""<<escape_xml(xapi_unescape(it->second))<<"\"]";
  }
}


void print_out(const InputAnalyzer& analyzer)
{
  if (analyzer.meta_found)
    std::cout<<"out meta;";
  else
    std::cout<<"out;";
}

void process_nodes(std::string input, bool is_star = false, bool force_meta = false)
{
  InputAnalyzer analyzer(input, force_meta);
  if (analyzer.timeout != 0)
    std::cout<<"[timeout:"<<analyzer.timeout<<"];";

  std::cout<<"node";
  if (analyzer.bbox_found)
    print_bbox(analyzer);
  print_key_values(analyzer);
  print_meta_filter(analyzer);
  std::cout<<";";

  if (!is_star)
    print_out(analyzer);
}

void process_ways(std::string input, bool is_star = false, bool force_meta = false)
{
  InputAnalyzer analyzer(input, force_meta);
  if (!is_star && analyzer.timeout != 0)
    std::cout<<"[timeout:"<<analyzer.timeout<<"];";
  std::cout<<"(";
  if (is_star)
    std::cout<<"._;";

  std::cout<<"way";
  if (analyzer.bbox_found)
    print_bbox(analyzer);
  print_key_values(analyzer);
  print_meta_filter(analyzer);
  std::cout<<";";

  std::cout<<"node(w););";
  print_out(analyzer);
}

void process_relations(std::string input, bool is_star = false, bool force_meta = false)
{
  InputAnalyzer analyzer(input, force_meta);
  if (!is_star && analyzer.timeout != 0)
    std::cout<<"[timeout:"<<analyzer.timeout<<"];";

  std::cout<<"rel";
  if (analyzer.bbox_found)
    print_bbox(analyzer);
  print_key_values(analyzer);
  print_meta_filter(analyzer);
  std::cout<<";";

  print_out(analyzer);
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
    input = input.substr(input.find(',') + 1);
    south = input.substr(0, input.find(','));
    input = input.substr(input.find(',') + 1);
    east = input.substr(0, input.find(','));
    input = input.substr(input.find(',') + 1);
    north = input;
    std::cout << "(node(" << escape_xml(south) << "," << escape_xml(west) << ","
              << escape_xml(north) << "," << escape_xml(east) << ");"
                 "rel(bn)->.foo;"
                 "way(bn);"
                 "node(w)->.foo;"
                 "rel(bw);"
                 ");out meta;";
  }
  else
  {
    std::cout<<"Error: Query must start with 'node', 'way', 'relation', or '*'\n";
    return 1;
  }

  return 0;
}
