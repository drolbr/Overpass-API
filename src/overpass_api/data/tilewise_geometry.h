/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__DATA__TILEWISE_GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__DATA__TILEWISE_GEOMETRY_H

#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "way_geometry_store.h"


bool is_compound_idx(Uint31_Index idx)
{
  return idx.val() & 0x80000000u;
}


class Great_Circle
{
public:
  Great_Circle(const Point_Double& lhs, const Point_Double& rhs)
  {
//     std::cout<<"gc "<<lhs.lat<<' '<<lhs.lon<<' '<<rhs.lat<<' '<<rhs.lon
//         <<' '<<(lhs.lon < rhs.lon)<<(rhs.lon - lhs.lon > 180.)<<
//         (rhs.lon < lhs.lon)<<(lhs.lon - rhs.lon < 180.)<<'\n';
    
    double lhs_s = sin(lhs.lat/180.*M_PI);
    double lhs_cos = cos(lhs.lat/180.*M_PI);
    double lhs_cs = lhs_cos * sin(lhs.lon/180.*M_PI);
    double lhs_cc = lhs_cos * cos(lhs.lon/180.*M_PI);
    
    double rhs_s = sin(rhs.lat/180.*M_PI);
    double rhs_cos = cos(rhs.lat/180.*M_PI);
    double rhs_cs = rhs_cos * sin(rhs.lon/180.*M_PI);
    double rhs_cc = rhs_cos * cos(rhs.lon/180.*M_PI);
    
    ortho_s = lhs_cs * rhs_cc - lhs_cc * rhs_cs;
    ortho_cs = lhs_cc * rhs_s - lhs_s * rhs_cc;
    ortho_cc = lhs_s * rhs_cs - lhs_cs * rhs_s;
    if ((lhs.lon < rhs.lon && rhs.lon - lhs.lon > 180.)
        || (rhs.lon < lhs.lon && lhs.lon - rhs.lon < 180.))
    {
      ortho_s = -ortho_s;
      ortho_cs = -ortho_cs;
      ortho_cc = -ortho_cc;
    }
    double norm = sqrt(ortho_s*ortho_s + ortho_cs*ortho_cs + ortho_cc*ortho_cc);
    if (norm > 0)
    {
      ortho_s /= norm;
      ortho_cs /= norm;
      ortho_cc /= norm;
    }
//     std::cout<<ortho_s<<' '<<ortho_cs<<' '<<ortho_cc<<'\n'
//         <<(ortho_s*ortho_s + ortho_cs*ortho_cs + ortho_cc*ortho_cc)<<'\n'
//         <<asin(ortho_s)/M_PI*180.<<' '<<asin(ortho_cs/sqrt(1 - ortho_s*ortho_s))/M_PI*180.<<'\n';
  }

  double lat_of(double lon)
  {
    //rotate ortho such that the longitude to use for cartesian computation is always zero
    double g_cc = ortho_cc*cos(lon/180.*M_PI) + ortho_cs*sin(lon/180.*M_PI);
    double norm_prod = sqrt(g_cc*g_cc + ortho_s*ortho_s);
    if (g_cc > norm_prod)
      return 90.;
    return asin(g_cc/norm_prod)/M_PI*180.;
  }

private:
  double ortho_s;
  double ortho_cs;
  double ortho_cc;
};


Point_Double make_point_double(Quad_Coord arg)
{
  return Point_Double(lat(arg.ll_upper, arg.ll_lower), lon(arg.ll_upper, arg.ll_lower)); 
}


void add_idx_sequence(uint32 lat_lhs, uint32 lat_rhs, int32 lon, std::vector< Uint31_Index >& result)
{
  uint32 min_lat = std::min(lat_lhs, lat_rhs);
  uint32 max_lat = std::max(lat_lhs, lat_rhs);
  for (uint32 lat = (min_lat & 0xffff0000) + 0x10000; lat + 0x10000 < max_lat; lat += 0x10000)
    result.push_back(Uint31_Index(ll_upper_(lat, lon)));
}


