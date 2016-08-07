/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_AREA_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_AREA_H

#include <cstring>
#include <map>
#include <set>
#include <vector>

#include <iomanip>
#include <iostream>
#include "type_node.h"

#include "basic_types.h"

using namespace std;

struct Aligned_Segment
{
  uint32 ll_upper_;
  uint64 ll_lower_a, ll_lower_b;
  
  bool operator<(const Aligned_Segment& b) const
  {
    return (ll_upper_ < b.ll_upper_);
  }
};

struct Area
{
  typedef Uint32_Index Id_Type;
  
  static Aligned_Segment segment_from_ll_quad
      (uint32 from_lat, int32 from_lon, uint32 to_lat, int32 to_lon)
  {
    Aligned_Segment result;
    uint32 a_ll_upper(::ll_upper(from_lat, from_lon) ^ 0x40000000);
    uint32 b_ll_upper(::ll_upper(to_lat, to_lon) ^ 0x40000000);
    result.ll_upper_ = a_ll_upper & 0xffffff00;
    result.ll_lower_a = (uint64)::ll_lower(from_lat, from_lon) |
        (((uint64)a_ll_upper & 0xff)<<32);
    result.ll_lower_b = (uint64)::ll_lower(to_lat, to_lon) |
        (((uint64)b_ll_upper & 0xff)<<32);
    
    return result;
  }
  
  static int32 proportion(int32 clow, int32 cmid, int32 cup, int32 low, int32 up)
  {
    if (cup == clow)
      return low;
    return (((int64)up - low))*((int64)cmid - clow)/((int64)cup - clow) + low;
  }
  
  static void calc_horiz_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint32 from_lat, uint32 from_lon, uint32 to_lat, uint32 to_lon)
  {
    if ((from_lat & 0xfff00000) == (to_lat & 0xfff00000))
      aligned_segments.push_back(segment_from_ll_quad
          (from_lat, from_lon, to_lat, to_lon));
    else if (from_lat < to_lat)
    {
      uint32 split_lat((from_lat & 0xfff00000) + 0x100000);
      aligned_segments.push_back(segment_from_ll_quad
          (from_lat, from_lon, split_lat - 1,
           proportion(from_lat, split_lat, to_lat, from_lon, to_lon)));
      for (; split_lat < (to_lat & 0xfff00000); split_lat += 0x100000)
      {
	aligned_segments.push_back(segment_from_ll_quad
	    (split_lat, proportion(from_lat, split_lat, to_lat, from_lon, to_lon),
	     split_lat + 0xfffff,
	     proportion(from_lat, split_lat + 0x100000, to_lat, from_lon, to_lon)));
      }
      aligned_segments.push_back(segment_from_ll_quad
          (split_lat, proportion(from_lat, split_lat, to_lat, from_lon, to_lon),
           to_lat, to_lon));
    }
    else
    {
      uint32 split_lat((to_lat & 0xfff00000) + 0x100000);
      aligned_segments.push_back(segment_from_ll_quad
          (to_lat, to_lon, split_lat - 1,
           proportion(to_lat, split_lat, from_lat, to_lon, from_lon)));
      for (; split_lat < (from_lat & 0xfff00000); split_lat += 0x100000)
      {
	aligned_segments.push_back(segment_from_ll_quad
	    (split_lat, proportion(to_lat, split_lat, from_lat, to_lon, from_lon),
	     split_lat + 0xfffff,
	     proportion(to_lat, split_lat + 0x100000, from_lat, to_lon, from_lon)));
      }
      aligned_segments.push_back(segment_from_ll_quad
          (split_lat, proportion(to_lat, split_lat, from_lat, to_lon, from_lon),
           from_lat, from_lon));
    }
  }
  
  
  static void calc_vert_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint32 from_lat, int32 from_lon, uint32 to_lat, int32 to_lon)
  {
    if ((from_lon & 0xfff00000) == (to_lon & 0xfff00000))
      calc_horiz_aligned_segments
          (aligned_segments, from_lat, from_lon, to_lat, to_lon);
    else if (from_lon < to_lon)
    {
      int32 split_lon((from_lon & 0xfff00000) + 0x100000);
      calc_horiz_aligned_segments
          (aligned_segments, from_lat, from_lon,
           proportion(from_lon, split_lon - 1, to_lon, from_lat, to_lat), split_lon - 1);
      for (; split_lon < (int32)(to_lon & 0xfff00000); split_lon += 0x100000)
	calc_horiz_aligned_segments
	    (aligned_segments,
	     proportion(from_lon, split_lon, to_lon, from_lat, to_lat), split_lon,
	     proportion(from_lon, split_lon + 0xfffff, to_lon, from_lat, to_lat),
	     split_lon + 0xfffff);
      calc_horiz_aligned_segments
	  (aligned_segments,
	   proportion(from_lon, split_lon, to_lon, from_lat, to_lat), split_lon,
	   to_lat, to_lon);
    }
    else
    {
      int32 split_lon((to_lon & 0xfff00000) + 0x100000);
      calc_horiz_aligned_segments
          (aligned_segments, to_lat, to_lon,
           proportion(to_lon, split_lon - 1, from_lon, to_lat, from_lat), split_lon - 1);
      for (; split_lon < (int32)(from_lon & 0xfff00000); split_lon += 0x100000)
	calc_horiz_aligned_segments
	    (aligned_segments,
	     proportion(to_lon, split_lon, from_lon, to_lat, from_lat), split_lon,
	     proportion(to_lon, split_lon + 0xfffff, from_lon, to_lat, from_lat),
	     split_lon + 0xfffff);
      calc_horiz_aligned_segments
	  (aligned_segments,
	   proportion(to_lon, split_lon, from_lon, to_lat, from_lat), split_lon,
	   from_lat, from_lon);
    }
  }
  
  
  static void calc_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint32 from_lat, int32 from_lon, uint32 to_lat, int32 to_lon)
  {
    if ((from_lon < -900000000) && (to_lon > 900000000))
    {
      calc_vert_aligned_segments
          (aligned_segments, to_lat, to_lon,
           proportion(to_lon, 1800000000, (uint64)from_lon + 3600000000ul,
		      to_lat, from_lat), 1800000000);
      calc_vert_aligned_segments
          (aligned_segments,
           proportion((uint64)to_lon - 3600000000ul, -1800000000, from_lon,
		      to_lat, from_lat), -1800000000, from_lat, from_lon);
    }
    else if ((to_lon < -900000000) && (from_lon > 900000000))
    {
      calc_vert_aligned_segments
          (aligned_segments, from_lat, from_lon,
	   proportion(from_lon, 1800000000, (uint64)to_lon + 3600000000ul,
		      from_lat, to_lat), 1800000000);
      calc_vert_aligned_segments
          (aligned_segments,
	   proportion((uint64)from_lon - 3600000000ul, -1800000000, to_lon,
		      from_lat, to_lat), -1800000000, to_lat, to_lon);
    }
    else
      calc_vert_aligned_segments
        (aligned_segments, from_lat, from_lon, to_lat, to_lon);
  }
      
      
  static void calc_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint64 from, uint64 to)
  {
    uint32 from_lat = ::ilat(from>>32, from&0xffffffff);
    uint32 to_lat = ::ilat(to>>32, to&0xffffffff);
    int32 from_lon = ::ilon(from>>32, from&0xffffffff);
    int32 to_lon = ::ilon(to>>32, to&0xffffffff);

    calc_aligned_segments(aligned_segments, from_lat, from_lon, to_lat, to_lon);
  }

        
      
  static void calc_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       double from_lat, double from_lon, double to_lat, double to_lon)
  {
    calc_aligned_segments(aligned_segments,
			  ::ilat(from_lat), ::ilon(from_lon), ::ilat(to_lat), ::ilon(to_lon));
  }
};


