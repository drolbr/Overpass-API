#include <cmath>
#include <cstdlib>
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

struct Accept_All : public Data_Modifier
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

struct Accept_Print_1 : public Data_Modifier
{
  virtual bool admit_node(uint id) const { return (id % 10000 == 0); }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return (id % 1000 == 0); }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return (id % 4 == 0); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
};

struct Accept_Print_2 : public Data_Modifier
{
  virtual bool admit_node(uint id) const { return (id % 10000 == 0); }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 1000 == 0); }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 4 == 0); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return false; }
};

struct Accept_Print_3 : public Data_Modifier
{
  virtual bool admit_node(uint id) const { return (id % 10000 == 0); }
  virtual bool admit_node_skeleton(uint id) const { return false; }
  virtual bool admit_node_tags(uint id) const { return false; }
  virtual bool admit_way(uint id) const { return (id % 1000 == 0); }
  virtual bool admit_way_skeleton(uint id) const { return false; }
  virtual bool admit_way_tags(uint id) const { return false; }
  virtual bool admit_relation(uint id) const { return (id % 4 == 0); }
  virtual bool admit_relation_skeleton(uint id) const { return false; }
  virtual bool admit_relation_tags(uint id) const { return false; }
};

struct Accept_Recurse_1 : public Data_Modifier
{
  Accept_Recurse_1(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return ((id > pattern_size) && (id <= pattern_size*3/2+1)); }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_2 : public Data_Modifier
{
  Accept_Recurse_2(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return ((id % pattern_size == pattern_size/2+1)
          && (id < 2*pattern_size*pattern_size)); }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_3 : public Data_Modifier
{
  Accept_Recurse_3(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_4 : public Data_Modifier
{
  Accept_Recurse_4(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const
      { return (id == pattern_size*pattern_size - pattern_size/2); }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_5 : public Data_Modifier
{
  Accept_Recurse_5(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const
      { return (id <= pattern_size*(pattern_size/2-1)); }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_6 : public Data_Modifier
{
  Accept_Recurse_6(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const
      { return ((id == 1) || (id == 2)
          || (id == pattern_size+1) || (id == pattern_size+2)
	  || (id == pattern_size*pattern_size)); }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_7 : public Data_Modifier
{
  Accept_Recurse_7(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const
      { return ((id == 2) || (id == 8) || (id == 10) || (id == 11)); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_8 : public Data_Modifier
{
  Accept_Recurse_8(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const
      { return ((id == 1) || (id == 2)
          || (id == pattern_size/2*(pattern_size/2-1) + 1)
	  || (id == pattern_size/2*(pattern_size/2+1) + 1)); }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const { return false; }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_9 : public Data_Modifier
{
  Accept_Recurse_9(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const
      { return ((id == 6) || (id == 8) || (id == 10) || (id == 11)); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_10 : public Data_Modifier
{
  Accept_Recurse_10(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const
      { return ((id == 1) || (id == 2)); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

struct Accept_Recurse_11 : public Data_Modifier
{
  Accept_Recurse_11(uint pattern_size_) : pattern_size(pattern_size_) {}
  
  virtual bool admit_node(uint id) const { return false; }
  virtual bool admit_node_skeleton(uint id) const { return true; }
  virtual bool admit_node_tags(uint id) const { return true; }
  virtual bool admit_way(uint id) const { return false; }
  virtual bool admit_way_skeleton(uint id) const { return true; }
  virtual bool admit_way_tags(uint id) const { return true; }
  virtual bool admit_relation(uint id) const
      { return ((id == 9) || (id == 10)); }
  virtual bool admit_relation_skeleton(uint id) const { return true; }
  virtual bool admit_relation_tags(uint id) const { return true; }
  
  private:
    uint pattern_size;
};

vector< pair< string, string > > collect_tags(string prefix, uint id)
{
  vector< pair< string, string > > tags;
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
     uint begin_id, uint end_id,
     const Data_Modifier* modifier)
{
  int sqrt_ = sqrt(end_id - begin_id);
  for (int i = 0; i < sqrt_; ++i)
  {
    for (int j = 0; j < sqrt_; ++j)
    {
      if (!modifier->admit_node(begin_id + i*sqrt_ + j))
	continue;
      if (!modifier->admit_node_skeleton(begin_id + i*sqrt_ + j) &&
	  !modifier->admit_node_tags(begin_id + i*sqrt_ + j))
      {
	cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j)<<"\"/>\n";
	continue;
      }
      cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j)<<"\""
      " lat=\""<<((north - south)/sqrt_*(0.5 + i) + south)<<"\""
      " lon=\""<<((east - west)/sqrt_*(0.5 + j) + west)<<"\"";
      vector< pair< string, string > > tags
        = collect_tags("node", begin_id + i*sqrt_ + j);
      if ((tags.empty()) || !modifier->admit_node_tags(begin_id + i*sqrt_ + j))
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
  (uint id, uint start_node_ref, uint end_node_ref, uint stepping,
   const Data_Modifier* modifier)
{
  if (!modifier->admit_way(id))
    return;
  if (!modifier->admit_way_skeleton(id) && !modifier->admit_way_tags(id))
  {
    cout<<"  <way id=\""<<id<<"\"/>\n";
    return;
  }
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
  if (modifier->admit_way_tags(id))
  {
    vector< pair< string, string > > tags = collect_tags("way", id);
    for (vector< pair< string, string > >::const_iterator
        it = tags.begin(); it != tags.end(); ++it)
      cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  }
  cout<<"  </way>\n";
}

void create_relation
    (uint id, const vector< uint >& refs, const vector< uint >& types,
     const Data_Modifier* modifier)
{
  if (!modifier->admit_relation(id))
    return;
  if (!modifier->admit_relation_skeleton(id) &&
      !modifier->admit_relation_tags(id))
  {
    cout<<"  <relation id=\""<<id<<"\"/>\n";
    return;
  }
  
  vector< string > roles = V< string >("zero")("one")("two")("three");
  vector< string > typenames = V< string >("node")("way")("relation");
  
  cout<<"  <relation id=\""<<id<<"\">\n";
  for (uint i = 0; i < refs.size(); ++i)
  {
    cout<<"    <member type=\""<<typenames[types[i]]<<
    "\" ref=\""<<refs[i]<<
    "\" role=\""<<roles[(refs[i] + types[i]) % 4]<<"\"/>\n";
  }
  if (modifier->admit_relation_tags(id))
  {
    vector< pair< string, string > > tags = collect_tags("relation", id);
    for (vector< pair< string, string > >::const_iterator
        it = tags.begin(); it != tags.end(); ++it)
      cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  }
  cout<<"  </relation>\n";
}

void create_node_test_pattern
    (double south, double north, double west, double east, uint id, uint size,
     const Data_Modifier* modifier)
{
  fill_bbox_with_nodes
      (south, north, west, east,
       id*size*size + 1, (id+1)*size*size + 1, modifier);
}

void create_way_test_pattern(uint id, uint size, const Data_Modifier* modifier)
{
  uint way_id_offset = id * (2*(size/2+1)*(size/2-1) + size/2);
  uint node_id_offset = id*size*size;
  
  // Draw a rectangular mesh of streets of length 1 segment.
  for (uint i = 1; i < size/2; ++i)
    for (uint j = 1; j <= size/2; ++j)
      create_way(way_id_offset + (i-1)*size/2 + j,
		 node_id_offset + i*size + j, node_id_offset + i*size + j + 1, 1,
		 modifier);
  way_id_offset += size/2*(size/2-1);
  for (uint i = 1; i <= size/2; ++i)
    for (uint j = 1; j < size/2; ++j)
      create_way(way_id_offset + (i-1)*(size/2-1) + j,
		 node_id_offset + (i-1)*size + j + 1,
		 node_id_offset + i*size + j + 1, size, modifier);
		 
  way_id_offset += size/2*(size/2-1);
  
  // Draw long straight ways from south to north.
  for (uint j = size/2+1; j < size; ++j)
    create_way(way_id_offset + j - size/2,
	       node_id_offset + j,
	       node_id_offset + size*(size-1) + j, size, modifier);  
  way_id_offset += size/2-1;
  // Draw long straight ways from east to west.
  for (uint i = size/2+1; i < size; ++i)
    create_way(way_id_offset + i - size/2,
	       node_id_offset + size*i,
	       node_id_offset + size*(i-1) + 1, 1, modifier);
	       
  way_id_offset += size/2-1;
  // Draw diagonal ways from northwest to southeast
  for (uint i = 0; i < size/2; ++i)
    create_way(way_id_offset + i + 1,
	       node_id_offset + size*(i+size/2) + 1,
	       node_id_offset + i+size/2 + 1, (size-1)*(i+size/2), modifier);
}

void create_relation_test_pattern(uint id, uint size, const Data_Modifier* modifier)
{
  uint way_id_offset = id * (2*(size/2+1)*(size/2-1) + size/2);
  uint node_id_offset = id*size*size;
  uint relation_id_offset = id*11;

  //create three small and two big relations based only on nodes
  create_relation(relation_id_offset + 1,
		  V< uint >(node_id_offset + 1)(node_id_offset + size + 2),
		  V< uint >(0)(0), modifier);
  create_relation
      (relation_id_offset + 2,
       V< uint >(node_id_offset + 1)(node_id_offset + 2)
         (node_id_offset + size + 2)(node_id_offset + size + 1)(node_id_offset + 1),
       V< uint >(0)(0)(0)(0)(0), modifier);
  create_relation(relation_id_offset + 3,
		  V< uint >(node_id_offset + 1)(node_id_offset + size*size),
		  V< uint >(0)(0), modifier);
  create_relation
      (relation_id_offset + 4,
       V< uint >(node_id_offset + 1)(node_id_offset + size)
         (node_id_offset + size*size)(node_id_offset + size*(size-1)),
       V< uint >(0)(0)(0)(0), modifier);
  create_relation(relation_id_offset + 5,
		  V< uint >(node_id_offset + size/2*size/2),
		  V< uint >(0), modifier);

  //create one small and one big relation based only on ways
  create_relation
      (relation_id_offset + 6,
       V< uint >(way_id_offset + 1)(way_id_offset + 2)
         (way_id_offset + size/2*(size/2-1) + 1)(way_id_offset + size/2*(size/2+1) + 1),
       V< uint >(1)(1)(1)(1), modifier);
  create_relation
      (relation_id_offset + 7,
       V< uint >(way_id_offset + size*(size/2-1) + 1)
                (way_id_offset + size*(size/2-1) + size/2)
		(way_id_offset + size*(size/2-1) + size - 1),
       V< uint >(1)(1)(1), modifier);
       
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
       V< uint >(0)(1)(0)(1)(0)(1)(0)(1)(0), modifier);
  
  // relations on relations
  create_relation
      (relation_id_offset + 9,
       V< uint >(relation_id_offset + 1)(relation_id_offset + 2)(relation_id_offset + 3)
         (relation_id_offset + 4)(relation_id_offset + 5)(relation_id_offset + 6),
       V< uint >(2)(2)(2)(2)(2)(2), modifier);
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
       V< uint >(2)(0)(1)(0)(1)(0)(1)(0)(1)(0)(2), modifier);
       
   // a big relation
   vector< uint > refs, types;
   int max_num_node_members = (65535 - (2*(size/2+1)*(size/2-1) + size/2))/2;
   if (max_num_node_members > size*size)
     max_num_node_members = size*size;
   for (int i = 0; i < max_num_node_members; ++i)
   {
     refs.push_back(node_id_offset + i + 1);
     types.push_back(0);
   }
   for (int i = 0; i < (2*(size/2+1)*(size/2-1) + size/2); ++i)
   {
     refs.push_back(way_id_offset + i + 1);
     types.push_back(1);
   }
   for (int i = 0; i < max_num_node_members; ++i)
   {
     refs.push_back(node_id_offset + i + 1);
     types.push_back(0);
   }
   create_relation(relation_id_offset + 11, refs, types, modifier);
}

int main(int argc, char* args[])
{
  uint pattern_size = 2;
  if (argc > 1)
    pattern_size = atoi(args[1]);
  Data_Modifier* modifier = 0;
  if (argc > 2)
  {
    if (string(args[2]) == "print_1")
      modifier = new Accept_Print_1;
    else if (string(args[2]) == "print_2")
      modifier = new Accept_Print_2;
    else if (string(args[2]) == "print_3")
      modifier = new Accept_Print_3;
    else if (string(args[2]) == "print_4")
      modifier = new Accept_Print_1; //print_1 and print_4 are equal
    else if (string(args[2]) == "recurse_1")
      modifier = new Accept_Recurse_1(pattern_size);
    else if (string(args[2]) == "recurse_2")
      modifier = new Accept_Recurse_2(pattern_size);
    else if (string(args[2]) == "recurse_3")
      modifier = new Accept_Recurse_3(pattern_size);
    else if (string(args[2]) == "recurse_4")
      modifier = new Accept_Recurse_4(pattern_size);
    else if (string(args[2]) == "recurse_5")
      modifier = new Accept_Recurse_5(pattern_size);
    else if (string(args[2]) == "recurse_6")
      modifier = new Accept_Recurse_6(pattern_size);
    else if (string(args[2]) == "recurse_7")
      modifier = new Accept_Recurse_7(pattern_size);
    else if (string(args[2]) == "recurse_8")
      modifier = new Accept_Recurse_8(pattern_size);
    else if (string(args[2]) == "recurse_9")
      modifier = new Accept_Recurse_9(pattern_size);
    else if (string(args[2]) == "recurse_10")
      modifier = new Accept_Recurse_10(pattern_size);
    else if (string(args[2]) == "recurse_11")
      modifier = new Accept_Recurse_11(pattern_size);
    else
      modifier = new Accept_All;
  }
  else
    modifier = new Accept_All;
  
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm>\n";
  
  create_node_test_pattern(51.0, 52.0, 7.0, 8.0, 0, pattern_size, modifier);
  create_node_test_pattern(47.9, 48.1, -0.2, 0.2, 1, pattern_size, modifier);
  create_node_test_pattern(-10.0, 80.0, -15.0, 90.0, 2, pattern_size, modifier);
  create_node_test_pattern(30.0, 50.0, -120.0, -60.0, 3, pattern_size, modifier);
  
  for (uint i = 0; i < 3; ++i)
    create_way_test_pattern(i, pattern_size, modifier);
  
  for (uint i = 0; i < 3; ++i)
    create_relation_test_pattern(i, pattern_size, modifier);
  
  cout<<"</osm>\n";
}
