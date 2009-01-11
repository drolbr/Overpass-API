#ifndef SCRIPT_DATATYPES
#define SCRIPT_DATATYPES

#include <map>
#include <set>
#include <string>
#include <vector>
#include <math.h>

#include <mysql.h>

using namespace std;

struct Node
{
  public:
    Node(int id_, int lat_, int lon_) : id(id_), lat(lat_), lon(lon_) {}
    
    const int id;
    int lat, lon;
};

inline bool operator<(const Node& node_1, const Node& node_2)
{
  return (node_1.id < node_2.id);
}

struct Way
{
  public:
    Way(int id_) : id(id_) {}
    
    const int id;
    vector< int > members;
};

inline bool operator<(const Way& way_1, const Way& way_2)
{
  return (way_1.id < way_2.id);
}

struct Relation
{
  public:
    Relation(int id_) : id(id_) {}
    
    const int id;
    set< pair< int, int > > node_members;
    set< pair< int, int > > way_members;
    set< pair< int, int > > relation_members;
};

inline bool operator<(const Relation& relation_1, const Relation& relation_2)
{
  return (relation_1.id < relation_2.id);
}

struct Line_Segment
{
  public:
    Line_Segment(int west_lat_, int west_lon_, int east_lat_, int east_lon_)
  : west_lon(west_lon_), west_lat(west_lat_), east_lon(east_lon_),  east_lat(east_lat_) {}
    
    const int west_lon, west_lat;
    const int east_lon, east_lat;
};

inline bool operator<(const Line_Segment& line_segment_1, const Line_Segment& line_segment_2)
{
  if (line_segment_1.west_lon < line_segment_2.west_lon)
    return true;
  if (line_segment_1.west_lon > line_segment_2.west_lon)
    return false;
  if (line_segment_1.west_lat < line_segment_2.west_lat)
    return true;
  if (line_segment_1.west_lat > line_segment_2.west_lat)
    return false;
  if (line_segment_1.east_lon < line_segment_2.east_lon)
    return true;
  if (line_segment_1.east_lon > line_segment_2.east_lon)
    return false;
  return (line_segment_1.east_lat < line_segment_2.east_lat);
}

struct Area
{
  public:
    Area(int id_) : id(id_) {}
    
    const int id;
    set< Line_Segment > segments;
};

const int REF_NODE = 1;
const int REF_WAY = 2;
const int REF_RELATION = 3;

inline bool operator<(const Area& area_1, const Area& area_2)
{
  return (area_1.id < area_2.id);
}

inline int calc_idx(int a)
{
  return ((int)floor(((double)a)/10000000));
}

class Set
{
  public:
    Set() {}
    Set(const set< Node >& nodes_,
	const set< Way >& ways_,
        const set< Relation >& relations_)
  : nodes(nodes_), ways(ways_), relations(relations_) {}
    Set(const set< Node >& nodes_,
	const set< Way >& ways_,
	const set< Relation >& relations_,
	const set< Area >& areas_)
  : nodes(nodes_), ways(ways_), relations(relations_), areas(areas_) {}
    
    const set< Node >& get_nodes() const { return nodes; }
    const set< Way >& get_ways() const { return ways; }
    const set< Relation >& get_relations() const { return relations; }
    const set< Area >& get_areas() const { return areas; }
  
  private:
    set< Node > nodes;
    set< Way > ways;
    set< Relation > relations;
    set< Area > areas;
};

class Statement
{
  public:
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement) = 0;
    virtual string get_name() = 0;
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) = 0;
    virtual ~Statement() {}
};

//-----------------------------------------------------------------------------

inline int in_lat_lon(const char* input)
{
  double val(atof(input));
  if (val < 0)
    return (int)(val*10000000 - 0.5);
  return (int)(val*10000000 + 0.5);
}

inline int in_lat_lon(const string& input)
{
  double val(atof(input.c_str()));
  if (val < 0)
    return (int)(val*10000000 - 0.5);
  return (int)(val*10000000 + 0.5);
}

// faster but less portable
//
// inline int in_lat_lon(const string& input)
// {
//   unsigned int i(0), j(10000000), size(input.size());
//   bool minus(false);
//   int res(0);
//   if (input[0] == '-')
//   {
//     minus = true;
//     ++i;
//   }
//   while ((i < size) && (input[i] >= '0') && (input[i] <= '9'))
//   {
//     res = 10*res + (input[i] - '0');
//     ++i;
//   }
//   if (input[i] == '.')
//     ++i;
//   while ((i < size) && (input[i] >= '0') && (input[i] <= '9'))
//   {
//     res = 10*res + (input[i] - '0');
//     ++i;
//     j = j/10;
//   }
//   res = res*j;
//   if ((i < size) || (j == 0))
//     return 200*10000000;
//   else
//     return (minus ? -res : res);
// }

#endif
