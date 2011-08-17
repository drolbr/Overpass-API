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

struct Print_Control
{
  Print_Control() : uid(0) {}
  Print_Control(uint uid_) : uid(uid_) {}
  
  void meta_data(uint id, int variant) const;
  bool print_allowed(uint id, int variant) const;
  
  private:
    uint uid;
};

void Print_Control::meta_data(uint id, int variant) const
{
  cout<<" version=\""<<((id + variant) % 256 + 1)<<"\""
        " timestamp=\"20"<<setw(2)<<setfill('0')<<(variant % 10)
      <<"-01-01T"<<setw(2)<<setfill('0')<<((id / 3600) % 24)
      <<":"<<setw(2)<<setfill('0')<<((id / 60) % 60)
      <<":"<<setw(2)<<setfill('0')<<(id % 60)<<"Z\""
        " changeset=\""<<(id + variant + 10)<<"\""
	" uid=\""<<((id/100 + variant) % 9990 + 10)<<"\""
	" user=\"User_"<<((id/100 + variant) % 9990 + 10)<<"\"";
}

bool Print_Control::print_allowed(uint id, int variant) const
{
  if (uid == 0)
    return true;

  return (uid == ((id/100 + variant) % 9990 + 10));
}

void fill_bbox_with_nodes
    (double south, double north, double west, double east,
     uint begin_id, uint end_id, const Print_Control& print_control,
     bool after_altered = false)
{
  int sqrt_ = sqrt(end_id - begin_id);
  for (int i = 0; i < sqrt_; ++i)
  {
    for (int j = 0; j < sqrt_; ++j)
    {
      if (print_control.print_allowed(begin_id + i*sqrt_ + j, after_altered ? 4 : 1))
      {
        cout<<"  <node id=\""<<(begin_id + i*sqrt_ + j)<<"\""
              " lat=\""<<fixed<<setprecision(7)<<((north - south)/sqrt_*(0.5 + i) + south)<<"\""
              " lon=\""<<fixed<<setprecision(7)<<((east - west)/sqrt_*(0.5 + j) + west)<<"\"";
        print_control.meta_data(begin_id + i*sqrt_ + j, after_altered ? 4 : 1);
        cout<<"/>\n";
      }
    }
  }
}

void create_way
  (uint id, uint start_node_ref, uint end_node_ref, uint stepping,
   const Print_Control& print_control, bool after_altered = false)
{
  if (!print_control.print_allowed(id, after_altered ? 5 : 2))
    return;
  
  cout<<"  <way id=\""<<id<<"\"";
  print_control.meta_data(id, after_altered ? 5 : 2);
  cout<<">\n";
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
  cout<<"  </way>\n";
}

void create_relation
    (uint id, const vector< uint >& refs, const vector< uint >& types,
     const Print_Control& print_control, bool after_altered = false)
{
  if (!print_control.print_allowed(id, after_altered ? 6 : 3))
    return;
  
  vector< string > roles = V< string >("zero")("one")("two")("three");
  vector< string > typenames = V< string >("node")("way")("relation");
  
  cout<<"  <relation id=\""<<id<<"\"";
  print_control.meta_data(id, after_altered ? 6 : 3);
  cout<<">\n";
  for (uint i = 0; i < refs.size(); ++i)
  {
    cout<<"    <member type=\""<<typenames[types[i]]<<
          "\" ref=\""<<refs[i]<<
          "\" role=\""<<roles[(refs[i] + types[i]) % 4]<<"\"/>\n";
  }
  cout<<"  </relation>\n";
}

void create_node_test_pattern
    (double south, double north, double west, double east, uint id, uint size,
     const Print_Control& print_control, bool after_altered = false)
{
  fill_bbox_with_nodes
      (south, north, west, east,
       id*size*size + 1, (id+1)*size*size + 1, print_control, after_altered);
}

void create_way_test_pattern(uint id, uint size,
			     const Print_Control& print_control,
			     bool pos_altered = false, bool after_altered = false)
{
  if (pos_altered)
  {
    for (uint i = 0; i < size*size-1; ++i)
      create_way(id*size*size + i+1, id*size*size + (i%size)+1, id*size*size + (i%size)+2, 1,
		 print_control, after_altered);
  }
  else
  {
    for (uint i = id*size*size + 1; i < (id+1)*size*size; ++i)
      create_way(i, i, i+1, 1, print_control, after_altered);
  }
}

