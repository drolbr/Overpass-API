#include "../expat/expat_justparse_interface.h"
#include "../overpass_api/core/settings.h"
#include "../overpass_api/frontend/cgi-helper.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

string get_cs_part(const string& s, string::size_type& pos)
{
  if (pos == string::npos)
    return "";
  string::size_type old_pos = pos;
  pos = s.find(',', pos);
  if (pos == string::npos)
    return s.substr(old_pos);
  ++pos;
  return s.substr(old_pos, pos - old_pos - 1);
}

int main(int argc, char *argv[])
{
  string base_directory = "/opt/pt_diagrams/";
  
  string query_string = cgi_get_to_text();
  
  string::size_type pos = 0;
  int file_id = atoi(get_cs_part(query_string, pos).c_str());
  double lat = atof(get_cs_part(query_string, pos).c_str());
  double lon = atof(get_cs_part(query_string, pos).c_str());
  double pivot_x = atof(get_cs_part(query_string, pos).c_str());
  double pivot_y = atof(get_cs_part(query_string, pos).c_str());
  double pivot_lat = atof(get_cs_part(query_string, pos).c_str());
  double pivot_lon = atof(get_cs_part(query_string, pos).c_str());
  double scale = atof(get_cs_part(query_string, pos).c_str());
  
  cout<<"Content-type: image/svg+xml\n\n";

  ostringstream source_file_name;
  source_file_name<<base_directory<<file_id<<".svg";
  ifstream in(source_file_name.str().c_str());
  
  string buf;
  getline(in, buf);
  while (in.good())
  {
    if (buf.find("</svg>") != string::npos)
    {
      double x = pivot_x + (lon - pivot_lon)*cos(lat/90.0*acos(0))/360.0*40*1000*1000/scale;
      double y = pivot_y - (lat - pivot_lat)/360.0*40*1000*1000/scale;
      cout<<"<polyline fill=\"#ffff00\" stroke=\"none\""
      " points=\""<<x<<" "<<y - 80<<", "<<x + 80<<" "<<y - 80<<", "<<x + 80<<" "<<y - 40<<
      ", "<<x<<" "<<y - 40<<"\"/>\n";
      cout<<"<polyline fill=\"none\" stroke=\"#000000\" stroke-width=\"5px\""
      " points=\""<<x<<" "<<y<<", "<<x<<" "<<y - 80<<", "<<x + 80<<" "<<y - 80<<
      ", "<<x + 80<<" "<<y - 40<<", "<<x<<" "<<y - 40<<"\"/>\n";
    }
    cout<<buf<<'\n';
    getline(in, buf);
  }
  
/*  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
  "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
  "     version=\"1.1\" baseProfile=\"full\"\n"
  "     width=\"400px\" height=\"200px\">\n"
  "\n"
  "<text x=\"10\" y=\"10\">"<<query_string<<"</text>\n"
  "<text x=\"10\" y=\"30\">"<<base_directory<<"</text>\n"
  "<text x=\"10\" y=\"50\">"<<lat<<"</text>\n"
  "<text x=\"10\" y=\"70\">"<<lon<<"</text>\n"
  "<text x=\"10\" y=\"90\">"<<pivot_x<<"</text>\n"
  "<text x=\"10\" y=\"110\">"<<pivot_y<<"</text>\n"
  "<text x=\"10\" y=\"130\">"<<pivot_lat<<"</text>\n"
  "<text x=\"10\" y=\"150\">"<<pivot_lon<<"</text>\n"
  "<text x=\"10\" y=\"170\">"<<scale<<"</text>\n"
  "<text x=\"10\" y=\"190\">"<<file_id<<"</text>\n"
  "\n"
  "</svg>\n";*/
  
  return 0;
}
