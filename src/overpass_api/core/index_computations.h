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

#ifndef DE__OSM3S___OVERPASS_API__CORE__INDEX_COMPUTATIONS_H
#define DE__OSM3S___OVERPASS_API__CORE__INDEX_COMPUTATIONS_H

#include "../../template_db/ranges.h"
#include "basic_types.h"

#include <algorithm>
#include <set>
#include <vector>


inline uint32 ll_upper(uint32 ilat, uint32 ilon);
inline uint32 ll_upper_(uint32 ilat, int32 ilon);
inline uint32 upper_ilat(uint32 quadtile);
inline uint32 upper_ilon(uint32 quadtile);
inline uint32 ilat(uint32 ll_upper, uint32 ll_lower);
inline int32 ilon(uint32 ll_upper, uint32 ll_lower);

inline uint32 calc_index(const std::vector< uint32 >& node_idxs);
inline std::pair< Uint32_Index, Uint32_Index > calc_bbox_bounds(Uint31_Index way_rel_idx);
inline std::vector< Uint32_Index > calc_node_children(const std::vector< uint32 >& way_rel_idxs);
inline std::vector< Uint31_Index > calc_children(const std::vector< uint32 >& way_rel_idxs);
inline std::vector< uint32 > calc_parents(const std::vector< uint32 >& node_idxs);

inline Ranges< Uint32_Index > calc_ranges(double south, double north, double west, double east);

inline void recursively_calc_ranges
    (uint32 south, uint32 north, int32 west, int32 east,
     uint32 bitlevel, Ranges< Uint32_Index >& ranges);

/** ------------------------------------------------------------------------ */

struct Uint31_Compare
{
  bool operator()(uint32 a, uint32 b) const
  {
    return (a & 0x7fffffff) < (b & 0x7fffffff);
  }
};

inline uint32 ll_upper(uint32 ilat, uint32 ilon)
{
  ilat &= 0xffff0000;
  ilat |= (ilat>>8);
  ilat &= 0xff00ff00;
  ilat |= (ilat>>4);
  ilat &= 0xf0f0f0f0;
  ilat |= (ilat>>2);
  ilat &= 0xcccccccc;
  ilat |= (ilat>>1);
  ilat &= 0xaaaaaaaa;

  ilon &= 0xffff0000;
  ilon |= (ilon>>8);
  ilon &= 0xff00ff00;
  ilon |= (ilon>>4);
  ilon &= 0xf0f0f0f0;
  ilon |= (ilon>>2);
  ilon &= 0xcccccccc;
  ilon |= (ilon>>1);
  ilon &= 0xaaaaaaaa;

  return ilat | (ilon>>1);
}

inline uint32 ll_upper_(uint32 ilat, int32 ilon)
{
  return (ll_upper(ilat, ilon) ^ 0x40000000);
}

inline uint32 upper_ilat(uint32 quadtile)
{
  uint32 result = (quadtile & 0xaaaaaaaa)>>1;
  result |= (result>>1);
  result &= 0x33333333;
  result |= (result>>2);
  result &= 0x0f0f0f0f;
  result |= (result>>4);
  result &= 0x00ff00ff;
  result |= (result>>8);

  return result & 0xffff;
}

inline uint32 upper_ilon(uint32 quadtile)
{
  uint32 result = (quadtile & 0x55555555);
  result |= (result>>1);
  result &= 0x33333333;
  result |= (result>>2);
  result &= 0x0f0f0f0f;
  result |= (result>>4);
  result &= 0x00ff00ff;
  result |= (result>>8);

  return result & 0xffff;
}

