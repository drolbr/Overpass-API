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

#include "basic_types.h"

#include <algorithm>
#include <set>
#include <vector>


inline uint32 ll_upper(uint32 ilat, int32 ilon);
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
inline std::set< std::pair< Uint31_Index, Uint31_Index > > calc_parents
    (const std::set< std::pair< Uint31_Index, Uint31_Index > >& total_idxs);

inline std::vector< std::pair< uint32, uint32 > > calc_ranges
    (double south, double north, double west, double east);

inline void recursively_calc_ranges
    (uint32 south, uint32 north, int32 west, int32 east,
     uint32 bitlevel, std::vector< std::pair< uint32, uint32 > >& ranges);

/** ------------------------------------------------------------------------ */

struct Uint31_Compare
{
  bool operator()(uint32 a, uint32 b) const
  {
    return (a & 0x7fffffff) < (b & 0x7fffffff);
  }
};

inline uint32 ll_upper(uint32 ilat, int32 ilon)
{
  uint32 result = 0;

  for (uint32 i(0); i < 16; ++i)
  {
    result |= ((0x1<<(i+16))&ilat)>>(15-i);
    result |= ((0x1<<(i+16))&(uint32)ilon)>>(16-i);
  }

  return result;
}

inline uint32 ll_upper_(uint32 ilat, int32 ilon)
{
  return (ll_upper(ilat, ilon) ^ 0x40000000);
}


inline uint32 upper_ilat(uint32 quadtile)
{
  uint32 result = (quadtile>>1) & 0x15555555;
  result = (result | (result>>1)) & 0x33333333;
  result = (result | (result>>2)) & 0xf0f0f0f;
  result = (result | (result>>4)) & 0xff00ff;
  return (result | (result>>8)) & 0xffff;
}


inline uint32 upper_ilon(uint32 quadtile)
{
  uint32 result = quadtile & 0x55555555;
  result = (result | (result>>1)) & 0x33333333;
  result = (result | (result>>2)) & 0xf0f0f0f;
  result = (result | (result>>4)) & 0xff00ff;
  return (result | (result>>8)) & 0xffff;
}


namespace
{
  struct Bbox_Extent
  {
    Bbox_Extent(uint32 lat, uint32 lon) : lat_min(lat), lat_max(lat), lon_min(lon), lon_max(lon) {}

    Bbox_Extent(uint32 idx)
    {
      set(idx);
    }

    void set(uint32 lat_min_, uint32 lon_min_, uint32 size)
    {
      lat_min = lat_min_;
      lon_min = lon_min_;
      lat_max = ll_upper((upper_ilat(lat_min) + size)<<16, 0);
      lon_max = ll_upper(0, (upper_ilon(lon_min) + size)<<16);
    }

    void set(uint32 idx)
    {
      if (idx & 0x00000001)
        set(idx & 0x2aaaaaa8, idx & 0x55555554, 3);
      else if (idx & 0x00000002)
        set(idx & 0x2aaaaa80, idx & 0x55555540, 0xf);
      else if (idx & 0x00000004)
        set(idx & 0x2aaaa800, idx & 0x55555400, 0x3f);
      else if (idx & 0x00000008)
        set(idx & 0x2aaa8000, idx & 0x55554000, 0xff);
      else if (idx & 0x00000010)
        set(idx & 0x2aa80000, idx & 0x55540000, 0x3ff);
      else if (idx & 0x00000020)
        set(idx & 0x2a800000, idx & 0x55400000, 0xfff);
      else if (idx & 0x00000040)
        set(idx & 0x28000000, idx & 0x54000000, 0x3fff);
      else
        set(0x80000080, 0u, 0u);
    }

    uint32 lat_min, lat_max, lon_min, lon_max;
  };
}


