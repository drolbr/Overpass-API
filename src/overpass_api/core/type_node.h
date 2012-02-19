/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_NODE_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_NODE_H

#include "basic_types.h"
#include "index_computations.h"

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

struct Node
{
  uint32 id;
  uint32 ll_upper;
  uint32 ll_lower_;
  vector< pair< string, string > > tags;
  
  Node() {}
  
  Node(uint32 id_, double lat, double lon)
      : id(id_), ll_upper(ll_upper_(lat, lon)), ll_lower_(ll_lower(lat, lon))
  {}
  
  Node(uint32 id_, uint32 ll_upper_, uint32 ll_lower__)
      : id(id_), ll_upper(ll_upper_), ll_lower_(ll_lower__)
  {}
  
  static uint32 ll_upper_(double lat, double lon)
  {
    uint32 ilat((lat + 91.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));

    uint32 temp = ::ll_upper(ilat, ilon);
    return (temp ^ 0x40000000);
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
    result ^= 0x80000000;
    
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
    result ^= 0x80000000;
    
    return result;
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