inline uint32 calc_index(const std::vector< uint32 >& node_idxs)
{
  if (node_idxs.empty())
    return 0xfe;

  // Calculate the bounding box of the appearing indices.

  std::vector< uint32 >::const_iterator it = node_idxs.begin();
  uint32 lat_min = *it & 0x2aaaaaaa;
  uint32 lat_max = lat_min;
  uint32 lon_min = *it & 0x55555555;
  uint32 lon_max = lon_min;

  if (*it & 0x80000000)
  {
    if (*it & 0x00000001)
    {
      lat_min = *it & 0x2aaaaaa8;
      lon_min = *it & 0x55555554;
      lat_max = ll_upper((upper_ilat(lat_min) + 3)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 3)<<16);
    }
    else if (*it & 0x00000002)
    {
      lat_min = *it & 0x2aaaaa80;
      lon_min = *it & 0x55555540;
      lat_max = ll_upper((upper_ilat(lat_min) + 0xf)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 0xf)<<16);
    }
    else if (*it & 0x00000004)
    {
      lat_min = *it & 0x2aaaa800;
      lon_min = *it & 0x55555400;
      lat_max = ll_upper((upper_ilat(lat_min) + 0x3f)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 0x3f)<<16);
    }
    else if (*it & 0x00000008)
    {
      lat_min = *it & 0x2aaa8000;
      lon_min = *it & 0x55554000;
      lat_max = ll_upper((upper_ilat(lat_min) + 0xff)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 0xff)<<16);
    }
    else if (*it & 0x00000010)
    {
      lat_min = *it & 0x2aa80000;
      lon_min = *it & 0x55540000;
      lat_max = ll_upper((upper_ilat(lat_min) + 0x3ff)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 0x3ff)<<16);
    }
    else if (*it & 0x00000020)
    {
      lat_min = *it & 0x2a800000;
      lon_min = *it & 0x55400000;
      lat_max = ll_upper((upper_ilat(lat_min) + 0xfff)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 0xfff)<<16);
    }
    else if (*it & 0x00000040)
    {
      lat_min = *it & 0x28000000;
      lon_min = *it & 0x54000000;
      lat_max = ll_upper((upper_ilat(lat_min) + 0x3fff)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + 0x3fff)<<16);
    }
    else // *it == 0x80000080
      return 0x80000080;
  }

  for (++it; it != node_idxs.end(); ++it)
  {
    if (*it & 0x80000000)
    {
      uint32 lat = 0;
      uint32 lon = 0;
      uint32 lat_u = 0;
      uint32 lon_u = 0;

      if (*it & 0x00000001)
      {
	lat = *it & 0x2aaaaaa8;
	lon = *it & 0x55555554;
	lat_u = ll_upper((upper_ilat(lat) + 3)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 3)<<16);
      }
      else if (*it & 0x00000002)
      {
	lat = *it & 0x2aaaaa80;
	lon = *it & 0x55555540;
	lat_u = ll_upper((upper_ilat(lat) + 0xf)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 0xf)<<16);
      }
      else if (*it & 0x00000004)
      {
	lat = *it & 0x2aaaa800;
	lon = *it & 0x55555400;
	lat_u = ll_upper((upper_ilat(lat) + 0x3f)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 0x3f)<<16);
      }
      else if (*it & 0x00000008)
      {
	lat = *it & 0x2aaa8000;
	lon = *it & 0x55554000;
	lat_u = ll_upper((upper_ilat(lat) + 0xff)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 0xff)<<16);
      }
      else if (*it & 0x00000010)
      {
	lat = *it & 0x2aa80000;
	lon = *it & 0x55540000;
	lat_u = ll_upper((upper_ilat(lat) + 0x3ff)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 0x3ff)<<16);
      }
      else if (*it & 0x00000020)
      {
	lat = *it & 0x2a800000;
	lon = *it & 0x55400000;
	lat_u = ll_upper((upper_ilat(lat) + 0xfff)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 0xfff)<<16);
      }
      else if (*it & 0x00000040)
      {
	lat = *it & 0x28000000;
	lon = *it & 0x54000000;
	lat_u = ll_upper((upper_ilat(lat) + 0x3fff)<<16, 0);
	lon_u = ll_upper(0, (upper_ilon(lon) + 0x3fff)<<16);
      }
      else // *it == 0x80000080
	return 0x80000080;

      if (lat < lat_min)
	lat_min = lat;
      if (lat_u > lat_max)
	lat_max = lat_u;
      if (lon < lon_min)
	lon_min = lon;
      if (lon_u > lon_max)
	lon_max = lon_u;
    }
    else
    {
      uint32 lat = *it & 0x2aaaaaaa;
      uint32 lon = *it & 0x55555555;

      if (lat < lat_min)
        lat_min = lat;
      else if (lat > lat_max)
	lat_max = lat;
      if (lon < lon_min)
	lon_min = lon;
      else if (lon > lon_max)
	lon_max = lon;
    }
  }


  // Evaluate the bounding box.

  if ((lat_max == lat_min) && (lon_max == lon_min))
    return node_idxs.front();

  uint32 ilat_min = upper_ilat(lat_min);
  uint32 ilat_max = upper_ilat(lat_max);
  uint32 ilon_min = upper_ilon(lon_min);
  uint32 ilon_max = upper_ilon(lon_max);

  if (((ilat_max & 0xfffe) - (ilat_min & 0xfffe) < 4) &&
      ((ilon_max & 0xfffe) - (ilon_min & 0xfffe) < 4))
    return (((lon_min | lat_min) & 0xfffffffc) | 0x80000001);

  if (((ilat_max & 0xfff8) - (ilat_min & 0xfff8) < 0x10) &&
      ((ilon_max & 0xfff8) - (ilon_min & 0xfff8) < 0x10))
    return (((lon_min | lat_min) & 0xffffffc0) | 0x80000002);

  if (((ilat_max & 0xffe0) - (ilat_min & 0xffe0) < 0x40) &&
      ((ilon_max & 0xffe0) - (ilon_min & 0xffe0) < 0x40))
    return (((lon_min | lat_min) & 0xfffffc00) | 0x80000004);

  if (((ilat_max & 0xff80) - (ilat_min & 0xff80) < 0x100) &&
      ((ilon_max & 0xff80) - (ilon_min & 0xff80) < 0x100))
    return (((lon_min | lat_min) & 0xffffc000) | 0x80000008);

  if (((ilat_max & 0xfe00) - (ilat_min & 0xfe00) < 0x400) &&
      ((ilon_max & 0xfe00) - (ilon_min & 0xfe00) < 0x400))
    return (((lon_min | lat_min) & 0xfffc0000) | 0x80000010);

  if (((ilat_max & 0xf800) - (ilat_min & 0xf800) < 0x1000) &&
      ((ilon_max & 0xf800) - (ilon_min & 0xf800) < 0x1000))
    return (((lon_min | lat_min) & 0xffc00000) | 0x80000020);

  if (((ilat_max & 0xe000) - (ilat_min & 0xe000) < 0x4000) &&
      ((ilon_max & 0xe000) - (ilon_min & 0xe000) < 0x4000))
    return (((lon_min | lat_min) & 0xfc000000) | 0x80000040);

  return 0x80000080;
}


inline std::pair< Uint32_Index, Uint32_Index > calc_bbox_bounds(Uint31_Index way_rel_idx)
{
  if (way_rel_idx.val() & 0x80000000)
  {
    uint32 lat = 0;
    uint32 lon = 0;
    uint32 offset = 0;

    if (way_rel_idx.val() & 0x00000001)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x2aaaaaa8);
      lon = upper_ilon(way_rel_idx.val() & 0x55555554);
      offset = 2;
    }
    else if (way_rel_idx.val() & 0x00000002)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x2aaaaa80);
      lon = upper_ilon(way_rel_idx.val() & 0x55555540);
      offset = 8;
    }
    else if (way_rel_idx.val() & 0x00000004)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x2aaaa800);
      lon = upper_ilon(way_rel_idx.val() & 0x55555400);
      offset = 0x20;
    }
    else if (way_rel_idx.val() & 0x00000008)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x2aaa8000);
      lon = upper_ilon(way_rel_idx.val() & 0x55554000);
      offset = 0x80;
    }
    else if (way_rel_idx.val() & 0x00000010)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x2aa80000);
      lon = upper_ilon(way_rel_idx.val() & 0x55540000);
      offset = 0x200;
    }
    else if (way_rel_idx.val() & 0x00000020)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x2a800000);
      lon = upper_ilon(way_rel_idx.val() & 0x55400000);
      offset = 0x800;
    }
    else if (way_rel_idx.val() & 0x00000040)
    {
      lat = upper_ilat(way_rel_idx.val() & 0x28000000);
      lon = upper_ilon(way_rel_idx.val() & 0x54000000);
      offset = 0x2000;
    }
    else // way_rel_idx.val() == 0x80000080
    {
      lat = 0;
      lon = 0;
      offset = 0x8000;
    }

    return std::make_pair(ll_upper(lat<<16, lon<<16),
		      ll_upper((lat+2*offset-1)<<16, (lon+2*offset-1)<<16)+1);
  }
  else if (way_rel_idx.val() > 0xff)
    return std::make_pair(way_rel_idx.val(), way_rel_idx.val() + 1);
  else
    return std::make_pair(0u, 1u);
}


