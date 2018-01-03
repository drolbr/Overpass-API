#include "four_field_index.h"
#include "index_computations.h"


#include <cmath>
#include <iomanip>
#include <sstream>


namespace
{
  int32 exchange_value(Four_Field_Entry& entry, uint32 ilat, int32 ilon, int32 value)
  {
    int32 result;
  
    if (ilat & 0x00010000)
    {
      if (ilon & 0x00010000)
      {
        result = entry.ne;
        entry.ne = value;
      }
      else
      {
        result = entry.nw;
        entry.nw = value;
      }
    }
    else
    {
      if (ilon & 0x00010000)
      {
        result = entry.se;
        entry.se = value;
      }
      else
      {
        result = entry.sw;
        entry.sw = value;
      }
    }
  
    return result;
  }
}


int32 Four_Field_Index::add_point(double lat, double lon, int32 value)
{
  if (lat < -90. || lat > 90. || lon < -180. || lon > 180.)
    return 0;
  
  uint32 ilat = ::ilat(lat);
  int32 ilon = ::ilon(lon);
  
  return exchange_value(make_available(ilat & 0xfffe0000, ilon & 0xfffe0000, 15),
      ilat, ilon, value);
}


Four_Field_Entry Four_Field_Index::add_segment(
    double lhs_lat, double lhs_lon, double rhs_lat, double rhs_lon, int32 value)
{
  Four_Field_Entry result;
  
  if (lhs_lat < -90. || lhs_lat > 90. || lhs_lon < -180. || lhs_lon > 180.
      || rhs_lat < -90. || rhs_lat > 90. || rhs_lon < -180. || rhs_lon > 180.)
    return result;
  if (fabs(lhs_lat - rhs_lat) > .0065536)
    return result;
  if (fabs(lhs_lon - rhs_lon) > .0065536)
  {
    if (fabs(lhs_lon - rhs_lon) - 360 < -.0065536)
      return result;
  }
  
  uint32 lhs_ilat = ::ilat(lhs_lat);
  int32 lhs_ilon = ::ilon(lhs_lon);
  uint32 rhs_ilat = ::ilat(rhs_lat);
  int32 rhs_ilon = ::ilon(rhs_lon);
  
  result.sw = exchange_value(make_available(lhs_ilat & 0xfffe0000, lhs_ilon & 0xfffe0000, 15),
      lhs_ilat, lhs_ilon, value);
  if ((lhs_ilon & 0xffff0000) != (rhs_ilon & 0xffff0000))
    result.se = exchange_value(make_available(lhs_ilat & 0xfffe0000, rhs_ilon & 0xfffe0000, 15),
        lhs_ilat, rhs_ilon, value);
  if ((lhs_ilat & 0xffff0000) != (rhs_ilat & 0xffff0000))
  {
    result.nw = exchange_value(make_available(rhs_ilat & 0xfffe0000, lhs_ilon & 0xfffe0000, 15),
        rhs_ilat, lhs_ilon, value);
    if ((lhs_ilon & 0xffff0000) != (rhs_ilon & 0xffff0000))
      result.ne = exchange_value(make_available(rhs_ilat & 0xfffe0000, rhs_ilon & 0xfffe0000, 15),
          rhs_ilat, rhs_ilon, value);
  }
  
  return result;
}