struct Area_Location
{
  uint32 id;
  vector< uint32 > used_indices;
  vector< pair< string, string > > tags;
  
  Area_Location() {}
  
  Area_Location(uint32 id_, const vector< uint32 >& used_indices_)
  : id(id_), used_indices(used_indices_) {}
  
  bool operator<(const Area_Location& a) const
  {
    return (this->id < a.id);
  }
  
  bool operator==(const Area_Location& a) const
  {
    return (this->id == a.id);
  }

  uint32 calc_index()
  {
    if (used_indices.empty())
      return 0;

    return ::calc_index(used_indices);
  }
};


struct Area_Skeleton
{
  typedef Area::Id_Type Id_Type;
  
  Id_Type id;
  vector< uint32 > used_indices;
  
  Area_Skeleton() : id(0u) {}
  
  Area_Skeleton(void* data) : id(0u)
  {
    id = *(uint32*)data;
    for (uint i(0); i < *((uint32*)data + 1); ++i)
      used_indices.push_back(*((uint32*)data + i + 2));
  }
  
  Area_Skeleton(const Area_Location& loc)
      : id(loc.id), used_indices(loc.used_indices) {}
  
  uint32 size_of() const
  {
    return 8 + 4*used_indices.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (8 + 4 * *((uint32*)data + 1));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id.val();
    *((uint32*)data + 1) = used_indices.size();
    uint i(2);
    for (vector< uint32 >::const_iterator it(used_indices.begin());
    it != used_indices.end(); ++it)
    {
      *((uint32*)data + i) = *it;
      ++i;
    }
  }
  
  static Area::Id_Type get_id(const void* data)
  {
    return *(const uint32*)data;
  }

  bool operator<(const Area_Skeleton& a) const
  {
    return (this->id < a.id);
  }
  
  bool operator==(const Area_Skeleton& a) const
  {
    return (this->id == a.id);
  }
};

struct Area_Block
{
  typedef Area::Id_Type Id_Type;
  
  Id_Type id;
  vector< uint64 > coors;
  
  Area_Block() : id(0u) {}
  
  Area_Block(void* data) : id(*(uint32*)data)
  {
    id = *(uint32*)data;
    coors.resize(*((uint16*)data + 2));
    for (int i(0); i < *((uint16*)data + 2); ++i)
      coors[i] = (*(uint64*)((uint8*)data + 6 + 5*i)) & (uint64)0xffffffffffull;
  }
  
  Area_Block(uint32 id_, const vector< uint64 >& coors_)
  : id(id_), coors(coors_) {}
  
  uint32 size_of() const
  {
    return 6 + 5*coors.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (6 + 5 * *((uint16*)data + 2));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id.val();
    *((uint16*)data + 2) = coors.size();
    for (uint i(0); i < coors.size(); ++i)
    {
      *(uint32*)((uint8*)data + 6 + 5*i) = coors[i];
      *((uint8*)data + 10 + 5*i) = (coors[i])>>32;
    }
  }
  
  bool operator<(const Area_Block& a) const
  {
    if (this->id < a.id)
      return true;
    else if (a.id < this->id)
      return false;
    return (this->coors < a.coors);
  }
  
  bool operator==(const Area_Block& a) const
  {
    return ((this->id == a.id) && (this->coors == a.coors));
  }
};

#endif
