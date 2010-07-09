#ifndef TYPE_NODE_DEFINED
#define TYPE_NODE_DEFINED

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "basic_types.h"

using namespace std;

struct Aligned_Segment
{
  uint32 ll_upper_;
  uint64 ll_lower_a, ll_lower_b;
};

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
    for (int i(0); 65536.0/10000000.0*(i-1) < north - south; ++i)
    {
      for (int j(0); 65536.0/10000000.0*(j-1) < east - west; ++j)
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
  
  static void calc_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint64 from, uint64 to)
  {
    uint32 from_lat, to_lat;
    int32 from_lon, to_lon;
    
    for (uint32 i(0); i < 16; i+=1)
    {
      from_lat |= ((0x1<<(31-2*i))&(from>>32))<<i;
      from_lat |= ((0x1<<(31-2*i))&from)>>(16-i);
    }
    for (uint32 i(0); i < 16; i+=1)
    {
      to_lat |= ((0x1<<(31-2*i))&(to>>32))<<i;
      to_lat |= ((0x1<<(31-2*i))&to)>>(16-i);
    }
    for (uint32 i(0); i < 16; i+=1)
    {
      from_lon |= ((0x1<<(30-2*i))&(from>>32))<<(i+1);
      from_lon |= ((0x1<<(30-2*i))&from)>>(15-i);
    }
    for (uint32 i(0); i < 16; i+=1)
    {
      to_lon |= ((0x1<<(30-2*i))&(to>>32))<<(i+1);
      to_lon |= ((0x1<<(30-2*i))&to)>>(15-i);
    }
    
    if ((from_lon < -900000000) && (to_lon > 900000000))
    {
      calc_vert_aligned_segments
          (aligned_segments, from_lat, from_lon,
	   ((uint64)to_lat - from_lat)*((uint64)from_lon + 1800000000)/
	    ((uint64)360000000ull + from_lon - to_lon) + to_lat, -1800000000);
      calc_vert_aligned_segments
          (aligned_segments,
	   ((uint64)to_lat - from_lat)*((uint64)from_lon + 1800000000)/
	    ((uint64)360000000ull + from_lon - to_lon) + to_lat, 1800000000,
	   to_lat, to_lon);
    }
    else if ((to_lon < -900000000) && (from_lon > 900000000))
    {
      calc_vert_aligned_segments
          (aligned_segments, to_lat, to_lon,
	   ((uint64)from_lat - to_lat)*((uint64)to_lon + 1800000000)/
	    ((uint64)360000000ull + to_lon - from_lon) + from_lat, -1800000000);
      calc_vert_aligned_segments
          (aligned_segments,
	   ((uint64)from_lat - to_lat)*((uint64)to_lon + 1800000000)/
	    ((uint64)360000000ull + to_lon - from_lon) + from_lat, 1800000000,
	   from_lat, from_lon);
    }
    else if (from_lon < to_lon)
      calc_vert_aligned_segments(aligned_segments,
				 from_lat, from_lon, to_lat, to_lon);
    else
      calc_vert_aligned_segments(aligned_segments,
				 to_lat, to_lon, from_lat, from_lon);
  }

  static void calc_vert_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint32 from_lat, uint32 from_lon, uint32 to_lat, uint32 to_lon)
  {
    if ((from_lon & 0xfff00000) == (to_lon & 0xfff00000))
    {
      calc_horiz_aligned_segments(aligned_segments,
				  from_lat, from_lon, to_lat, to_lon);
      return;
    }
    int32 split_lon((from_lon & 0xfff00000) + 0x100000);
    calc_horiz_aligned_segments
        (aligned_segments, from_lat, from_lon,
	 ((uint64)to_lat - from_lat)*((uint64)split_lon - from_lon)/(to_lon - from_lon)
	 + from_lat, split_lon);
    for (; split_lon < (to_lon & 0xfff00000); split_lon += 0x100000)
      calc_horiz_aligned_segments
        (aligned_segments,
	 ((uint64)to_lat - from_lat)*((uint64)split_lon - from_lon)/(to_lon - from_lon)
	 + from_lat, split_lon,
	 ((uint64)to_lat - from_lat)*((uint64)split_lon + 0x100000 - from_lon)/
	   (to_lon - from_lon),
         split_lon + 0x100000);
    calc_horiz_aligned_segments
      (aligned_segments,
       ((uint64)to_lat - from_lat)*((uint64)split_lon - from_lon)/(to_lon - from_lon)
       + from_lat, split_lon, to_lat, to_lon);
  }

  static void calc_horiz_aligned_segments
      (vector< Aligned_Segment >& aligned_segments,
       uint32 from_lat, uint32 from_lon, uint32 to_lat, uint32 to_lon)
  {
    if ((from_lat & 0xfff00000) == (to_lat & 0xfff00000))
    {
      Aligned_Segment seg;
      seg.ll_upper_ = ll_upper(from_lat, from_lon);
      seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(from_lat, from_lon);
      seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower(to_lat, to_lon);
      seg.ll_upper_ &= 0xffffff00;
      aligned_segments.push_back(seg);
    }
    else if (from_lat < to_lat)
    {
      Aligned_Segment seg;
      uint32 split_lat((from_lat & 0xfff00000) + 0x100000);
      seg.ll_upper_ = ll_upper(from_lat, from_lon);
      seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(from_lat, from_lon);
      seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower
          (split_lat,
	   (int32)(((uint64)to_lon - from_lon)*((uint64)split_lat - from_lat)/
	    (to_lat - from_lat) + from_lon));
      seg.ll_upper_ &= 0xffffff00;
      aligned_segments.push_back(seg);
      for (; split_lat < (to_lat & 0xfff00000); split_lat += 0x100000)
      {
	int32 split_lon(((uint64)to_lon - from_lon)*((uint64)split_lat - from_lat)/
	    (to_lat - from_lat) + from_lon);
	seg.ll_upper_ = ll_upper(split_lat, split_lon);
	seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(split_lat, split_lon);
	seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower
	  (split_lat + 0x100000,
	   ((uint64)to_lon - from_lon)*((uint64)split_lat + 0x100000 - from_lat)/
	   (to_lat - from_lat) + from_lat);
        seg.ll_upper_ &= 0xffffff00;
	aligned_segments.push_back(seg);
      }
      int32 split_lon(((uint64)to_lon - from_lon)*((uint64)split_lat - from_lat)/
	  (to_lat - from_lat) + from_lon);
      seg.ll_upper_ = ll_upper(split_lat, split_lon);
      seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(split_lat, split_lon);
      seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower(to_lat, to_lon);
      seg.ll_upper_ &= 0xffffff00;
      aligned_segments.push_back(seg);       
    }
    else
    {
      Aligned_Segment seg;
      uint32 split_lat((to_lat & 0xfff00000) + 0x100000);
      seg.ll_upper_ = ll_upper(to_lat, to_lon);
      seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(to_lat, to_lon);
      seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower
          (split_lat,
	   ((uint64)from_lon - to_lon)*((uint64)split_lat - to_lat)/
	    (from_lat - to_lat) + to_lon);
      seg.ll_upper_ &= 0xffffff00;
      aligned_segments.push_back(seg);
      for (; split_lat < (from_lat & 0xfff00000); split_lat += 0x100000)
      {
	int32 split_lon(((uint64)from_lon - to_lon)*((uint64)split_lat - to_lat)/
	    (from_lat - to_lat) + to_lon);
	seg.ll_upper_ = ll_upper(split_lat, split_lon);
	seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(split_lat, split_lon);
	seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower
	  (split_lat + 0x100000,
	   ((uint64)from_lon - to_lon)*((uint64)split_lat + 0x100000 - to_lat)/
	   (from_lat - to_lat) + to_lat);
        seg.ll_upper_ &= 0xffffff00;
	aligned_segments.push_back(seg);
      }
      int32 split_lon(((uint64)from_lon - to_lon)*((uint64)split_lat - to_lat)/
	  (from_lat - to_lat) + to_lon);
      seg.ll_upper_ = ll_upper(split_lat, split_lon);
      seg.ll_lower_a = ((uint64)seg.ll_upper_<<32) | ll_lower(split_lat, split_lon);
      seg.ll_lower_b = ((uint64)seg.ll_upper_<<32) | ll_lower(from_lat, from_lon);
      seg.ll_upper_ &= 0xffffff00;
      aligned_segments.push_back(seg);       
    }
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