void create_relation_test_pattern(uint id, uint size,
				  const Print_Control& print_control,
				  bool pos_altered = false, bool after_altered = false)
{
  if (pos_altered)
  {
    create_relation(id*size + 1,
		    V< uint >((id+1)*size*size - 1)((id+1)*size*size - 2),
		    V< uint >(0)(0), print_control, after_altered);
    create_relation(id*size + 2,
		    V< uint >((id+1)*size*size - 1)((id+1)*size*size - 2),
		    V< uint >(1)(1), print_control, after_altered);
    create_relation(id*size + 3,
		    V< uint >((id+1)*size*size - 1)((id+1)*size*size - 2),
		    V< uint >(0)(1), print_control, after_altered);
  }
  else
  {
    create_relation(id*size + 1,
		    V< uint >(id*size*size + 1)(id*size*size + 2),
		    V< uint >(0)(0), print_control, after_altered);
    create_relation(id*size + 2,
		    V< uint >(id*size*size + 1)(id*size*size + 2),
		    V< uint >(1)(1), print_control, after_altered);
    create_relation(id*size + 3,
		    V< uint >(id*size*size + 1)(id*size*size + 2),
		    V< uint >(0)(1), print_control, after_altered);
  }
}

int main(int argc, char* args[])
{
  uint pattern_size = 2;
  uint uid = 0;
  if (argc > 1)
    pattern_size = atoi(args[1]);
  enum { before, diff, after } pattern = before;
  if (argc > 2)
  {
    if (string(args[2]) == "diff")
      pattern = diff;
    else if (string(args[2]) == "after")
      pattern = after;
  }
  if (argc > 3)
  {
    if (string(args[3]).substr(0, 4) == "uid=")
      uid = atoll(args[3] + 4);
  }
  
  Print_Control print_control(uid);

  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osm-derived>\n"
  "<note>The data included in this document is from www.openstreetmap.org. It has there been "
  "collected by a large group of contributors. For individual attribution of each item please "
  "refer to http://www.openstreetmap.org/api/0.6/[node|way|relation]/#id/history </note>\n"
  "<meta osm_base=\"mock-up-init\"/>\n\n";

  if (pattern == before)
  {
    create_node_test_pattern(10.0, 11.0, 1.0, 2.0, 0, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 2.0, 3.0, 1, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 3.0, 4.0, 2, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 4.0, 5.0, 3, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 5.0, 6.0, 4, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 6.0, 7.0, 5, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 6.0, 7.0, 6, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 6.0, 7.0, 7, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 6.0, 7.0, 8, pattern_size, print_control);
    
    for (uint i = 0; i < 6; ++i)
      create_way_test_pattern(i, pattern_size, print_control);
    
    for (uint i = 0; i < 6; ++i)
      create_relation_test_pattern(i, pattern_size, print_control);
  }
  else if (pattern == diff)
  {
    create_node_test_pattern(10.0, 11.0, 2.0, 3.0, 1, pattern_size, print_control, true);
    create_node_test_pattern(11.0, 12.0, 3.0, 4.0, 2, pattern_size, print_control);
    cout<<"<delete>\n";
    create_node_test_pattern(10.0, 11.0, 4.0, 5.0, 3, pattern_size, print_control);
    cout<<"</delete>\n";
    create_node_test_pattern(10.0, 11.0, 5.0, 6.0, 4, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 6.0, 7.0, 5, pattern_size, print_control);
    cout<<"<delete>\n";
    create_node_test_pattern(10.0, 11.0, 7.0, 8.0, 6, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 8.0, 9.0, 7, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 9.0, 10.0, 8, pattern_size, print_control);
    cout<<"</delete>\n";
    
    create_way_test_pattern(1, pattern_size, print_control, false, true);
    cout<<"<delete>\n";
    create_way_test_pattern(3, pattern_size, print_control);
    cout<<"</delete>\n";
    create_way_test_pattern(4, pattern_size, print_control, true, false);
    
    create_relation_test_pattern(1, pattern_size, print_control, false, true);
    cout<<"<delete>\n";
    create_relation_test_pattern(3, pattern_size, print_control);
    cout<<"</delete>\n";
    create_relation_test_pattern(5, pattern_size, print_control, true, false);
  }  
  else if (pattern == after)
  {
    create_node_test_pattern(10.0, 11.0, 1.0, 2.0, 0, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 2.0, 3.0, 1, pattern_size, print_control, true);
    create_node_test_pattern(11.0, 12.0, 3.0, 4.0, 2, pattern_size, print_control);
    // nodes 3 deleted
    create_node_test_pattern(10.0, 11.0, 5.0, 6.0, 4, pattern_size, print_control);
    create_node_test_pattern(10.0, 11.0, 6.0, 7.0, 5, pattern_size, print_control);
    // nodes 6 to 8 deleted
    
    create_way_test_pattern(0, pattern_size, print_control);
    create_way_test_pattern(1, pattern_size, print_control, false, true);
    create_way_test_pattern(2, pattern_size, print_control);
    create_way_test_pattern(4, pattern_size, print_control, true, false);
    create_way_test_pattern(5, pattern_size, print_control);
    
    create_relation_test_pattern(0, pattern_size, print_control);
    create_relation_test_pattern(1, pattern_size, print_control, false, true);
    create_relation_test_pattern(2, pattern_size, print_control);
    create_relation_test_pattern(4, pattern_size, print_control);
    create_relation_test_pattern(5, pattern_size, print_control, true, false);
  }
  
  cout<<"\n</osm-derived>\n";
}
