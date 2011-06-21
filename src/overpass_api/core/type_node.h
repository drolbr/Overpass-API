#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_NODE_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_NODE_H

#include <cstring>
#include <iostream> //DEBUG
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
  
  Node(uint32 id_, uint32 ll_upper__, uint32 ll_lower__)
  : id(id_), ll_upper_(ll_upper__), ll_lower_(ll_lower__)
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
  
  static uint32 ll_upper(uint32 ilat, int32 ilon)
  {
    uint32 result(0);
    
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
  
  static uint32 ll_lower(uint32 ilat, int32 ilon)
  {
    uint32 result(0);
    
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
  
  static uint32 ilat(uint32 ll_upper, uint32 ll_lower)
  {
    uint32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<(31-2*i))&ll_upper)<<i;
      result |= ((0x1<<(31-2*i))&ll_lower)>>(16-i);
    }
    
    return result;
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

  static int32 ilon(uint32 ll_upper, uint32 ll_lower)
  {
    int32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<(30-2*i))&ll_upper)<<(i+1);
      result |= ((0x1<<(30-2*i))&ll_lower)>>(15-i);
    }
    
    return result;
  }

  static vector< pair< uint32, uint32 > >* calc_ranges
    (double south, double north, double west, double east);
//   {
//     vector< pair< uint32, uint32 > >* ranges;
//     if (west <= east)
//       ranges = new vector< pair< uint32, uint32 > >();
//     else
//     {
//       ranges = calc_ranges(south, north, west, 180.0);
//       west = -180.0;
//     }
//     for (int i(0); 65536.0/10000000.0*(i-1) < north - south; ++i)
//     {
//       for (int j(0); 65536.0/10000000.0*(j-1) < east - west; ++j)
//       {
// 	pair< uint32, uint32 > range;
// 	range.first = Node::ll_upper
// 	(south + 65536.0/10000000.0*i, west + 65536.0/10000000.0*j);
// 	range.second = range.first + 1;
// 	ranges->push_back(range);
//       }
//     }
//     return ranges;
//   }

  static void recursively_calc_ranges
      (uint32 south, uint32 north, int32 west, int32 east,
       uint32 bitlevel, vector< pair< uint32, uint32 > >& ranges);
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

// Calculates the ranges touched by the given bbox.
// This function implicitly depends on the chosen coordinate encoding.
inline vector< pair< uint32, uint32 > >* Node::calc_ranges
    (double south, double north, double west, double east)
{
  vector< pair< uint32, uint32 > >* ranges;
  ranges = new vector< pair< uint32, uint32 > >();

  uint32 isouth((south + 91.0)*10000000+0.5);
  uint32 inorth((north + 91.0)*10000000+0.5);
  int32 iwest(west*10000000 + (west > 0 ? 0.5 : -0.5));
  int32 ieast(east*10000000 + (east > 0 ? 0.5 : -0.5));
  
  if (west <= east)
  {
    if ((west < 0) && (east >= 0))
    {
      Node::recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   0, ieast & 0xffff0000, 1, *ranges);
      Node::recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   iwest & 0xffff0000, 0xffff0000, 1, *ranges);
    }
    else
      Node::recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   iwest & 0xffff0000, ieast & 0xffff0000, 1, *ranges);
  }
  else
  {
    Node::recursively_calc_ranges
        (isouth & 0xffff0000, inorth & 0xffff0000,
	 iwest & 0xffff0000, int32(180.0*10000000 + 0.5) & 0xffff0000, 1, *ranges);
    Node::recursively_calc_ranges
        (isouth & 0xffff0000, inorth & 0xffff0000,
	 int32(-180.0*10000000 - 0.5) & 0xffff0000, ieast & 0xffff0000, 1, *ranges);
  }
  return ranges;
}

// Adds recursively the ranges based on the given products of
// closed intervals.
// The expected assertion for the recursion is that the indices are equal
// on the first bitlevel bits. Also, indexes must have the last 16 bit set to zero.
inline void Node::recursively_calc_ranges
    (uint32 south, uint32 north, int32 west, int32 east,
     uint32 bitlevel, vector< pair< uint32, uint32 > >& ranges)
{
  int32 dist = ((0xffff0000u>>bitlevel)&0xffff0000);
  
  // If the difference is exactly dist, the indices fill the whole square
  // and we can add this square to the ranges.
  if ((south + dist == north) && (west + dist == east))
  {
    ranges.push_back
        (make_pair(ll_upper(south, west),
		   ll_upper(north, east) + 1));
    return;
  }
  
  // Shift dist to obtain a proper recursion.
  dist = ((dist>>1)&0xffff0000);
  
  if ((north | dist) != (south | dist))
  {
    // We need to split between northern and southern part.
    if ((east | dist) != (west | dist))
    {
      // We also need to split between western and eastern part.
      Node::recursively_calc_ranges
          (south, south | dist, west, west | dist,
	   bitlevel + 1, ranges);
      Node::recursively_calc_ranges
          (south, south | dist, (west | dist) + 0x10000, east,
	   bitlevel + 1, ranges);
      Node::recursively_calc_ranges
          ((south | dist) + 0x10000, north, west, west | dist,
	   bitlevel + 1, ranges);
      Node::recursively_calc_ranges
          ((south | dist) + 0x10000, north, (west | dist) + 0x10000, east,
	   bitlevel + 1, ranges);
    }
    else
    {
      // We don't need to split because east and west lie in the
      // same half of the current square.
      Node::recursively_calc_ranges
          (south, south | dist,  west, east,
	   bitlevel + 1, ranges);
      Node::recursively_calc_ranges
          ((south | dist) + 0x10000, north, west, east,
	   bitlevel + 1, ranges);
    }
  }
  else
  {
    // We don't need to split because north and south lie in the
    // same half of the current square.
    if ((east | dist) != (west | dist))
    {
      // We only need to split between western and eastern part.
      Node::recursively_calc_ranges
          (south, north, west, west | dist,
	   bitlevel + 1, ranges);
      Node::recursively_calc_ranges
          (south, north, (west | dist) + 0x10000, east,
	   bitlevel + 1, ranges);
    }
    else
      // We need no split at all.
      Node::recursively_calc_ranges
          (south, north, west, east, bitlevel + 1, ranges);
  }
}

#endif
