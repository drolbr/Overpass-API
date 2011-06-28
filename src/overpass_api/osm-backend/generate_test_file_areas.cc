#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

typedef unsigned int uint;

template < typename T >
struct V : public vector< T >
{
  V(const T& t) : vector< T >(1, t) {}
  V& operator()(const T& t)
  {
    push_back(t);
    return *this;
  }
};

struct Data_Modifier
{
  virtual bool admit_node(uint id) const = 0;
  virtual bool admit_node_skeleton(uint id) const = 0;
  virtual bool admit_node_tags(uint id) const = 0;
  virtual bool admit_way(uint id) const = 0;
  virtual bool admit_way_skeleton(uint id) const = 0;
  virtual bool admit_way_tags(uint id) const = 0;
  virtual bool admit_relation(uint id) const = 0;
  virtual bool admit_relation_skeleton(uint id) const = 0;
  virtual bool admit_relation_tags(uint id) const = 0;
};

struct Accept_All_Tags : public Data_Modifier
{
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
};

struct Accept_All : public Accept_All_Tags
{
  virtual bool admit_node(uint id) const { return true; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return true; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return true; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
};

vector< pair< string, string > > collect_tags(string prefix, uint id)
{
  vector< pair< string, string > > tags;
  if (id < 100)
    tags.push_back(make_pair< string, string >
        (prefix + "_key", prefix + "_few"));
  if (id % 11 == 0)
  {
    ostringstream buf;
    buf<<prefix<<"_value_"<<(id / 11 + 1);
    tags.push_back(make_pair< string, string >
    (prefix + "_key_11", buf.str()));
  }
  if (id % 15 == 0)
    tags.push_back(make_pair< string, string >
    (prefix + "_key_15", prefix + "_value_15"));
  if ((id % 2 == 0) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(make_pair< string, string >
        (prefix + "_key_2/4", prefix + "_value_0"));
  if ((id % 4 == 1) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(make_pair< string, string >
        (prefix + "_key_2/4", prefix + "_value_1"));
  if (id % 5 == 0)
    tags.push_back(make_pair< string, string >
        (prefix + "_key_5", prefix + "_value_5"));
  if (id % 7 == 0)
  {
    ostringstream buf;
    buf<<prefix<<"_value_"<<(id % 21 / 7);
    tags.push_back(make_pair< string, string >
        (prefix + "_key_7", buf.str()));
  }
  if (id == 2310)
    tags.push_back(make_pair< string, string >
        (prefix + "_unique", prefix + "_2310"));
  return tags;
}

void create_node
    (uint id, double lat, double lon)
{
  cout<<"  <node id=\""<<id<<"\" lat=\""
      <<fixed<<setprecision(7)<<lat<<"\" lon=\""<<lon<<"\"/>\n";
}
   
void create_way
  (uint id, uint start_node_ref, uint end_node_ref, uint stepping,
   const vector< pair< string, string > >& tags)
{
  cout<<"  <way id=\""<<id<<"\">\n";
  if (start_node_ref < end_node_ref)
  {
    for (uint ref = start_node_ref; ref <= end_node_ref; ref += stepping)
      cout<<"    <nd ref=\""<<ref<<"\"/>\n";
  }
  else
  {
    for (uint ref = start_node_ref; (int)ref >= (int)end_node_ref; ref -= stepping)
      cout<<"    <nd ref=\""<<ref<<"\"/>\n";
  }
  for (vector< pair< string, string > >::const_iterator
      it = tags.begin(); it != tags.end(); ++it)
    cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  cout<<"  </way>\n";
}

void create_way
  (uint id, const vector< uint >& refs,
   const vector< pair< string, string > >& tags)
{
  cout<<"  <way id=\""<<id<<"\">\n";
  for (vector< uint >::const_iterator it(refs.begin()); it != refs.end(); ++it)
    cout<<"    <nd ref=\""<<*it<<"\"/>\n";
  for (vector< pair< string, string > >::const_iterator
      it = tags.begin(); it != tags.end(); ++it)
    cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  cout<<"  </way>\n";
}

void create_relation
    (uint id, const vector< uint >& refs, const vector< uint >& types,
     const vector< pair< string, string > >& tags)
{
  vector< string > roles = V< string >("zero")("one")("two")("three");
  vector< string > typenames = V< string >("node")("way")("relation");
  
  cout<<"  <relation id=\""<<id<<"\">\n";
  for (uint i = 0; i < refs.size(); ++i)
  {
    cout<<"    <member type=\""<<typenames[types[i]]<<
    "\" ref=\""<<refs[i]<<
    "\" role=\""<<roles[(refs[i] + types[i]) % 4]<<"\"/>\n";
  }
  for (vector< pair< string, string > >::const_iterator
      it = tags.begin(); it != tags.end(); ++it)
    cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  cout<<"  </relation>\n";
}

void create_node_test_rectangle
    (uint id, double south, double north, double west, double east)
{
  create_node(id, south, west);
  create_node(id + 1, north, west);
  create_node(id + 2, north, east);
  create_node(id + 3, south, east);
}

void create_way_test_rectangle
    (uint id, uint node_ref, const vector< pair< string, string > >& tags)
{
  create_way(id, node_ref, node_ref + 3, 1, tags);
}

void create_triangle_test_pattern_nodes
    (uint id, double lat, double lon, double scale)
{
  create_node(id,      lat + 1*scale, lon + 1*scale);
  create_node(id + 1,  lat + 5*scale, lon + 1*scale);
  create_node(id + 2,  lat + 1*scale, lon + 5*scale);

  create_node(id + 3,  lat + 1*scale, lon + 7*scale);
  create_node(id + 4,  lat + 1*scale, lon + 11*scale);
  create_node(id + 5,  lat + 5*scale, lon + 7*scale);

  create_node(id + 6,  lat + 1*scale, lon + 13*scale);
  create_node(id + 7,  lat + 1*scale, lon + 15*scale);
  create_node(id + 8,  lat + 5*scale, lon + 13*scale);

  create_node(id + 9,  lat + 1*scale, lon + 19*scale);
  create_node(id + 10, lat + 1*scale, lon + 23*scale);
  create_node(id + 11, lat + 3*scale, lon + 19*scale);

  create_node(id + 12, lat + 1*scale, lon + 25*scale);
  create_node(id + 13, lat + 5*scale, lon + 29*scale);
  create_node(id + 14, lat + 1*scale, lon + 29*scale);

  create_node(id + 15, lat + 1*scale, lon + 31*scale);
  create_node(id + 16, lat + 1*scale, lon + 35*scale);
  create_node(id + 17, lat + 5*scale, lon + 35*scale);

  create_node(id + 18, lat + 1*scale, lon + 39*scale);
  create_node(id + 19, lat + 1*scale, lon + 41*scale);
  create_node(id + 20, lat + 5*scale, lon + 41*scale);

  create_node(id + 21, lat + 1*scale, lon + 43*scale);
  create_node(id + 22, lat + 1*scale, lon + 47*scale);
  create_node(id + 23, lat + 3*scale, lon + 47*scale);
}

void create_triangle_test_pattern_ways(uint id, uint node_ref, double scale)
{
  for (uint i = 1; i <= 8; ++i)
  {
    vector< pair< string, string > > tags;
    ostringstream v_dr, v_scale;
    v_dr<<i;
    tags.push_back(make_pair< string, string >("triangle", v_dr.str()));
    v_scale<<fixed<<setprecision(7)<<scale;
    tags.push_back(make_pair< string, string >("scale", v_scale.str()));
    create_way(id + i - 1,
	       V< uint >(node_ref + 3*i-3)(node_ref + 3*i-2)(node_ref + 3*i-1)
	        (node_ref + 3*i-3), tags);
  }
}

void create_shapes_test_pattern_nodes
    (uint id, double lat, double lon, double scale)
{
  create_node(id, lat + 1*scale, lon + 2*scale);
  create_node(id + 1, lat + 2*scale, lon + 2*scale);
  create_node(id + 2, lat + 2*scale, lon + 1*scale);
  create_node(id + 3, lat + 3*scale, lon + 1*scale);
  create_node(id + 4, lat + 3*scale, lon + 2*scale);
  create_node(id + 5, lat + 4*scale, lon + 2*scale);
  create_node(id + 6, lat + 4*scale, lon + 3*scale);
  create_node(id + 7, lat + 3*scale, lon + 3*scale);
  create_node(id + 8, lat + 3*scale, lon + 4*scale);
  create_node(id + 9, lat + 2*scale, lon + 4*scale);
  create_node(id + 10, lat + 2*scale, lon + 3*scale);
  create_node(id + 11, lat + 1*scale, lon + 3*scale);
  
  for (uint i = 0; i < 4; ++i)
  {
    create_node(id + 12 + 2*i, lat + 1*scale, lon + (6+i)*scale);
    create_node(id + 13 + 2*i, lat + 2*scale, lon + (6+i)*scale);
  }
}

void create_shapes_test_pattern_ways(uint id, uint node_ref, double scale)
{
  vector< pair< string, string > > tags;
  ostringstream v_scale;
  v_scale<<fixed<<setprecision(7)<<scale;
  tags.push_back(make_pair< string, string >("scale", v_scale.str()));
  
  tags.push_back(make_pair< string, string >("shapes", "1"));
  create_way(id, V< uint >(node_ref)(node_ref + 1)(node_ref + 2)
      (node_ref + 3)(node_ref + 4)(node_ref + 5)(node_ref + 6)(node_ref + 7)
      (node_ref + 8)(node_ref + 9)(node_ref + 10)(node_ref + 11)(node_ref), tags);
  tags.pop_back();
      
  tags.push_back(make_pair< string, string >("shapes", "2"));
  create_way(id + 1, V< uint >(node_ref + 12)(node_ref + 14)(node_ref + 16)
      (node_ref + 18)(node_ref + 19)(node_ref + 17)(node_ref + 15)(node_ref + 13)
      (node_ref + 12), tags);
}

void create_multpolys_test_pattern_nodes
    (uint id, double lat, double lon, double scale)
{
  for (uint i = 0; i < 4; ++i)
  {
    create_node(id + 2*i, lat + 1*scale, lon + (1+i)*scale);
    create_node(id + 1 + 2*i, lat + 2*scale, lon + (1+i)*scale);
  }
  
  create_node(id + 11, lat + 1*scale, lon + 6*scale);
  create_node(id + 12, lat + 1*scale, lon + 8*scale);
  create_node(id + 13, lat + 3*scale, lon + 6*scale);
  create_node(id + 14, lat + 3*scale, lon + 8*scale);
  create_node(id + 15, lat + 2*scale, lon + 7*scale);
}

void create_multpolys_test_pattern_ways(uint id, uint node_ref, double scale)
{
  vector< pair< string, string > > tags;
  for (uint i = 0; i < 3; ++i)
  {
    create_way(id + 2*i, V< uint >(node_ref + 2*i)(node_ref + 2 + 2*i), tags);
    create_way(id + 2*i + 1, V< uint >(node_ref + 1 + 2*i)(node_ref + 3 + 2*i), tags);
  }
  create_way(id + 6, V< uint >(node_ref)(node_ref + 1), tags);
  create_way(id + 7, V< uint >(node_ref + 6)(node_ref + 7), tags);

  create_way(id + 11, V< uint >(node_ref + 11)(node_ref + 12)(node_ref + 14), tags);
  create_way(id + 12, V< uint >(node_ref + 11)(node_ref + 13)(node_ref + 14), tags);
  create_way(id + 13, V< uint >(node_ref + 11)(node_ref + 15), tags);
  create_way(id + 14, V< uint >(node_ref + 15)(node_ref + 14), tags);
}

void create_multpolys_test_pattern_relations(uint id, uint way_ref, double scale)
{
  vector< pair< string, string > > tags;
  ostringstream v_scale;
  v_scale<<fixed<<setprecision(7)<<scale;
  tags.push_back(make_pair< string, string >("scale", v_scale.str()));

  tags.push_back(make_pair< string, string >("multpoly", "1"));
  create_relation(id, V< uint >(way_ref)(way_ref + 1)(way_ref + 2)(way_ref + 3)
      (way_ref + 4)(way_ref + 5)(way_ref + 6)(way_ref + 7),
      V< uint >(1)(1)(1)(1)(1)(1)(1)(1), tags);
  tags.pop_back();

  tags.push_back(make_pair< string, string >("multpoly", "2"));
  create_relation(id + 1, V< uint >(way_ref + 11)(way_ref + 13)(way_ref + 14),
      V< uint >(1)(1)(1), tags);
  tags.pop_back();

  tags.push_back(make_pair< string, string >("multpoly", "3"));
  create_relation(id + 2, V< uint >(way_ref + 12)(way_ref + 13)(way_ref + 14),
      V< uint >(1)(1)(1), tags);
  tags.pop_back();

  tags.push_back(make_pair< string, string >("multpoly", "4"));
  create_relation(id + 3, V< uint >(way_ref + 11)(way_ref + 12), V< uint >(1)(1), tags);
  tags.pop_back();
}

int main(int argc, char* args[])
{
  uint pattern_size = 2;
  if (argc > 1)
    pattern_size = atoi(args[1]);

  cout<<
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<osm>\n";    
    
  create_triangle_test_pattern_nodes(  1,  0.0,    0.0, 1.0);
  create_triangle_test_pattern_nodes(101, 10.0,    0.0, 0.1);
  create_triangle_test_pattern_nodes(201, 11.0,    0.0, 0.01);
  create_triangle_test_pattern_nodes(301, 11.1,    0.0, 0.001);
  create_triangle_test_pattern_nodes(401, 11.11,   0.0, 0.0001);
  create_triangle_test_pattern_nodes(501, 11.111,  0.0, 0.00001);
  create_triangle_test_pattern_nodes(601, 11.1111, 0.0, 0.000001);

  create_shapes_test_pattern_nodes(1001,  0.0,    50.0, 1.0);
  create_shapes_test_pattern_nodes(1101, 10.0,    10.0, 0.1);
  create_shapes_test_pattern_nodes(1201, 11.0,    10.0, 0.01);
  create_shapes_test_pattern_nodes(1301, 11.1,    10.0, 0.001);
  create_shapes_test_pattern_nodes(1401, 11.11,   10.0, 0.0001);
  create_shapes_test_pattern_nodes(1501, 11.111,  10.0, 0.00001);
  create_shapes_test_pattern_nodes(1601, 11.1111, 10.0, 0.000001);

  create_multpolys_test_pattern_nodes(2001, 10.0, 20.0, 0.1);
  
  create_triangle_test_pattern_ways( 1,   1, 1.0);
  create_triangle_test_pattern_ways(11, 101, 0.1);
  create_triangle_test_pattern_ways(21, 201, 0.01);
  create_triangle_test_pattern_ways(31, 301, 0.001);
  create_triangle_test_pattern_ways(41, 401, 0.0001);
  create_triangle_test_pattern_ways(51, 501, 0.00001);
  create_triangle_test_pattern_ways(61, 601, 0.000001);

  create_shapes_test_pattern_ways(101, 1001, 1.0);
  create_shapes_test_pattern_ways(111, 1101, 0.1);
  create_shapes_test_pattern_ways(121, 1201, 0.01);
  create_shapes_test_pattern_ways(131, 1301, 0.001);
  create_shapes_test_pattern_ways(141, 1401, 0.0001);
  create_shapes_test_pattern_ways(151, 1501, 0.00001);
  create_shapes_test_pattern_ways(161, 1601, 0.000001);
  
  create_multpolys_test_pattern_ways(201, 2001, 0.1);

  create_multpolys_test_pattern_relations(1, 201, 0.1);
  
  cout<<"</osm>\n";
}