std::vector< Uint31_Index > touched_indexes(Quad_Coord lhs, Quad_Coord rhs)
{
  uint32 lat_lhs = ilat(lhs.ll_upper, lhs.ll_lower);
  int32 lon_lhs = ilon(lhs.ll_upper, lhs.ll_lower);
  uint32 lat_rhs = ilat(rhs.ll_upper, rhs.ll_lower);
  int32 lon_rhs = ilon(rhs.ll_upper, rhs.ll_lower);

  if (lat_lhs == lat_rhs && std::abs(lon_rhs - lon_lhs) <= 1)
    return std::vector< Uint31_Index >();
  if (lon_lhs == lon_rhs && (lat_rhs - lat_lhs <= 1 || lat_lhs - lat_rhs <= 1))
    return std::vector< Uint31_Index >();
  if ((lat_rhs - lat_lhs <= 1 || lat_lhs - lat_rhs <= 1) && std::abs(lon_rhs - lon_lhs) <= 1)
  {
    int32 lon_boundary = ((lon_lhs < lon_rhs ? lon_rhs : lon_lhs) & 0xffff0000);
    uint32 lat_boundary = ((double)lat_rhs - lat_lhs)*(lon_boundary - lon_lhs)/(lon_rhs - lon_lhs) + lat_lhs;
    return std::vector< Uint31_Index >(1, Uint31_Index(ll_upper_(lat_boundary, lon_boundary)));
  }
  
  std::vector< Uint31_Index > result;
  if ((lon_lhs & 0xffff0000) == (lon_rhs & 0xffff0000))
    add_idx_sequence(lat_lhs, lat_rhs, lon_lhs, result);
  else
  {
    Great_Circle gc(make_point_double(lhs), make_point_double(rhs));
    Quad_Coord min = lon_lhs < lon_rhs ? lhs : rhs;
    Quad_Coord max = lon_lhs < lon_rhs ? rhs : lhs;
    int32 i_ilon = ilon(min.ll_upper, min.ll_lower);
    int32 ilon_max = ilon(max.ll_upper, max.ll_lower);
    if (ilon_max - i_ilon < 1800000000)
    {
      uint32 i_ilat = ilat(gc.lat_of(lon((i_ilon & 0xffff0000) + 0x10000)));
      add_idx_sequence(ilat(min.ll_upper, min.ll_lower), i_ilat, i_ilon, result);
      for (i_ilon = (i_ilon & 0xffff0000) + 0x10000; i_ilon + 0x10000 < ilon_max; i_ilon += 0x10000)
      {
        uint32 last_ilat = i_ilat;
        uint32 i_ilat = ilat(gc.lat_of(lon(i_ilon + 0x10000)));
        add_idx_sequence(last_ilat, i_ilat, i_ilon, result);
      }
      add_idx_sequence(i_ilat, ilat(max.ll_upper, max.ll_lower), ilon_max, result);
    }
    else if (ilon_max - i_ilon == 1800000000)
    {
      uint64 ilat_min = ilat(min.ll_upper, min.ll_lower);
      uint64 ilat_max = ilat(max.ll_upper, max.ll_lower);
      if (ilat_min + ilat_max > 2*910000000)
      {
        add_idx_sequence(ilat_min, 1810000000, i_ilon, result);
        add_idx_sequence(ilat_max, 1810000000, ilon_max, result);
      }
      else
      {
        add_idx_sequence(10000000, ilat_min, i_ilon, result);
        add_idx_sequence(10000000, ilat_max, ilon_max, result);
      }
    }
    else
    {
      //TODO: antimeridian
    }
  }
  
  return result;
}


Uint31_Index touched_index(Quad_Coord lhs, Quad_Coord rhs)
{
  uint32 lat_lhs = ilat(lhs.ll_upper, lhs.ll_lower)>>16;
  int32 lon_lhs = ilon(lhs.ll_upper, lhs.ll_lower)>>16;
  uint32 lat_rhs = ilat(rhs.ll_upper, rhs.ll_lower)>>16;
  int32 lon_rhs = ilon(rhs.ll_upper, rhs.ll_lower)>>16;

  if (lat_lhs == lat_rhs && std::abs(lon_rhs - lon_lhs) <= 1)
    return Uint31_Index(0u);
  if (lon_lhs == lon_rhs && (lat_rhs - lat_lhs <= 1 || lat_lhs - lat_rhs <= 1))
    return Uint31_Index(0u);

  if ((lat_rhs - lat_lhs <= 1 || lat_lhs - lat_rhs <= 1) && std::abs(lon_rhs - lon_lhs) <= 1)
  {
    lat_lhs = ilat(lhs.ll_upper, lhs.ll_lower);
    lon_lhs = ilon(lhs.ll_upper, lhs.ll_lower);
    lat_rhs = ilat(rhs.ll_upper, rhs.ll_lower);
    lon_rhs = ilon(rhs.ll_upper, rhs.ll_lower);
    int32 lon_boundary = (std::max(lon_lhs, lon_rhs) & 0xffff0000);
    uint32 lat_boundary = ((double)lat_rhs - lat_lhs)*(lon_boundary - lon_lhs)/(lon_rhs - lon_lhs) + lat_lhs;
    
    int32 missing_lon = lon_lhs;
    if (lat_lhs < lat_rhs)
      missing_lon = (lat_boundary < (lat_rhs & 0xffff0000) ? lon_rhs : lon_lhs);
    else
      missing_lon = (lat_boundary < (lat_lhs & 0xffff0000) ? lon_lhs : lon_rhs);
    return Uint31_Index(ll_upper_(lat_boundary, missing_lon));
  }
  return Uint31_Index(0xffu);
}


struct Segment
{
  Segment(uint32 lat_lhs, int32 lon_lhs, uint32 lat_rhs, int32 lon_rhs)
  {
    populate(lat_lhs, lon_lhs, lat_rhs, lon_rhs);
  }
  
  Segment(const Quad_Coord& lhs, const Quad_Coord& rhs)
  {
    populate(
        ilat(lhs.ll_upper, lhs.ll_lower), ilon(lhs.ll_upper, lhs.ll_lower),
        ilat(rhs.ll_upper, rhs.ll_lower), ilon(rhs.ll_upper, rhs.ll_lower));
  }
  
  void populate(uint32 ilat_lhs, int32 ilon_lhs, uint32 ilat_rhs, int32 ilon_rhs)
  {
    if (ilon_lhs < ilon_rhs)
    {
      ilat_west = ilat_lhs;
      ilon_west = ilon_lhs;
      ilat_east = ilat_rhs;
      ilon_east = ilon_rhs;
    }
    else
    {
      ilat_west = ilat_rhs;
      ilon_west = ilon_rhs;
      ilat_east = ilat_lhs;
      ilon_east = ilon_lhs;
    }
  }
  
