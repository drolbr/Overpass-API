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

#include "../expat/expat_justparse_interface.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

double brim;

const double PI = acos(0)*2;

struct Display_Class
{
  Display_Class() : key(""), value(""), text(""), limit(-1) {}

  std::string key;
  std::string value;
  std::string text;
  double limit;
};

std::vector< Display_Class > display_classes;

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
  std::string network, ref, operator_;

  brim = 0.0;

  int argi(1);
  // check on early run for options only
  while (argi < argc)
  {
    if (!strncmp("--options=", argv[argi], 10))
    {
      FILE* options_file(fopen(((std::string)(argv[argi])).substr(10).c_str(), "r"));
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
      brim = atof(((std::string)(argv[argi])).substr(7).c_str());
    if (!strncmp("--network=", argv[argi], 10))
      network = std::string(argv[argi]).substr(10);
    if (!strncmp("--ref=", argv[argi], 6))
      ref = std::string(argv[argi]).substr(6);
    if (!strncmp("--operator=", argv[argi], 11))
      operator_ = std::string(argv[argi]).substr(11);
    ++argi;
  }

  if (network.find(';') != std::string::npos)
    std::cout<<"rel[network=\""<<network<<"\"]";
  else
    std::cout<<"rel(if:lrs_in(\""<<network<<"\", t[\"network\"]))";
  std::cout<<"[ref=\""<<ref<<"\"]";
  if (operator_ != "")
    std::cout<<"[operator=\""<<operator_<<"\"]";
  std::cout<<";\n"
      "node(r)->.stops;\n";

  if (brim == 0.0)
    std::cout<<"( ._; .stops; );\n"
        "out;\n";
  else
  {
    std::cout<<"(\n"
        "  node(around.stops:"<<brim<<")[railway];\n"
        "  node(around.stops:"<<brim<<")[highway];\n"
        "  node(around.stops:"<<brim<<")[public_transport];\n"
        "  node(around.stops:"<<brim<<")[amenity=ferry_terminal];\n"
        ");\n"
        "( ._; rel(bn);\n";

    for (std::vector< Display_Class >::const_iterator it(display_classes.begin());
        it != display_classes.end(); ++it)
      std::cout<<"node(around.stops:"<<brim<<")["<<it->key<<"="<<it->value<<"]\n";

    std::cout<<"\n);\n"
        "out;\n";
  }

  return 0;
}