inline std::vector< Uint32_Index > calc_node_children(const std::vector< uint32 >& way_rel_idxs)
{
  std::vector< Uint32_Index > result;

  std::vector< std::pair< uint32, uint32 > > ranges;

  for (std::vector< uint32 >::const_iterator it = way_rel_idxs.begin();
      it != way_rel_idxs.end(); ++it)
  {
    if (*it & 0x80000000)
    {
      uint32 lat = 0;
      uint32 lon = 0;
      uint32 lat_u = 0;
      uint32 lon_u = 0;
      uint32 offset = 0;

      if (*it & 0x00000001)
      {
	lat = upper_ilat(*it & 0x2aaaaaa8);
	lon = upper_ilon(*it & 0x55555554);
	offset = 2;
      }
      else if (*it & 0x00000002)
      {
	lat = upper_ilat(*it & 0x2aaaaa80);
	lon = upper_ilon(*it & 0x55555540);
	offset = 8;
      }
      else if (*it & 0x00000004)
      {
	lat = upper_ilat(*it & 0x2aaaa800);
	lon = upper_ilon(*it & 0x55555400);
	offset = 0x20;
      }
      else if (*it & 0x00000008)
      {
	lat = upper_ilat(*it & 0x2aaa8000);
	lon = upper_ilon(*it & 0x55554000);
	offset = 0x80;
      }
      else if (*it & 0x00000010)
      {
	lat = upper_ilat(*it & 0x2aa80000);
	lon = upper_ilon(*it & 0x55540000);
	offset = 0x200;
      }
      else if (*it & 0x00000020)
      {
	lat = upper_ilat(*it & 0x2a800000);
	lon = upper_ilon(*it & 0x55400000);
	offset = 0x800;
      }
      else if (*it & 0x00000040)
      {
	lat = upper_ilat(*it & 0x28000000);
	lon = upper_ilon(*it & 0x54000000);
	offset = 0x2000;
      }
      else // *it == 0x80000080
      {
	lat = 0;
	lon = 0;
	offset = 0x8000;
      }

      ranges.push_back(std::make_pair(ll_upper(lat<<16, lon<<16),
				 ll_upper((lat+offset-1)<<16, (lon+offset-1)<<16)+1));
      ranges.push_back(std::make_pair(ll_upper(lat<<16, (lon+offset)<<16),
				 ll_upper((lat+offset-1)<<16, (lon+2*offset-1)<<16)+1));
      ranges.push_back(std::make_pair(ll_upper((lat+offset)<<16, lon<<16),
				 ll_upper((lat+2*offset-1)<<16, (lon+offset-1)<<16)+1));
      ranges.push_back(std::make_pair(ll_upper((lat+offset)<<16, (lon+offset)<<16),
				 ll_upper((lat+2*offset-1)<<16, (lon+2*offset-1)<<16)+1));
      for (uint32 i = lat; i <= lat_u; ++i)
      {
	for (uint32 j = lon; j <= lon_u; ++j)
	  result.push_back(ll_upper(i<<16, j<<16));
      }
    }
    else
      ranges.push_back(std::make_pair(*it, (*it) + 1));
  }

  sort(ranges.begin(), ranges.end());
  uint32 pos = 0;
  for (std::vector< std::pair< uint32, uint32 > >::const_iterator it = ranges.begin();
      it != ranges.end(); ++it)
  {
    if (pos < it->first)
      pos = it->first;
    for (; pos < it->second; ++pos)
      result.push_back(pos);
  }
  return result;
}


struct Idx_Bbox
{
  Idx_Bbox(uint32 ll_upper)
  {
    if (ll_upper & 0x80000000)
    {
      if (ll_upper & 0x00000001)
      {
        lat = upper_ilat(ll_upper & 0x2aaaaaa8);
        lon = upper_ilon(ll_upper & 0x55555554);
        lat_u = lat + 3;
        lon_u = lon + 3;
      }
      else if (ll_upper & 0x00000002)
      {
        lat = upper_ilat(ll_upper & 0x2aaaaa80);
        lon = upper_ilon(ll_upper & 0x55555540);
        lat_u = lat + 0xf;
        lon_u = lon + 0xf;
      }
      else if (ll_upper & 0x00000004)
      {
        lat = upper_ilat(ll_upper & 0x2aaaa800);
        lon = upper_ilon(ll_upper & 0x55555400);
        lat_u = lat + 0x3f;
        lon_u = lon + 0x3f;
      }
      else if (ll_upper & 0x00000008)
      {
        lat = upper_ilat(ll_upper & 0x2aaa8000);
        lon = upper_ilon(ll_upper & 0x55554000);
        lat_u = lat + 0xff;
        lon_u = lon + 0xff;
      }
      else if (ll_upper & 0x00000010)
      {
        lat = upper_ilat(ll_upper & 0x2aa80000);
        lon = upper_ilon(ll_upper & 0x55540000);
        lat_u = lat + 0x3ff;
        lon_u = lon + 0x3ff;
      }
      else if (ll_upper & 0x00000020)
      {
        lat = upper_ilat(ll_upper & 0x2a800000);
        lon = upper_ilon(ll_upper & 0x55400000);
        lat_u = lat + 0xfff;
        lon_u = lon + 0xfff;
      }
      else if (ll_upper & 0x00000040)
      {
        lat = upper_ilat(ll_upper & 0x28000000);
        lon = upper_ilon(ll_upper & 0x54000000);
        lat_u = lat + 0x3fff;
        lon_u = lon + 0x3fff;
      }
      else // ll_upper == 0x80000080
      {
        lat = 0;
        lon = 0;
        lat_u = 0x7fff;
        lon_u = 0xffff;
      }
    }
    else
    {
      lat = upper_ilat(ll_upper);
      lon = upper_ilon(ll_upper);
      lat_u = lat;
      lon_u = lon;
    }
  }
  