  uint32 ilat_west;
  int32 ilon_west;
  uint32 ilat_east;
  int32 ilon_east;
};


template< typename Segment_Collector >
void make_entries(const Segment_Collector& collector, const std::vector< Quad_Coord >& geom)
{
  if (geom.empty())
    return;

  for (uint i = 1; i < geom.size(); ++i)
  {
    if (geom[i].ll_upper == geom[i-1].ll_upper)
      collector.push(geom[i].ll_upper, Segment(geom[i-1], geom[i]));
    else
    {
      Uint31_Index extra_idx = touched_index(geom[i-1], geom[i]);
//         std::cout<<std::dec<<ilat(geom[i-1].ll_upper, geom[i-1].ll_lower)<<' '<<ilon(geom[i-1].ll_upper, geom[i-1].ll_lower)<<' '
//             <<ilat(geom[i].ll_upper, geom[i].ll_lower)<<' '<<ilon(geom[i].ll_upper, geom[i].ll_lower)<<' '
//             <<std::hex<<extra_idx.val()<<'\n';
      if (extra_idx.val() == 0xff)
        calculate_auxiliary_points(collector, geom[i-1], geom[i]);
      else
      {
        collector.push(geom[i-1].ll_upper, Segment(geom[i-1], geom[i]));
        collector.push(geom[i].ll_upper, Segment(geom[i-1], geom[i]));
        if (!(extra_idx.val() == 0))
          collector.push(extra_idx, Segment(geom[i-1], geom[i]));
      }
    }
  }
}


template< typename Segment_Collector >
void calculate_auxiliary_points(const Segment_Collector& collector, Quad_Coord lhs, Quad_Coord rhs)
{
  uint32 lat_lhs = ilat(lhs.ll_upper, lhs.ll_lower);
  int32 lon_lhs = ilon(lhs.ll_upper, lhs.ll_lower);
  uint32 lat_rhs = ilat(rhs.ll_upper, rhs.ll_lower);
  int32 lon_rhs = ilon(rhs.ll_upper, rhs.ll_lower);

  if ((lon_lhs & 0xffff0000) == (lon_rhs & 0xffff0000))
    calculate_south_north_sequence(collector, lat_lhs, lon_lhs, lat_rhs, lon_rhs);
  else
  {
    Great_Circle gc(make_point_double(lhs), make_point_double(rhs));
    Quad_Coord min = lon_lhs < lon_rhs ? lhs : rhs;
    Quad_Coord max = lon_lhs < lon_rhs ? rhs : lhs;
    int32 i_ilon = ilon(min.ll_upper, min.ll_lower);
    int32 ilon_max = ilon(max.ll_upper, max.ll_lower);
    if ((int64)ilon_max - i_ilon < 1800000000)
    {
//         std::cout<<"aux_gc "<<ilat(min.ll_upper, min.ll_lower)<<' '<<ilon(min.ll_upper, min.ll_lower)
//             <<' '<<ilat(max.ll_upper, max.ll_lower)<<' '<<ilon(max.ll_upper, max.ll_lower)<<'\n';
      uint32 i_ilat = ilat(gc.lat_of(lon((i_ilon & 0xffff0000) + 0x10000)));
      calculate_south_north_sequence(
          collector, ilat(min.ll_upper, min.ll_lower), i_ilon,
          i_ilat, (i_ilon & 0xffff0000) + 0x10000);
      for (i_ilon = (i_ilon & 0xffff0000) + 0x10000; i_ilon + 0x10000 < ilon_max;
          i_ilon += 0x10000)
      {
        uint32 last_ilat = i_ilat;
        i_ilat = ilat(gc.lat_of(lon(i_ilon + 0x10000)));
        calculate_south_north_sequence(
            collector, last_ilat, i_ilon, i_ilat, i_ilon + 0x10000);
      }
      
      calculate_south_north_sequence(
          collector, i_ilat, i_ilon, ilat(max.ll_upper, max.ll_lower), ilon_max);
    }
    else if (ilon_max - i_ilon == 1800000000)
    {
//         std::cout<<"aux_180 "<<ilat(min.ll_upper, min.ll_lower)<<' '<<ilon(min.ll_upper, min.ll_lower)
//             <<' '<<ilat(max.ll_upper, max.ll_lower)<<' '<<ilon(max.ll_upper, max.ll_lower)<<'\n';
      uint64 ilat_min = ilat(min.ll_upper, min.ll_lower);
      uint64 ilat_max = ilat(max.ll_upper, max.ll_lower);
      if (ilat_min + ilat_max > 2*910000000)
      {
        calculate_south_north_sequence(collector, ilat_min, i_ilon, 1810000000, i_ilon);
        calculate_south_north_sequence(collector, ilat_max, ilon_max, 1810000000, ilon_max);
      }
      else
      {
        calculate_south_north_sequence(collector, 10000000, i_ilon, ilat_min, i_ilon);
        calculate_south_north_sequence(collector, 10000000, ilon_max, ilat_max, ilon_max);
      }
    }
    else
    {
//         std::cout<<"aux_anti "<<ilat(min.ll_upper, min.ll_lower)<<' '<<ilon(min.ll_upper, min.ll_lower)
//             <<' '<<ilat(max.ll_upper, max.ll_lower)<<' '<<ilon(max.ll_upper, max.ll_lower)<<'\n';
      if ((i_ilon & 0xffff0000) == (-1800000000 & 0xffff0000))
        calculate_south_north_sequence(
            collector, ilat(gc.lat_of(-180.)), -1800000000, ilat(min.ll_upper, min.ll_lower), i_ilon);
      else
      {
        uint32 i_ilat = ilat(gc.lat_of(-180.));
        int32 ilon_max = i_ilon;
        i_ilon = -1800000000;
        calculate_south_north_sequence(
            collector, ilat(min.ll_upper, min.ll_lower), i_ilon,
            i_ilat, (i_ilon & 0xffff0000) + 0x10000);
        for (i_ilon = (i_ilon & 0xffff0000) + 0x10000; i_ilon + 0x10000 < ilon_max;
            i_ilon += 0x10000)
        {
          uint32 last_ilat = i_ilat;
          i_ilat = ilat(gc.lat_of(lon(i_ilon + 0x10000)));
          calculate_south_north_sequence(
              collector, last_ilat, i_ilon, i_ilat, i_ilon + 0x10000);
        }
        
        calculate_south_north_sequence(
            collector, i_ilat, i_ilon, ilat(min.ll_upper, min.ll_lower), ilon_max);
      }
      
      if ((ilon_max & 0xffff0000) == (1800000000 & 0xffff0000))
        calculate_south_north_sequence(
            collector, ilat(max.ll_upper, max.ll_lower), ilon_max, ilat(gc.lat_of(180.)), 1800000000);
      else
      {
        i_ilon = ilon_max;
        uint32 i_ilat = ilat(gc.lat_of(lon((i_ilon & 0xffff0000) + 0x10000)));
        calculate_south_north_sequence(
            collector, ilat(max.ll_upper, max.ll_lower), i_ilon,
            i_ilat, (i_ilon & 0xffff0000) + 0x10000);
        for (i_ilon = (i_ilon & 0xffff0000) + 0x10000; i_ilon + 0x10000 < 1800000000;
            i_ilon += 0x10000)
        {
          uint32 last_ilat = i_ilat;
          i_ilat = ilat(gc.lat_of(lon(i_ilon + 0x10000)));
          calculate_south_north_sequence(
              collector, last_ilat, i_ilon, i_ilat, i_ilon + 0x10000);
        }
        
        calculate_south_north_sequence(
            collector, i_ilat, i_ilon, ilat(gc.lat_of(180.)), 1800000000);
      }
    }
  }
}


