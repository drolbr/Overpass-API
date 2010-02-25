/*****************************************************************
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <iomanip>
#include <iostream>

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
  }
}

void end(const char *el)
{
}

int main(int argc, char *argv[])
{
  brim = 0.009;
  
  int argi(1);
  while (argi < argc)
  {
    if (!strncmp("--size=", argv[argi], 7))
      brim = atof(((string)(argv[argi])).substr(7).c_str())/1000.0/40000.0*360.0;
    ++argi;
  }
  
  cout<<"<osm-script timeout=\"180\" element-limit=\"10000000\">\n"
      <<"\n"
      <<"<union>\n";
  
  // read the XML input
  parse(stdin, start, end);
  
  cout<<"</union>\n"
      <<"<union>\n"
      <<"  <item/>\n"
      <<"  <recurse type=\"node-relation\"/>\n"
      <<"</union>\n"
      <<"<print mode=\"body\"/>\n"
      <<"\n"
      <<"</osm-script>\n";
  
  return 0;
}
