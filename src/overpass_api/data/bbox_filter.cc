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

#include "bbox_filter.h"


const std::set< std::pair< Uint32_Index, Uint32_Index > >& Bbox_Filter::get_ranges_32() const
{
  if (ranges_32.empty() && bbox.valid())
    ::get_ranges_32(bbox.south, bbox.north, bbox.west, bbox.east).swap(ranges_32);
  return ranges_32;
}


const std::set< std::pair< Uint31_Index, Uint31_Index > >& Bbox_Filter::get_ranges_31() const
{
  if (ranges_31.empty())
    calc_parents(get_ranges_32()).swap(ranges_31);
  return ranges_31;
}


template< typename Index, typename Coord >
void filter_by_bbox(const Bbox_Double& bbox, std::map< Index, std::vector< Coord > >& nodes)
{
  uint32 south = ilat_(bbox.south);
  uint32 north = ilat_(bbox.north);
  int32 west = ilon_(bbox.west);
  int32 east = ilon_(bbox.east);

  for (typename std::map< Index, std::vector< Coord > >::iterator it = nodes.begin(); it != nodes.end(); ++it)
  {
    std::vector< Coord > local_into;
    for (typename std::vector< Coord >::const_iterator iit = it->second.begin();
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


void Bbox_Filter::filter(Set& into, uint64 timestamp) const
{
  if (!bbox.valid())
    return;

  // process nodes
  filter_by_bbox(bbox, into.nodes);
  filter_by_bbox(bbox, into.attic_nodes);

  const std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges = get_ranges_31();

  // pre-process ways to reduce the load of the expensive filter
  filter_ways_by_ranges(into.ways, ranges);
  filter_ways_by_ranges(into.attic_ways, ranges);

  // pre-filter relations
  filter_relations_by_ranges(into.relations, ranges);
  filter_relations_by_ranges(into.attic_relations, ranges);

  //TODO: filter areas
}


bool Bbox_Filter::matches(const std::vector< Quad_Coord >& way_geometry) const
{
  std::vector< Quad_Coord >::const_iterator nit = way_geometry.begin();
  if (nit == way_geometry.end())
    return false;

  // ways with single node only
  if (way_geometry.size() == 1)
    return bbox.contains(Point_Double(::lat(nit->ll_upper, nit->ll_lower), ::lon(nit->ll_upper, nit->ll_lower)));

  Point_Double first(::lat(nit->ll_upper, nit->ll_lower), ::lon(nit->ll_upper, nit->ll_lower));
  for (++nit; nit != way_geometry.end(); ++nit)
  {
    Point_Double second(::lat(nit->ll_upper, nit->ll_lower), ::lon(nit->ll_upper, nit->ll_lower));
    if (bbox.intersects(first, second))
      return true;
    first = second;
  }
  return false;
}


template< typename Way_Skeleton >
void filter_ways_expensive(const Bbox_Filter& filter, const Way_Geometry_Store& way_geometries,
    std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  if (!filter.get_bbox().valid())
    return;

  for (typename std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways.begin();
      it != ways.end(); ++it)
  {
    std::vector< Way_Skeleton > local_into;
    for (typename std::vector< Way_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      if (filter.matches(way_geometries.get_geometry(*iit)))
        local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename Relation_Skeleton >
void filter_relations_expensive(const Bbox_Filter& filter,
    const std::vector< std::pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id,
    const std::vector< std::pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id,
    const Way_Geometry_Store& way_geometries,
    std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations)
{
  for (typename std::map< Uint31_Index, std::vector< Relation_Skeleton > >::iterator it = relations.begin();
      it != relations.end(); ++it)
  {
    std::vector< Relation_Skeleton > local_into;
    for (typename std::vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
    {
      for (std::vector< Relation_Entry >::const_iterator nit = iit->members.begin();
          nit != iit->members.end(); ++nit)
      {
        if (nit->type == Relation_Entry::NODE)
        {
          const std::pair< Uint32_Index, const Node_Skeleton* >* second_nd =
              binary_search_for_pair_id(node_members_by_id, nit->ref);
          if (!second_nd)
            continue;

          if (filter.get_bbox().contains(Point_Double(
              ::lat(second_nd->first.val(), second_nd->second->ll_lower),
              ::lon(second_nd->first.val(), second_nd->second->ll_lower))))
          {
            local_into.push_back(*iit);
            break;
          }
        }
        else if (nit->type == Relation_Entry::WAY)
        {
          const std::pair< Uint31_Index, const Way_Skeleton* >* second_nd =
              binary_search_for_pair_id(way_members_by_id, nit->ref32());
          if (!second_nd)
            continue;
          if (filter.matches(way_geometries.get_geometry(*second_nd->second)))
          {
            local_into.push_back(*iit);
            break;
          }
        }
      }
    }
    it->second.swap(local_into);
  }
}


void Bbox_Filter::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp) const
{
  if (!bbox.valid())
    return;

  //Process ways
  filter_ways_expensive(*this, Way_Geometry_Store(into.ways, query, rman), into.ways);

  {
    //Process relations

    // Retrieve all nodes referred by the relations.
    std::map< Uint32_Index, std::vector< Node_Skeleton > > node_members
        = relation_node_members(&query, rman, into.relations, &get_ranges_32());
    std::vector< std::pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id
        = order_by_id(node_members, Order_By_Node_Id());

    // Retrieve all ways referred by the relations.
    std::map< Uint31_Index, std::vector< Way_Skeleton > > way_members_
        = relation_way_members(&query, rman, into.relations, &get_ranges_31());
    std::vector< std::pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id
        = order_by_id(way_members_, Order_By_Way_Id());

    filter_relations_expensive(*this, node_members_by_id, way_members_by_id,
        Way_Geometry_Store(way_members_, query, rman), into.relations);
  }

  if (timestamp != NOW)
  {
    //Process attic ways
    filter_ways_expensive(*this, Way_Geometry_Store(into.attic_ways, timestamp, query, rman), into.attic_ways);

    //Process attic relations

    // Retrieve all nodes referred by the relations.
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > node_members
        = relation_node_members(&query, rman, into.attic_relations, timestamp, &get_ranges_32());
    std::vector< std::pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id
        = order_attic_by_id(node_members, Order_By_Node_Id());

    // Retrieve all ways referred by the relations.
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > way_members_
        = relation_way_members(&query, rman, into.attic_relations, timestamp, &get_ranges_31());
    std::vector< std::pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id
        = order_attic_by_id(way_members_, Order_By_Way_Id());

    filter_relations_expensive(*this, node_members_by_id, way_members_by_id,
        Way_Geometry_Store(way_members_, timestamp, query, rman), into.attic_relations);
  }

  //TODO: filter areas
}