template< typename Segment_Collector >
void calculate_south_north_sequence(
    const Segment_Collector& collector, uint32 lat_lhs, int32 lon_lhs, uint32 lat_rhs, int32 lon_rhs)
{
//     std::cout<<"sn "<<lat_lhs<<' '<<lon_lhs<<' '<<lat_rhs<<' '<<lon_rhs<<'\n';
  if ((lat_lhs & 0xffff0000) == (lat_rhs & 0xffff0000))
  {
    collector.push(Uint31_Index(ll_upper_(lat_lhs, lon_lhs)), Segment(lat_lhs, lon_lhs, lat_rhs, lon_rhs));
    return;
  }

  if (lat_rhs < lat_lhs)
  {
    std::swap(lat_lhs, lat_rhs);
    std::swap(lon_lhs, lon_rhs);
  }
  int32 min_lon = std::min(lon_lhs, lon_rhs);

  uint32 i_lat = (lat_lhs & 0xffff0000) + 0x10000;
  int32 i_lon = lon_lhs + ((double)i_lat - lat_lhs)/((int32)lat_rhs - (int32)lat_lhs)*(lon_rhs - lon_lhs);
//     std::cout<<"DEBUG_B "<<std::hex<<ll_upper_(lat_lhs, min_lon)<<' '
//         <<std::dec<<lat_lhs<<' '<<lon_lhs<<' '<<i_lat<<' '<<i_lon<<'\n';
  collector.push(Uint31_Index(ll_upper_(lat_lhs, min_lon)), Segment(lat_lhs, lon_lhs, i_lat, i_lon));

  while (i_lat + 0x10000 < lat_rhs)
  {
    uint32 last_ilat = i_lat;
    int32 last_ilon = i_lon;
    i_lat += 0x10000;
    i_lon = lon_lhs + ((double)i_lat - lat_lhs)/((int32)lat_rhs - (int32)lat_lhs)*(lon_rhs - lon_lhs);
//       std::cout<<"DEBUG_D "<<std::hex<<ll_upper_(last_ilat, min_lon)<<' '
//           <<std::dec<<last_ilat<<' '<<last_ilon<<' '<<i_lat<<' '<<i_lon<<'\n';
    collector.push(Uint31_Index(ll_upper_(last_ilat, min_lon)), Segment(last_ilat, last_ilon, i_lat, i_lon));
  }
  
//     std::cout<<"DEBUG_C "<<std::hex<<ll_upper_(i_lat, min_lon)<<' '
//         <<std::dec<<i_lat<<' '<<i_lon<<' '<<lat_rhs<<' '<<lon_rhs<<'\n';
  collector.push(Uint31_Index(ll_upper_(i_lat, min_lon)), Segment(i_lat, i_lon, lat_rhs, lon_rhs));
}


// fwd iterator only
class Tilewise_Area_Iterator
{
public:
  enum Relative_Position { outside, inside, border };
  