  int compare(const Idx_Bbox& rhs)
  {
    bool lhs_bigger = (lat < rhs.lat || lon < rhs.lon || lat_u > rhs.lat_u || lon_u > rhs.lon_u);
    bool rhs_bigger = (lat > rhs.lat || lon > rhs.lon || lat_u < rhs.lat_u || lon_u < rhs.lon_u);
    return (lhs_bigger<<1) | rhs_bigger;
  }
  
  int32 lat;
  int32 lon;
  int32 lat_u;
  int32 lon_u;
};


inline std::vector< Uint31_Index > calc_children(const std::vector< uint32 >& way_rel_idxs)
{
  std::vector< Uint31_Index > result;

  for (std::vector< uint32 >::const_iterator it = way_rel_idxs.begin();
      it != way_rel_idxs.end(); ++it)
  {
    if (*it & 0x80000000)
    {
      Idx_Bbox bbox(*it);

      if (!(*it & 0x0000007f))
        result.push_back(0x80000080);
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0x3fff); i += 0x2000)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 0x3fff); j += 0x2000)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfc000000) | 0x80000040);
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0xfff); i += 0x800)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 0xfff); j += 0x800)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xffc00000) | 0x80000020);
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0x3ff); i += 0x200)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 0x3ff); j += 0x200)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfffc0000) | 0x80000010);
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0xff); i += 0x80)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 0xff); j += 0x80)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xffffc000) | 0x80000008);
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0x3f); i += 0x20)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 0x3f); j += 0x20)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfffffc00) | 0x80000004);
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0xf); i += 8)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 0xf); j += 8)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xffffffc0) | 0x80000002);
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 3); i += 2)
      {
	for (int32 j = bbox.lon; j <= (bbox.lon_u - 3); j += 2)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfffffffc) | 0x80000001);
      }
      for (int32 i = bbox.lat; i <= bbox.lat_u; ++i)
      {
	for (int32 j = bbox.lon; j <= bbox.lon_u; ++j)
	  result.push_back(ll_upper(i<<16, j<<16));
      }
    }
    else
      result.push_back(*it);
  }

  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());
  return result;
}


inline Ranges< Uint31_Index > calc_children(std::vector< Uint31_Index >&& parents)
{
  Ranges< Uint31_Index > result;
  
  if (parents.empty())
    return result;
  std::sort(parents.begin(), parents.end());
  auto i_src = parents.begin()+1;
  auto i_target = parents.begin();
  Idx_Bbox bbox_target(i_target->val());
  while (i_src != parents.end())
  {
    Idx_Bbox bbox_src(i_src->val());
    uint comp = bbox_target.compare(bbox_src);
    if (comp == 3)
      ++i_target;
    if (comp == 1 || comp == 3)
    {
      *i_target = *i_src;
      bbox_target = bbox_src;
    }
    ++i_src;
  }
  parents.erase(i_target+1, parents.end());

  for (auto i : parents)
  {
    if (i.val() & 0x80000000)
    {
      Idx_Bbox bbox(i.val());

      if (!(i.val() & 0x0000007f))
        result.push_back(0x80000080, 0x81);
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0x3fff); i += 0x2000)
      {
        for (int32 j = bbox.lon; j <= (bbox.lon_u - 0x3fff); j += 0x2000)
        {
          Uint31_Index idx((ll_upper(i<<16, j<<16) & 0xfc000000) | 0x80000040);
          result.push_back(idx, inc(idx));
        }
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0xfff); i += 0x800)
      {
        for (int32 j = bbox.lon; j <= (bbox.lon_u - 0xfff); j += 0x800)
        {
          Uint31_Index idx((ll_upper(i<<16, j<<16) & 0xffc00000) | 0x80000020);
          result.push_back(idx, inc(idx));
        }
      }
      for (int32 i = bbox.lat; i <= (bbox.lat_u - 0x3ff); i += 0x200)
      {
        for (int32 j = bbox.lon; j <= (bbox.lon_u - 0x3ff); j += 0x200)
        {
          Uint31_Index idx((ll_upper(i<<16, j<<16) & 0xfffc0000) | 0x80000010);
          result.push_back(idx, inc(idx));
        }
      }
      for (int32 i = bbox.lat; i <= bbox.lat_u; i += 2)
      {
        for (int32 j = bbox.lon; j <= bbox.lon_u; j += 2)
        {
          uint32 idx = ll_upper(i<<16, j<<16);
          uint32 lower = idx;
          if (((idx & 0xf) == 0x0 && ((idx & 0x3ffff) == 0x10 || (idx & 0x3fffff) == 0x20
                  || (idx & 0x3ffffff) == 0x40 || (idx & 0x3fffffff) == 0x80))
              || ((idx & 0x3ff) == 0x4 && (i > (bbox.lat_u - 0x3f) || j > (bbox.lon_u - 0x3d)))
              || ((idx & 0x3fff) == 0x8 && (i > (bbox.lat_u - 0xfd) || j > (bbox.lon_u - 0xff))))
          {
            result.push_back(Uint31_Index(idx), Uint31_Index(idx | 0x80000000));
            lower = idx+1;
          }
          if (i > (bbox.lat_u - 3) || j > (bbox.lon_u - 3))
          {
            result.push_back(Uint31_Index(lower), Uint31_Index((idx+1) | 0x80000000));
            lower = idx+2;
          }
          if ((idx & 0x3f) == 0x2 && (i > (bbox.lat_u - 0xf) || j > (bbox.lon_u - 0xf)))
          {
            result.push_back(Uint31_Index(lower), Uint31_Index((idx+2) | 0x80000000));
            lower = idx+3;
          }
          result.push_back(Uint31_Index(lower), Uint31_Index(idx+4));
        }
      }
    }
    else
      result.push_back(i, inc(i));
  }

  result.sort();
  return result;
}