Four_Field_Entry& Four_Field_Index::make_available(uint32 lat, int32 lon, int significant_bits)
{
  if (tree.empty())
  {
    base_lat = lat;
    base_lon = lon;
    base_significant_bits = significant_bits;
    
    tree.push_back(Four_Field_Entry());
    return tree.back();
  }
  
  uint32 bitmask = ~(0xffffffffu>>base_significant_bits);
  while ((lat & bitmask) != (base_lat & bitmask) || (lon & bitmask) != (base_lon & bitmask))
  {
    --base_significant_bits;
    
    tree.push_back(tree[0]);
    tree[0] = Four_Field_Entry();
    if (base_lat & (0x80000000u>>base_significant_bits))
    {
      if (base_lon & (0x80000000u>>base_significant_bits))
        tree[0].ne = -(int)tree.size()+1;
      else
        tree[0].nw = -(int)tree.size()+1;
    }
    else
    {
      if (base_lon & (0x80000000u>>base_significant_bits))
        tree[0].se = -(int)tree.size()+1;
      else
        tree[0].sw = -(int)tree.size()+1;
    }
    
    bitmask = ~(0xffffffffu>>base_significant_bits);
    base_lat &= bitmask;
    base_lon &= bitmask;
  }
  
  int cur_bits = base_significant_bits;
  uint cur_pos = 0;
  while (cur_bits < significant_bits)
  {
    if (lat & (0x80000000u>>cur_bits))
    {
      if (lon & (0x80000000u>>cur_bits))
      {
        if (tree[cur_pos].ne < 0)
          cur_pos = -tree[cur_pos].ne;
        else
        {
          tree[cur_pos].ne = -(int)tree.size();
          cur_pos = tree.size();
          tree.push_back(Four_Field_Entry());
        }
      }
      else
      {
        if (tree[cur_pos].nw < 0)
          cur_pos = -tree[cur_pos].nw;
        else
        {
          tree[cur_pos].nw = -(int)tree.size();
          cur_pos = tree.size();
          tree.push_back(Four_Field_Entry());
        }
      }
    }
    else
    {
      if (lon & (0x80000000u>>cur_bits))
      {
        if (tree[cur_pos].se < 0)
          cur_pos = -tree[cur_pos].se;
        else
        {
          tree[cur_pos].se = -(int)tree.size();
          cur_pos = tree.size();
          tree.push_back(Four_Field_Entry());
        }
      }
      else
      {
        if (tree[cur_pos].sw < 0)
          cur_pos = -tree[cur_pos].sw;
        else
        {
          tree[cur_pos].sw = -(int)tree.size();
          cur_pos = tree.size();
          tree.push_back(Four_Field_Entry());
        }
      }
    }
    if (cur_pos == tree.size())
      tree.push_back(Four_Field_Entry());
    
    ++cur_bits;
  }
  
  return tree[cur_pos];
}


namespace
{
  void print_index(std::ostringstream& out, const std::vector< Four_Field_Entry >& tree,
      uint32 base_lat, int32 base_lon, int significant_bits, int32 value)
  {
    out<<" {("<<std::fixed<<std::setprecision(7)<<::lat(base_lat)<<", "
        <<std::fixed<<std::setprecision(7)<<::lon(base_lon)<<", "
        <<std::fixed<<std::setprecision(7)<<::lat(base_lat + (0x80000000u>>significant_bits) - 1)<<", "
        <<std::fixed<<std::setprecision(7)<<::lon(base_lon + (0x80000000u>>significant_bits) - 1)<<"), "
        <<value<<"} ";
  }


  void print_indexes(std::ostringstream& out, const std::vector< Four_Field_Entry >& tree,
      uint pos, uint32 base_lat, int32 base_lon, int significant_bits)
  {
    const Four_Field_Entry& entry = tree[pos];
  
    if (entry.sw < 0)
      print_indexes(out, tree, -entry.sw, base_lat, base_lon, significant_bits+1);
    else if (entry.sw > 0)
      print_index(out, tree, base_lat, base_lon, significant_bits, entry.sw);
  
    if (entry.se < 0)
      print_indexes(out, tree, -entry.se, base_lat, base_lon + (0x80000000u>>significant_bits), significant_bits+1);
    else if (entry.se > 0)
      print_index(out, tree, base_lat, base_lon + (0x80000000u>>significant_bits), significant_bits, entry.se);
  
    if (entry.nw < 0)
      print_indexes(out, tree, -entry.nw, base_lat + (0x80000000u>>significant_bits), base_lon, significant_bits+1);
    else if (entry.nw > 0)
      print_index(out, tree, base_lat + (0x80000000u>>significant_bits), base_lon, significant_bits, entry.nw);
  
    if (entry.ne < 0)
      print_indexes(out, tree, -entry.ne, base_lat + (0x80000000u>>significant_bits),
          base_lon + (0x80000000u>>significant_bits), significant_bits+1);
    else if (entry.ne > 0)
      print_index(out, tree, base_lat + (0x80000000u>>significant_bits),
          base_lon + (0x80000000u>>significant_bits), significant_bits, entry.ne);
  }
}


std::string Four_Field_Index::to_string() const
{
  std::ostringstream out;
  out<<"[";
  
  if (!tree.empty())
    print_indexes(out, tree, 0, base_lat, base_lon, base_significant_bits);
  
  return out.str() + "]";
}