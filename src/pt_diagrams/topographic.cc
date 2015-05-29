#include "read_input.h"
#include "test_output.h"
#include "../expat/expat_justparse_interface.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

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
  NodeFeature(const double& lat_, const double& lon_)
  : lat(lat_), lon(lon_) {}
  
  double lat;
  double lon;
};

bool operator<(const NodeFeature& a, const NodeFeature& b)
{
  if (a.lat != b.lat)
    return (a.lat < b.lat);
  return (a.lon < b.lon);
}

bool operator==(const NodeFeature& a, const NodeFeature& b)
{
  return (a.lat == b.lat && a.lon == b.lon);
}

struct PolySegmentFeature
{
  vector< NodeFeature > lat_lon;
  int shift;
  
  PolySegmentFeature(int shift_) : shift(shift_) {}
};

struct RouteFeature
{
  vector< PolySegmentFeature > polysegs;
  string color;
};

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

/*double great_circle_dist(double lat1, double lon1, double lat2, double lon2)
{
  double scalar_prod =
      sin(lat1/90.0*acos(0))*sin(lat2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*sin(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*sin(lon2/90.0*acos(0)) +
      cos(lat1/90.0*acos(0))*cos(lon1/90.0*acos(0))*cos(lat2/90.0*acos(0))*cos(lon2/90.0*acos(0));
  if (scalar_prod > 1)
    scalar_prod = 1;
  return acos(scalar_prod)*(20*1000*1000/acos(0));
}*/

double flat_dist(double lat1, double lon1, double lat2, double lon2)
{
  return sqrt((lat1 - lat2)*(lat1 - lat2) + (lon1 - lon2)*(lon1 - lon2));
}

double line_dist(double a_lat, double a_lon, double b_lat, double b_lon,
		 double pt_lat, double pt_lon)
{
  double rel_pos = ((pt_lat - a_lat)*(b_lat - a_lat) + (pt_lon - a_lon)*(b_lon - a_lon))/
    ((b_lat - a_lat)*(b_lat - a_lat) + (b_lon - a_lon)*(b_lon - a_lon));
  if (rel_pos < 0)
    return sqrt((pt_lat - a_lat)*(pt_lat - a_lat) + (pt_lon - a_lon)*(pt_lon - a_lon));
  else if (rel_pos > 1.0)
    return sqrt((pt_lat - b_lat)*(pt_lat - b_lat) + (pt_lon - b_lon)*(pt_lon - b_lon));
  else
    return abs(((pt_lat - a_lat)*(a_lon - b_lon) + (pt_lon - a_lon)*(b_lat - a_lat))/
        sqrt((b_lat - a_lat)*(b_lat - a_lat) + (b_lon - a_lon)*(b_lon - a_lon)));
}