inline Ranges< Uint31_Index > calc_children_(const std::vector< Uint31_Index >& parents)
{
  auto copy = parents;
  return calc_children(std::move(copy));
}


inline Ranges< Uint32_Index > calc_children(const Ranges< Uint31_Index >& way_rel_idxs)
{
  Ranges< Uint32_Index > ranges;

  for (auto it = way_rel_idxs.begin(); it != way_rel_idxs.end(); ++it)
  {
    for (Uint31_Index idx = it.lower_bound(); idx < it.upper_bound(); idx = inc(idx))
    {
      if (idx.val() & 0x80000000)
      {
	uint32 lat = 0;
	uint32 lon = 0;
	uint32 offset = 0;

	if (idx.val() & 0x00000001)
	{
	  lat = upper_ilat(idx.val() & 0x2aaaaaa8);
	  lon = upper_ilon(idx.val() & 0x55555554);
	  offset = 2;
	}
	else if (idx.val() & 0x00000002)
	{
	  lat = upper_ilat(idx.val() & 0x2aaaaa80);
	  lon = upper_ilon(idx.val() & 0x55555540);
	  offset = 8;
	}
	else if (idx.val() & 0x00000004)
	{
	  lat = upper_ilat(idx.val() & 0x2aaaa800);
	  lon = upper_ilon(idx.val() & 0x55555400);
	  offset = 0x20;
	}
	else if (idx.val() & 0x00000008)
	{
	  lat = upper_ilat(idx.val() & 0x2aaa8000);
	  lon = upper_ilon(idx.val() & 0x55554000);
	  offset = 0x80;
	}
	else if (idx.val() & 0x00000010)
	{
	  lat = upper_ilat(idx.val() & 0x2aa80000);
	  lon = upper_ilon(idx.val() & 0x55540000);
	  offset = 0x200;
	}
	else if (idx.val() & 0x00000020)
	{
	  lat = upper_ilat(idx.val() & 0x2a800000);
	  lon = upper_ilon(idx.val() & 0x55400000);
	  offset = 0x800;
	}
	else if (idx.val() & 0x00000040)
	{
	  lat = upper_ilat(idx.val() & 0x28000000);
	  lon = upper_ilon(idx.val() & 0x54000000);
	  offset = 0x2000;
	}
	else // idx.val() == 0x80000080
	{
	  lat = 0;
	  lon = 0;
	  offset = 0x8000;
	}

	ranges.push_back(
            ll_upper(lat<<16, lon<<16),
            ll_upper((lat+offset-1)<<16, (lon+offset-1)<<16)+1);
	ranges.push_back(
            ll_upper(lat<<16, (lon+offset)<<16),
            ll_upper((lat+offset-1)<<16, (lon+2*offset-1)<<16)+1);
        ranges.push_back(
            ll_upper((lat+offset)<<16, lon<<16),
            ll_upper((lat+2*offset-1)<<16, (lon+offset-1)<<16)+1);
	ranges.push_back(
            ll_upper((lat+offset)<<16, (lon+offset)<<16),
            ll_upper((lat+2*offset-1)<<16, (lon+2*offset-1)<<16)+1);
      }
      else
	ranges.push_back(idx.val(), idx.val() + 1);
    }
  }
  ranges.sort();

  return ranges;
}


inline std::set< Uint31_Index > calc_parents(const std::set< Uint31_Index >& node_idxs)
{
  std::set< Uint31_Index > result;
  result.insert(0x80000080);

  for (std::set< Uint31_Index >::const_iterator it = node_idxs.begin();
      it != node_idxs.end(); ++it)
  {
    result.insert(*it);

    uint32 lat = upper_ilat(it->val() & 0x2aaaaaa8) & 0xfffe;
    uint32 lon = upper_ilon(it->val() & 0x55555554) & 0xfffe;
    result.insert(ll_upper((lat - 2)<<16, (lon - 2)<<16) | 0x80000001);
    result.insert(ll_upper(lat<<16, (lon - 2)<<16) | 0x80000001);
    result.insert(ll_upper((lat - 2)<<16, lon<<16) | 0x80000001);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000001);

    lat = lat & 0xfff8;
    lon = lon & 0xfff8;
    result.insert(ll_upper((lat - 8)<<16, (lon - 8)<<16) | 0x80000002);
    result.insert(ll_upper(lat<<16, (lon - 8)<<16) | 0x80000002);
    result.insert(ll_upper((lat - 8)<<16, lon<<16) | 0x80000002);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000002);

    lat = lat & 0xffe0;
    lon = lon & 0xffe0;
    result.insert(ll_upper((lat - 0x20)<<16, (lon - 0x20)<<16) | 0x80000004);
    result.insert(ll_upper(lat<<16, (lon - 0x20)<<16) | 0x80000004);
    result.insert(ll_upper((lat - 0x20)<<16, lon<<16) | 0x80000004);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000004);

    lat = lat & 0xff80;
    lon = lon & 0xff80;
    result.insert(ll_upper((lat - 0x80)<<16, (lon - 0x80)<<16) | 0x80000008);
    result.insert(ll_upper(lat<<16, (lon - 0x80)<<16) | 0x80000008);
    result.insert(ll_upper((lat - 0x80)<<16, lon<<16) | 0x80000008);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000008);

    lat = lat & 0xfe00;
    lon = lon & 0xfe00;
    result.insert(ll_upper((lat - 0x200)<<16, (lon - 0x200)<<16) | 0x80000010);
    result.insert(ll_upper(lat<<16, (lon - 0x200)<<16) | 0x80000010);
    result.insert(ll_upper((lat - 0x200)<<16, lon<<16) | 0x80000010);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000010);

    lat = lat & 0xf800;
    lon = lon & 0xf800;
    result.insert(ll_upper((lat - 0x800)<<16, (lon - 0x800)<<16) | 0x80000020);
    result.insert(ll_upper(lat<<16, (lon - 0x800)<<16) | 0x80000020);
    result.insert(ll_upper((lat - 0x800)<<16, lon<<16) | 0x80000020);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000020);

    lat = lat & 0xe000;
    lon = lon & 0xe000;
    result.insert(ll_upper((lat - 0x2000)<<16, (lon - 0x2000)<<16) | 0x80000040);
    result.insert(ll_upper(lat<<16, (lon - 0x2000)<<16) | 0x80000040);
    result.insert(ll_upper((lat - 0x2000)<<16, lon<<16) | 0x80000040);
    result.insert(ll_upper(lat<<16, lon<<16) | 0x80000040);
  }

  return result;
}


