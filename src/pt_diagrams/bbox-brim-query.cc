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

#include "../expat/expat_justparse_interface.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

double brim;

const double PI = acos(0)*2;

struct Display_Class
{
  Display_Class() : key(""), value(""), text(""), limit(-1) {}
  
  string key;
  string value;
  string text;
  double limit;
};

vector< Display_Class > display_classes;

void options_start(const char *el, const char **attr)
{
  if (!strcmp(el, "correspondences"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "limit"))
	brim = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "display"))
  {
    Display_Class dc;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	dc.key = attr[i+1];
      else if (!strcmp(attr[i], "v"))
	dc.value = attr[i+1];
      else if (!strcmp(attr[i], "text"))
	dc.text = attr[i+1];
      else if (!strcmp(attr[i], "limit"))
	dc.limit = atof(attr[i+1]);
    }
    dc.limit = dc.limit/1000.0/40000.0*360.0;
    display_classes.push_back(dc);
  }
}

void options_end(const char *el)
{
}

int main(int argc, char *argv[])
{
  string network, ref, operator_;
  
  brim = 0.0;
  
  int argi(1);
  // check on early run for options only
  while (argi < argc)
  {
    if (!strncmp("--options=", argv[argi], 10))
    {
      FILE* options_file(fopen(((string)(argv[argi])).substr(10).c_str(), "r"));
      if (!options_file)
	return 0;
      parse(options_file, options_start, options_end);
    }
    ++argi;
  }
  // get every other argument
  argi = 1;
  while (argi < argc)
  {
    if (!strncmp("--size=", argv[argi], 7))
      brim = atof(((string)(argv[argi])).substr(7).c_str());
    if (!strncmp("--network=", argv[argi], 10))
      network = string(argv[argi]).substr(10);
    if (!strncmp("--ref=", argv[argi], 6))
      ref = string(argv[argi]).substr(6);
    if (!strncmp("--operator=", argv[argi], 11))
      operator_ = string(argv[argi]).substr(11);
    ++argi;
  }
  
	//The case insensitve search (case=ignore) was added to the script for the
	//network and ref values to reduce zero results returned for a user
	//who capitalizes the word incorrectly when submitting the web form
  cout<<"<osm-script>\n"
      <<"\n"
      <<"<query type=\"relation\">\n"
      <<"  <has-kv case=\"ignore\" k=\"network\" regv=\""<<network<<"\"/>\n"
      <<"  <has-kv case=\"ignore\" k=\"ref\" regv=\""<<ref<<"\"/>\n";
  if (operator_ != "")
    cout<<"  <has-kv k=\"operator\" v=\""<<operator_<<"\"/>\n";
  cout<<"</query>\n"
      <<"<recurse type=\"relation-node\" into=\"stops\"/>\n";
      
  if (brim == 0.0)
  {
    cout<<"<union>\n"
        <<"  <item/>\n"
	<<"  <item set=\"stops\"/>\n"
	<<"</union>\n"
	<<"<print/>\n"
	<<"\n"
	<<"</osm-script>\n";
  }
  else
  {
    cout<<"<union>\n"
        <<"  <query type=\"node\">\n"
	<<"    <around from=\"stops\" radius=\""<<brim<<"\"/>\n"
	<<"    <has-kv k=\"railway\"/>\n"
	<<"  </query>\n"
	<<"  <query type=\"node\">\n"
	<<"    <around from=\"stops\" radius=\""<<brim<<"\"/>\n"
	<<"    <has-kv k=\"highway\"/>\n"
	<<"  </query>\n"
	<<"  <query type=\"node\">\n"
	<<"    <around from=\"stops\" radius=\""<<brim<<"\"/>\n"
	<<"    <has-kv k=\"public_transport\"/>\n"
	<<"  </query>\n"
	<<"  <query type=\"node\">\n"
	<<"    <around from=\"stops\" radius=\""<<brim<<"\"/>\n"
	<<"    <has-kv k=\"amenity\" v=\"ferry_terminal\"/>\n"
	<<"  </query>\n"
	<<"</union>\n"
	<<"<union>\n"
	<<"  <item/>\n"
	<<"  <recurse type=\"node-relation\"/>\n";
	
    for (vector< Display_Class >::const_iterator it(display_classes.begin());
        it != display_classes.end(); ++it)
    {
      cout<<"  <query type=\"node\">\n"
      <<"    <around from=\"stops\" radius=\""<<brim<<"\"/>\n"
      <<"    <has-kv k=\""<<it->key<<"\" v=\""<<it->value<<"\"/>\n"
      <<"  </query>\n";
    }
    
    cout<<"</union>\n"
        <<"<print/>\n"
        <<"\n"
        <<"</osm-script>\n";
  }
  
  return 0;
}
