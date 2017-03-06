/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#include "../data/collect_members.h"
#include "../data/way_geometry_store.h"

#include <map>


template< typename Object >
std::set< std::pair< Uint32_Index, Uint32_Index > > small_way_nd_indices
    (const Statement* stmt, Resource_Manager& rman,
     typename std::map< Uint31_Index, std::vector< Object > >::const_iterator ways_begin,
     typename std::map< Uint31_Index, std::vector< Object > >::const_iterator ways_end)
{
  std::vector< uint32 > parents;
  
  for (typename std::map< Uint31_Index, std::vector< Object > >::const_iterator
    it(ways_begin); it != ways_end; ++it)
  {
    if (!(it->first.val() & 0x80000000) || ((it->first.val() & 0x1) != 0)) // Adapt 0x3
      parents.push_back(it->first.val());
  }
  sort(parents.begin(), parents.end());
  parents.erase(unique(parents.begin(), parents.end()), parents.end());
  
  if (stmt)
    rman.health_check(*stmt);
  
  return collect_node_req(stmt, rman, std::vector< Node::Id_Type >(), parents);
}


template< typename Object >
std::vector< Node::Id_Type > small_way_nd_ids(const std::map< Uint31_Index, std::vector< Object > >& ways)
{
  std::vector< Node::Id_Type > ids;
  for (typename std::map< Uint31_Index, std::vector< Object > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
  {
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x1) == 0))
      continue;
    for (typename std::vector< Object >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      for (std::vector< Node::Id_Type >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
        ids.push_back(*it3);
    }
  }
  
  sort(ids.begin(), ids.end());
  ids.erase(unique(ids.begin(), ids.end()), ids.end());
  
  return ids;
}


std::map< Uint32_Index, std::vector< Node_Skeleton > > small_way_members
    (const Statement* stmt, Resource_Manager& rman,
     const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  std::map< Uint32_Index, std::vector< Node_Skeleton > > result;
  
  collect_items_range(stmt, rman, *osm_base_settings().NODES,
                      small_way_nd_indices< Way_Skeleton >(stmt, rman, ways.begin(), ways.end()),
                      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), result);
  
  return result;
}


Way_Geometry_Store::Way_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways, const Statement& query, Resource_Manager& rman)
{
  // Retrieve all nodes referred by the ways.
  std::map< Uint32_Index, std::vector< Node_Skeleton > > way_members_ = small_way_members(&query, rman, ways);
  
  // Order node ids by id.
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it = way_members_.begin();
      it != way_members_.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());
}


Way_Geometry_Store::Way_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
     const Statement& query, Resource_Manager& rman)
{
  // Retrieve all nodes referred by the ways.
  std::map< Uint32_Index, std::vector< Node_Skeleton > > current;
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > attic;
  collect_items_range_by_timestamp(&query, rman, 
      small_way_nd_indices< Attic< Way_Skeleton > >(&query, rman, ways.begin(), ways.end()),
      Id_Predicate< Node_Skeleton >(small_way_nd_ids(ways)), current, attic);
  
  keep_matching_skeletons(nodes, current, attic, timestamp);
}


std::vector< Quad_Coord > Way_Geometry_Store::get_geometry(const Way_Skeleton& way) const
{
  if (way.geometry.empty())
    return make_geometry(way, nodes);
  else
    return way.geometry;
}


Way_Bbox_Geometry_Store::Way_Bbox_Geometry_Store(
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
    const Statement& query, Resource_Manager& rman,
    double south_, double north_, double west_, double east_)
  : Way_Geometry_Store(ways, query, rman),
    south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{}


Way_Bbox_Geometry_Store::Way_Bbox_Geometry_Store(
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
    const Statement& query, Resource_Manager& rman,
    double south_, double north_, double west_, double east_)
  : Way_Geometry_Store(ways, timestamp, query, rman),
    south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{}


bool Way_Bbox_Geometry_Store::matches_bbox(uint32 ll_upper, uint32 ll_lower) const
{
  if (north < south)
    return true;
  uint32 lat(::ilat(ll_upper, ll_lower));
  int32 lon(::ilon(ll_upper, ll_lower));
  return (lat >= south && lat <= north &&
     ((lon >= west && lon <= east)
            || (east < west && (lon >= west || lon <= east))));
}


std::vector< Quad_Coord > Way_Bbox_Geometry_Store::get_geometry(const Way_Skeleton& way) const
{
  std::vector< Quad_Coord > result = Way_Geometry_Store::get_geometry(way);
  
  if (result.empty())
    ;
  else if (result.size() == 1)
  {
    if (!matches_bbox(result.begin()->ll_upper, result.begin()->ll_lower))
      *result.begin() = Quad_Coord(0u, 0u);
  }
  else
  {
    bool this_matches = matches_bbox(result[0].ll_upper, result[0].ll_lower);
    bool next_matches = matches_bbox(result[1].ll_upper, result[1].ll_lower);
    if (!this_matches && !next_matches)
      result[0] = Quad_Coord(0u, 0u);
    for (uint i = 1; i < result.size() - 1; ++i)
    {
      bool last_matches = this_matches;
      this_matches = next_matches;
      next_matches = matches_bbox(result[i+1].ll_upper, result[i+1].ll_lower);
      if (!last_matches && !this_matches && !next_matches)
        result[i] = Quad_Coord(0u, 0u);
    }
    if (!this_matches && !next_matches)
      result[result.size()-1] = Quad_Coord(0u, 0u);
  }
        
  return result;
}