  struct Index_Block
  {
    Index_Block() : sw_is_inside(false) {}
    
    std::vector< Segment > segments;
    bool sw_is_inside;
  };

  // non-const, but relies to pointers into each vec.
  // That way, objects can be moved from or marked as done.
  Tilewise_Area_Iterator(
      const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways_,
      const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways_,
      const Statement& stmt, Resource_Manager& rman)
  : ways(&ways_), attic_ways(&attic_ways_), cur_it(ways->begin()), attic_it(attic_ways->begin()),
    cur_geom_store(*ways, stmt, rman), attic_geom_store(*attic_ways, stmt, rman)
  {
    refill();
  }
  
  void next()
  {
    propagate_inside_flag();
    queue.erase(queue.begin());
    if (queue.empty())
      refill();
  }
  bool is_end() const { return queue.empty(); }
  
  const std::map< const Way_Skeleton*, Index_Block >& get_obj() const { return queue.begin()->second; }
  Uint31_Index get_idx() const { return queue.begin()->first; }
  
  Relative_Position rel_position(uint32 ll_upper, uint32 ll_lower, bool accept_border)
  {
    return rel_pos_ilat_ilon(ilat(ll_upper, ll_lower), ilon(ll_upper, ll_lower), accept_border);
  }

  Relative_Position rel_position(const std::vector< Segment >& segments, bool accept_border)
  {
    for (std::vector< Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    {
      Relative_Position status = rel_pos_ilat_ilon(it->ilat_west, it->ilon_west, false);
      if (status == inside)
        return status;
      status = rel_pos_ilat_ilon(it->ilat_east, it->ilon_east, false);
      if (status == inside)
        return status;

      const std::map< const Way_Skeleton*, Index_Block >& way_blocks = get_obj();
      for (std::map< const Way_Skeleton*, Index_Block >::const_iterator bit = way_blocks.begin();
          bit != way_blocks.end(); ++bit)
      {
        const Index_Block& block = bit->second;
        std::vector< std::pair< uint32, int32 > > touching_coords;

        for (std::vector< Segment >::const_iterator ait = block.segments.begin(); ait != block.segments.end(); ++ait)
        {
          if (ait->ilon_west <= it->ilon_east && it->ilon_west <= ait->ilon_east)
          {
            // Compute the scalar products of both endpoints of one segment with the other segment's direction
            // If they have different signs then we know that the intersection of both lines is within the segment
            double scal_ait_west = ((double)ait->ilat_west - it->ilat_west)*(it->ilon_east - it->ilon_west)
                - (ait->ilon_west - it->ilon_west)*((double)it->ilat_east - it->ilat_west);
            double scal_ait_east = ((double)ait->ilat_east - it->ilat_west)*(it->ilon_east - it->ilon_west)
                - (ait->ilon_east - it->ilon_west)*((double)it->ilat_east - it->ilat_west);
            if (scal_ait_east * scal_ait_west > 0)
              continue;

            double scal_it_west = ((double)it->ilat_west - ait->ilat_west)*(ait->ilon_east - ait->ilon_west)
                - (it->ilon_west - ait->ilon_west)*((double)ait->ilat_east - ait->ilat_west);
            double scal_it_east = ((double)it->ilat_east - ait->ilat_west)*(ait->ilon_east - ait->ilon_west)
                - (it->ilon_east - ait->ilon_west)*((double)ait->ilat_east - ait->ilat_west);
            if (scal_it_east * scal_it_west > 0)
              continue;
            
//             std::cout<<"DEBUG "<<bit->first->id.val()
//                 <<"  "<<it->ilat_west<<' '<<it->ilon_west<<"  "<<it->ilat_east<<' '<<it->ilon_east
//                 <<"  "<<ait->ilat_west<<' '<<ait->ilon_west<<"  "<<ait->ilat_east<<' '<<ait->ilon_east
//                 <<"  "<<scal_ait_west<<' '<<scal_ait_east<<' '<<scal_it_west<<' '<<scal_it_east<<'\n';
//             std::cout<<"DEBUG "<<((double)ait->ilat_east - it->ilat_west)<<' '<<(it->ilon_west - it->ilon_east)
//                 <<' '<<(ait->ilon_east - it->ilon_west)<<' '<<((double)it->ilat_east - it->ilat_west)<<'\n';

            if (scal_ait_west == 0)
            {
              touching_coords.push_back(std::make_pair(ait->ilat_west, ait->ilon_west));
              if (scal_ait_east == 0)
                touching_coords.push_back(std::make_pair(ait->ilat_east, ait->ilon_east));
            }
            else if (scal_ait_east == 0)
              touching_coords.push_back(std::make_pair(ait->ilat_east, ait->ilon_east));
            else if (scal_it_west == 0)
              touching_coords.push_back(std::make_pair(it->ilat_west, it->ilon_west));
            else if (scal_it_east != 0)
              return inside;
          }
        }
        
        if (!touching_coords.empty())
        {
          touching_coords.push_back(std::make_pair(it->ilat_west, it->ilon_west));
          touching_coords.push_back(std::make_pair(it->ilat_east, it->ilon_east));
          std::sort(touching_coords.begin(), touching_coords.end());
          touching_coords.erase(std::unique(touching_coords.begin(), touching_coords.end()), touching_coords.end());
          
          for (std::vector< std::pair< uint32, int32 > >::size_type i = 1; i < touching_coords.size(); ++i)
          {
            Relative_Position status = rel_pos_ilat_ilon(
                ((uint64)touching_coords[i-1].first + touching_coords[i].first)/2,
                ((int64)touching_coords[i-1].second + touching_coords[i].second)/2, false);
            if (status == inside)
              return inside;
          }
        }
      }
    }
    return outside;
  }

private:
  const std::map< Uint31_Index, std::vector< Way_Skeleton > >* ways;
  const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >* attic_ways;
  std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator cur_it;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_it;
  Way_Geometry_Store cur_geom_store;
  Way_Geometry_Store attic_geom_store;
  std::map< Uint31_Index, std::map< const Way_Skeleton*, Index_Block > > queue;

  struct Segment_Collector
  {
    Segment_Collector(
        std::map< Uint31_Index, std::map< const Way_Skeleton*, Index_Block > >& queue_, const Way_Skeleton& skel_)
        : queue(&queue_), skel(&skel_) {}
    void push(Uint31_Index idx, const Segment& segment) const
    { (*queue)[idx][skel].segments.push_back(segment); }
    
  private:
    std::map< Uint31_Index, std::map< const Way_Skeleton*, Index_Block > >* queue;
    const Way_Skeleton* skel;
  };

  void refill()
  {
    while (cur_it != ways->end() || attic_it != attic_ways->end())
    {
      Uint31_Index idx = (cur_it != ways->end() && (attic_it == attic_ways->end() || cur_it->first < attic_it->first)
          ? cur_it->first : attic_it->first);
      if (cur_it != ways->end() && cur_it->first == idx)
      {
        for (std::vector< Way_Skeleton >::const_iterator it = cur_it->second.begin(); it != cur_it->second.end(); ++it)
        {
          if (!it->nds.empty() && it->nds.front() == it->nds.back())
            make_entries(Segment_Collector(queue, *it), cur_geom_store.get_geometry(*it));
        }
        ++cur_it;
      }
      if (attic_it != attic_ways->end() && attic_it->first == idx)
      {
        for (std::vector< Attic< Way_Skeleton > >::const_iterator it = attic_it->second.begin();
            it != attic_it->second.end(); ++it)
        {
          if (!it->nds.empty() && it->nds.front() == it->nds.back())
            make_entries(Segment_Collector(queue, *it), attic_geom_store.get_geometry(*it));
        }
        ++attic_it;
      }

      if (!is_compound_idx(idx) && !queue.empty())
        break;
    }
  }
  
  void propagate_inside_flag()
  {
    Uint31_Index idx = get_idx();
    uint32 south = ilat(idx.val(), 0u);
    int32 west = ilon(idx.val(), 0u);
//     std::cout<<"propagate_inside_flag "<<south<<' '<<west<<'\n';
    const std::map< const Way_Skeleton*, Index_Block >& way_blocks = get_obj();
    
    for (std::map< const Way_Skeleton*, Index_Block >::const_iterator bit = way_blocks.begin();
        bit != way_blocks.end(); ++bit)
    {
      const Index_Block& block = bit->second;
      bool is_inside = block.sw_is_inside;
      
      for (std::vector< Segment >::const_iterator it = block.segments.begin(); it != block.segments.end(); ++it)
      {
//         std::cout<<"DEBUG "<<std::hex<<idx.val()<<' '
//             <<std::dec<<' '<<it->ilat_west<<' '<<it->ilon_west<<' '<<it->ilat_east<<' '<<it->ilon_east<<'\n';
        if ((it->ilat_west <= south) ^ (it->ilat_east <= south))
        {
          double isect_lon = it->ilon_west +
              ((double)south - it->ilat_west)
              *(it->ilon_east - it->ilon_west)/((int32)it->ilat_east - (int32)it->ilat_west);
//           std::cout<<"isect_lon "<<isect_lon<<'\n';
          if (west <= isect_lon && isect_lon < west + 0x10000)
            is_inside = !is_inside;
        }
      }
      
//       std::cout<<"propagate_inside_flag "<<south<<' '<<west<<' '
//           <<ilat(idx.val(), 0u)<<' '<<ilon(idx.val(), 0u)+0x10000<<' '<<is_inside<<'\n';
      if (is_inside)
        queue[Uint31_Index(ll_upper_(ilat(idx.val(), 0u), ilon(idx.val(), 0u)+0x10000))][bit->first].sw_is_inside = is_inside;
      
      if (west < -1800000000)
      {
        is_inside = block.sw_is_inside;
        
        for (std::vector< Segment >::const_iterator it = block.segments.begin(); it != block.segments.end(); ++it)
        {
          if (it->ilon_west == -1800000000 && it->ilon_east != -1800000000)
            is_inside = !is_inside;
        }
        
        if (is_inside)
        {
          //std::cout<<"is_inside "<<(ilat(idx.val(), 0u)+0x10000)<<' '<<ilon(idx.val(), 0u)<<'\n';
          queue[Uint31_Index(ll_upper_(ilat(idx.val(), 0u)+0x10000, ilon(idx.val(), 0u)))][bit->first].sw_is_inside = is_inside;
        }
      }
    }
  }

  Relative_Position rel_pos_ilat_ilon(uint32 lat_p, int32 lon_p, bool accept_border)
  {
    Uint31_Index idx = get_idx();
    uint32 south = ilat(idx.val(), 0u);
    int32 west = ilon(idx.val(), 0u);
    const std::map< const Way_Skeleton*, Index_Block >& way_blocks = get_obj();
    bool total_is_inside = false;
    
    for (std::map< const Way_Skeleton*, Index_Block >::const_iterator bit = way_blocks.begin();
        bit != way_blocks.end(); ++bit)
    {
      const Index_Block& block = bit->second;
      bool is_inside = block.sw_is_inside;

      for (std::vector< Segment >::const_iterator it = block.segments.begin(); it != block.segments.end(); ++it)
      {
        if (it->ilat_west > south && it->ilat_east > south)
        {
          // northern segment not touching the southern boundary
          if (it->ilon_west == lon_p)
          {
            if (it->ilat_west == lat_p)
              return border;
            if (it->ilon_west == it->ilon_east)
            {
              if (std::min(it->ilat_west, it->ilat_east) <= lat_p && lat_p <= std::max(it->ilat_west, it->ilat_east))
                return border;
            }
            else
              is_inside ^= (it->ilat_west < lat_p);
          }
          else if (it->ilon_east == lon_p)
          {
            if (it->ilat_east == lat_p)
              return border;
          }
          else if (it->ilon_west < lon_p && lon_p < it->ilon_east)
          {
            double isect_lat = it->ilat_west +
              ((double)lon_p - it->ilon_west)
              *((int32)it->ilat_east - (int32)it->ilat_west)/(it->ilon_east - it->ilon_west);
            if (isect_lat - .5 < lat_p)
            {
              if (lat_p < isect_lat + .5)
                return border;
              is_inside = !is_inside;
            }
          }
        }
        else if ((it->ilat_west <= south) ^ (it->ilat_east <= south))
        {
          double isect_lon = it->ilon_west +
              ((double)south - it->ilat_west)
              *(it->ilon_east - it->ilon_west)/((int32)it->ilat_east - (int32)it->ilat_west);
          if (west <= isect_lon && isect_lon < west + 0x10000)
          {
            if (it->ilat_west <= south)
            {
              if (it->ilon_east <= lon_p)
              {
                if (it->ilon_east == lon_p)
                {
                  if (it->ilat_east == lat_p)
                    return border;
                  if (it->ilon_west == it->ilon_east && lat_p <= it->ilat_east)
                    return border;
                }
                is_inside = !is_inside;
              }
              else if (isect_lon < lon_p)
              {
                double isect_lat = it->ilat_west +
                    ((double)lon_p - it->ilon_west)
                    *((int32)it->ilat_east - (int32)it->ilat_west)/(it->ilon_east - it->ilon_west);
                if (lat_p < isect_lat + .5)
                {
                  if (isect_lat - .5 < lat_p)
                    return border;
                  is_inside = !is_inside;
                }
              }
            }
            else
            {
              if (isect_lon <= lon_p)
              {
                if (isect_lon == lon_p)
                {
                  if (lat_p == south)
                    return border;
                  if (isect_lon == it->ilon_west && lat_p <= it->ilat_west)
                    return border;
                }
                is_inside = !is_inside;
              }
              else if (it->ilon_west <= lon_p)
              {
                double isect_lat = it->ilat_west +
                    ((double)lon_p - it->ilon_west)
                    *((int32)it->ilat_east - (int32)it->ilat_west)/(it->ilon_east - it->ilon_west);
                if (isect_lat - .5 < lat_p)
                {
                  if (lat_p < isect_lat + .5)
                    return border;
                  is_inside = !is_inside;
                }
              }
            }
          }
          else
          {
            // northern segment similar to above
            if (it->ilon_west == lon_p)
            {
              if (it->ilat_west == lat_p)
                return border;
              // Here it->ilon_west == lon_p implies south < it->ilat_west
              is_inside ^= (it->ilat_west < lat_p);
            }
            else if (it->ilon_east == lon_p)
            {
              if (it->ilat_east == lat_p)
                return border;
            }
            else if (it->ilon_west < lon_p && lon_p < it->ilon_east)
            {
              double isect_lat = it->ilat_west +
                ((double)lon_p - it->ilon_west)
                *((int32)it->ilat_east - (int32)it->ilat_west)/(it->ilon_east - it->ilon_west);
              if (isect_lat - .5 < lat_p)
              {
                if (lat_p < isect_lat + .5)
                  return border;
                is_inside = !is_inside;
              }
            }
          }
        }
        else if (it->ilat_west == south && it->ilat_east == south)
        {
          // segment on the southern boundary
          if (lat_p == south)
          {
            if (it->ilon_west <= lon_p && lon_p <= it->ilon_east)
              return border;
          }

          if (it->ilon_west == -1800000000)
            is_inside = !is_inside;
        }
//         std::cout<<"DEBUG "<<lat_p<<' '<<lon_p<<' '<<it->ilat_west<<' '<<it->ilon_west<<' '<<it->ilat_east<<' '<<it->ilon_east<<' '<<is_inside<<'\n';
      }
      total_is_inside |= is_inside;
    }
    
    return total_is_inside ? inside : outside;
  }
};


// fwd iterator only
class Tilewise_Way_Iterator
{
public:
  struct Index_Block
  {
    std::vector< Segment > segments;
  };
  
