#ifndef TYPE_NODE_DEFINED
#define TYPE_NODE_DEFINED

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "basic_types.h"

using namespace std;

struct Node
{
  uint32 id;
  uint32 ll_upper_;
  uint32 ll_lower_;
  vector< pair< string, string > > tags;
  
  Node() {}
  
  Node(uint32 id_, double lat, double lon)
  : id(id_), ll_upper_(ll_upper(lat, lon)), ll_lower_(ll_lower(lat, lon))
  {}
  
  static uint32 ll_upper(double lat, double lon)
  {
    uint32 result(0), ilat((lat + 91.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
    
    for (uint32 i(0); i < 16; ++i)
    {
      result |= ((0x1<<(i+16))&ilat)>>(15-i);
      result |= ((0x1<<(i+16))&(uint32)ilon)>>(16-i);
    }
    
    return result;
  }
  
  static uint32 ll_lower(double lat, double lon)
  {
    uint32 result(0), ilat((lat + 91.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
    
    for (uint32 i(0); i < 16; ++i)
    {
      result |= ((0x1<<i)&ilat)<<(i+1);
      result |= ((0x1<<i)&(uint32)ilon)<<i;
    }
    
    return result;
  }
  
  static double lat(uint32 ll_upper, uint32 ll_lower)
  {
    uint32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<(31-2*i))&ll_upper)<<i;
      result |= ((0x1<<(31-2*i))&ll_lower)>>(16-i);
    }
    
    return ((double)result)/10000000 - 91.0;
  }
  
  static double lon(uint32 ll_upper, uint32 ll_lower)
  {
    int32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<(30-2*i))&ll_upper)<<(i+1);
      result |= ((0x1<<(30-2*i))&ll_lower)>>(15-i);
    }
    
    return ((double)result)/10000000;
  }

  static vector< pair< uint32, uint32 > >* calc_ranges
    (double south, double north, double west, double east)
  {
    vector< pair< uint32, uint32 > >* ranges;
    if (west <= east)
      ranges = new vector< pair< uint32, uint32 > >();
    else
    {
      ranges = calc_ranges(south, north, west, 180.0);
      west = -180.0;
    }
    for (int i(0); 65536.0/10000000.0*i <= north - south; ++i)
    {
      for (int j(0); 65536.0/10000000.0*j <= east - west; ++j)
      {
	pair< uint32, uint32 > range;
	range.first = Node::ll_upper
	(south + 65536.0/10000000.0*i, west + 65536.0/10000000.0*j);
	range.second = range.first + 1;
	ranges->push_back(range);
      }
    }
    return ranges;
  }
};

struct Node_Comparator_By_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id < b.id);
  }
};

struct Node_Equal_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id == b.id);
  }
};

struct Node_Skeleton
{
  uint32 id;
  uint32 ll_lower;
  
  Node_Skeleton() {}
  
  Node_Skeleton(void* data)
  {
    id = *(uint32*)data;
    ll_lower = *(uint32*)((uint8*)data+4);
  }
  
  Node_Skeleton(const Node& node)
  : id(node.id), ll_lower(node.ll_lower_) {}
  
  Node_Skeleton(uint32 id_, uint32 ll_lower_)
  : id(id_), ll_lower(ll_lower_) {}
  
  uint32 size_of() const
  {
    return 8;
  }
  
  static uint32 size_of(void* data)
  {
    return 8;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *(uint32*)((uint8*)data+4) = ll_lower;
  }
  
  bool operator<(const Node_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Node_Skeleton& a) const
  {
    return this->id == a.id;
  }
};

#endif