double projection_pos(double a_lat, double a_lon, double b_lat, double b_lon,
		      double pt_lat, double pt_lon)
{
  return ((pt_lat - a_lat)*(b_lat - a_lat) + (pt_lon - a_lon)*(b_lon - a_lon))/
      ((b_lat - a_lat)*(b_lat - a_lat) + (b_lon - a_lon)*(b_lon - a_lon));
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

bool operator==(const Placed_Node& a, const Placed_Node& b)
{
  return (a.lat == b.lat && a.lon == b.lon);
}

struct Placed_Segment
{
  Placed_Segment(const Placed_Node& from_, const Placed_Node& to_)
  : from(from_), to(to_)
  {
    if (to_ < from_)
    {
      from = to_;
      to = from_;
    }
  }
  
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

struct Placed_Way
{
  vector< Placed_Node > nds;
};

bool operator<(const Placed_Way& a, const Placed_Way& b)
{
  return (a.nds < b.nds);
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
      if (flat_dist(node.lat, node.lon, it->second.lat, it->second.lon) < min_dist)
      {
	min_dist = flat_dist(node.lat, node.lon, it->second.lat, it->second.lon);
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
      if (flat_dist(node.lat, node.lon, it->second.lat, it->second.lon) < min_dist)
      {
	min_dist = flat_dist(node.lat, node.lon, it->second.lat, it->second.lon);
	result = it->second;
      }
    }
  }
  return result;
}

double transform_lon(const double& lat, const double& lon, const double& pivot_lon)
{
  return (lon - pivot_lon)*cos(lat/90.0*acos(0));
}

pair< double, double > center_of_polygon(Way& way)
{
  if (way.nds.empty())
    return make_pair(0.0, 0.0);
  vector< Node* >::const_iterator it = way.nds.begin();
  double max_lat = (*it)->lat;
  double min_lat = (*it)->lat;
  double max_lon = (*it)->lon;
  double min_lon = (*it)->lon;
  for (++it; it != way.nds.end(); ++it)
  {
    if ((*it)->lat > max_lat)
      max_lat = (*it)->lat;
    if ((*it)->lat < min_lat)
      min_lat = (*it)->lat;
    if ((*it)->lon > max_lon)
      max_lon = (*it)->lon;
    if ((*it)->lon < min_lon)
      min_lon = (*it)->lon;
  }
  return make_pair((min_lat + max_lat)/2.0, (min_lon + max_lon)/2.0);
}

struct Way_Position
{
  Way_Position(Way* way_, int segment_idx_, double pos_,
	       const string& color_, const string& name_)
      : way(way_), segment_idx(segment_idx_), pos(pos_), color(color_), name(name_) {}
  
  Way* way;
  int segment_idx;
  double pos;
  string color, name;
};

bool operator<(const Way_Position& a, const Way_Position& b)
{
  if (a.way != b.way)
    return (a.way < b.way);
  if (a.segment_idx != b.segment_idx)
    return (a.segment_idx < b.segment_idx);
  return (a.pos < b.pos);
}

Way_Position project_to_route(const Relation& relation, const StopFeature& stop)
{
  double min_dist = 40000000;
  Way_Position result(0, 0, 0.0, "", "");
  for (vector< pair< OSMElement*, string > >::const_iterator
      it = relation.members.begin(); it != relation.members.end(); ++it)
  {
    Way* way = dynamic_cast< Way* >(it->first);
    if (way == 0 || it->second == "platform" || way->nds.size() < 2)
      continue;
    for (vector< Node* >::size_type i = 1; i < way->nds.size(); ++i)
    {
      double line_dist_ = line_dist(way->nds[i-1]->lat, way->nds[i-1]->lon,
				    way->nds[i]->lat, way->nds[i]->lon, stop.lat, stop.lon);
      if (line_dist_ < min_dist)
      {
	min_dist = line_dist_;
	result.way = way;
	result.segment_idx = i-1;
	result.pos = projection_pos(way->nds[i-1]->lat, way->nds[i-1]->lon,
				    way->nds[i]->lat, way->nds[i]->lon, stop.lat, stop.lon);
	if (result.pos < 0.0)
	  result.pos = 0.0;
	else if (result.pos > 1.0)
	  result.pos = 1.0;
      }
    }
  }
  return result;
}

Features extract_features(OSMData& osm_data, double pivot_lon, double threshold)
{
  Features features;

  // Convert lons into relative lons w.r.t. to their real distance to the pivot_lon
  for (map< uint32, Node* >::iterator it = osm_data.nodes.begin();
      it != osm_data.nodes.end(); ++it)
    it->second->lon = transform_lon(it->second->lat, it->second->lon, pivot_lon);

  // Detect stops and attach them to ways.
  map< Way*, vector< Way_Position > > stops_per_way;
  for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
      it != osm_data.relations.end(); ++it)
  {
    string color(it->second->tags["color"]);
    if (color == "")
      color = default_color(it->second->tags["ref"]);
    for (vector< pair< OSMElement*, string > >::const_iterator
        it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
    {
      StopFeature stop_feature("", "", 100.0, 0.0);
      Node* node(dynamic_cast< Node* >(it2->first));
      if (node)
	stop_feature = StopFeature(node->tags["name"], color, node->lat, node->lon);
      Way* way(dynamic_cast< Way* >(it2->first));
      if (way != 0 && it2->second == "platform")
      {
	pair< double, double > position = center_of_polygon(*way);
	stop_feature = StopFeature(way->tags["name"], color,
				   position.first, position.second);
      }
      if (stop_feature.lat == 100.0)
	continue;
      Way_Position way_pos = project_to_route(*it->second, stop_feature);
      if (way_pos.way == 0)
	continue;
      way_pos.color = stop_feature.color;
      way_pos.name = stop_feature.name;
      stops_per_way[way_pos.way].push_back(way_pos);
      
/*      // Debug code
      stop_feature.lat = (1.0 - way_pos.pos) * way_pos.way->nds[way_pos.segment_idx]->lat
          + way_pos.pos * way_pos.way->nds[way_pos.segment_idx+1]->lat;
      stop_feature.lon = (1.0 - way_pos.pos) * way_pos.way->nds[way_pos.segment_idx]->lon
          + way_pos.pos * way_pos.way->nds[way_pos.segment_idx+1]->lon;
      features.stops.push_back(stop_feature);*/
    }
  }
  for (map< Way*, vector< Way_Position > >::iterator it = stops_per_way.begin();
      it != stops_per_way.end(); ++it)
    sort(it->second.begin(), it->second.end());
    
  // --------------------------------------------------------------------------

  // Coarse nodes

  const double deg_threshold = threshold/40000000*360.0;
  
  // Debug output
//   for (map< uint32, Way* >::const_iterator it = osm_data.ways.begin();
//       it != osm_data.ways.end(); ++it)
//   {
//     for (vector< Node* >::const_iterator it2 = it->second->nds.begin();
//         it2 != it->second->nds.end(); ++it2)
//       features.stops.push_back(StopFeature("", "#0000ff", (*it2)->lat, (*it2)->lon));
//   }

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
//   for (map< Rough_Coord, vector< Node* > >::const_iterator it = node_buckets.begin();
//       it != node_buckets.end(); ++it)
//     features.stops.push_back(StopFeature
//         ("", "#007777", it->first.lat * deg_threshold + deg_threshold/2.0,
// 	 it->first.lon * deg_threshold + deg_threshold/2.0));

  map< Rough_Coord, vector< Placed_Node > > intermediate_lattice_nodes;
  for (map< Rough_Coord, vector< Node* > >::const_iterator it = node_buckets.begin();
      it != node_buckets.end(); ++it)
  {
    double lat_sum = 0.0;
    double lon_sum = 0.0;
    unsigned int count = 0;
    for (int lat = it->first.lat; lat <= it->first.lat+1; ++lat)
    {
      for (int lon = it->first.lon; lon <= it->first.lon+1; ++lon)
      {
	map< Rough_Coord, vector< Node* > >::const_iterator
	    it2 = node_buckets.find(Rough_Coord(lat, lon));
	if (it2 == node_buckets.end())
	  continue;
	for (vector< Node* >::const_iterator it3 = it2->second.begin();
	    it3 != it2->second.end(); ++it3)
	{
	  lat_sum += (*it3)->lat;
	  lon_sum += (*it3)->lon;
	  ++count;
	}
      }
    }
    intermediate_lattice_nodes[Rough_Coord(lat_sum / count, lon_sum / count, deg_threshold)]
        .push_back(Placed_Node(lat_sum / count, lon_sum / count, count));
  }
  
  // Debug output
//   for (map< Rough_Coord, vector< Placed_Node > >::const_iterator
//       it = intermediate_lattice_nodes.begin();
//       it != intermediate_lattice_nodes.end(); ++it)
//   {
//     for (vector< Placed_Node >::const_iterator it2 = it->second.begin();
//         it2 != it->second.end(); ++it2)
//       features.stops.push_back(StopFeature("", "#00ff00", it2->lat, it2->lon));
//   }

  map< Rough_Coord, Placed_Node > boxed_lattice_nodes;
  for (map< Rough_Coord, vector< Placed_Node > >::const_iterator
      it = intermediate_lattice_nodes.begin();
      it != intermediate_lattice_nodes.end(); ++it)
  {
    double lat_sum = 0.0;
    double lon_sum = 0.0;
    unsigned int count = 0;
    for (vector< Placed_Node >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
    {
      lat_sum += it2->lat * it2->weight;
      lon_sum += it2->lon * it2->weight;
      count += it2->weight;
    }
    boxed_lattice_nodes.insert(make_pair
        (it->first, Placed_Node(lat_sum / count, lon_sum / count, count)));
  }

  map< Rough_Coord, Placed_Node > lattice_nodes;
  for (map< Rough_Coord, Placed_Node >::const_iterator
      it = boxed_lattice_nodes.begin(); it != boxed_lattice_nodes.end(); ++it)
  {
    map< Rough_Coord, Placed_Node >::const_iterator
        it_left = boxed_lattice_nodes.find
	    (Rough_Coord(it->first.lat, it->first.lon - 1));
    if (it_left != boxed_lattice_nodes.end() &&
        flat_dist(it->second.lat, it->second.lon,
		it_left->second.lat, it_left->second.lon) < 0.5*deg_threshold)
      continue;

    map< Rough_Coord, Placed_Node >::const_iterator
        it_up = boxed_lattice_nodes.find
	    (Rough_Coord(it->first.lat - 1, it->first.lon));
    if (it_up != boxed_lattice_nodes.end() &&
        flat_dist(it->second.lat, it->second.lon,
		  it_up->second.lat, it_up->second.lon) < 0.5*deg_threshold)
      continue;

    map< Rough_Coord, Placed_Node >::const_iterator
        it_left_up = boxed_lattice_nodes.find
	    (Rough_Coord(it->first.lat - 1, it->first.lon - 1));
    if (it_left_up != boxed_lattice_nodes.end() &&
        flat_dist(it->second.lat, it->second.lon,
		  it_left_up->second.lat, it_left_up->second.lon) < 0.5*deg_threshold)
      continue;

    double lat_sum = it->second.lat * it->second.weight;
    double lon_sum = it->second.lon * it->second.weight;
    unsigned int count = it->second.weight;
    
    map< Rough_Coord, Placed_Node >::const_iterator
        it_right = boxed_lattice_nodes.find
	    (Rough_Coord(it->first.lat, it->first.lon + 1));
    if (it_right != boxed_lattice_nodes.end() &&
        flat_dist(it->second.lat, it->second.lon,
		  it_right->second.lat, it_right->second.lon) < 0.5*deg_threshold)
    {
      lat_sum = it_right->second.lat * it_right->second.weight;
      lon_sum = it_right->second.lon * it_right->second.weight;
      count = it_right->second.weight;
    }
        
    map< Rough_Coord, Placed_Node >::const_iterator
        it_down = boxed_lattice_nodes.find
	    (Rough_Coord(it->first.lat, it->first.lon + 1));
    if (it_down != boxed_lattice_nodes.end() &&
        flat_dist(it->second.lat, it->second.lon,
		  it_down->second.lat, it_down->second.lon) < 0.5*deg_threshold)
    {
      lat_sum = it_down->second.lat * it_down->second.weight;
      lon_sum = it_down->second.lon * it_down->second.weight;
      count = it_down->second.weight;
    }
        
    map< Rough_Coord, Placed_Node >::const_iterator
        it_right_down = boxed_lattice_nodes.find
	    (Rough_Coord(it->first.lat, it->first.lon + 1));
    if (it_right_down != boxed_lattice_nodes.end() &&
        flat_dist(it->second.lat, it->second.lon,
		  it_right_down->second.lat, it_right_down->second.lon) < 0.5*deg_threshold)
    {
      lat_sum = it_right_down->second.lat * it_right_down->second.weight;
      lon_sum = it_right_down->second.lon * it_right_down->second.weight;
      count = it_right_down->second.weight;
    }
        
    lattice_nodes.insert(make_pair
        (it->first, Placed_Node(lat_sum / count, lon_sum / count, count)));
  }
  
  // Debug output
  /*for (map< Rough_Coord, Placed_Node >::const_iterator it = lattice_nodes.begin();
      it != lattice_nodes.end(); ++it)
  {
    ostringstream buf;
    buf<<it->second.lat<<' '<<it->second.lon;
    features.stops.push_back(StopFeature(buf.str(), "#777700", it->second.lat, it->second.lon));
  }*/
  
  map< Placed_Segment, set< Way* > > segments;
  map< Placed_Segment, vector< Way_Position > > stops_per_segment;
  for (map< uint32, Way* >::const_iterator it(osm_data.ways.begin());
      it != osm_data.ways.end(); ++it)
  {
/*    for (vector< Way_Position >::const_iterator stop_it = stops_per_way[it->second].begin();
        stop_it != stops_per_way[it->second].end(); ++stop_it)
      cerr<<stop_it->name<<' '<<stop_it->way<<' '<<stop_it->segment_idx<<' '<<stop_it->pos<<'\n';*/
    
    if (it->second->nds.size() < 2)
      continue;
    vector< Node* >::const_iterator it2 = it->second->nds.begin();
    vector< Node* >::const_iterator last_it2 = it2;
    vector< Node* >::size_type count = 0;
    vector< Way_Position >::const_iterator stop_it = stops_per_way[it->second].begin();
    Placed_Segment last_segment(Placed_Node(0.0, 0.0), Placed_Node(0.0, 0.0));
    bool last_segment_is_forward = false;
    //cerr<<'='<<' '<<(*it2)->lat<<' '<<(*it2)->lon<<'\n';
    for (++it2; it2 != it->second->nds.end(); ++it2)
    {
      //cerr<<count<<' '<<(*it2)->lat<<' '<<(*it2)->lon<<'\n';
      
      map< double, Placed_Node > nodes_to_pass;
      nodes_to_pass.insert(make_pair(0.0, find_nearest(lattice_nodes, **last_it2, deg_threshold)));
      nodes_to_pass.insert(make_pair(1.0, find_nearest(lattice_nodes, **it2, deg_threshold)));
      if (nodes_to_pass.find(0.0)->second == nodes_to_pass.find(1.0)->second)
      {
	++count;
	++last_it2;
	continue;
      }
      double min_lat = (*it2)->lat;
      if ((*last_it2)->lat < min_lat)
	min_lat = (*last_it2)->lat;
      double min_lon = (*it2)->lon;
      if ((*last_it2)->lon < min_lon)
	min_lon = (*last_it2)->lon;
      double max_lat = (*it2)->lat;
      if ((*last_it2)->lat > max_lat)
	max_lat = (*last_it2)->lat;
      double max_lon = (*it2)->lon;
      if ((*last_it2)->lon > max_lon)
	max_lon = (*last_it2)->lon;
      map< Rough_Coord, Placed_Node >::const_iterator it_lattice
          = lattice_nodes.lower_bound
	      (Rough_Coord(min_lat - deg_threshold, min_lon - deg_threshold, deg_threshold));
      map< Rough_Coord, Placed_Node >::const_iterator it_lattice_end
          = lattice_nodes.upper_bound
	      (Rough_Coord(max_lat + deg_threshold, max_lon + deg_threshold, deg_threshold));
      for (; it_lattice != it_lattice_end; ++it_lattice)
      {
/*	cout<<(*it2)->lat<<' '<<(*it2)->lon<<' '
	    <<(*last_it2)->lat<<' '<<(*last_it2)->lon<<' '
	    <<it_lattice->second.lat<<' '<<it_lattice->second.lon<<' '
	    <<line_dist((*it2)->lat, (*it2)->lon, (*last_it2)->lat, (*last_it2)->lon,
			it_lattice->second.lat, it_lattice->second.lon)<<' '
	    <<projection_pos
	      ((*it2)->lat, (*it2)->lon, (*last_it2)->lat, (*last_it2)->lon,
	       it_lattice->second.lat, it_lattice->second.lon)<<'\n';*/
	if (line_dist((*it2)->lat, (*it2)->lon, (*last_it2)->lat, (*last_it2)->lon,
	    it_lattice->second.lat, it_lattice->second.lon) < deg_threshold/2.0)
	  nodes_to_pass.insert(make_pair(projection_pos
	      ((*it2)->lat, (*it2)->lon, (*last_it2)->lat, (*last_it2)->lon,
	       it_lattice->second.lat, it_lattice->second.lon), it_lattice->second));
      }
      
      // Set the iterators such that only the shortest interval from the starting point
      // to the destination point is included
      map< double, Placed_Node >::const_iterator it3 = nodes_to_pass.find(0.0);
      map< double, Placed_Node >::const_iterator it3_end = nodes_to_pass.find(1.0);
      map< double, Placed_Node >::const_iterator it3_seek = it3;
      while (it3_seek != it3_end)
      {
	if (it3_seek->second == it3->second)
	  it3 = it3_seek;
	++it3_seek;
      }
      it3_seek = it3;
      while (!(it3_seek->second == it3_end->second))
	++it3_seek;
      it3_end = it3_seek;
      ++it3_end;
      
      if (it3 != it3_end)
      {
        map< double, Placed_Node >::const_iterator last_it3 = it3;
	//cerr<<it3->first<<'\n';
        for (++it3; it3 != it3_end; ++it3)
        {
	  //cerr<<it3->first<<'\n';
	  last_segment = Placed_Segment(it3->second, last_it3->second);
	  last_segment_is_forward = (last_segment.from == last_it3->second);
	  for (; stop_it != stops_per_way[it->second].end() && stop_it->segment_idx < count;
	      ++stop_it)
	  {
	    stops_per_segment[last_segment].push_back(
	        Way_Position(0, 0, last_segment_is_forward ? 0.0 : 1.0,
			     stop_it->color, stop_it->name));
	    //cerr<<"A "<<last_segment.from.lon<<' '<<last_segment.to.lon<<'\n';
	  }
	  for (; stop_it != stops_per_way[it->second].end() && stop_it->segment_idx == count
	      && stop_it->pos < last_it3->first; ++stop_it)
	  {
	    stops_per_segment[last_segment].push_back(
	        Way_Position(0, 0, last_segment_is_forward ? 0.0 : 1.0,
			     stop_it->color, stop_it->name));
	    //cerr<<"D "<<last_segment.from.lon<<' '<<last_segment.to.lon<<'\n';
	  }
	  for (; stop_it != stops_per_way[it->second].end() && stop_it->segment_idx == count
	      && stop_it->pos < it3->first; ++stop_it)
	  {
	    stops_per_segment[last_segment].push_back(
	        Way_Position(0, 0, last_segment_is_forward ?
		             (stop_it->pos - last_it3->first)/(it3->first - last_it3->first) :
			     1.0 - (stop_it->pos - last_it3->first)/(it3->first - last_it3->first),
			     stop_it->color, stop_it->name));
/*	    cerr<<"B "<<last_segment.from.lat<<' '<<last_segment.from.lon<<' '
	        <<last_segment.to.lat<<' '<<last_segment.to.lon<<' '
		<<last_it3->first<<' '<<stop_it->pos<<' '<<it3->first<<' '
	        <<stops_per_segment[last_segment].back().pos<<'\n';*/
	  }
	  
	  segments[Placed_Segment(it3->second, last_it3->second)].insert(it->second);
	  ++last_it3;
        }
      }
      
      ++count;
      ++last_it2;
    }
    for (; stop_it != stops_per_way[it->second].end(); ++stop_it)
    {
      stops_per_segment[last_segment].push_back(
          Way_Position(0, 0, last_segment_is_forward ? 1.0 : 0.0, stop_it->color, stop_it->name));
      //cerr<<"C "<<last_segment.from.lon<<' '<<last_segment.to.lon<<'\n';
    }
  }

  // Debug output
/*  RouteFeature route_feature;
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
  features.routes.push_back(route_feature);*/

  // Debug output
/*  for (map< Placed_Segment, vector< Way_Position > >::const_iterator
      it = stops_per_segment.begin(); it != stops_per_segment.end(); ++it)
  {
    for (vector< Way_Position >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
    {
      features.stops.push_back(StopFeature
          (it2->name, it2->color,
	   (1.0 - it2->pos) * it->first.from.lat + it2->pos * it->first.to.lat,
	   (1.0 - it2->pos) * it->first.from.lon + it2->pos * it->first.to.lon));
      cerr<<it2->name<<' '<<it->first.from.lat<<' '<<it->first.from.lon
          <<' '<<it->first.to.lat<<' '<<it->first.to.lon<<' '<<it2->pos<<'\n';
    }
  }*/
  
  map< Way*, set< string > > colors_per_way;
  for (map< uint32, Relation* >::const_iterator it(osm_data.relations.begin());
      it != osm_data.relations.end(); ++it)
  {
    RouteFeature route_feature;
    string color(it->second->tags["color"]);
    if (color == "")
      color = default_color(it->second->tags["ref"]);
    for (vector< pair< OSMElement*, string > >::const_iterator
        it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
    {
      Way* way(dynamic_cast< Way* >(it2->first));
      if (way)
	colors_per_way[way].insert(color);
    }
  }

  map< Placed_Segment, set< string > > colors_per_segment;
  for (map< Placed_Segment, set< Way* > >::const_iterator it = segments.begin();
      it != segments.end(); ++it)
  {
    set< string > colors;
    for (set< Way* >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
      colors.insert(colors_per_way[*it2].begin(), colors_per_way[*it2].end());
    colors_per_segment[it->first] = colors;
  }

  map< Placed_Node, vector< const Placed_Segment* > > segments_per_node;
  for (map< Placed_Segment, set< string > >::const_iterator it = colors_per_segment.begin();
      it != colors_per_segment.end(); ++it)
  {
    segments_per_node[it->first.from].push_back(&it->first);
    segments_per_node[it->first.to].push_back(&it->first);
  }
  
  //Debug output
//   for (map< Placed_Node, vector< const Placed_Segment* > >::const_iterator
//       it = segments_per_node.begin(); it != segments_per_node.end(); ++it)
//   {
//     ostringstream out;
//     out<<it->second.size();
//     if (it->second.size() == 2)
//       out<<' '<<colors_per_segment[*it->second[0]].size()
//           <<' '<<colors_per_segment[*it->second[1]].size()<<'\n';
//     features.stops.push_back(StopFeature(out.str(), "#000000", it->first.lat, it->first.lon));
//   }

  map< Placed_Way, set< string > > colors_per_placed_way;
  for (map< Placed_Segment, set< string > >::const_iterator it = colors_per_segment.begin();
      it != colors_per_segment.end(); ++it)
  {
    bool join_from = (segments_per_node[it->first.from].size() == 2
        && colors_per_segment.find(*segments_per_node[it->first.from][0])->second == it->second
	&& colors_per_segment.find(*segments_per_node[it->first.from][1])->second == it->second);
    bool join_to = (segments_per_node[it->first.to].size() == 2
        && colors_per_segment.find(*segments_per_node[it->first.to][0])->second == it->second
	&& colors_per_segment.find(*segments_per_node[it->first.to][1])->second == it->second);
    if (join_from && join_to)
      continue;
    Placed_Way way;
    Placed_Node current_node(0, 0);
    if (join_from)
    {
      way.nds.push_back(it->first.to);
      way.nds.push_back(it->first.from);
      current_node = it->first.from;
    }
    else
    {
      way.nds.push_back(it->first.from);
      way.nds.push_back(it->first.to);
      current_node = it->first.to;
    }
    const Placed_Segment* current_segment = &it->first;
    while (segments_per_node[current_node].size() == 2)
    {
      uint new_idx = (segments_per_node[current_node][0] == current_segment ? 1 : 0);
      if (colors_per_segment.find(*current_segment)->second
	  != colors_per_segment.find(*segments_per_node[current_node][new_idx])->second)
	break;
      current_segment = segments_per_node[current_node][new_idx];
      if (segments_per_node[current_node][new_idx]->from == current_node)
      {
	way.nds.push_back(current_segment->to);
	current_node = current_segment->to;
      }
      else
      {
	way.nds.push_back(current_segment->from);
	current_node = current_segment->from;
      }
    }
    
    //Debug
/*    if (way.nds.size() == 2)
    {
      way.nds.push_back(way.nds[1]);
      way.nds[1].lat = (way.nds[0].lat + way.nds[2].lat)/2.0;
      way.nds[1].lon = (way.nds[0].lon + way.nds[2].lon)/2.0;
    }*/
    
    if (!(way.nds.back() < way.nds.front()))
      colors_per_placed_way.insert(make_pair(way, it->second));
  }

  // Debug output
  map< string, RouteFeature > routes_per_color;
  for (map< Placed_Way, set< string > >::const_iterator it = colors_per_placed_way.begin();
      it != colors_per_placed_way.end(); ++it)
  {
    int i = 1-it->second.size();
    for (set< string >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
    {
      PolySegmentFeature poly_segment(i);
      for (vector< Placed_Node >::const_iterator it3 = it->first.nds.begin();
          it3 != it->first.nds.end(); ++it3)
        poly_segment.lat_lon.push_back(NodeFeature(it3->lat, it3->lon));
      routes_per_color[*it2].polysegs.push_back(poly_segment);
      routes_per_color[*it2].color = *it2;
      i += 2;
    }
  }
  
  for (map< string, RouteFeature >::const_iterator it = routes_per_color.begin();
      it != routes_per_color.end(); ++it)
    features.routes.push_back(it->second);

  map< const Placed_Way*, vector< Way_Position > > stops_per_placed_way;
  for (map< Placed_Way, set< string > >::const_iterator it = colors_per_placed_way.begin();
      it != colors_per_placed_way.end(); ++it)
  {
    vector< Way_Position > not_yet_unified_ways;
    //cerr<<"Way \n"<<it->first.nds[0].lat<<' '<<it->first.nds[0].lon<<'\n';
    for (vector< Placed_Node >::size_type i = 1; i < it->first.nds.size(); ++i)
    {
      //cerr<<i-1<<' '<<it->first.nds[i].lat<<' '<<it->first.nds[i].lon<<'\n';
      map< Placed_Segment, vector< Way_Position > >::iterator stop_it
        = stops_per_segment.find(Placed_Segment(it->first.nds[i-1], it->first.nds[i]));
      if (stop_it == stops_per_segment.end() || stop_it->second.empty())
	continue;
      bool segment_is_forward = (stop_it->first.from == it->first.nds[i-1]);
      sort(stop_it->second.begin(), stop_it->second.end());
      if (segment_is_forward)
      {
        for (vector< Way_Position >::const_iterator stop_it2 = stop_it->second.begin();
            stop_it2 != stop_it->second.end(); ++stop_it2)
	{
	  not_yet_unified_ways.push_back(
	      Way_Position(0, i-1, stop_it2->pos, stop_it2->color, stop_it2->name));
	  //cerr<<stop_it2->name<<'\n';
	}
      }
      else
      {
        for (vector< Way_Position >::const_reverse_iterator stop_it2 = stop_it->second.rbegin();
            stop_it2 != stop_it->second.rend(); ++stop_it2)
	{
	  not_yet_unified_ways.push_back(
	      Way_Position(0, i-1, 1.0 - stop_it2->pos, stop_it2->color, stop_it2->name));
	  //cerr<<stop_it2->name<<'\n';
	}
      }
    }
    
    // Debug output
    for (vector< Way_Position >::const_iterator it2 = not_yet_unified_ways.begin();
        it2 != not_yet_unified_ways.end(); ++it2)
      features.stops.push_back(StopFeature
          (it2->name, it2->color,
	   (1.0 - it2->pos) * it->first.nds[it2->segment_idx].lat
	       + it2->pos * it->first.nds[it2->segment_idx+1].lat,
	   (1.0 - it2->pos) * it->first.nds[it2->segment_idx].lon
	       + it2->pos * it->first.nds[it2->segment_idx+1].lon));

    vector< double > accumulated_dist_per_segment;
    double total_dist = 0;
    for (vector< Placed_Node >::size_type i = 1; i < it->first.nds.size(); ++i)
    {
      accumulated_dist_per_segment.push_back(total_dist);
      total_dist += flat_dist(it->first.nds[i-1].lat, it->first.nds[i-1].lon,
			      it->first.nds[i].lat, it->first.nds[i].lon);
    }
    
    stops_per_placed_way[&it->first].swap(not_yet_unified_ways);
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
	    /**cos((south + north)*(1/2.0/90.0*acos(0)))*/;
      }
	    
      double vpos(double lat)
      {
	return (north_ - lat)*(1000000.0/9.0)/m_per_pixel_;
      }
      
      double hpos(double lat, double lon)
      {
	return (lon - pivot_lon_)*(1000000.0/9.0)/m_per_pixel_
	/**cos(lat*(1/90.0*acos(0)))*/ + pivot_hpos_;
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

void place_default_point(const Vec_2_Dim& p0, const Vec_2_Dim& p1, int shift,
			 double line_dist, ostream& out)
{
  Vec_2_Dim v = p1 + (-1.0)*p0;
  Vec_2_Dim v_norm = (1.0/v.length())*v;
  Vec_2_Dim target = p0 + shift*line_dist*turn_left(v_norm);
  if (2*abs(shift) < v.length())
    target = target + abs(shift)*line_dist*v_norm;
  else
    target = target + 0.5*v;
  out<<target.x<<' '<<target.y;
}

string shifted(double before_x, double before_y, double x, double y,
	       double after_x, double after_y, int shift_before, int shift_after)
{
  const double LINE_DIST = 2.0;
  
  if (shift_before*shift_after >= 0)
  {
    Vec_2_Dim in_vec(x - before_x, y - before_y);
    in_vec = (1.0/in_vec.length())*in_vec;
    Vec_2_Dim left_vec(turn_left(in_vec));
    Vec_2_Dim out_vec(after_x - x, after_y - y);
    out_vec = (1.0/out_vec.length())*out_vec;
    
    if ((out_vec.x*left_vec.x + out_vec.y*left_vec.y)*shift_before < 0.0)
    {
      Vec_2_Dim draw_vec(turn_left(in_vec + out_vec));
      draw_vec = (1.0/draw_vec.length())*draw_vec;
      Vec_2_Dim left_out_vec(turn_left(out_vec));
      
      ostringstream out;
      out<<x + LINE_DIST*left_vec.x*shift_before<<' '
         <<y + LINE_DIST*left_vec.y*shift_before<<", ";
      out<<x + LINE_DIST*draw_vec.x*0.5*(shift_before + shift_after)<<' '
         <<y + LINE_DIST*draw_vec.y*0.5*(shift_before + shift_after)<<", ";
      out<<x + LINE_DIST*left_out_vec.x*shift_after<<' '
         <<y + LINE_DIST*left_out_vec.y*shift_after;
      return out.str();
    }
    else
    {
      ostringstream out;
      place_default_point(Vec_2_Dim(x, y), Vec_2_Dim(before_x, before_y),
			  -shift_before, LINE_DIST, out);
      out<<", ";
      place_default_point(Vec_2_Dim(x, y), Vec_2_Dim(after_x, after_y),
			  shift_after, LINE_DIST, out);
      return out.str();
    }
  }
  else
  {
    ostringstream out;
    place_default_point(Vec_2_Dim(x, y), Vec_2_Dim(before_x, before_y),
			-shift_before, LINE_DIST, out);
    out<<", ";
    place_default_point(Vec_2_Dim(x, y), Vec_2_Dim(after_x, after_y),
			shift_after, LINE_DIST, out);
    return out.str();
  }
}

string shifted(double x, double y, double after_x, double after_y, int shift, bool forward)
{
  const double LINE_DIST = 2.0;
  
  ostringstream out;
  if (forward)
  {
    out<<x<<' '<<y<<", ";
    place_default_point(Vec_2_Dim(x, y), Vec_2_Dim(after_x, after_y),
			shift, LINE_DIST, out);
  }
  else
  {
    place_default_point(Vec_2_Dim(x, y), Vec_2_Dim(after_x, after_y),
			shift, LINE_DIST, out);
    out<<", "<<x<<' '<<y;
  }
  return out.str();
}

void sketch_features
    (ostream& out, const OSMData& osm_data,
     const Features& features,
     double pivot_lon, double m_per_pixel, double stop_font_size,
     double south = 100.0, double north = 100.0, double west = 200.0, double east = 200.0)
{
  ::Bbox bbox(::calc_bbox(osm_data));
  
  // if a restriction has been set, take the bbox from there.
  if (south != 100.0)
  {
    bbox.north = north;
    bbox.south = south;
    bbox.east = east;
    bbox.west = west;
  }
  else
  {
    bbox.north += 0.001*m_per_pixel;
    bbox.south -= 0.001*m_per_pixel;
    bbox.east += 0.003*m_per_pixel;
    bbox.west -= 0.003*m_per_pixel;
  }
  
  // expand the bounding box to avoid elements scratching the frame
  ::Coord_Transform coord_transform
      (bbox.north, bbox.south, bbox.west, bbox.east,
       m_per_pixel, pivot_lon);
  
  out<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
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
    out<<"<circle cx=\""
        <<coord_transform.hpos(it->lat, it->lon)
        <<"\" cy=\""
	<<coord_transform.vpos(it->lat)
        <<"\" r=\"6\" fill=\""<<it->color<<"\"/>\n";
	out<<"<text x=\""
	    <<coord_transform.hpos(it->lat, it->lon) + 6
	    <<"\" y=\""<<coord_transform.vpos(it->lat) - 6
	    <<"\" font-size=\""<<stop_font_size<<"px\">"<<it->name<<"</text>\n";
  }
  out<<'\n';
  
  for (vector< RouteFeature >::const_iterator it(features.routes.begin());
      it != features.routes.end(); ++it)
  {
    map< NodeFeature, vector< const PolySegmentFeature* > > polys_per_node;
    for (vector< PolySegmentFeature >::const_iterator it2(it->polysegs.begin());
        it2 != it->polysegs.end(); ++it2)
    {
      if (it2->lat_lon.size() < 2)
	continue;
      polys_per_node[it2->lat_lon.front()].push_back(&*it2);
      polys_per_node[it2->lat_lon.back()].push_back(&*it2);
    }
    
    for (vector< PolySegmentFeature >::const_iterator it2(it->polysegs.begin());
        it2 != it->polysegs.end(); ++it2)
    {
      if (it2->lat_lon.size() < 2)
	continue;
      
      out<<"<polyline fill=\"none\" stroke=\""<<it->color
          <<"\" stroke-width=\"3px\" points=\"";
	  
      // Prepare coordinates to enable forward looking.
      vector< double > x_s;
      vector< double > y_s;
      vector< int > shifts;
      bool display_front = true;
      bool display_back = true;
      if (polys_per_node[it2->lat_lon.front()].size() == 2)
      {
	display_front = false;
	const PolySegmentFeature* other_polyseg = polys_per_node[it2->lat_lon.front()][0];
	if (other_polyseg == &*it2)
	  other_polyseg = polys_per_node[it2->lat_lon.front()][1];
	double lat, lon;
	if (other_polyseg->lat_lon.front() == it2->lat_lon.front())
	{
	  lat = other_polyseg->lat_lon[1].lat;
	  lon = other_polyseg->lat_lon[1].lon;
	  shifts.push_back(-other_polyseg->shift);
	}
	else
	{
	  lat = other_polyseg->lat_lon[other_polyseg->lat_lon.size()-2].lat;
	  lon = other_polyseg->lat_lon[other_polyseg->lat_lon.size()-2].lon;
	  shifts.push_back(other_polyseg->shift);
	}
	x_s.push_back(coord_transform.hpos(lat, lon));
	y_s.push_back(coord_transform.vpos(lat));
      }
      for (vector< NodeFeature >::const_iterator it3(it2->lat_lon.begin());
          it3 != it2->lat_lon.end(); ++it3)
      {
	x_s.push_back(coord_transform.hpos(it3->lat, it3->lon));
	y_s.push_back(coord_transform.vpos(it3->lat));
	shifts.push_back(it2->shift);
      }
      if (polys_per_node[it2->lat_lon.back()].size() == 2)
      {
	display_back = false;
	const PolySegmentFeature* other_polyseg = polys_per_node[it2->lat_lon.back()][0];
	if (other_polyseg == &*it2)
	  other_polyseg = polys_per_node[it2->lat_lon.back()][1];
	double lat, lon;
	if (other_polyseg->lat_lon.front() == it2->lat_lon.back())
	{
	  lat = other_polyseg->lat_lon[1].lat;
	  lon = other_polyseg->lat_lon[1].lon;
	  shifts.back() = other_polyseg->shift;
	}
	else
	{
	  lat = other_polyseg->lat_lon[other_polyseg->lat_lon.size()-2].lat;
	  lon = other_polyseg->lat_lon[other_polyseg->lat_lon.size()-2].lon;
	  shifts.back() = -other_polyseg->shift;
	}
	x_s.push_back(coord_transform.hpos(lat, lon));
	y_s.push_back(coord_transform.vpos(lat));
      }
      
      if (x_s.size() >= 2)
      {
	unsigned int line_size = x_s.size();
	if (display_front)
	  out<<shifted(x_s[0], y_s[0], x_s[1], y_s[1], shifts[0], true);
	for (unsigned int i = 1; i < line_size-1; ++i)
	  out<<", "<<shifted(x_s[i-1], y_s[i-1], x_s[i], y_s[i], x_s[i+1], y_s[i+1],
			      shifts[i-1], shifts[i]);
	if (display_back)
	  out<<", "<<shifted(x_s[line_size-1], y_s[line_size-1],
			      x_s[line_size-2], y_s[line_size-2], -shifts[line_size-1], false);	
      }
      out<<"\"/>\n";
    }
    out<<'\n';
  }
  
  out<<"</svg>\n";
}

void restrict_to_bbox(Features& features, double north, double south, double west, double east)
{
  vector< StopFeature > stops;
  for (vector< StopFeature >::const_iterator it = features.stops.begin();
      it != features.stops.end(); ++it)
  {
    if (it->lat >= south && it->lat <= north && it->lon >= west && it->lon <= east)
      stops.push_back(*it);
  }
  features.stops.swap(stops);
  
  vector< RouteFeature > routes;
  for (vector< RouteFeature >::iterator it = features.routes.begin();
      it != features.routes.end(); ++it)
  {
    for (vector< PolySegmentFeature >::iterator it2 = it->polysegs.begin();
        it2 != it->polysegs.end(); ++it2)
    {
      vector< NodeFeature > lat_lon;
      for (vector< NodeFeature >::const_iterator it3 = it2->lat_lon.begin();
          it3 != it2->lat_lon.end(); ++it3)
      {
	if (it3->lat >= south && it3->lat <= north && it3->lon >= west && it3->lon <= east)
	  lat_lon.push_back(*it3);
      }
      it2->lat_lon.swap(lat_lon);
    }
  }
}

struct Map_Configuration
{
  double north, south, east, west;
  string name;
  vector< Map_Configuration >::size_type parent;
};

vector< Map_Configuration > config_data;
vector< vector< Map_Configuration >::size_type > config_data_stack;

void lokotree_start(const char *el, const char **attr)
{
  if (!strcmp(el, "map"))
  {
    Map_Configuration config;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "s"))
	config.south = atof(attr[i+1]);
      else if (!strcmp(attr[i], "n"))
	config.north = atof(attr[i+1]);
      else if (!strcmp(attr[i], "e"))
	config.east = atof(attr[i+1]);
      else if (!strcmp(attr[i], "w"))
	config.west = atof(attr[i+1]);
      else if (!strcmp(attr[i], "zoom"))
	config.name = attr[i+1];
    }
    if (config_data_stack.empty())
      config.parent = config_data.size();
    else
      config.parent = config_data_stack.back();
    config_data_stack.push_back(config_data.size());
    config_data.push_back(config);
  }
}

void lokotree_end(const char *el)
{
  if (!strcmp(el, "map"))
    config_data_stack.pop_back();
}

int main(int argc, char *argv[])
{
  int argi(1);
  double pivot_lon(-200.0), stop_font_size(12.0), scale(10.0),
      north(100.0), south(100.0), east(200.0), west(200.0), threshold(50.0);
  while (argi < argc)
  {
    if (!strncmp("--pivot-lon=", argv[argi], 12))
      pivot_lon = atof(((string)(argv[argi])).substr(12).c_str());
    else if (!strncmp("--stop-font-size=", argv[argi], 17))
      stop_font_size = atof(((string)(argv[argi])).substr(17).c_str());
    else if (!strncmp("--scale=", argv[argi], 8))
      scale = atof(((string)(argv[argi])).substr(8).c_str());
    else if (!strncmp("--north=", argv[argi], 8))
      north = atof(((string)(argv[argi])).substr(8).c_str());
    else if (!strncmp("--south=", argv[argi], 8))
      south = atof(((string)(argv[argi])).substr(8).c_str());
    else if (!strncmp("--west=", argv[argi], 7))
      west = atof(((string)(argv[argi])).substr(7).c_str());
    else if (!strncmp("--east=", argv[argi], 7))
      east = atof(((string)(argv[argi])).substr(7).c_str());
    else if (!strncmp("--threshold=", argv[argi], 12))
      threshold = atof(((string)(argv[argi])).substr(12).c_str());
    else if (!strncmp("--config=", argv[argi], 9))
    {
      FILE* config_file(fopen(((string)(argv[argi])).substr(9).c_str(), "r"));
      parse(config_file, lokotree_start, lokotree_end);
    }
    ++argi;
  }
  
  // read the XML input
  const OSMData& current_data(read_osm());
  
  // choose pivot_lon automatically
  if (pivot_lon == -200.0)
    pivot_lon = middle_lon(current_data);

  Features features(extract_features
      (const_cast< OSMData& >(current_data), pivot_lon, threshold));
  
  if (!config_data.empty())
  {
    int i = 0;
    for (vector< Map_Configuration >::const_iterator it = config_data.begin();
        it != config_data.end(); ++it)
    {
      Features local_features = features;

      double lat_scale = ((it->north - it->south)/360.0*40*1000*1000)/600.0;
      double lon_scale = (
          (transform_lon((it->north + it->south)/2.0, it->east, pivot_lon) - 
	   transform_lon((it->north + it->south)/2.0, it->west, pivot_lon))
	   /360.0*40*1000*1000)/600.0;
      scale = max(lat_scale, lon_scale);
      
      restrict_to_bbox(local_features, it->north, it->south,
		       transform_lon((it->north + it->south)/2.0, it->west, pivot_lon),
		       transform_lon((it->north + it->south)/2.0, it->east, pivot_lon));
      ostringstream file_name;
      file_name<<i++<<".svg";
      ofstream out(file_name.str().c_str());
      sketch_features(out, current_data, local_features, 0.0, scale,
		      stop_font_size, it->south, it->north,
		      transform_lon((it->north + it->south)/2.0, it->west, pivot_lon),
		      transform_lon((it->north + it->south)/2.0, it->east, pivot_lon));

      cout<<it->parent<<","<<(- transform_lon
              ((it->north + it->south)/2.0, it->west, pivot_lon))*(1000000.0/9.0)/scale
          <<","<<0<<","<<it->north<<","<<pivot_lon<<","<<scale
          <<",\""<<it->name<<"\"\n";
    }
  }
  else if (north != 100.0 && south != 100.0 && east != 200.0 && west != 200.0)
  {
    restrict_to_bbox(features, north, south,
		     transform_lon((north + south)/2.0, west, pivot_lon),
		     transform_lon((north + south)/2.0, east, pivot_lon));
    sketch_features(cout, current_data, features, 0.0, scale,
		    stop_font_size, south, north,
		    transform_lon((north + south)/2.0, west, pivot_lon),
		    transform_lon((north + south)/2.0, east, pivot_lon));
  }
  else
    sketch_features(cout, current_data, features, 0.0, scale,
		    stop_font_size);

  //sketch_unscaled_osm_data(current_data);
    
  //sketch_osm_data(current_data, middle_lon(current_data), 1.0);
  
  return 0;
}
