/*****************************************************************
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <iomanip>
#include <iostream>
#include <vector>

#include <math.h>
#include "../expat_justparse_interface.h"

using namespace std;

const double PI = acos(0)*2;

double brim;

string frame_template()
{
  return "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
    "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
    "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
    "     version=\"1.1\" baseProfile=\"full\"\n"
    "     width=\"$width;px\" height=\"$height;px\">\n"
    "\n"
    "<headline/>\n"
    "\n"
    "<stops-diagram/>\n"
    "\n"
    "</svg>\n";
}

struct Display_Class
{
  Display_Class() : key(""), value(""), text(""), limit(-1) {}
  
  string key;
  string value;
  string text;
  double limit;
};

vector< Display_Class > display_classes;

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "node"))
  {
    double lat(100.0), lon(0.0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "lat"))
	lat = atof(attr[i+1]);
      else if (!strcmp(attr[i], "lon"))
	lon = atof(attr[i+1]);
    }
    cout<<"  <query type=\"node\">\n"
	<<"    <bbox-query "
	<<"n=\""<<setprecision(14)<<lat + brim<<"\" s=\""<<setprecision(14)<<lat - brim<<"\" "
	<<"w=\""<<setprecision(14)<<lon - brim/cos(lat/180.0*PI)<<"\" e=\""<<setprecision(14)<<lon + brim/cos(lat/180.0*PI)<<"\"/>\n"
	<<"    <has-kv k=\"railway\"/>\n"
	<<"  </query>\n"
	<<"  <query type=\"node\">\n"
	<<"    <bbox-query "
	<<"n=\""<<setprecision(14)<<lat + brim<<"\" s=\""<<setprecision(14)<<lat - brim<<"\" "
	<<"w=\""<<setprecision(14)<<lon - brim/cos(lat/180.0*PI)<<"\" e=\""<<setprecision(14)<<lon + brim/cos(lat/180.0*PI)<<"\"/>\n"
	<<"    <has-kv k=\"highway\"/>\n"
	<<"  </query>\n";
    for (vector< Display_Class >::const_iterator it(display_classes.begin());
        it != display_classes.end(); ++it)
    {
      double limit(it->limit);
      if (limit < 0)
	limit = brim;
      cout<<"  <query type=\"node\">\n"
	  <<"    <bbox-query "
	  <<"n=\""<<setprecision(14)<<lat + limit<<"\" s=\""<<setprecision(14)<<lat - limit<<"\" "
	  <<"w=\""<<setprecision(14)<<lon - limit/cos(lat/180.0*PI)<<"\" e=\""<<setprecision(14)<<lon + limit/cos(lat/180.0*PI)<<"\"/>\n"
	  <<"    <has-kv k=\""<<it->key<<"\" v=\""<<it->value<<"\"/>\n"
	  <<"  </query>\n";
    }
  }
}

void end(const char *el)
{
}

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
  bool display_only_corrs(false);
  
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
    if (!strncmp("--only-corrs", argv[argi], 12))
      display_only_corrs = true;
    ++argi;
  }
  
  if (display_only_corrs)
  {
    cout<<brim<<'\n';
    return 0;
  }
  brim = brim/1000.0/40000.0*360.0;
  
  cout<<"<osm-script>\n"
      <<"\n"
      <<"<union>\n";
  
  // read the XML input
  parse(stdin, start, end);
  
  cout<<"</union>\n"
      <<"<print mode=\"body\"/>\n"
      <<"<union>\n"
      //<<"  <item/>\n"
      <<"  <recurse type=\"node-relation\"/>\n"
      <<"</union>\n"
      <<"<print mode=\"body\"/>\n"
      <<"\n"
      <<"</osm-script>\n";
  
  return 0;
}
