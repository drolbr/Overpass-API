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
    result.push_back(Uint31_Index(ll_upper(lat, lon)));
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
    uint32 lat_boundary = (double(lat_rhs - lat_lhs))*(lon_boundary - lon_lhs)/(lon_rhs - lon_lhs) + lat_lhs;
    return std::vector< Uint31_Index >(1, Uint31_Index(ll_upper(lat_boundary, lon_boundary)));
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


// fwd iterator only
class Tilewise_Area_Iterator
{
public:
  struct Entry
  {
    Entry(const Way_Skeleton* ref_, uint i_begin_, uint i_end_)
        : ref(ref_), i_begin(i_begin_), i_end(i_end_) {}
    
    const Way_Skeleton* ref;
    uint i_begin;
    uint i_end;
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
    queue.erase(queue.begin());
    if (queue.empty())
      refill();
  }
  bool is_end() const { return queue.empty(); }
  
  const std::vector< Entry >& get_obj() const { return queue.begin()->second; }
  Uint31_Index get_idx() const { return queue.begin()->first; }
  
private:
  const std::map< Uint31_Index, std::vector< Way_Skeleton > >* ways;
  const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >* attic_ways;
  std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator cur_it;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator attic_it;
  Way_Geometry_Store cur_geom_store;
  Way_Geometry_Store attic_geom_store;
  std::map< Uint31_Index, std::vector< Entry > > queue;
  
  void refill()
  {
    while (cur_it != ways->end() && attic_it != attic_ways->end())
    {
      Uint31_Index idx = (cur_it != ways->end() && (attic_it == attic_ways->end() || cur_it->first < attic_it->first)
          ? cur_it->first : attic_it->first);
      if (cur_it != ways->end() && cur_it->first == idx)
      {
        for (std::vector< Way_Skeleton >::const_iterator it = cur_it->second.begin(); it != cur_it->second.end(); ++it)
          make_entries(*it, cur_geom_store.get_geometry(*it));
        ++cur_it;
      }
      if (attic_it != attic_ways->end() && attic_it->first == idx)
      {
        for (std::vector< Attic< Way_Skeleton > >::const_iterator it = attic_it->second.begin();
            it != attic_it->second.end(); ++it)
          make_entries(*it, attic_geom_store.get_geometry(*it));
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

    uint i_begin = 0;
    for (uint i = 1; i < geom.size(); ++i)
    {
      if (geom[i].ll_upper != geom[i-1].ll_upper)
      {
        queue[geom[i-1].ll_upper].push_back(Entry(&skel, i_begin, i+1));
        i_begin =  i-1;
        
        std::vector< Uint31_Index > touched = touched_indexes(geom[i-1], geom[i]);
        for (std::vector< Uint31_Index >::const_iterator it = touched.begin(); it != touched.end(); ++it)
          queue[*it].push_back(Entry(&skel, i_begin, i+1));
      }
    }
    queue[geom.back().ll_upper].push_back(Entry(&skel, i_begin, geom.size()));
  }
};


#endif