inline std::vector< uint32 > calc_parents(const std::vector< uint32 >& node_idxs)
{
  std::vector< uint32 > result;
  result.push_back(0x80000080);

  for (std::vector< uint32 >::const_iterator it = node_idxs.begin();
  it != node_idxs.end(); ++it)
  {
    result.push_back(*it);

    uint32 lat = upper_ilat(*it & 0x2aaaaaa8) & 0xfffe;
    uint32 lon = upper_ilon(*it & 0x55555554) & 0xfffe;
    result.push_back(ll_upper((lat - 2)<<16, (lon - 2)<<16) | 0x80000001);
    result.push_back(ll_upper(lat<<16, (lon - 2)<<16) | 0x80000001);
    result.push_back(ll_upper((lat - 2)<<16, lon<<16) | 0x80000001);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000001);

    lat = lat & 0xfff8;
    lon = lon & 0xfff8;
    result.push_back(ll_upper((lat - 8)<<16, (lon - 8)<<16) | 0x80000002);
    result.push_back(ll_upper(lat<<16, (lon - 8)<<16) | 0x80000002);
    result.push_back(ll_upper((lat - 8)<<16, lon<<16) | 0x80000002);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000002);

    lat = lat & 0xffe0;
    lon = lon & 0xffe0;
    result.push_back(ll_upper((lat - 0x20)<<16, (lon - 0x20)<<16) | 0x80000004);
    result.push_back(ll_upper(lat<<16, (lon - 0x20)<<16) | 0x80000004);
    result.push_back(ll_upper((lat - 0x20)<<16, lon<<16) | 0x80000004);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000004);

    lat = lat & 0xff80;
    lon = lon & 0xff80;
    result.push_back(ll_upper((lat - 0x80)<<16, (lon - 0x80)<<16) | 0x80000008);
    result.push_back(ll_upper(lat<<16, (lon - 0x80)<<16) | 0x80000008);
    result.push_back(ll_upper((lat - 0x80)<<16, lon<<16) | 0x80000008);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000008);

    lat = lat & 0xfe00;
    lon = lon & 0xfe00;
    result.push_back(ll_upper((lat - 0x200)<<16, (lon - 0x200)<<16) | 0x80000010);
    result.push_back(ll_upper(lat<<16, (lon - 0x200)<<16) | 0x80000010);
    result.push_back(ll_upper((lat - 0x200)<<16, lon<<16) | 0x80000010);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000010);

    lat = lat & 0xf800;
    lon = lon & 0xf800;
    result.push_back(ll_upper((lat - 0x800)<<16, (lon - 0x800)<<16) | 0x80000020);
    result.push_back(ll_upper(lat<<16, (lon - 0x800)<<16) | 0x80000020);
    result.push_back(ll_upper((lat - 0x800)<<16, lon<<16) | 0x80000020);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000020);

    lat = lat & 0xe000;
    lon = lon & 0xe000;
    result.push_back(ll_upper((lat - 0x2000)<<16, (lon - 0x2000)<<16) | 0x80000040);
    result.push_back(ll_upper(lat<<16, (lon - 0x2000)<<16) | 0x80000040);
    result.push_back(ll_upper((lat - 0x2000)<<16, lon<<16) | 0x80000040);
    result.push_back(ll_upper(lat<<16, lon<<16) | 0x80000040);
  }

  sort(result.begin(), result.end(), Uint31_Compare());
  result.erase(unique(result.begin(), result.end()), result.end());

  return result;
}


inline void push_back_interval(Ranges< Uint31_Index >& ranges, uint32 idx)
{
  ranges.push_back(Uint31_Index(idx), Uint31_Index((idx + 1) & 0x7fffffff));
}


inline void blur_index(
    uint32 distance, uint32 bitmask, uint32 lower_idx, uint32 upper_idx, Ranges< Uint31_Index >& result)
{
  uint32 source_bitmask = 0x10000 - distance;
  uint32 min_lat = upper_ilat(lower_idx) & source_bitmask;
  uint32 min_lon = upper_ilon(lower_idx) & source_bitmask;
  uint32 max_lat = upper_ilat(upper_idx) & source_bitmask;
  uint32 max_lon = upper_ilon(upper_idx) & source_bitmask;
  for (uint32 lat = min_lat - distance; lat <= max_lat; lat += distance)
    push_back_interval(result, ll_upper(lat<<16, (min_lon - distance)<<16) | bitmask);
  for (uint32 lon = min_lon; lon <= max_lon; lon += distance)
    push_back_interval(result, ll_upper((min_lat - distance)<<16, lon<<16) | bitmask);
  push_back_interval(result, ll_upper(max_lat<<16, max_lon<<16) | bitmask);
}


