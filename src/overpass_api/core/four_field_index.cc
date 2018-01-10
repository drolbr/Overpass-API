#include "four_field_index.h"
#include "index_computations.h"


#include <cmath>
#include <iomanip>
#include <sstream>


Four_Field_Index::Four_Field_Index(Area_Oracle* area_oracle_)
    : base_lat(0), base_lon(2000000000), base_significant_bits(0), area_oracle(area_oracle_),
    min_lat(::ilat(-90.)), max_lat(::ilat(90.)), min_lon(::ilon(-180.)), max_lon(::ilon(180.)) {}


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


void Four_Field_Index::compute_inside_parts()
{
  if (area_oracle && !tree.empty())
    compute_inside_parts(base_lat, base_lon, base_significant_bits, 0, 0, 0, 0);
}


Area_Oracle::point_status Four_Field_Index::get_point_status(double lat, double lon)
{
  if (!area_oracle || tree.empty())
    return 0;
  
  uint32 ilat = ::ilat(lat);
  int32 ilon = ::ilon(lon);
  
  int cur_bits = base_significant_bits;
  uint cur_pos = 0;
  while (cur_bits < 32)
  {
    if (ilat & (0x80000000u>>cur_bits))
    {
      if (ilon & (0x80000000u>>cur_bits))
      {
        if (tree[cur_pos].ne < 0)
          cur_pos = -tree[cur_pos].ne;
        else if (tree[cur_pos].ne > 0)
          return area_oracle->get_point_status(tree[cur_pos].ne, lat, lon);
        else
          return 0;
      }
      else
      {
        if (tree[cur_pos].nw < 0)
          cur_pos = -tree[cur_pos].nw;
        else if (tree[cur_pos].nw > 0)
          return area_oracle->get_point_status(tree[cur_pos].nw, lat, lon);
        else
          return 0;
      }
    }
    else
    {
      if (ilon & (0x80000000u>>cur_bits))
      {
        if (tree[cur_pos].se < 0)
          cur_pos = -tree[cur_pos].se;
        else if (tree[cur_pos].se > 0)
          return area_oracle->get_point_status(tree[cur_pos].se, lat, lon);
        else
          return 0;
      }
      else
      {
        if (tree[cur_pos].sw < 0)
          cur_pos = -tree[cur_pos].sw;
        else if (tree[cur_pos].sw > 0)
          return area_oracle->get_point_status(tree[cur_pos].sw, lat, lon);
        else
          return 0;
      }
    }
    
    ++cur_bits;
  }
  
  return 0;
}


void Four_Field_Index::compute_inside_parts(uint32 lat, int32 lon, int significant_bits, unsigned int pos,
    bool sw, bool* r_se, bool* r_nw)
{
  if (!significant_bits || min_lon <= lon + int32(0x80000000u>>significant_bits))
  {
    bool se = false;
    bool nw = false;
    bool ne = false;
  
    if (tree[pos].sw < 0)
      compute_inside_parts(lat, lon, significant_bits+1, -tree[pos].sw, sw, &se, &nw);
    else if (tree[pos].sw > 0)
      area_oracle->build_area(sw, tree[pos].sw, &se, &nw);
    else
    {
      tree[pos].sw = sw;
      se = sw;
      nw = sw;
    }
    
    if (tree[pos].se < 0)
      compute_inside_parts(lat, lon + (0x80000000u>>significant_bits), significant_bits+1, -tree[pos].se,
          se, r_se, 0);
    else if (tree[pos].se > 0)
      area_oracle->build_area(se, tree[pos].se, r_se, 0);
    else
    {
      tree[pos].se = se;
      if (r_se)
        *r_se = se;
    }
    
    if (tree[pos].nw < 0)
      compute_inside_parts(lat + (0x80000000u>>significant_bits), lon, significant_bits+1, -tree[pos].nw,
          nw, &ne, r_nw);
    else if (tree[pos].nw > 0)
      area_oracle->build_area(nw, tree[pos].nw, &ne, r_nw);
    else
    {
      tree[pos].nw = nw;
      ne = nw;
      if (r_nw)
        *r_nw = nw;
    }
    
    if (tree[pos].ne < 0)
      compute_inside_parts(lat + (0x80000000u>>significant_bits), lon + (0x80000000u>>significant_bits),
          significant_bits+1, -tree[pos].ne, ne, 0, 0);
    else if (tree[pos].ne > 0)
      area_oracle->build_area(ne, tree[pos].ne, 0, 0);
    else
      tree[pos].ne = ne;
  }
  else
  {
    bool ne = false;
  
    if (tree[pos].se < 0)
      compute_inside_parts(lat, lon + (0x80000000u>>significant_bits), significant_bits+1, -tree[pos].se,
          sw, r_se, &ne);
    else if (tree[pos].se > 0)
      area_oracle->build_area(sw, tree[pos].se, r_se, &ne);
    else
    {
      tree[pos].se = sw;
      if (r_se)
        *r_se = sw;
    }
    
    if (tree[pos].ne < 0)
      compute_inside_parts(lat + (0x80000000u>>significant_bits), lon + (0x80000000u>>significant_bits),
          significant_bits+1, -tree[pos].ne, ne, 0, r_nw);
    else if (tree[pos].ne > 0)
      area_oracle->build_area(ne, tree[pos].ne, 0, r_nw);
    else
    {
      tree[pos].ne = ne;
      if (r_nw)
        *r_nw = ne;
    }
  }
}


