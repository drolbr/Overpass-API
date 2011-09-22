#include <algorithm>
#include <cmath>
#include <cstring>
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

struct NodeFeature
{
  NodeFeature(const double& lat_, const double& lon_, int shift_)
  : lat(lat_), lon(lon_), shift(shift_) {}
  
  double lat;
  double lon;
  int shift;
};

struct PolySegmentFeature
{
  vector< NodeFeature > lat_lon;
  string color;
  
  PolySegmentFeature(string color_) : color(color_) {}
};

typedef vector< PolySegmentFeature > RouteFeature;

struct Features
{
  vector< RouteFeature > routes;
  vector< StopFeature > stops;
};

struct Rough_Coord
{
  Rough_Coord(double lat_, double lon_, double deg_threshold)
  {
    lat = std::floor(lat_ / deg_threshold);
    lon = std::floor(lon_ / deg_threshold);
  }
  Rough_Coord(int lat_, int lon_) : lat(lat_), lon(lon_) {}
  
  int lat;
  int lon;
};

bool operator<(const Rough_Coord& a, const Rough_Coord& b)
{
  if (a.lat != b.lat)
    return (a.lat < b.lat);
  return (a.lon < b.lon);
}

double great_circle_dist(double lat1, double lon1, double lat2, double lon2)
{
  double scalar_prod =
      sin(lat1/90.0*acos(0))*sin(lat2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*sin(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*sin(lon2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*cos(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*cos(lon2/90.0*acos(0));
  if (scalar_prod > 1)
    scalar_prod = 1;
  return acos(scalar_prod)*(20*1000*1000/acos(0));
}

struct Placed_Node
{
  Placed_Node(double lat_, double lon_, uint weight_ = 1)
    : lat(lat_), lon(lon_), weight(weight_) {}
  
  double lat;
  double lon;
  uint weight;
};

bool operator<(const Placed_Node& a, const Placed_Node& b)
{
  if (a.lat != b.lat)
    return (a.lat < b.lat);
  return (a.lon < b.lon);
}

struct Placed_Segment
{
  Placed_Segment(const Placed_Node& from_, const Placed_Node& to_)
  : from(from_), to(to_) {}
  
  Placed_Node from;
  Placed_Node to;
};

bool operator<(const Placed_Segment& a, const Placed_Segment& b)
{
  if (a.from < b.from)
    return true;
  else if (b.from < a.from)
    return false;
  return (a.to < b.to);
}

Placed_Node find_nearest(const map< Rough_Coord, Placed_Node >& lattice_nodes,
			 const Node& node, double deg_threshold)
{
  Rough_Coord rough(node.lat, node.lon, deg_threshold);
  Placed_Node result(100.0, 0.0);
  double min_dist = 40000000;
  for (int lat = rough.lat-1; lat <= rough.lat+1; ++lat)
  {
    for (int lon = rough.lon-1; lon <= rough.lon+1; ++lon)
    {
      map< Rough_Coord, Placed_Node >::const_iterator it
          = lattice_nodes.find(Rough_Coord(lat, lon));
      if (it == lattice_nodes.end())
	continue;
      if (great_circle_dist(node.lat, node.lon, it->second.lat, it->second.lon) < min_dist)
      {
	min_dist = great_circle_dist(node.lat, node.lon, it->second.lat, it->second.lon);
	result = it->second;
      }
    }
  }
  if (min_dist < 40000000)
    return result;
  for (int lat = rough.lat-3; lat <= rough.lat+3; ++lat)
  {
    for (int lon = rough.lon-3; lon <= rough.lon+3; ++lon)
    {
      map< Rough_Coord, Placed_Node >::const_iterator it
          = lattice_nodes.find(Rough_Coord(lat, lon));
      if (it == lattice_nodes.end())
	continue;
      if (great_circle_dist(node.lat, node.lon, it->second.lat, it->second.lon) < min_dist)
      {
	min_dist = great_circle_dist(node.lat, node.lon, it->second.lat, it->second.lon);
	result = it->second;
      }
    }
  }
  return result;
}

Features extract_features(const OSMData& osm_data)
{
  const double THRESHOLD = 50.0;
  Features features;
  
//   map< string, uint32 > name_to_node;
//   
//   for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
//       it != osm_data.relations.end(); ++it)
//   {
//     string color(it->second->tags["color"]);
//     if (color == "")
//       color = default_color(it->second->tags["ref"]);
//     for (vector< pair< OSMElement*, string > >::const_iterator
//         it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
//     {
//       Node* node(dynamic_cast< Node* >(it2->first));
//       if (node)
//       {
// 	if ((node->tags["highway"] == "bus_stop") ||
// 	    (node->tags["railway"] == "station") ||
// 	    (node->tags["railway"] == "tram_stop"))
// 	{
// 	  pair< map< string, uint32 >::iterator, bool >
// 	      itp(name_to_node.insert
// 	          (make_pair(node->tags["name"], features.stops.size())));
// 	  if (itp.second)
// 	  {
// 	    features.stops.push_back(StopFeature
// 	      (node->tags["name"], color, node->lat, node->lon));
// 	  }
// 	  else
// 	  {
// 	    double avg_lat((node->lat + features.stops[itp.first->second].lat)/2);
// 	    double avg_lon((node->lon + features.stops[itp.first->second].lon)/2);
// 	    features.stops[itp.first->second].lat = avg_lat;
// 	    features.stops[itp.first->second].lon = avg_lon;
// 	  }
// 	}
//       }
//     }
//   }
// 
//   map< Way*, map< string, pair< int, bool > > > colors_per_way;
//   for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
//       it != osm_data.relations.end(); ++it)
//   {
//     RouteFeature route_feature;
//     string color(it->second->tags["color"]);
//     if (color == "")
//       color = default_color(it->second->tags["ref"]);
//     for (vector< pair< OSMElement*, string > >::const_iterator
//         it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
//     {
//       Way* way(dynamic_cast< Way* >(it2->first));
//       if (way)
//       {
// 	int pos = colors_per_way[way].size();
// 	if (colors_per_way[way].find(color) == colors_per_way[way].end())
// 	  colors_per_way[way].insert(make_pair(color, make_pair(pos, it2->second != "backward")));
//       }
//     }
//   }
//   
//   for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
//       it != osm_data.relations.end(); ++it)
//   {
//     Node* last_node(NULL);
//     RouteFeature route_feature;
//     string color(it->second->tags["color"]);
//     if (color == "")
//       color = default_color(it->second->tags["ref"]);
//     for (vector< pair< OSMElement*, string > >::const_iterator
//         it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
//     {
//       Way* way(dynamic_cast< Way* >(it2->first));
//       if (way)
//       {
// 	PolySegmentFeature* poly_segment(NULL);
// 	if (it2->second == "backward")
// 	{
// 	  if (way->nds.back() == last_node)
// 	    poly_segment = &route_feature.back();
// 	  else
// 	  {
// 	    route_feature.push_back(PolySegmentFeature(color));
// 	    poly_segment = &route_feature.back();
// 	    poly_segment->lat_lon.push_back
// 	        (NodeFeature(way->nds.back()->lat, way->nds.back()->lon,
// 			     (0/*colors_per_way[way][color].second ? -1 : 1*/)*
// 			     ((int)colors_per_way[way].size() - 1 - 2*colors_per_way[way][color].first)));
// 	  }
// 	  vector< Node* >::const_reverse_iterator it3(way->nds.rbegin());
// 	  if (it3 != way->nds.rend())
// 	    ++it3;
// 	  for (; it3 != way->nds.rend(); ++it3)
// 	    poly_segment->lat_lon.push_back
//                 (NodeFeature((*it3)->lat, (*it3)->lon,
// 			     (/*colors_per_way[way][color].second ?*/ -1 /*: 1*/)*
// 			     ((int)colors_per_way[way].size() - 1 - 2*colors_per_way[way][color].first)));
// 	  last_node = way->nds.front();
// 	}
// 	else
// 	{
// 	  if (way->nds.front() == last_node)
// 	    poly_segment = &route_feature.back();
// 	  else
// 	  {
// 	    route_feature.push_back(PolySegmentFeature(color));
// 	    poly_segment = &route_feature.back();
// 	    poly_segment->lat_lon.push_back
// 	        (NodeFeature(way->nds.front()->lat, way->nds.front()->lon,
//  			     (0/*colors_per_way[way][color].second ? 1 : -1*/)*
// 			     ((int)colors_per_way[way].size() - 1 - 2*colors_per_way[way][color].first)));
// 	  }
// 	  vector< Node* >::const_iterator it3(way->nds.begin());
// 	  if (it3 != way->nds.end())
// 	    ++it3;
// 	  for (; it3 != way->nds.end(); ++it3)
// 	    poly_segment->lat_lon.push_back
// 	    (NodeFeature((*it3)->lat, (*it3)->lon,
// /*			 (colors_per_way[way][color].second ? 1 : -1)**/
// 			 ((int)colors_per_way[way].size() - 1 - 2*colors_per_way[way][color].first)));
// 	  last_node = way->nds.back();
// 	}
//       }
//     }
//     features.routes.push_back(route_feature);
//   }

//   RouteFeature route_feature;
//   for (map< uint32, Way* >::const_iterator it(osm_data.ways.begin());
//       it != osm_data.ways.end(); ++it)
//   {
//     if (it->second->nds.size() < 2)
//       continue;
//     PolySegmentFeature poly_segment("#ff0000");    
//     for (vector< Node* >::const_iterator it2 = it->second->nds.begin();
//         it2 != it->second->nds.end(); ++it2)
//       poly_segment.lat_lon.push_back(NodeFeature((*it2)->lat, (*it2)->lon, 0));
//     route_feature.push_back(poly_segment);
//   }
//   features.routes.push_back(route_feature);

// ----------------------------------------------------------------------------

//   map< Node*, uint > node_count;
//   for (map< uint32, Way* >::const_iterator it(osm_data.ways.begin());
//       it != osm_data.ways.end(); ++it)
//   {
//     if (it->second->nds.size() < 2)
//       continue;
//     vector< Node* >::const_iterator it2 = it->second->nds.begin();
//     ++node_count[*it2];
//     vector< Node* >::const_iterator it2_end = it->second->nds.end();
//     --it2_end;
//     ++node_count[*it2_end];
//     for (++it2; it2 != it2_end; ++it2)
//       node_count[*it2] += 2;
//   }
// //   for (map< Node*, uint >::const_iterator it = node_count.begin(); it != node_count.end(); ++it)
// //   {
// //     if (it->second != 2)
// //       features.stops.push_back(StopFeature("", "#000000", it->first->lat, it->first->lon));
// //   }
// 
//   set< pair< double, double > > lat_lon;
//   for (map< Node*, uint >::const_iterator it = node_count.begin(); it != node_count.end(); ++it)
//   {
//     if (it->second != 2)
//       lat_lon.insert(make_pair(it->first->lat, it->first->lon));
//   }
//   while (true)
//   {
//     double min_dist = 40.0*1000*1000;
//     pair< double, double > min_a, min_b;
//     for (set< pair< double, double > >::const_iterator it1 = lat_lon.begin();
//         it1 != lat_lon.end(); ++it1)
//     {
//       set< pair< double, double > >::const_iterator it2 = it1;
//       ++it2;
//       for (; it2 != lat_lon.end(); ++it2)
//       {
// 	if (great_circle_dist(it1->first, it1->second, it2->first, it2->second) < min_dist)
// 	{
// 	  min_dist = great_circle_dist(it1->first, it1->second, it2->first, it2->second);
// 	  min_a = *it1;
// 	  min_b = *it2;
// 	}
//       }
//     }
//     cerr<<'.';
//     if (min_dist >= 50)
//       break;
//     lat_lon.erase(min_a);
//     lat_lon.erase(min_b);
//     lat_lon.insert(make_pair((min_a.first + min_b.first)/2.0, (min_a.second + min_b.second)/2.0));
//   }
//   cerr<<'\n';
//   for (set< pair< double, double > >::const_iterator it = lat_lon.begin();
//       it != lat_lon.end(); ++it)
//   {
//     features.stops.push_back(StopFeature("", "#000000", it->first, it->second));
//   }
// 
//   map< Node*, set< pair< double, double > >::const_iterator > lat_lon_per_node;
//   for (map< Node*, uint >::const_iterator it = node_count.begin(); it != node_count.end(); ++it)
//   {
//     if (it->second != 2)
//       lat_lon_per_node.insert(make_pair(it->first, lat_lon.end()));
//   }
//   for (map< Node*, set< pair< double, double > >::const_iterator >::iterator
//       it = lat_lon_per_node.begin(); it != lat_lon_per_node.end(); ++it)
//   {
//     double min_dist = 40.0*1000*1000;
//     for (set< pair< double, double > >::const_iterator it2 = lat_lon.begin();
//         it2 != lat_lon.end(); ++it2)
//     {
//       if (great_circle_dist(it->first->lat, it->first->lon, it2->first, it2->second) < min_dist)
//       {
// 	min_dist = great_circle_dist(it->first->lat, it->first->lon, it2->first, it2->second);
// 	it->second = it2;
//       }
//     }
//   }
//     
//   map< Way*, pair< pair< double, double >, pair< double, double > > > start_end;
//   for (map< uint32, Way* >::const_iterator it(osm_data.ways.begin());
//       it != osm_data.ways.end(); ++it)
//   {
//     if (it->second->nds.size() < 2)
//       continue;
//     vector< Node* >::const_iterator it2 = it->second->nds.begin();
//     if (lat_lon_per_node.find(*it2) != lat_lon_per_node.end())
//       start_end[it->second].first =
//           make_pair(lat_lon_per_node[*it2]->first, lat_lon_per_node[*it2]->second);
//     else
//       start_end[it->second].first = make_pair((*it2)->lat, (*it2)->lon);
//     vector< Node* >::const_iterator it2_end = it->second->nds.end();
//     --it2_end;
//     if (lat_lon_per_node.find(*it2_end) != lat_lon_per_node.end())
//       start_end[it->second].second =
//           make_pair(lat_lon_per_node[*it2_end]->first, lat_lon_per_node[*it2_end]->second);
//     else
//       start_end[it->second].second = make_pair((*it2_end)->lat, (*it2_end)->lon);
//     if (start_end[it->second].first == start_end[it->second].second)
//     {
//       bool trivial = true;
//       for (++it2; it2 != it2_end; ++it2)
// 	trivial &= (great_circle_dist((*it2)->lat, (*it2)->lon,
// 	    start_end[it->second].first.first, start_end[it->second].first.second) < 50.0);
//       if (trivial)
// 	start_end.erase(it->second);
//     }
//   }
//   
//   map< pair< pair< double, double >, pair< double, double > >, vector< Way* > > reverse_start_end;
//   for (map< Way*, pair< pair< double, double >, pair< double, double > > >::const_iterator
//       it = start_end.begin(); it != start_end.end(); ++it)
//     reverse_start_end[it->second].push_back(it->first);
//   
// /*  map< Way*, vector< pair< double, double > > > joined_ways;
//   for (map< pair< pair< double, double >, pair< double, double > >, vector< Way* > >::const_iterator
//       it = reverse_start_end.begin(); it != reverse_start_end.end(); ++it)
//   {
//     for (vector< Way* >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
//     {
//       ...
//     }
//   }*/
//   
//   RouteFeature route_feature;
//   for (map< uint32, Way* >::const_iterator it(osm_data.ways.begin());
//       it != osm_data.ways.end(); ++it)
//   {
//     if (start_end.find(it->second) == start_end.end())
//       continue;
//     PolySegmentFeature poly_segment("#ff0000");
//     vector< Node* >::const_iterator it2 = it->second->nds.begin();
//     poly_segment.lat_lon.push_back(
//         NodeFeature(start_end[it->second].first.first, start_end[it->second].first.second, 0));
//     vector< Node* >::const_iterator it2_end = it->second->nds.end();
//     --it2_end;
//     for (++it2; it2 != it2_end; ++it2)
//       poly_segment.lat_lon.push_back(NodeFeature((*it2)->lat, (*it2)->lon, 0));
//     poly_segment.lat_lon.push_back(
//         NodeFeature(start_end[it->second].second.first, start_end[it->second].second.second, 0));
//     route_feature.push_back(poly_segment);
//   }
//   features.routes.push_back(route_feature);

// ----------------------------------------------------------------------------

  const double deg_threshold = THRESHOLD/40000000*360.0;
  
  // Debug output
  for (map< uint32, Way* >::const_iterator it = osm_data.ways.begin();
      it != osm_data.ways.end(); ++it)
  {
    for (vector< Node* >::const_iterator it2 = it->second->nds.begin();
        it2 != it->second->nds.end(); ++it2)
      features.stops.push_back(StopFeature("", "#0000ff", (*it2)->lat, (*it2)->lon));
  }

  map< Rough_Coord, vector< Node* > > node_buckets;
//   for (map< uint32, Node* >::const_iterator it = osm_data.nodes.begin();
//       it != osm_data.nodes.end(); ++it)
//       node_buckets[Rough_Coord(it->second->lat, it->second->lon, deg_threshold)]
// 			       .push_back(it->second);
  for (map< uint32, Way* >::const_iterator it = osm_data.ways.begin();
      it != osm_data.ways.end(); ++it)
  {
    for (vector< Node* >::const_iterator it2 = it->second->nds.begin();
        it2 != it->second->nds.end(); ++it2)
      node_buckets[Rough_Coord((*it2)->lat, (*it2)->lon, deg_threshold)].push_back(*it2);
  }

  // Debug output
  for (map< Rough_Coord, vector< Node* > >::const_iterator it = node_buckets.begin();
      it != node_buckets.end(); ++it)
    features.stops.push_back(StopFeature
        ("", "#00ff00", it->first.lat * deg_threshold + deg_threshold/2.0,
	 it->first.lon * deg_threshold + deg_threshold/2.0));

  map< Rough_Coord, Placed_Node > lattice_nodes;
  for (map< Rough_Coord, vector< Node* > >::const_iterator it = node_buckets.begin();
      it != node_buckets.end(); ++it)
  {
    double lat_sum = 0.0;
    double lon_sum = 0.0;
    unsigned int count = 0;
    for (int lat = it->first.lat-1; lat <= it->first.lat+1; ++lat)
    {
      for (int lon = it->first.lon-1; lon <= it->first.lon+1; ++lon)
      {
	map< Rough_Coord, vector< Node* > >::const_iterator
	    it2 = node_buckets.find(Rough_Coord(lat, lon));
	if (it2 == node_buckets.end())
	  continue;
	for (vector< Node* >::const_iterator it3 = it2->second.begin();
	    it3 != it2->second.end(); ++it3)
	{
	  if ((*it3)->lat < it->first.lat * deg_threshold - deg_threshold/2.0)
	    continue;
	  if ((*it3)->lat > it->first.lat * deg_threshold + deg_threshold*3.0/2.0)
	    continue;
	  if ((*it3)->lon < it->first.lon * deg_threshold - deg_threshold/2.0)
	    continue;
	  if ((*it3)->lon > it->first.lon * deg_threshold + deg_threshold*3.0/2.0)
	    continue;
	  lat_sum += (*it3)->lat;
	  lon_sum += (*it3)->lon;
	  ++count;
	}
      }
    }
    if (count == 0)
      continue;
/*    if (lat_sum / count < it->first.lat * deg_threshold)
      continue;
    if (lat_sum / count > it->first.lat * deg_threshold + deg_threshold)
      continue;
    if (lon_sum / count < it->first.lon * deg_threshold)
      continue;
    if (lon_sum / count > it->first.lon * deg_threshold + deg_threshold)
      continue;*/
    lattice_nodes.insert(make_pair
        (it->first, Placed_Node(lat_sum / count, lon_sum / count, count)));
  }
  
  // Debug output
  for (map< Rough_Coord, Placed_Node >::const_iterator it = lattice_nodes.begin();
      it != lattice_nodes.end(); ++it)
    features.stops.push_back(StopFeature("", "#000000", it->second.lat, it->second.lon));

  map< Placed_Segment, set< Way* > > segments;
  for (map< uint32, Way* >::const_iterator it(osm_data.ways.begin());
      it != osm_data.ways.end(); ++it)
  {
    if (it->second->nds.size() < 2)
      continue;
    vector< Node* >::const_iterator it2 = it->second->nds.begin();
    vector< Node* >::const_iterator last_it2 = it2;
    for (++it2; it2 != it->second->nds.end(); ++it2)
    {
      segments[Placed_Segment
          (find_nearest(lattice_nodes, **it2, deg_threshold),
	   find_nearest(lattice_nodes, **last_it2, deg_threshold))].insert(it->second);
      ++last_it2;
    }
  }

  // Debug output
  RouteFeature route_feature;
  for (map< Placed_Segment, set< Way* > >::const_iterator it = segments.begin();
      it !=  segments.end(); ++it)
  {
    PolySegmentFeature poly_segment("#ff0000");
    poly_segment.lat_lon.push_back(
        NodeFeature(it->first.from.lat, it->first.from.lon, 0));
    poly_segment.lat_lon.push_back(
        NodeFeature(it->first.to.lat, it->first.to.lon, 0));
    route_feature.push_back(poly_segment);
  }
  features.routes.push_back(route_feature);
  
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
  
  // Transforms (lat,lon) coordinates into coordinates on the map
  class Coord_Transform
  {
    public:
      Coord_Transform
          (double north, double south, double west, double east,
	   double m_per_pixel, double pivot_lon)
	  : north_(north), south_(south), west_(west), east_(east),
	    m_per_pixel_(m_per_pixel), pivot_lon_(pivot_lon)
      {
	pivot_hpos_ =
	    (pivot_lon - west)*(1000000.0/9.0)/m_per_pixel
	    *cos((south + north)*(1/2.0/90.0*acos(0)));
      }
	    
      double vpos(double lat)
      {
	return (north_ - lat)*(1000000.0/9.0)/m_per_pixel_;
      }
      
      double hpos(double lat, double lon)
      {
	return (lon - pivot_lon_)*(1000000.0/9.0)/m_per_pixel_
	*cos(lat*(1/90.0*acos(0))) + pivot_hpos_;
      }
      
    private:
      double north_, south_, west_, east_;
      double m_per_pixel_, pivot_lon_, pivot_hpos_;
  };
}

struct Vec_2_Dim
{
  Vec_2_Dim(double x_, double y_) : x(x_), y(y_) {}
  
  double x;
  double y;
  
  double length() const
  {
    return sqrt(x*x + y*y);
  }
};

Vec_2_Dim operator*(double scal, const Vec_2_Dim& v)
{
  return Vec_2_Dim(scal*v.x, scal*v.y);
}

Vec_2_Dim operator+(const Vec_2_Dim& v, const Vec_2_Dim& w)
{
  return Vec_2_Dim(v.x + w.x, v.y + w.y);
}

Vec_2_Dim turn_left(const Vec_2_Dim& v)
{
  return Vec_2_Dim(v.y, -v.x);
}

string shifted(double before_x, double before_y, double x, double y,
	       double after_x, double after_y, int shift)
{
  const double LINE_DIST = 2.0;
  
  bool invalid = false;
  Vec_2_Dim in_vec(x - before_x, y - before_y);
  in_vec = (1.0/in_vec.length())*in_vec;
  invalid |= (in_vec.length() < 0.1);
  Vec_2_Dim left_vec(turn_left(in_vec));
  left_vec = (1.0/left_vec.length())*left_vec;
  Vec_2_Dim out_vec(after_x - x, after_y - y);
  invalid |= (out_vec.length() < 0.1);
  out_vec = (1.0/out_vec.length())*out_vec;
  Vec_2_Dim draw_vec(turn_left(in_vec + out_vec));
  invalid |= (draw_vec.length() < 0.1);
  if (invalid)
  {
    ostringstream out;
    out<<x<<' '<<y;
    return out.str();
  }
  if (in_vec.x*out_vec.x + in_vec.y*out_vec.y < 0.0)
  {
    if ((shift <= 0 && (left_vec.x*out_vec.x + left_vec.y*out_vec.y > 0.0)) ||
        (shift >= 0 && (left_vec.x*out_vec.x + left_vec.y*out_vec.y < 0.0)))
    {
      Vec_2_Dim left_out_vec(turn_left(out_vec));
      left_out_vec = (1.0/left_out_vec.length())*left_out_vec;
      draw_vec = (1.0/draw_vec.length())*draw_vec;
    
      ostringstream out;
      out<<x + LINE_DIST*left_vec.x*shift<<' '<<y + LINE_DIST*left_vec.y*shift<<", ";
      out<<x + LINE_DIST*draw_vec.x*shift<<' '<<y + LINE_DIST*draw_vec.y*shift<<", ";
      out<<x + LINE_DIST*left_out_vec.x*shift<<' '<<y + LINE_DIST*left_out_vec.y*shift;
      //out<<x<<' '<<y;
      return out.str();
    }
  }
  draw_vec = (LINE_DIST/(draw_vec.x*left_vec.x + draw_vec.y*left_vec.y))*draw_vec;
  
  ostringstream out;
  //out<<x<<' '<<y<<", ";
  //out<<x + 10.0*draw_vec.x<<' '<<y + 10.0*draw_vec.y<<", ";
  //out<<x + 10.0*draw_vec.x*shift<<' '<<y + 10.0*draw_vec.y*shift<<", ";
  out<<x + draw_vec.x*shift<<' '<<y + draw_vec.y*shift;//<<", ";
  //out<<x<<' '<<y;
  return out.str();
}

void sketch_features
    (const OSMData& osm_data,
     const Features& features,
     double pivot_lon, double m_per_pixel, double stop_font_size)
{
  ::Bbox bbox(::calc_bbox(osm_data));
  
  //expand the bounding box to avoid elements scratching the frame
  bbox.north += 0.001*m_per_pixel;
  bbox.south -= 0.001*m_per_pixel;
  bbox.east += 0.003*m_per_pixel;
  bbox.west -= 0.003*m_per_pixel;
  ::Coord_Transform coord_transform
      (bbox.north, bbox.south, bbox.west, bbox.east,
       m_per_pixel, pivot_lon);
  
  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
  "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
  "     version=\"1.1\" baseProfile=\"full\"\n"
  "     width=\""
      <<coord_transform.hpos((bbox.south + bbox.north)/2.0, bbox.east)<<"px\" "
  "height=\""<<coord_transform.vpos(bbox.south)<<"px\">\n"
  "\n";
  
  for (vector< StopFeature >::const_iterator it(features.stops.begin());
      it != features.stops.end(); ++it)
  {
    cout<<"<circle cx=\""
        <<coord_transform.hpos(it->lat, it->lon)
        <<"\" cy=\""
	<<coord_transform.vpos(it->lat)
        <<"\" r=\"6\" fill=\""<<it->color<<"\"/>\n";
	cout<<"<text x=\""
	    <<coord_transform.hpos(it->lat, it->lon) + 6
	    <<"\" y=\""<<coord_transform.vpos(it->lat) - 6
	    <<"\" font-size=\""<<stop_font_size<<"px\">"<<it->name<<"</text>\n";
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
      vector< double > x_s;
      vector< double > y_s;
      vector< int > shifts;
      for (vector< NodeFeature >::const_iterator it3(it2->lat_lon.begin());
          it3 != it2->lat_lon.end(); ++it3)
      {
	x_s.push_back(coord_transform.hpos(it3->lat, it3->lon));
	y_s.push_back(coord_transform.vpos(it3->lat));
	shifts.push_back(it3->shift);
      }
      
      if (x_s.size() >= 2)
      {
	unsigned int line_size = x_s.size();
	cout<<x_s[0]<<' '<<y_s[0];	
	for (unsigned int i = 1; i < line_size; ++i)
	  cout<<", "<<shifted(x_s[i-1], y_s[i-1], x_s[i], y_s[i], x_s[i+1], y_s[i+1], shifts[i]);	
	cout<<", "<<x_s[line_size-1]<<' '<<y_s[line_size-1];	
      }
      cout<<"\"/>\n";
    }
    cout<<'\n';
  }
  
  cout<<"</svg>\n";
}

int main(int argc, char *argv[])
{
  int argi(1);
  double pivot_lon(-200.0), stop_font_size(12.0), scale(10.0);
  while (argi < argc)
  {
    if (!strncmp("--pivot-lon=", argv[argi], 12))
      pivot_lon = atof(((string)(argv[argi])).substr(12).c_str());
    else if (!strncmp("--stop-font-size=", argv[argi], 17))
      stop_font_size = atof(((string)(argv[argi])).substr(17).c_str());
    else if (!strncmp("--scale=", argv[argi], 8))
      scale = atof(((string)(argv[argi])).substr(8).c_str());
    ++argi;
  }
  
  // read the XML input
  const OSMData& current_data(read_osm());
  
  Features features(extract_features(current_data));
  
  // choose pivot_lon automatically
  if (pivot_lon == -200.0)
    pivot_lon = middle_lon(current_data);
  
  sketch_features(current_data, features, pivot_lon, scale,
		  stop_font_size);

  //sketch_unscaled_osm_data(current_data);
    
  //sketch_osm_data(current_data, middle_lon(current_data), 1.0);
  
  return 0;
}
