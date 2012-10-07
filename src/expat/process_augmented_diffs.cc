/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of PT_Diagrams.
*
* PT_Diagrams is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PT_Diagrams is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with PT_Diagrams.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../expat/escape_xml.h"
#include "../expat/expat_justparse_interface.h"

using namespace std;


struct Tag
{
  Tag(string name_) : name(name_) {}
  
  map< string, string > attributes;
  vector< Tag* > children;
  string name;
};


vector< Tag* > stack;
Tag* root;


void start(const char* el, const char** attr)
{
  Tag* tag = new Tag(el);
  
  for (unsigned int i(0); attr[i]; i += 2)
  {
    pair< string, string > attribute;
    attribute.first = attr[i];
    attribute.second = attr[i+1];
    tag->attributes.insert(attribute);
  }
  
  if (!stack.empty())
    stack.back()->children.push_back(tag);
  else
    root = tag;
  
  stack.push_back(tag);
}


void end(const char *el)
{
  stack.pop_back();
}


void print_tag(const Tag& tag, string indent)
{
  if (tag.name == "action")
  {
    map< string, string >::const_iterator type_it = tag.attributes.find("type");
    if (type_it == tag.attributes.end())
      return;
    else if (type_it->second == "create") {
      cout<<indent<<"<insert>\n";
      for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
        print_tag(**it, indent + "  ");
      cout<<indent<<"</insert>\n";
    }
    else if (type_it->second == "info")
    {
      cout<<indent<<"<keep>\n";
      for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
        print_tag(**it, indent + "  ");
      cout<<indent<<"</keep>\n";
    }
    else if (type_it->second == "modify")
    {
      cout<<indent<<"<erase>\n";
      print_tag(*tag.children[0]->children[0], indent + "  ");
      cout<<indent<<"</erase>\n";
      cout<<indent<<"<insert>\n";
      print_tag(*tag.children[1]->children[0], indent + "  ");
      cout<<indent<<"</insert>\n";
    }
    else if (type_it->second == "delete")
    {
      cout<<indent<<"<erase>\n";
      print_tag(*tag.children[0]->children[0], indent + "  ");
      cout<<indent<<"</erase>\n";
    }
    return;
  }
  else if (tag.name == "bounds")
    return;
  
  cout<<indent<<"<"<<tag.name;
  for (map< string, string >::const_iterator it = tag.attributes.begin();
       it != tag.attributes.end(); ++it)
     cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
  if (tag.children.empty())
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
      print_tag(**it, indent + "  ");
    cout<<indent<<"</"<<tag.name<<">\n";
  }
}


int main(int argc, char *argv[])
{
  int argi(1);
  string bbox;
  while (argi < argc)
  {
    if (!strncmp("--bbox=", argv[argi], 7))
      bbox = ((string)(argv[argi])).substr(7).c_str();
    ++argi;
  }
  
  // read the XML input
  root = 0;
  stack.clear();
  parse(stdin, start, end);
  
  cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  print_tag(*root, "");
  
  return 0;
}
