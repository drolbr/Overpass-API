#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "read_input.h"
#include "test_output.h"

using namespace std;

double middle_lon(const OSMData& current_data)
{
  double east(-180.0), west(180.0);
  for (map< uint32, Node* >::const_iterator it(current_data.nodes.begin());
  it != current_data.nodes.end(); ++it)
  {
    if (it->second->lon < west)
      west = it->second->lon;
    if (it->second->lon > east)
      east = it->second->lon;
  }
  
  return (east + west)/2.0;
}

const vector< unsigned int >& default_colors()
{
  static vector< unsigned int > colors;
  if (colors.empty())
  {
    colors.push_back(0x0000ff);
    colors.push_back(0x00ffff);
    colors.push_back(0x007777);
    colors.push_back(0x00ff00);
    colors.push_back(0xffff00);
    colors.push_back(0x777700);
    colors.push_back(0x777777);
    colors.push_back(0xff0000);
    colors.push_back(0xff00ff);
    colors.push_back(0x770077);
  }
  return colors;
}

char hex(int i)
{
  if (i < 10)
    return (i + '0');
  else
    return (i + 'a' - 10);
}

string default_color(string ref)
{
  unsigned int numval(0), pos(0), color(0);
  while ((pos < ref.size()) && (!isdigit(ref[pos])))
    ++pos;
  while ((pos < ref.size()) && (isdigit(ref[pos])))
  {
    numval = numval*10 + (ref[pos] - 48);
    ++pos;
  }
  
  if (numval == 0)
  {
    for (unsigned int i(0); i < ref.size(); ++i)
      numval += i*(unsigned char)ref[i];
    numval = numval % 1000;
  }
  
  if (numval < 10)
    color = default_colors()[numval];
  else if (numval < 100)
  {
    unsigned int color1 = default_colors()[numval % 10];
    unsigned int color2 = default_colors()[numval/10];
    color = ((((color1 & 0x0000ff)*3 + (color2 & 0x0000ff))/4) & 0x0000ff)
    | ((((color1 & 0x00ff00)*3 + (color2 & 0x00ff00))/4) & 0x00ff00)
    | ((((color1 & 0xff0000)*3 + (color2 & 0xff0000))/4) & 0xff0000);
  }
  else
  {
    unsigned int color1 = default_colors()[numval % 10];
    unsigned int color2 = default_colors()[numval/10%10];
    unsigned int color3 = default_colors()[numval/100%10];
    color = ((((color1 & 0x0000ff)*10 + (color2 & 0x0000ff)*5 + (color3 & 0x0000ff))
    /16) & 0x0000ff)
    | ((((color1 & 0x00ff00)*10 + (color2 & 0x00ff00)*5 + (color3 & 0x00ff00))
    /16) & 0x00ff00)
    | ((((color1 & 0xff0000)*10 + (color2 & 0xff0000)*5 + (color3 & 0xff0000))
    /16) & 0xff0000);
  }
  
  string result("#......");
  result[1] = hex((color & 0xf00000)/0x100000);
  result[2] = hex((color & 0x0f0000)/0x10000);
  result[3] = hex((color & 0x00f000)/0x1000);
  result[4] = hex((color & 0x000f00)/0x100);
  result[5] = hex((color & 0x0000f0)/0x10);
  result[6] = hex(color & 0x00000f);
  return result;
};

struct StopFeature
{
  string name;
  string color;
  double lat;
  double lon;
  
  StopFeature(string name_, string color_, double lat_, double lon_)
  : name(name_), color(color_), lat(lat_), lon(lon_) {}
};

struct PolySegmentFeature
{
  vector< pair< double, double > > lat_lon;
  string color;
  
  PolySegmentFeature(string color_) : color(color_) {}
};

typedef vector< PolySegmentFeature > RouteFeature;

struct Features
{
  vector< RouteFeature > routes;
  vector< StopFeature > stops;
};

Features extract_features(const OSMData& osm_data)
{
  Features features;
  map< string, uint32 > name_to_node;
  
  for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
      it != osm_data.relations.end(); ++it)
  {
    string color(it->second->tags["color"]);
    if (color == "")
      color = default_color(it->second->tags["ref"]);
    for (vector< pair< OSMElement*, string > >::const_iterator
        it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
    {
      Node* node(dynamic_cast< Node* >(it2->first));
      if (node)
      {
	if ((node->tags["highway"] == "bus_stop") ||
	    (node->tags["railway"] == "station"))
	{
	  pair< map< string, uint32 >::iterator, bool >
	      itp(name_to_node.insert
	          (make_pair(node->tags["name"], features.stops.size())));
	  if (itp.second)
	  {
	    features.stops.push_back(StopFeature
	      (node->tags["name"], color, node->lat, node->lon));
	  }
	  else
	  {
	    double avg_lat((node->lat + features.stops[itp.first->second].lat)/2);
	    double avg_lon((node->lon + features.stops[itp.first->second].lon)/2);
	    features.stops[itp.first->second].lat = avg_lat;
	    features.stops[itp.first->second].lon = avg_lon;
	  }
	}
      }
    }
  }
  
  for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
      it != osm_data.relations.end(); ++it)
  {
    Node* last_node(NULL);
    RouteFeature route_feature;
    string color(it->second->tags["color"]);
    if (color == "")
      color = default_color(it->second->tags["ref"]);
    for (vector< pair< OSMElement*, string > >::const_iterator
        it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
    {
      Way* way(dynamic_cast< Way* >(it2->first));
      if (way)
      {
	PolySegmentFeature* poly_segment(NULL);
	if (it2->second == "backward")
	{
	  if (way->nds.back() == last_node)
	    poly_segment = &route_feature.back();
	  else
	  {
	    route_feature.push_back(PolySegmentFeature(color));
	    poly_segment = &route_feature.back();
	    poly_segment->lat_lon.push_back
	        (make_pair(way->nds.back()->lat, way->nds.back()->lon));
	  }
	  vector< Node* >::const_reverse_iterator it3(way->nds.rbegin());
	  if (it3 != way->nds.rend())
	    ++it3;
	  for (; it3 != way->nds.rend(); ++it3)
	    poly_segment->lat_lon.push_back
                (make_pair((*it3)->lat, (*it3)->lon));
	  last_node = way->nds.front();
	}
	else
	{
	  if (way->nds.front() == last_node)
	    poly_segment = &route_feature.back();
	  else
	  {
	    route_feature.push_back(PolySegmentFeature(color));
	    poly_segment = &route_feature.back();
	    poly_segment->lat_lon.push_back
	        (make_pair(way->nds.front()->lat, way->nds.front()->lon));
	  }
	  vector< Node* >::const_iterator it3(way->nds.begin());
	  if (it3 != way->nds.end())
	    ++it3;
	  for (; it3 != way->nds.end(); ++it3)
	    poly_segment->lat_lon.push_back
                (make_pair((*it3)->lat, (*it3)->lon));
	  last_node = way->nds.back();
	}
      }
    }
    features.routes.push_back(route_feature);
  }
  
  return features;
}

