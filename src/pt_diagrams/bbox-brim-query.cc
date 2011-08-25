#include "../expat/expat_justparse_interface.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

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
  string network, ref;
  
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
    ++argi;
  }
  
  cout<<"<osm-script>\n"
      <<"\n"
      <<"<query type=\"relation\">\n"
      <<"  <has-kv k=\"network\" v=\""<<network<<"\"/>\n"
      <<"  <has-kv k=\"ref\" v=\""<<ref<<"\"/>\n"
      <<"</query>\n"
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