namespace
{
  void print_index(std::ostringstream& out, const std::vector< Four_Field_Entry >& tree,
      uint32 base_lat, int32 base_lon, int significant_bits, int32 value,
      uint32 min_lat, uint32 max_lat, int32 min_lon, int32 max_lon)
  {
    if (base_lat < max_lat && base_lon < max_lon
        && base_lat + (0x80000000u>>significant_bits) - 1 > min_lat
        && base_lon + int32((0x80000000u>>significant_bits) - 1) > min_lon)
      out<<" {("<<std::fixed<<std::setprecision(7)<<::lat(std::max(base_lat, min_lat))<<", "
          <<std::fixed<<std::setprecision(7)<<::lon(std::max(base_lon, min_lon))<<", "
          <<std::fixed<<std::setprecision(7)
          <<::lat(std::min(base_lat + (0x80000000u>>significant_bits) - 1, max_lat))<<", "
          <<std::fixed<<std::setprecision(7)
          <<::lon(std::min(base_lon + int32((0x80000000u>>significant_bits) - 1), max_lon))<<"), "
          <<value<<"} ";
  }


  void print_indexes(std::ostringstream& out, const std::vector< Four_Field_Entry >& tree,
      uint pos, uint32 base_lat, int32 base_lon, int significant_bits,
      uint32 min_lat, uint32 max_lat, int32 min_lon, int32 max_lon)
  {
    const Four_Field_Entry& entry = tree[pos];
  
    if (entry.sw < 0)
      print_indexes(out, tree, -entry.sw, base_lat, base_lon, significant_bits+1,
          min_lat, max_lat, min_lon, max_lon);
    else if (entry.sw > 0)
      print_index(out, tree, base_lat, base_lon, significant_bits, entry.sw,
          min_lat, max_lat, min_lon, max_lon);
  
    if (entry.se < 0)
      print_indexes(out, tree, -entry.se, base_lat, base_lon + (0x80000000u>>significant_bits), significant_bits+1,
          min_lat, max_lat, min_lon, max_lon);
    else if (entry.se > 0)
      print_index(out, tree, base_lat, base_lon + (0x80000000u>>significant_bits), significant_bits, entry.se,
          min_lat, max_lat, min_lon, max_lon);
  
    if (entry.nw < 0)
      print_indexes(out, tree, -entry.nw, base_lat + (0x80000000u>>significant_bits), base_lon, significant_bits+1,
          min_lat, max_lat, min_lon, max_lon);
    else if (entry.nw > 0)
      print_index(out, tree, base_lat + (0x80000000u>>significant_bits), base_lon, significant_bits, entry.nw,
          min_lat, max_lat, min_lon, max_lon);
  
    if (entry.ne < 0)
      print_indexes(out, tree, -entry.ne, base_lat + (0x80000000u>>significant_bits),
          base_lon + (0x80000000u>>significant_bits), significant_bits+1,
          min_lat, max_lat, min_lon, max_lon);
    else if (entry.ne > 0)
      print_index(out, tree, base_lat + (0x80000000u>>significant_bits),
          base_lon + (0x80000000u>>significant_bits), significant_bits, entry.ne,
          min_lat, max_lat, min_lon, max_lon);
  }
}


std::string Four_Field_Index::to_string() const
{
  std::ostringstream out;
  out<<"[";
  
  if (!tree.empty())
    print_indexes(out, tree, 0, base_lat, base_lon, base_significant_bits,
        min_lat, max_lat, min_lon, max_lon);
  
  return out.str() + "]";
}