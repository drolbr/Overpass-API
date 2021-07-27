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
      : fake_lat((lhs.lat + rhs.lat)/2.) {}
  double lat_of(double lon)
  {
    return fake_lat;
  }

private:
  double fake_lat;
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
  
  return result;
}


Uint31_Index touched_index(Quad_Coord lhs, Quad_Coord rhs)
{
  uint32 lat_lhs = ilat(lhs.ll_upper, lhs.ll_lower);
  int32 lon_lhs = ilon(lhs.ll_upper, lhs.ll_lower);
  uint32 lat_rhs = ilat(rhs.ll_upper, rhs.ll_lower);
  int32 lon_rhs = ilon(rhs.ll_upper, rhs.ll_lower);

  if (lat_lhs == lat_rhs && std::abs(lon_rhs - lon_lhs) <= 1)
    return Uint31_Index(0u);
  if (lon_lhs == lon_rhs && (lat_rhs - lat_lhs <= 1 || lat_lhs - lat_rhs <= 1))
    return Uint31_Index(0u);

  if ((lat_rhs - lat_lhs <= 1 || lat_lhs - lat_rhs <= 1) && std::abs(lon_rhs - lon_lhs) <= 1)
  {
    int32 lon_boundary = ((lon_lhs < lon_rhs ? lon_rhs : lon_lhs) & 0xffff0000);
    uint32 lat_boundary = ((double)lat_rhs - lat_lhs)*(lon_boundary - lon_lhs)/(lon_rhs - lon_lhs) + lat_lhs;
    return Uint31_Index(ll_upper_(lat_boundary, lon_boundary));
  }
  return Uint31_Index(0xffu);
}


// fwd iterator only
class Tilewise_Area_Iterator
{
public:
  enum Relative_Position { outside, inside, border };
  
  struct Entry
  {
    Entry(uint32 lat_lhs, int32 lon_lhs, uint32 lat_rhs, int32 lon_rhs)
    {
      populate(lat_lhs, lon_lhs, lat_rhs, lon_rhs);
    }
    
    Entry(const Quad_Coord& lhs, const Quad_Coord& rhs)
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
//   struct Entry
//   {
//     Entry(const Way_Skeleton* ref_, uint i_begin_, uint i_end_)
//         : ref(ref_), i_begin(i_begin_), i_end(i_end_) {}
//     
//     const Way_Skeleton* ref;
//     uint i_begin;
//     uint i_end;
//   };
  
  struct Index_Block
  {
    Index_Block() : sw_is_inside(false) {}
    
    std::vector< Entry > segments;
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
  
  Relative_Position rel_position(uint32 ll_upper, uint32 ll_lower)
  {
    Uint31_Index idx = get_idx();
    uint32 south = ilat(idx.val(), 0u);
    int32 west = ilon(idx.val(), 0u);
    const std::map< const Way_Skeleton*, Index_Block >& way_blocks = get_obj();

    uint32 lat_p = ilat(ll_upper, ll_lower);
    int32 lon_p = ilon(ll_upper, ll_lower);
    bool total_is_inside = false;
    
    for (std::map< const Way_Skeleton*, Index_Block >::const_iterator bit = way_blocks.begin();
        bit != way_blocks.end(); ++bit)
    {
      const Index_Block& block = bit->second;
      bool is_inside = block.sw_is_inside;

      for (std::vector< Entry >::const_iterator it = block.segments.begin(); it != block.segments.end(); ++it)
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
        }
      }
      total_is_inside |= is_inside;
    }
    