inline void add_decomp_range(const std::pair< Uint32_Index, Uint32_Index >& range,
			     std::vector< std::pair< Uint32_Index, Uint32_Index > >& node_decomp)
{
  uint32 lower = range.first.val();
  while (lower < range.second.val())
  {
    uint32 upper = 0;
    if ((lower & 0x3) != 0 || lower + 4 > range.second.val())
      upper = lower + 1;
    else if ((lower & 0xf) != 0 || lower + 0x10 > range.second.val())
      upper = lower + 4;
    else if ((lower & 0x3f) != 0 || lower + 0x40 > range.second.val())
      upper = lower + 0x10;
    else if ((lower & 0xff) != 0 || lower + 0x100 > range.second.val())
      upper = lower + 0x40;
    else if ((lower & 0x3ff) != 0 || lower + 0x400 > range.second.val())
      upper = lower + 0x100;
    else if ((lower & 0xfff) != 0 || lower + 0x1000 > range.second.val())
      upper = lower + 0x400;
    else if ((lower & 0x3fff) != 0 || lower + 0x4000 > range.second.val())
      upper = lower + 0x1000;
    else if ((lower & 0xffff) != 0 || lower + 0x10000 > range.second.val())
      upper = lower + 0x4000;
    else if ((lower & 0x3ffff) != 0 || lower + 0x40000 > range.second.val())
      upper = lower + 0x10000;
    else if ((lower & 0xfffff) != 0 || lower + 0x100000 > range.second.val())
      upper = lower + 0x40000;
    else if ((lower & 0x3fffff) != 0 || lower + 0x400000 > range.second.val())
      upper = lower + 0x100000;
    else if ((lower & 0xffffff) != 0 || lower + 0x1000000 > range.second.val())
      upper = lower + 0x400000;
    else if ((lower & 0x3ffffff) != 0 || lower + 0x4000000 > range.second.val())
      upper = lower + 0x1000000;
    else if ((lower & 0xfffffff) != 0 || lower + 0x10000000 > range.second.val())
      upper = lower + 0x4000000;
    else if ((lower & 0x3fffffff) != 0 || lower + 0x40000000 > range.second.val())
      upper = lower + 0x10000000;
    else
      upper = lower + 0x40000000;
    node_decomp.push_back(std::make_pair(lower, upper));
    lower = upper;
  }
}


inline void add_decomp_range(const Ranges< Uint32_Index >::Iterator& range,
			     std::vector< std::pair< Uint32_Index, Uint32_Index > >& node_decomp)
{
  add_decomp_range(std::pair< Uint32_Index, Uint32_Index >{ range.lower_bound(), range.upper_bound() }, node_decomp);
}


template< typename Container >
Ranges< Uint31_Index > calc_parents(const Container& node_idxs)
{
  std::vector< std::pair< Uint32_Index, Uint32_Index > > node_decomp;
  for (auto it = node_idxs.begin(); it != node_idxs.end(); ++it)
    add_decomp_range(*it, node_decomp);

  Ranges< Uint31_Index > result;
  result.push_back(0x80000080, 0x80000081);

  for (std::vector< std::pair< Uint32_Index, Uint32_Index > >::const_iterator
      it = node_decomp.begin(); it != node_decomp.end(); ++it)
  {
    result.push_back(it->first.val(), it->second.val());

    uint32 lower_idx = it->first.val() & 0x7ffffffc;
    uint32 upper_idx = (it->second.val() - 1) & 0x7ffffffc;
    blur_index(2, 0x80000001, lower_idx, upper_idx, result);

    lower_idx = it->first.val() & 0x7fffffc0;
    upper_idx = (it->second.val() - 1) & 0x7fffffc0;
    blur_index(8, 0x80000002, lower_idx, upper_idx, result);

    lower_idx = it->first.val() & 0x7ffffc00;
    upper_idx = (it->second.val() - 1) & 0x7ffffc00;
    blur_index(0x20, 0x80000004, lower_idx, upper_idx, result);

    lower_idx = it->first.val() & 0x7fffc000;
    upper_idx = (it->second.val() - 1) & 0x7fffc000;
    blur_index(0x80, 0x80000008, lower_idx, upper_idx, result);

    lower_idx = it->first.val() & 0x7ffc0000;
    upper_idx = (it->second.val() - 1) & 0x7ffc0000;
    blur_index(0x200, 0x80000010, lower_idx, upper_idx, result);

    lower_idx = it->first.val() & 0x7fc00000;
    upper_idx = (it->second.val() - 1) & 0x7fc00000;
    blur_index(0x800, 0x80000020, lower_idx, upper_idx, result);

    lower_idx = it->first.val() & 0x7c000000;
    upper_idx = (it->second.val() - 1) & 0x7c000000;
    blur_index(0x2000, 0x80000040, lower_idx, upper_idx, result);
  }
  result.sort();

  return result;
}


/**--------------------------------------------------------------------------*/

// Calculates the ranges touched by the given bbox.
// This function implicitly depends on the chosen coordinate encoding.
inline Ranges< Uint32_Index > calc_ranges(double south, double north, double west, double east)
{
  Ranges< Uint32_Index > ranges;

  uint32 isouth((south + 91.0)*10000000+0.5);
  uint32 inorth((north + 91.0)*10000000+0.5);
  int32 iwest(west*10000000 + (west > 0 ? 0.5 : -0.5));
  int32 ieast(east*10000000 + (east > 0 ? 0.5 : -0.5));

  if (west <= east)
  {
    if ((west < 0) && (east >= 0))
    {
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   0, ieast & 0xffff0000, 1, ranges);
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   iwest & 0xffff0000, 0xffff0000, 1, ranges);
    }
    else
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   iwest & 0xffff0000, ieast & 0xffff0000, 1, ranges);
  }
  else
  {
    if (west < 0)
    {
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   iwest & 0xffff0000, 0xffff0000, 1, ranges);
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   0, int32(180.0*10000000 + 0.5) & 0xffff0000, 1, ranges);
    }
    else
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   iwest & 0xffff0000, int32(180.0*10000000 + 0.5) & 0xffff0000, 1, ranges);

    if (east > 0)
    {
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   int32(-180.0*10000000 - 0.5) & 0xffff0000, 0xffff0000, 1, ranges);
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   0, ieast & 0xffff0000, 1, ranges);
    }
    else
      recursively_calc_ranges
          (isouth & 0xffff0000, inorth & 0xffff0000,
	   int32(-180.0*10000000 - 0.5) & 0xffff0000, ieast & 0xffff0000, 1, ranges);
  }
  ranges.sort();
  
  return ranges;
}