  template< typename Way_Skeleton >
  struct Status_Ref
  {
    Status_Ref(Uint31_Index idx_, const Way_Skeleton& skel_) : idx(idx_), skel(&skel_),
        status(Tilewise_Area_Iterator::outside) {}

    Uint31_Index idx;
    const Way_Skeleton* skel;
    Tilewise_Area_Iterator::Relative_Position status;
  };

  // non-const, but relies to pointers into each vec.
  // That way, objects can be moved from or marked as done.
  Tilewise_Way_Iterator(
      const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways_,
      const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& attic_ways_,
      const Statement& stmt, Resource_Manager& rman)
  : ways(&ways_), attic_ways(&attic_ways_), cur_it(ways->begin()), attic_it(attic_ways->begin()),
   cur_geom_store(*ways, stmt, rman), attic_geom_store(*attic_ways, stmt, rman)
  {
    refill();
  }
  
  void next()
  {
    queue.erase(queue.begin());
    if (queue.empty())
      refill();
  }
  bool is_end() const { return queue.empty(); }
  
  const std::map< Status_Ref< Way_Skeleton >*, Index_Block >& get_current_obj() const
  { return queue.begin()->second.first; }
  const std::map< Status_Ref< Attic< Way_Skeleton > >*, Index_Block >& get_attic_obj() const
  { return queue.begin()->second.second; }
  Uint31_Index get_idx() const { return queue.begin()->first; }

private:
  const std::map< Uint31_Index, std::vector< Way_Skeleton > >* ways;
  const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >* attic_ways;
  std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator cur_it;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_it;
  std::map< Uint31_Index, std::vector< Status_Ref< Way_Skeleton > > > current_refs;
  std::map< Uint31_Index, std::vector< Status_Ref< Attic< Way_Skeleton > > > > attic_refs;
  Way_Geometry_Store cur_geom_store;
  Way_Geometry_Store attic_geom_store;
  std::map< Uint31_Index, std::pair< std::map< Status_Ref< Way_Skeleton >*, Index_Block >,
      std::map< Status_Ref< Attic< Way_Skeleton > >*, Index_Block > > > queue;