    return total_is_inside ? inside : outside;
  }
  
private:
  const std::map< Uint31_Index, std::vector< Way_Skeleton > >* ways;
  const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >* attic_ways;
  std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator cur_it;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_it;
  Way_Geometry_Store cur_geom_store;
  Way_Geometry_Store attic_geom_store;
  std::map< Uint31_Index, std::map< const Way_Skeleton*, Index_Block > > queue;
  
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
            make_entries(*it, cur_geom_store.get_geometry(*it));
        }
        ++cur_it;
      }
      if (attic_it != attic_ways->end() && attic_it->first == idx)
      {
        for (std::vector< Attic< Way_Skeleton > >::const_iterator it = attic_it->second.begin();
            it != attic_it->second.end(); ++it)
        {
          if (!it->nds.empty() && it->nds.front() == it->nds.back())
            make_entries(*it, attic_geom_store.get_geometry(*it));
        }
        ++attic_it;
      }

      if (!is_compound_idx(idx) && !queue.empty())
        break;
    }
  }
  
  void make_entries(const Way_Skeleton& skel, const std::vector< Quad_Coord >& geom)
  {
    if (geom.empty())
      return;

    for (uint i = 1; i < geom.size(); ++i)
    {
      if (geom[i].ll_upper == geom[i-1].ll_upper)
        queue[geom[i].ll_upper][&skel].segments.push_back(Entry(geom[i-1], geom[i]));
      else
      {
        Uint31_Index extra_idx = touched_index(geom[i-1], geom[i]);
        if (extra_idx.val() == 0xff)
          calculate_auxiliary_points(skel, geom[i-1], geom[i]);
        else
        {
          queue[geom[i-1].ll_upper][&skel].segments.push_back(Entry(geom[i-1], geom[i]));
          queue[geom[i].ll_upper][&skel].segments.push_back(Entry(geom[i-1], geom[i]));
          if (!(extra_idx.val() == 0))
            queue[extra_idx][&skel].segments.push_back(Entry(geom[i-1], geom[i]));
        }
      }
    }
  }

  void calculate_auxiliary_points(const Way_Skeleton& skel, Quad_Coord lhs, Quad_Coord rhs)
  {
    uint32 lat_lhs = ilat(lhs.ll_upper, lhs.ll_lower);
    int32 lon_lhs = ilon(lhs.ll_upper, lhs.ll_lower);
    uint32 lat_rhs = ilat(rhs.ll_upper, rhs.ll_lower);
    int32 lon_rhs = ilon(rhs.ll_upper, rhs.ll_lower);

    if ((lon_lhs & 0xffff0000) == (lon_rhs & 0xffff0000))
      calculate_south_north_sequence(skel, lat_lhs, lon_lhs, lat_rhs, lon_rhs);
    else
    {
      Great_Circle gc(make_point_double(lhs), make_point_double(rhs));
      Quad_Coord min = lon_lhs < lon_rhs ? lhs : rhs;
      Quad_Coord max = lon_lhs < lon_rhs ? rhs : lhs;
      //TODO: antimeridian
      int32 i_ilon = ilon(min.ll_upper, min.ll_lower);
      int32 ilon_max = ilon(max.ll_upper, max.ll_lower);
      uint32 i_ilat = ilat(gc.lat_of(lon((i_ilon & 0xffff0000) + 0x10000)));
      calculate_south_north_sequence(
          skel, ilat(min.ll_upper, min.ll_lower), i_ilon,
          i_ilat, (i_ilon & 0xffff0000) + 0x10000);
      for (i_ilon = (i_ilon & 0xffff0000) + 0x10000; i_ilon + 0x10000 < ilon_max;
          i_ilon += 0x10000)
      {
        uint32 last_ilat = i_ilat;
        i_ilat = ilat(gc.lat_of(lon(i_ilon + 0x10000)));
        calculate_south_north_sequence(
            skel, last_ilat, i_ilon, i_ilat, i_ilon + 0x10000);
      }
      
      calculate_south_north_sequence(
          skel, i_ilat, i_ilon, ilat(max.ll_upper, max.ll_lower), ilon_max);
    }
  }

  void calculate_south_north_sequence(
      const Way_Skeleton& skel, uint32 lat_lhs, int32 lon_lhs, uint32 lat_rhs, int32 lon_rhs)
  {
//     std::cout<<"DEBUG_A "<<std::hex<<ll_upper_(lat_lhs, lon_lhs)<<' '
//         <<std::dec<<lat_lhs<<' '<<lon_lhs<<' '<<lat_rhs<<' '<<lon_rhs<<'\n';
    if ((lat_lhs & 0xffff0000) == (lat_rhs & 0xffff0000))
    {
      queue[Uint31_Index(ll_upper_(lat_lhs, lon_lhs))][&skel].segments.push_back(
          Entry(lat_lhs, lon_lhs, lat_rhs, lon_rhs));
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
    queue[Uint31_Index(ll_upper_(lat_lhs, min_lon))][&skel].segments.push_back(
        Entry(lat_lhs, lon_lhs, i_lat, i_lon));

    while (i_lat + 0x10000 < lat_rhs)
    {
      uint32 last_ilat = i_lat;
      int32 last_ilon = i_lon;
      i_lat += 0x10000;
      i_lon = lon_lhs + ((double)i_lat - lat_lhs)/((int32)lat_rhs - (int32)lat_lhs)*(lon_rhs - lon_lhs);
//       std::cout<<"DEBUG_D "<<std::hex<<ll_upper_(last_ilat, min_lon)<<' '
//           <<std::dec<<last_ilat<<' '<<last_ilon<<' '<<i_lat<<' '<<i_lon<<'\n';
      queue[Uint31_Index(ll_upper_(last_ilat, min_lon))][&skel].segments.push_back(
          Entry(last_ilat, last_ilon, i_lat, i_lon));
    }
    
//     std::cout<<"DEBUG_C "<<std::hex<<ll_upper_(i_lat, min_lon)<<' '
//         <<std::dec<<i_lat<<' '<<i_lon<<' '<<lat_rhs<<' '<<lon_rhs<<'\n';
    queue[Uint31_Index(ll_upper_(i_lat, min_lon))][&skel].segments.push_back(
        Entry(i_lat, i_lon, lat_rhs, lon_rhs));
  }
  
  void propagate_inside_flag()
  {
    Uint31_Index idx = get_idx();
    uint32 south = ilat(idx.val(), 0u);
    int32 west = ilon(idx.val(), 0u);
    const std::map< const Way_Skeleton*, Index_Block >& way_blocks = get_obj();
    
    for (std::map< const Way_Skeleton*, Index_Block >::const_iterator bit = way_blocks.begin();
        bit != way_blocks.end(); ++bit)
    {
      const Index_Block& block = bit->second;
      bool is_inside = block.sw_is_inside;
      
      for (std::vector< Entry >::const_iterator it = block.segments.begin(); it != block.segments.end(); ++it)
      {
  //       std::cout<<"DEBUG "<<std::hex<<idx.val()<<' '
  //           <<((it->ilat_west <= south) ^ (it->ilat_east <= south))
  //           <<(west <= it->ilon_west +
  //               ((double)south - it->ilat_west)*(it->ilon_east - it->ilon_west)/((int32)it->ilat_east - (int32)it->ilat_west))
  //           <<' '<<std::dec<<west<<' '<<it->ilon_west<<' '
  //           <<((double)south - it->ilat_west)<<' '
  //           <<((double)south - it->ilat_west)*(it->ilon_east - it->ilon_west)<<' '
  //           <<((double)south - it->ilat_west)*(it->ilon_east - it->ilon_west)/((int32)it->ilat_east - (int32)it->ilat_west)<<'\n';
        if ((it->ilat_west <= south) ^ (it->ilat_east <= south))
        {
          double isect_lon = it->ilon_west +
              ((double)south - it->ilat_west)
              *(it->ilon_east - it->ilon_west)/((int32)it->ilat_east - (int32)it->ilat_west);
          if (west <= isect_lon && isect_lon < west + 0x10000)
            is_inside = !is_inside;
        }
      }
      
      if (is_inside)
        queue[Uint31_Index(ll_upper_(ilat(idx.val(), 0u), ilon(idx.val(), 0u)+0x10000))][bit->first].sw_is_inside = is_inside;
    }
  }
};


#endif