inline Ranges< Uint32_Index > get_ranges_32(double south, double north, double west, double east)
{
  return ::calc_ranges(south, north, west, east);
}


// Adds recursively the ranges based on the given products of
// closed intervals.
// The expected assertion for the recursion is that the indices are equal
// on the first bitlevel bits. Also, indexes must have the last 16 bit set to zero.
inline void recursively_calc_ranges
    (uint32 south, uint32 north, int32 west, int32 east,
     uint32 bitlevel, Ranges< Uint32_Index >& ranges)
{
  int32 dist = ((0xffff0000u>>bitlevel)&0xffff0000);

  // If the difference is exactly dist, the indices fill the whole square
  // and we can add this square to the ranges.
  if ((south + dist == north) && (west + dist == east))
  {
    ranges.push_back(ll_upper_(south, west), ll_upper_(north, east) + 1);
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
      recursively_calc_ranges
          (south, south | dist, west, west | dist,
	   bitlevel + 1, ranges);
      recursively_calc_ranges
          (south, south | dist, (west | dist) + 0x10000, east,
	   bitlevel + 1, ranges);
      recursively_calc_ranges
          ((south | dist) + 0x10000, north, west, west | dist,
	   bitlevel + 1, ranges);
      recursively_calc_ranges
          ((south | dist) + 0x10000, north, (west | dist) + 0x10000, east,
	   bitlevel + 1, ranges);
    }
    else
    {
      // We don't need to split because east and west lie in the
      // same half of the current square.
      recursively_calc_ranges
          (south, south | dist,  west, east,
	   bitlevel + 1, ranges);
      recursively_calc_ranges
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
      recursively_calc_ranges
          (south, north, west, west | dist,
	   bitlevel + 1, ranges);
      recursively_calc_ranges
          (south, north, (west | dist) + 0x10000, east,
	   bitlevel + 1, ranges);
    }
    else
      // We need no split at all.
      recursively_calc_ranges
          (south, north, west, east, bitlevel + 1, ranges);
  }
}


/*---------------------------------------------------------------------------*/


inline uint32 ilat_(double lat)
{
  return ((lat + 91.0)*10000000+0.5);
}


inline int32 ilon_(double lon)
{
  return (lon*10000000 + (lon > 0 ? 0.5 : -0.5));
}


inline uint32 ll_upper_(double lat, double lon)
{
  uint32 temp = ll_upper(ilat_(lat), ilon_(lon));
  return (temp ^ 0x40000000);
}


inline uint32 ll_lower(uint32 ilat, int32 ilon)
{
  uint32 result(0);

  for (uint32 i(0); i < 16; ++i)
  {
    result |= ((0x1<<i)&ilat)<<(i+1);
    result |= ((0x1<<i)&(uint32)ilon)<<i;
  }

  return result;
}


inline uint32 ll_lower(double lat, double lon)
{
  return ll_lower(ilat_(lat), ilon_(lon));
}


inline uint32 ilat(double lat)
{
  return (lat + 91.0)*10000000+0.5;
}


inline int32 ilon(double lon)
{
  return lon*10000000 + (lon > 0 ? 0.5 : -0.5);
}


// convert interleaved coordinates to plain integers
inline uint32 ilat(uint32 ll_upper, uint32 ll_lower)
{
  uint32 upper = ll_upper & 0xaaaaaaaa;
  upper = (upper | (upper<<1)) & 0xcccccccc;
  upper = (upper | (upper<<2)) & 0xf0f0f0f0;
  upper = (upper | (upper<<4)) & 0xff00ff00;
  upper = (upper | (upper<<8)) & 0xffff0000;

  uint32 lower = ll_lower & 0xaaaaaaaa;
  lower = (lower | (lower>>1)) & 0x99999999;
  lower = (lower | (lower>>2)) & 0x87878787;
  lower = (lower | (lower>>4)) & 0x807f807f;
  lower = (lower | (lower>>8)) & 0x80007fff;
  lower = (lower | (lower>>16)) & 0x0000ffff;

  return (upper | lower);
}


inline double lat(uint32 ll_upper, uint32 ll_lower)
{
  return ((double)ilat(ll_upper, ll_lower))/10000000 - 91.0;
}

inline double lat(uint32 ilat)
{
  return ((double)ilat)/10000000 - 91.0;
}


// convert interleaved coordinates to plain integers
inline int32 ilon(uint32 ll_upper, uint32 ll_lower)
{
  uint32 upper = (ll_upper ^ 0x40000000) & 0x55555555;
  upper = (upper | (upper<<1)) & 0x99999999;
  upper = (upper | (upper<<2)) & 0xe1e1e1e1;
  upper = (upper | (upper<<4)) & 0xfe01fe01;
  upper = (upper | (upper<<8)) & 0xfffe0001;
  upper = (upper | (upper<<16)) & 0xffff0000;

  uint32 lower = ll_lower & 0x55555555;
  lower = (lower | (lower>>1)) & 0x33333333;
  lower = (lower | (lower>>2)) & 0x0f0f0f0f;
  lower = (lower | (lower>>4)) & 0x00ff00ff;
  lower = (lower | (lower>>8)) & 0x0000ffff;

  return (upper | lower);
}


inline double lon(uint32 ll_upper, uint32 ll_lower)
{
  return ((double)ilon(ll_upper, ll_lower))/10000000;
}

inline double lon(int32 ilon)
{
  return ((double)ilon)/10000000;
}


inline bool is_compound_idx(Uint31_Index idx)
{
  return idx.val() & 0x80000000u;
}


#endif