  struct Current_Segment_Collector
  {
    Current_Segment_Collector(
        std::map< Uint31_Index, std::pair< std::map< Status_Ref< Way_Skeleton >*, Index_Block >,
            std::map< Status_Ref< Attic< Way_Skeleton > >*, Index_Block > > >& queue_,
        Status_Ref< Way_Skeleton >& skel_)
        : queue(&queue_), skel(&skel_) {}
    void push(Uint31_Index idx, const Segment& segment) const
    { (*queue)[idx].first[skel].segments.push_back(segment); }
    
  private:
    std::map< Uint31_Index, std::pair< std::map< Status_Ref< Way_Skeleton >*, Index_Block >,
        std::map< Status_Ref< Attic< Way_Skeleton > >*, Index_Block > > >* queue;
    Status_Ref< Way_Skeleton >* skel;
  };

  struct Attic_Segment_Collector
  {
    Attic_Segment_Collector(
        std::map< Uint31_Index, std::pair< std::map< Status_Ref< Way_Skeleton >*, Index_Block >,
            std::map< Status_Ref< Attic< Way_Skeleton > >*, Index_Block > > >& queue_,
        Status_Ref< Attic< Way_Skeleton > >& skel_)
        : queue(&queue_), skel(&skel_) {}
    void push(Uint31_Index idx, const Segment& segment) const
    { (*queue)[idx].second[skel].segments.push_back(segment); }
    
