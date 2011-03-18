#include <cmath>
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

vector< pair< string, string > > collect_tags(string prefix, uint id)
{
  vector< pair< string, string > > tags;
  if ((id % 2 == 0) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(make_pair< string, string >
        (prefix + "_key_2/4", prefix + "_value_0"));
  if ((id % 4 == 1) && ((prefix == "way") || (prefix == "relation")))
    tags.push_back(make_pair< string, string >
        (prefix + "_key_2/4", prefix + "_value_1"));
  if (id % 5 == 0)
    tags.push_back(make_pair< string, string >
        (prefix + "_key_5", prefix + "_value_5"));
  if (id % 15 == 0)
    tags.push_back(make_pair< string, string >
        (prefix + "_key_15", prefix + "_value_15"));
  if (id % 7 == 0)
  {
    ostringstream buf;
    buf<<prefix<<"_value_"<<(id % 21 / 7);
    tags.push_back(make_pair< string, string >
        (prefix + "_key_7", buf.str()));
  }
  if (id % 11 == 0)
  {
    ostringstream buf;
    buf<<prefix<<"_value_"<<(id / 11 + 1);
    tags.push_back(make_pair< string, string >
        (prefix + "key_11", buf.str()));
  }
  return tags;
}

void fill_bbox_with_nodes
    (double south, double north, double west, double east,
     uint begin_id, uint end_id)
{
  int sqrt_ = sqrt(end_id - begin_id);
  for (int i = 0; i < sqrt_; ++i)
  {
    for (int j = 0; j < sqrt_; ++j)
    {
      cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j)<<"\""
      " lat=\""<<((north - south)/sqrt_*(0.5 + i) + south)<<"\""
      " lon=\""<<((east - west)/sqrt_*(0.5 + j) + west)<<"\"";
      vector< pair< string, string > > tags
        = collect_tags("node", begin_id + i*sqrt_ + j);
      if (tags.empty())
	cout<<"/>\n";
      else
      {
	cout<<">\n";
	for (vector< pair< string, string > >::const_iterator
	    it = tags.begin(); it != tags.end(); ++it)
	  cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
	cout<<"  </node>\n";
      }
    }
  }
}

void create_way
  (uint id, uint start_node_ref, uint end_node_ref, uint stepping)
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
  vector< pair< string, string > > tags = collect_tags("way", id);
  for (vector< pair< string, string > >::const_iterator
    it = tags.begin(); it != tags.end(); ++it)
    cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  cout<<"  </way>\n";
}

void create_relation
    (uint id, const vector< uint >& refs, const vector< uint >& types)
{
  vector< string > roles = V< string >("zero")("one")("two")("three");
  vector< string > typenames = V< string >("node")("way")("relation");
  
  cout<<"  <relation id=\""<<id<<"\">\n";
  for (uint i = 0; i < refs.size(); ++i)
  {
    cout<<"    <member ref=\""<<refs[i]<<
    "\" type=\""<<typenames[types[i]]<<
    "\" role=\""<<roles[(refs[i] + types[i]) % 4]<<"\"/>\n";
  }
  vector< pair< string, string > > tags = collect_tags("relation", id);
  for (vector< pair< string, string > >::const_iterator
    it = tags.begin(); it != tags.end(); ++it)
    cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  cout<<"  </relation>\n";
}

void create_node_test_pattern
    (double south, double north, double west, double east, uint id, uint size)
{
  fill_bbox_with_nodes(south, north, west, east,
		       id*size*size + 1, (id+1)*size*size + 1);
}

void create_way_test_pattern(uint id, uint size)
{
  uint way_id_offset = id * (2*(size/2+1)*(size/2-1) + size/2);
  uint node_id_offset = id*size*size;
  
  // Draw a rectangular mesh of streets of length 1 segment.
  for (uint i = 1; i < size/2; ++i)
    for (uint j = 1; j <= size/2; ++j)
      create_way(way_id_offset + (i-1)*size/2 + j,
		 node_id_offset + i*size + j, node_id_offset + i*size + j + 1, 1);
  way_id_offset += size/2*(size/2-1);
  for (uint i = 1; i <= size/2; ++i)
    for (uint j = 1; j < size/2; ++j)
      create_way(way_id_offset + (i-1)*(size/2-1) + j,
		 node_id_offset + (i-1)*size + j + 1,
		 node_id_offset + i*size + j + 1, size);
		 
  way_id_offset += size/2*(size/2-1);
  
  // Draw long straight ways from south to north.
  for (uint j = size/2+1; j < size; ++j)
    create_way(way_id_offset + j - size/2,
	       node_id_offset + j,
	       node_id_offset + size*(size-1) + j, size);  
  way_id_offset += size/2-1;
  // Draw long straight ways from east to west.
  for (uint i = size/2+1; i < size; ++i)
    create_way(way_id_offset + i - size/2,
	       node_id_offset + size*i,
	       node_id_offset + size*(i-1) + 1, 1);
	       
  way_id_offset += size/2-1;
  // Draw diagonal ways from northwest to southeast
  for (uint i = 0; i < size/2; ++i)
    create_way(way_id_offset + i + 1,
	       node_id_offset + size*(i+size/2) + 1,
	       node_id_offset + i+size/2 + 1, (size-1)*(i+size/2));
}