inline uint32 calc_index(const std::vector< uint32 >& node_idxs)
{
  if (node_idxs.empty())
    return 0xfe;

  // Calculate the bounding box of the appearing indices.

  std::vector< uint32 >::const_iterator it = node_idxs.begin();
  if (*it == 0x80000080)
    return 0x80000080;

  Bbox_Extent extent(*it & 0x2aaaaaaa, *it & 0x55555555);

  if (*it & 0x80000000)
    extent.set(*it);

  for (++it; it != node_idxs.end(); ++it)
  {
    if (*it & 0x80000000)
    {
      if (*it == 0x80000080)
        return 0x80000080;

      Bbox_Extent current(*it);

      extent.lat_min = std::min(extent.lat_min, current.lat_min);
      extent.lat_max = std::max(extent.lat_max, current.lat_max);
      extent.lon_min = std::min(extent.lon_min, current.lon_min);
      extent.lon_max = std::max(extent.lon_max, current.lon_max);
    }
    else
    {
      uint32 lat = *it & 0x2aaaaaaa;
      uint32 lon = *it & 0x55555555;

      extent.lat_min = std::min(extent.lat_min, lat);
      extent.lat_max = std::max(extent.lat_max, lat);
      extent.lon_min = std::min(extent.lon_min, lon);
      extent.lon_max = std::max(extent.lon_max, lon);
    }
  }


  // Evaluate the bounding box.

  if ((extent.lat_max == extent.lat_min) && (extent.lon_max == extent.lon_min))
    return node_idxs.front();

  uint32 ilat_min = upper_ilat(extent.lat_min);
  uint32 ilat_max = upper_ilat(extent.lat_max);
  uint32 ilon_min = upper_ilon(extent.lon_min);
  uint32 ilon_max = upper_ilon(extent.lon_max);

  if (((ilat_max & 0xfffe) - (ilat_min & 0xfffe) < 4) &&
      ((ilon_max & 0xfffe) - (ilon_min & 0xfffe) < 4))
    return (((extent.lon_min | extent.lat_min) & 0xfffffffc) | 0x80000001);

  if (((ilat_max & 0xfff8) - (ilat_min & 0xfff8) < 0x10) &&
      ((ilon_max & 0xfff8) - (ilon_min & 0xfff8) < 0x10))
    return (((extent.lon_min | extent.lat_min) & 0xffffffc0) | 0x80000002);

  if (((ilat_max & 0xffe0) - (ilat_min & 0xffe0) < 0x40) &&
      ((ilon_max & 0xffe0) - (ilon_min & 0xffe0) < 0x40))
    return (((extent.lon_min | extent.lat_min) & 0xfffffc00) | 0x80000004);

  if (((ilat_max & 0xff80) - (ilat_min & 0xff80) < 0x100) &&
      ((ilon_max & 0xff80) - (ilon_min & 0xff80) < 0x100))
    return (((extent.lon_min | extent.lat_min) & 0xffffc000) | 0x80000008);

  if (((ilat_max & 0xfe00) - (ilat_min & 0xfe00) < 0x400) &&
      ((ilon_max & 0xfe00) - (ilon_min & 0xfe00) < 0x400))
    return (((extent.lon_min | extent.lat_min) & 0xfffc0000) | 0x80000010);

  if (((ilat_max & 0xf800) - (ilat_min & 0xf800) < 0x1000) &&
      ((ilon_max & 0xf800) - (ilon_min & 0xf800) < 0x1000))
    return (((extent.lon_min | extent.lat_min) & 0xffc00000) | 0x80000020);

  if (((ilat_max & 0xe000) - (ilat_min & 0xe000) < 0x4000) &&
      ((ilon_max & 0xe000) - (ilon_min & 0xe000) < 0x4000))
    return (((extent.lon_min | extent.lat_min) & 0xfc000000) | 0x80000040);

  return 0x80000080;
}


