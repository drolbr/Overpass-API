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

#ifndef DE__OSM3S___OVERPASS_API__DATA__BBOX_FILTER_H
#define DE__OSM3S___OVERPASS_API__DATA__BBOX_FILTER_H

#include "../core/datatypes.h"
#include "../core/geometry.h"
#include "collect_members.h"
#include "way_geometry_store.h"


struct Bbox_Filter
{
  Bbox_Filter(const Bbox_Double& bbox_) : bbox(bbox_) {}
  const Bbox_Double& get_bbox() const { return bbox; }
  const Ranges< Uint32_Index >& get_ranges_32() const;
  const Ranges< Uint31_Index >& get_ranges_31() const;

  bool matches(const std::vector< Quad_Coord >& way_geometry) const;

  void filter(Set& into) const;
  void filter(const Statement& query, Resource_Manager& rman, Set& into, bool with_attic) const;

private:
  Bbox_Double bbox;
  mutable Ranges< Uint32_Index > ranges_32;
  mutable Ranges< Uint31_Index > ranges_31;
};


#endif
