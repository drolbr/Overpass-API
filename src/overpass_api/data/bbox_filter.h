/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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


struct Bbox_Filter
{
  Bbox_Filter(const Bbox_Double& bbox_) : bbox(bbox_) {}
  const Bbox_Double& get_bbox() const { return bbox; }
  const std::set< std::pair< Uint32_Index, Uint32_Index > >& get_ranges_32() const;
  const std::set< std::pair< Uint31_Index, Uint31_Index > >& get_ranges_31() const;
  
  void filter(Set& into, uint64 timestamp) const;
  
private:
  Bbox_Double bbox;
  mutable std::set< std::pair< Uint32_Index, Uint32_Index > > ranges_32;
  mutable std::set< std::pair< Uint31_Index, Uint31_Index > > ranges_31;
};


inline const std::set< std::pair< Uint32_Index, Uint32_Index > >& Bbox_Filter::get_ranges_32() const
{
  if (ranges_32.empty() && bbox.valid())
    ::get_ranges_32(bbox.south, bbox.north, bbox.west, bbox.east).swap(ranges_32);
  return ranges_32;
}


inline const std::set< std::pair< Uint31_Index, Uint31_Index > >& Bbox_Filter::get_ranges_31() const
{
  if (ranges_31.empty())
    calc_parents(get_ranges_32()).swap(ranges_31);
  return ranges_31;
}


template< typename Index, typename Coord >
void filter_by_bbox(const Bbox_Double& bbox, map< Index, vector< Coord > >& nodes)
{
  uint32 south = ilat_(bbox.south);
  uint32 north = ilat_(bbox.north);
  int32 west = ilon_(bbox.west);
  int32 east = ilon_(bbox.east);

  for (typename map< Index, vector< Coord > >::iterator it = nodes.begin(); it != nodes.end(); ++it)
  {
    vector< Coord > local_into;
    for (typename vector< Coord >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      uint32 lat(::ilat(it->first.val(), iit->ll_lower));
      int32 lon(::ilon(it->first.val(), iit->ll_lower));
      if ((lat >= south) && (lat <= north) &&
          (((lon >= west) && (lon <= east))
            || ((east < west) && ((lon >= west) || (lon <= east)))))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


inline void Bbox_Filter::filter(Set& into, uint64 timestamp) const
{
  if (!bbox.valid())
    return;
    
  // process nodes
  filter_by_bbox(bbox, into.nodes);
  filter_by_bbox(bbox, into.attic_nodes);
  
  const set< pair< Uint31_Index, Uint31_Index > >& ranges = get_ranges_31();
  
  // pre-process ways to reduce the load of the expensive filter
  filter_ways_by_ranges(into.ways, ranges);
  filter_ways_by_ranges(into.attic_ways, ranges);
  
  // pre-filter relations
  filter_relations_by_ranges(into.relations, ranges);
  filter_relations_by_ranges(into.attic_relations, ranges);
  
  //TODO: filter areas
}


#endif