void create_relation_test_pattern(uint id, uint size)
{
  uint way_id_offset = id * (2*(size/2+1)*(size/2-1) + size/2);
  uint node_id_offset = id*size*size;
  uint relation_id_offset = id*11;

  //create three small and two big relations based only on nodes
  create_relation(relation_id_offset + 1,
		  V< uint >(node_id_offset + 1)(node_id_offset + size + 2),
		  V< uint >(0)(0));
  create_relation
      (relation_id_offset + 2,
       V< uint >(node_id_offset + 1)(node_id_offset + 2)
         (node_id_offset + size + 2)(node_id_offset + size + 1)(node_id_offset + 1),
       V< uint >(0)(0)(0)(0)(0));
  create_relation(relation_id_offset + 3,
		  V< uint >(node_id_offset + 1)(node_id_offset + size*size),
		  V< uint >(0)(0));
  create_relation
      (relation_id_offset + 4,
       V< uint >(node_id_offset + 1)(node_id_offset + size)
         (node_id_offset + size*size)(node_id_offset + size*(size-1)),
       V< uint >(0)(0)(0)(0));
  create_relation(relation_id_offset + 5,
		  V< uint >(node_id_offset + size/2*size/2),
		  V< uint >(0));

  //create one small and one big relation based only on ways
  create_relation
      (relation_id_offset + 6,
       V< uint >(way_id_offset + 1)(way_id_offset + 2)
         (way_id_offset + size/2*(size/2-1) + 1)(way_id_offset + size/2*(size/2+1) + 1),
       V< uint >(1)(1)(1)(1));
  create_relation
      (relation_id_offset + 7,
       V< uint >(way_id_offset + size*(size/2-1) + 1)
                (way_id_offset + size*(size/2-1) + size/2)
		(way_id_offset + size*(size/2-1) + size - 1),
       V< uint >(1)(1)(1));
       
  // mixed
  create_relation
      (relation_id_offset + 8,
       V< uint >(node_id_offset + 1)
         (way_id_offset + 1)
         (node_id_offset + 2)
	 (way_id_offset + 2)
         (node_id_offset + size + 2)
	 (way_id_offset + size/2*(size/2-1) + 1)
	 (node_id_offset + size + 1)
	 (way_id_offset + size/2*(size/2+1) + 1)
	 (node_id_offset + 1),
       V< uint >(0)(1)(0)(1)(0)(1)(0)(1)(0));
  
  // relations on relations
  create_relation
      (relation_id_offset + 9,
       V< uint >(relation_id_offset + 1)(relation_id_offset + 2)(relation_id_offset + 3)
         (relation_id_offset + 4)(relation_id_offset + 5)(relation_id_offset + 6),
       V< uint >(2)(2)(2)(2)(2)(2));
  create_relation
      (relation_id_offset + 10,
       V< uint >(relation_id_offset + 1)
         (node_id_offset + 1)
         (way_id_offset + 1)
         (node_id_offset + 2)
	 (way_id_offset + 2)
         (node_id_offset + size + 2)
	 (way_id_offset + size/2*(size/2-1) + 1)
	 (node_id_offset + size + 1)
	 (way_id_offset + size/2*(size/2+1) + 1)
	 (node_id_offset + 1)
	 (relation_id_offset + 2),
       V< uint >(2)(0)(1)(0)(1)(0)(1)(0)(1)(0)(2));
       
   // a big relation
   vector< uint > refs, types;
   int max_num_node_members = (65535 - (2*(size/2+1)*(size/2-1) + size/2))/2;
   if (max_num_node_members > size*size)
     max_num_node_members = size*size;
   for (int i = 0; i < size*size; ++i)
   {
     refs.push_back(node_id_offset + i + 1);
     types.push_back(0);
   }
   for (int i = 0; i < (2*(size/2+1)*(size/2-1) + size/2); ++i)
   {
     refs.push_back(way_id_offset + i + 1);
     types.push_back(1);
   }
   for (int i = 0; i < size*size; ++i)
   {
     refs.push_back(node_id_offset + i + 1);
     types.push_back(0);
   }
   create_relation(relation_id_offset + 11, refs, types);
}

int main(int argc, char* args[])
{
  uint pattern_size = 2000;
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size);
  create_node_test_pattern(47.9, 48.1, -0.2, 0.2, 1, pattern_size);
  create_node_test_pattern(-10.0, 80.0, -15.0, 90.0, 2, pattern_size);
  create_node_test_pattern(30.0, 50.0, -120.0, -60.0, 3, pattern_size);
  
  for (uint i = 0; i < 3; ++i)
    create_way_test_pattern(i, pattern_size);
  
  for (uint i = 0; i < 3; ++i)
    create_relation_test_pattern(i, pattern_size);
  
  cout<<"</osm>\n";
}