inline uint32 calc_index(const std::vector< Quad_Coord >& arg)
{
  std::vector< uint32 > ll_upper;
  ll_upper.reserve(arg.size());
  for (const auto& i : arg)
    ll_upper.push_back(i.ll_upper);
  return calc_index(ll_upper);
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

inline std::vector< Uint31_Index > calc_children(const std::vector< uint32 >& way_rel_idxs)
{
  std::vector< Uint31_Index > result;

  for (std::vector< uint32 >::const_iterator it = way_rel_idxs.begin();
      it != way_rel_idxs.end(); ++it)
  {
    if (*it & 0x80000000)
    {
      int32 lat = 0;
      int32 lon = 0;
      int32 lat_u = 0;
      int32 lon_u = 0;

      if (*it & 0x00000001)
      {
	lat = upper_ilat(*it & 0x2aaaaaa8);
	lon = upper_ilon(*it & 0x55555554);
	lat_u = lat + 3;
	lon_u = lon + 3;
      }
      else if (*it & 0x00000002)
      {
	lat = upper_ilat(*it & 0x2aaaaa80);
	lon = upper_ilon(*it & 0x55555540);
	lat_u = lat + 0xf;
	lon_u = lon + 0xf;
      }
      else if (*it & 0x00000004)
      {
	lat = upper_ilat(*it & 0x2aaaa800);
	lon = upper_ilon(*it & 0x55555400);
	lat_u = lat + 0x3f;
	lon_u = lon + 0x3f;
      }
      else if (*it & 0x00000008)
      {
	lat = upper_ilat(*it & 0x2aaa8000);
	lon = upper_ilon(*it & 0x55554000);
	lat_u = lat + 0xff;
	lon_u = lon + 0xff;
      }
      else if (*it & 0x00000010)
      {
	lat = upper_ilat(*it & 0x2aa80000);
	lon = upper_ilon(*it & 0x55540000);
	lat_u = lat + 0x3ff;
	lon_u = lon + 0x3ff;
      }
      else if (*it & 0x00000020)
      {
	lat = upper_ilat(*it & 0x2a800000);
	lon = upper_ilon(*it & 0x55400000);
	lat_u = lat + 0xfff;
	lon_u = lon + 0xfff;
      }
      else if (*it & 0x00000040)
      {
	lat = upper_ilat(*it & 0x28000000);
	lon = upper_ilon(*it & 0x54000000);
	lat_u = lat + 0x3fff;
	lon_u = lon + 0x3fff;
      }
      else // *it == 0x80000080
      {
	lat = 0;
	lon = 0;
	lat_u = 0x7fff;
	lon_u = 0xffff;
	result.push_back(0x80000080);
      }

      for (int32 i = lat; i <= (lat_u - 0x3fff); i += 0x2000)
      {
	for (int32 j = lon; j <= (lon_u - 0x3fff); j += 0x2000)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfc000000) | 0x80000040);
      }
      for (int32 i = lat; i <= (lat_u - 0xfff); i += 0x800)
      {
	for (int32 j = lon; j <= (lon_u - 0xfff); j += 0x800)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xffc00000) | 0x80000020);
      }
      for (int32 i = lat; i <= (lat_u - 0x3ff); i += 0x200)
      {
	for (int32 j = lon; j <= (lon_u - 0x3ff); j += 0x200)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfffc0000) | 0x80000010);
      }
      for (int32 i = lat; i <= (lat_u - 0xff); i += 0x80)
      {
	for (int32 j = lon; j <= (lon_u - 0xff); j += 0x80)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xffffc000) | 0x80000008);
      }
      for (int32 i = lat; i <= (lat_u - 0x3f); i += 0x20)
      {
	for (int32 j = lon; j <= (lon_u - 0x3f); j += 0x20)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfffffc00) | 0x80000004);
      }
      for (int32 i = lat; i <= (lat_u - 0xf); i += 8)
      {
	for (int32 j = lon; j <= (lon_u - 0xf); j += 8)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xffffffc0) | 0x80000002);
      }
      for (int32 i = lat; i <= (lat_u - 3); i += 2)
      {
	for (int32 j = lon; j <= (lon_u - 3); j += 2)
	  result.push_back((ll_upper(i<<16, j<<16) & 0xfffffffc) | 0x80000001);
      }
      for (int32 i = lat; i <= lat_u; ++i)
      {
	for (int32 j = lon; j <= lon_u; ++j)
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

inline std::pair< Uint31_Index, Uint31_Index > make_interval(uint32 idx)
{
  return std::make_pair(Uint31_Index(idx), Uint31_Index((idx + 1) & 0x7fffffff));
}

inline void blur_index(uint32 distance, uint32 bitmask, uint32 lower_idx, uint32 upper_idx,
		       std::vector< std::pair< Uint31_Index, Uint31_Index > >& result)
{
  uint32 source_bitmask = 0x10000 - distance;
  uint32 min_lat = upper_ilat(lower_idx) & source_bitmask;
  uint32 min_lon = upper_ilon(lower_idx) & source_bitmask;
  uint32 max_lat = upper_ilat(upper_idx) & source_bitmask;
  uint32 max_lon = upper_ilon(upper_idx) & source_bitmask;
  for (uint32 lat = min_lat - distance; lat <= max_lat; lat += distance)
    result.push_back(make_interval(ll_upper(lat<<16, (min_lon - distance)<<16) | bitmask));
  for (uint32 lon = min_lon; lon <= max_lon; lon += distance)
    result.push_back(make_interval(ll_upper((min_lat - distance)<<16, lon<<16) | bitmask));
  result.push_back(make_interval(ll_upper(max_lat<<16, max_lon<<16) | bitmask));
}

inline void add_decomp_range(const std::pair< Uint32_Index, Uint32_Index >& range,
			     std::vector< std::pair< Uint32_Index, Uint32_Index > >& node_decomp)
{
  uint32 lower = range.first.val();
  while (lower < range.second.val())
  {
    uint32 upper = 0;
    if ((lower & 0x3) != 0 || lower + 4 > range.second.val())
      upper =  lower + 1;
    else if ((lower & 0xf) != 0 || lower + 0x10 > range.second.val())
      upper =  lower + 4;
    else if ((lower & 0x3f) != 0 || lower + 0x40 > range.second.val())
      upper =  lower + 0x10;
    else if ((lower & 0xff) != 0 || lower + 0x100 > range.second.val())
      upper =  lower + 0x40;
    else if ((lower & 0x3ff) != 0 || lower + 0x400 > range.second.val())
      upper =  lower + 0x100;
    else if ((lower & 0xfff) != 0 || lower + 0x1000 > range.second.val())
      upper =  lower + 0x400;
    else if ((lower & 0x3fff) != 0 || lower + 0x4000 > range.second.val())
      upper =  lower + 0x1000;
    else if ((lower & 0xffff) != 0 || lower + 0x10000 > range.second.val())
      upper =  lower + 0x4000;
    else if ((lower & 0x3ffff) != 0 || lower + 0x40000 > range.second.val())
      upper =  lower + 0x10000;
    else if ((lower & 0xfffff) != 0 || lower + 0x100000 > range.second.val())
      upper =  lower + 0x40000;
    else if ((lower & 0x3fffff) != 0 || lower + 0x400000 > range.second.val())
      upper =  lower + 0x100000;
    else if ((lower & 0xffffff) != 0 || lower + 0x1000000 > range.second.val())
      upper =  lower + 0x400000;
    else if ((lower & 0x3ffffff) != 0 || lower + 0x4000000 > range.second.val())
      upper =  lower + 0x1000000;
    else if ((lower & 0xfffffff) != 0 || lower + 0x10000000 > range.second.val())
      upper =  lower + 0x4000000;
    else if ((lower & 0x3fffffff) != 0 || lower + 0x40000000 > range.second.val())
      upper =  lower + 0x10000000;
    else
      upper =  lower + 0x40000000;
    node_decomp.push_back(std::make_pair(lower, upper));
    lower = upper;
  }
}

inline std::set< std::pair< Uint31_Index, Uint31_Index > > calc_parents
    (const std::set< std::pair< Uint32_Index, Uint32_Index > >& node_idxs)
{
  std::vector< std::pair< Uint32_Index, Uint32_Index > > node_decomp;
  for (std::set< std::pair< Uint32_Index, Uint32_Index > >::const_iterator
      it = node_idxs.begin(); it != node_idxs.end(); ++it)
    add_decomp_range(*it, node_decomp);

  std::vector< std::pair< Uint31_Index, Uint31_Index > > result;
  result.push_back(std::make_pair(0x80000080, 0x80000081));

  for (std::vector< std::pair< Uint32_Index, Uint32_Index > >::const_iterator
      it = node_decomp.begin(); it != node_decomp.end(); ++it)
  {
    result.push_back(std::make_pair(it->first.val(), it->second.val()));

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

  sort(result.begin(), result.end());

  std::set< std::pair< Uint31_Index, Uint31_Index > > result_set;
  Uint31_Index last_first = result[0].first;
  Uint31_Index last_second = result[0].second;
  for (std::vector< std::pair< Uint31_Index, Uint31_Index > >::size_type i = 1;
      i < result.size(); ++i)
  {
    if (last_second < result[i].first)
    {
      result_set.insert(std::make_pair(last_first, last_second));
      last_first = result[i].first;
    }
    if (last_second < result[i].second)
      last_second = result[i].second;
  }
  result_set.insert(std::make_pair(last_first, last_second));

  return result_set;
}

/**--------------------------------------------------------------------------*/

// Calculates the ranges touched by the given bbox.
// This function implicitly depends on the chosen coordinate encoding.
inline std::vector< std::pair< uint32, uint32 > > calc_ranges
    (double south, double north, double west, double east)
{
  std::vector< std::pair< uint32, uint32 > > ranges;

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
  return ranges;
}


inline std::set< std::pair< Uint32_Index, Uint32_Index > > get_ranges_32(
    double south, double north, double west, double east)
{
  std::set< std::pair< Uint32_Index, Uint32_Index > > ranges;

  std::vector< std::pair< uint32, uint32 > > uint_ranges = ::calc_ranges(south, north, west, east);
  for (std::vector< std::pair< uint32, uint32 > >::const_iterator
      it(uint_ranges.begin()); it != uint_ranges.end(); ++it)
  {
    std::pair< Uint32_Index, Uint32_Index > range
      (std::make_pair(Uint32_Index(it->first), Uint32_Index(it->second)));
    ranges.insert(range);
  }

  return ranges;
}


// Adds recursively the ranges based on the given products of
// closed intervals.
// The expected assertion for the recursion is that the indices are equal
// on the first bitlevel bits. Also, indexes must have the last 16 bit set to zero.
inline void recursively_calc_ranges
    (uint32 south, uint32 north, int32 west, int32 east,
     uint32 bitlevel, std::vector< std::pair< uint32, uint32 > >& ranges)
{
  int32 dist = ((0xffff0000u>>bitlevel)&0xffff0000);

  // If the difference is exactly dist, the indices fill the whole square
  // and we can add this square to the ranges.
  if ((south + dist == north) && (west + dist == east))
  {
    ranges.push_back
        (std::make_pair(ll_upper_(south, west),
		   ll_upper_(north, east) + 1));
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


template< typename Index >
std::set< std::pair< Index, Index > > intersect_ranges
    (const std::set< std::pair< Index, Index > >& range_a,
     const std::set< std::pair< Index, Index > >& range_b)
{
  std::set< std::pair< Index, Index > > result;
  typename std::set< std::pair< Index, Index > >::const_iterator it_a = range_a.begin();
  typename std::set< std::pair< Index, Index > >::const_iterator it_b = range_b.begin();

  while (it_a != range_a.end() && it_b != range_b.end())
  {
    if (!(it_a->first < it_b->second))
      ++it_b;
    else if (!(it_b->first < it_a->second))
      ++it_a;
    else if (it_b->second < it_a->second)
    {
      result.insert(std::make_pair(std::max(it_a->first, it_b->first), it_b->second));
      ++it_b;
    }
    else
    {
      result.insert(std::make_pair(std::max(it_a->first, it_b->first), it_a->second));
      ++it_a;
    }
  }

  return result;
}


#endif
