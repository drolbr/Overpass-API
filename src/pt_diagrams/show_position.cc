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

struct Map_Configuration
{
  double pivot_x, pivot_y;
  double pivot_lat, pivot_lon;
  double scale;
  string name;
  vector< Map_Configuration >::size_type parent;
};

vector< Map_Configuration > read_config_file(const string& file_name)
{
  vector< Map_Configuration > result;
  ifstream in(file_name.c_str());
  string buf;
  getline(in, buf);
  while (in.good())
  {
    Map_Configuration config;
    string::size_type pos = 0;
    config.parent = atoi(get_cs_part(buf, pos).c_str());
    config.pivot_x = atof(get_cs_part(buf, pos).c_str());
    config.pivot_y = atof(get_cs_part(buf, pos).c_str());
    config.pivot_lat = atof(get_cs_part(buf, pos).c_str());
    config.pivot_lon = atof(get_cs_part(buf, pos).c_str());
    config.scale = atof(get_cs_part(buf, pos).c_str());
    config.name = buf.substr(pos+1, buf.size()-pos-2);
    result.push_back(config);
    
    getline(in, buf);
  }
  return result;
}

bool is_inside(double lat, double lon, const Map_Configuration& config)
{
  double x = config.pivot_x + 
      (lon - config.pivot_lon)*cos(lat/90.0*acos(0))/360.0*40*1000*1000/config.scale;
  double y = config.pivot_y - 
      (lat - config.pivot_lat)/360.0*40*1000*1000/config.scale;
  return (x >= 0.0 && x <= 600.0 && y >= 0.0 && y <= 600.0);
}

int main(int argc, char *argv[])
{
  string base_directory = "/opt/pt_diagrams/";
  string index_file_name = "index.csv";
  
  string query_string = cgi_get_to_text();
  
  string::size_type pos = 0;
  int file_id = atoi(get_cs_part(query_string, pos).c_str());
  double lat = atof(get_cs_part(query_string, pos).c_str());
  double lon = atof(get_cs_part(query_string, pos).c_str());
  
  vector< Map_Configuration > config_data = read_config_file(base_directory + index_file_name);
  
  int child_id = file_id;
  for (vector< Map_Configuration >::size_type i = 0; i < config_data.size(); ++i)
  {
    if (config_data[i].parent == file_id && is_inside(lat, lon, config_data[i]))
      child_id = i;
  }
  
  cout<<"Content-type: text/html\n\n";

  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
  "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
  "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\"\n"
  "      xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
  "      xmlns:xlink=\"http://www.w3.org/1999/xlink\" lang=\"en\"><head>\n"
  "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
  "<title>OSM3S Response</title>\n"
  "</head>\n"
  "<body>\n"
  "\n"
  "<h1>"<<config_data[file_id].name<<"</h1>\n"
  "\n"
  "<div style=\"position:absolute; top:110px; left:10px; width:690px; height:690px;\">\n"
  "  <object data=\"position_flag?"<<file_id<<","<<lat<<","<<lon<<","
      <<config_data[file_id].pivot_x<<","<<config_data[file_id].pivot_y<<","
      <<config_data[file_id].pivot_lat<<","<<config_data[file_id].pivot_lon<<","
      <<config_data[file_id].scale<<","<<"\" "
      "width=\"600px\" height=\"600px\" type=\"image/svg+xml\">\n"
  "    <img src=\""<<file_id<<".png\" alt=\"the map\"/>\n"
  "  </object>\n"
  "</div>\n"
  "\n"
  "<div style=\"position:absolute; top:110px; left:700px; width:690px; height:690px;\">\n"
  "  <a href=\"show_position?"<<child_id<<","<<lat<<","<<lon<<"\">\n"
  "    <object data=\"/plus.svg\" width=\"80px\" height=\"80px\" type=\"image/svg+xml\">\n"
  "      <img src=\"plus.png\" alt=\"plus sign\"/>\n"
  "    </object>\n"
  "  </a><br/>\n"
  "  <a href=\"show_position?"<<config_data[file_id].parent<<","<<lat<<","<<lon<<"\">\n"
  "    <object data=\"/minus.svg\" width=\"80px\" height=\"80px\" type=\"image/svg+xml\">\n"
  "      <img src=\"minus.png\" alt=\"minus sign\"/>\n"
  "    </object>\n"
  "  </a><br/>\n"
  "  <a href=\"plus.svg\">time</a><br/>\n"
  "  <a href=\"wheelchair.html\">chair</a>\n"
  "</div>\n"
  "\n"
  "</body>\n"
  "</html>\n";

  return 0;
}