  private:
    std::map< Uint31_Index, std::pair< std::map< Status_Ref< Way_Skeleton >*, Index_Block >,
      std::map< Status_Ref< Attic< Way_Skeleton > >*, Index_Block > > >* queue;
    Status_Ref< Attic< Way_Skeleton > >* skel;
  };

  void refill()
  {
    while (cur_it != ways->end() || attic_it != attic_ways->end())
    {
      Uint31_Index idx = (cur_it != ways->end() && (attic_it == attic_ways->end() || cur_it->first < attic_it->first)
          ? cur_it->first : attic_it->first);
      if (cur_it != ways->end() && cur_it->first == idx)
      {
        std::vector< Status_Ref< Way_Skeleton > >& refs = current_refs[idx];
        for (std::vector< Way_Skeleton >::const_iterator it = cur_it->second.begin(); it != cur_it->second.end(); ++it)
          refs.push_back(Status_Ref< Way_Skeleton >(idx, *it));
        for (std::vector< Status_Ref< Way_Skeleton > >::iterator it = refs.begin(); it != refs.end(); ++it)
          make_entries(Current_Segment_Collector(queue, *it), cur_geom_store.get_geometry(*it->skel));
        ++cur_it;
      }
      if (attic_it != attic_ways->end() && attic_it->first == idx)
      {
        std::vector< Status_Ref< Attic< Way_Skeleton > > >& refs = attic_refs[idx];
        for (std::vector< Attic< Way_Skeleton > >::const_iterator it = attic_it->second.begin();
            it != attic_it->second.end(); ++it)
          refs.push_back(Status_Ref< Attic< Way_Skeleton > >(idx, *it));
        for (std::vector< Status_Ref< Attic< Way_Skeleton > > >::iterator it = refs.begin(); it != refs.end(); ++it)
          make_entries(Attic_Segment_Collector(queue, *it), attic_geom_store.get_geometry(*it->skel));
        ++attic_it;
      }

      if (!is_compound_idx(idx) && !queue.empty())
        break;
    }
  }
};


#endif