namespace
{
  struct Bbox
  {
    double north, south, east, west;
    Bbox(double north_, double south_, double east_, double west_)
    : north(north_), south(south_), east(east_), west(west_) {}
  };
  
  Bbox calc_bbox(const OSMData& current_data)
  {
    double north(-90.0), south(90.0), east(-180.0), west(180.0);
    for (map< uint32, Node* >::const_iterator it(current_data.nodes.begin());
    it != current_data.nodes.end(); ++it)
    {
      if (it->second->lat < south)
	south = it->second->lat;
      if (it->second->lat > north)
	north = it->second->lat;
      if (it->second->lon < west)
	west = it->second->lon;
      if (it->second->lon > east)
	east = it->second->lon;
    }
    
    return Bbox(north, south, east, west);
  }
  
  double vpos(double lat, double north, double m_per_pixel)
  {
    return (north - lat)*(1000000.0/9.0)/m_per_pixel;
  }
  
  double hpos(double lat, double lon, double west, double m_per_pixel)
  {
    return (lon - west)*(1000000.0/9.0)/m_per_pixel*cos(lat/90.0*acos(0));
  }
}

void sketch_features
    (const OSMData& osm_data,
     const Features& features,
     double pivot_lon, double m_per_pixel)
{
  ::Bbox bbox(::calc_bbox(osm_data));
  
  //expand the bounding box to avoid elements scratching the frame
  bbox.north += 0.001*m_per_pixel;
  bbox.south -= 0.001*m_per_pixel;
  bbox.east += 0.002*m_per_pixel;
  bbox.west -= 0.002*m_per_pixel;
  
  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
  "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
  "     version=\"1.1\" baseProfile=\"full\"\n"
  "     width=\""<<hpos(bbox.south, bbox.east, bbox.west, m_per_pixel)<<"px\" "
  "height=\""<<vpos(bbox.south, bbox.north, m_per_pixel)<<"px\">\n"
  "\n";
  
  for (vector< StopFeature >::const_iterator it(features.stops.begin());
      it != features.stops.end(); ++it)
  {
    cout<<"<circle cx=\""
        <<hpos(it->lat, it->lon, bbox.west, m_per_pixel)
        <<"\" cy=\""
        <<vpos(it->lat, bbox.north, m_per_pixel)
        <<"\" r=\"6\" fill=\""<<it->color<<"\"/>\n";
	cout<<"<text x=\""<<hpos(it->lat, it->lon, bbox.west, m_per_pixel) + 6
	    <<"\" y=\""<<vpos(it->lat, bbox.north, m_per_pixel) - 6
	    <<"\" font-size=\"12px\">"<<it->name<<"</text>\n";
  }
  cout<<'\n';
  
  for (vector< RouteFeature >::const_iterator it(features.routes.begin());
      it != features.routes.end(); ++it)
  {
    for (vector< PolySegmentFeature >::const_iterator it2(it->begin());
        it2 != it->end(); ++it2)
    {
      cout<<"<polyline fill=\"none\" stroke=\""<<it2->color
          <<"\" stroke-width=\"3px\" points=\"";
      vector< pair< double, double > >::const_iterator it3(it2->lat_lon.begin());
      if (it3 != it2->lat_lon.end())
      {
	cout<<hpos(it3->first, it3->second, bbox.west, m_per_pixel)<<' '
	    <<vpos(it3->first, bbox.north, m_per_pixel);
	++it3;
      }
      for (; it3 != it2->lat_lon.end(); ++it3)
      {
	cout<<", "<<hpos(it3->first, it3->second, bbox.west, m_per_pixel)<<' '
	    <<vpos(it3->first, bbox.north, m_per_pixel);
      }
      cout<<"\"/>\n";
    }
    cout<<'\n';
  }
  
  cout<<"</svg>\n";
}

int main(int argc, char *argv[])
{
  // read the XML input
  const OSMData& current_data(read_osm());
  
  Features features(extract_features(current_data));
  
  sketch_features(current_data, features, middle_lon(current_data), 10.0);

  //sketch_unscaled_osm_data(current_data);
    
  //sketch_osm_data(current_data, middle_lon(current_data), 1.0);
  
  return 0;
}
