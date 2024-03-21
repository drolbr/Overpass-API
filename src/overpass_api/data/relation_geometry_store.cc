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

#include "collect_members.h"
#include "relation_geometry_store.h"


Relation_Geometry_Store::~Relation_Geometry_Store()
{
  delete way_geometry_store;
}


Relation_Geometry_Store::Relation_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
     const Statement& query, Resource_Manager& rman,
     double south_, double north_, double west_, double east_)
    : way_geometry_store(0), south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{
  if (relations.empty())
  {
    // Turn off bounding box, because it isn't needed anyway
    north = 0;
    south = 1;
  }
  Request_Context context(&query, rman);

  Ranges< Uint32_Index > node_ranges(south <= north
      ? get_ranges_32(south_, north_, west_, east_) : Ranges< Uint32_Index >::global());

  // Retrieve all nodes referred by the relations.
  Timeless< Uint32_Index, Node_Skeleton > node_members
      = relation_node_members(context, relations, {}, node_ranges, {}, true);

  // Order node ids by id.
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it = node_members.current.begin();
      it != node_members.current.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());

  Ranges< Uint31_Index > way_ranges(south <= north
      ? calc_parents(node_ranges) : Ranges< Uint31_Index >::global());

  // Retrieve all ways referred by the relations.
  Timeless< Uint31_Index, Way_Skeleton > way_members
      = relation_way_members(context, relations, {}, way_ranges, {}, true);

  way_geometry_store = new Way_Geometry_Store(way_members.current, query, rman);

  // Order way ids by id.
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = way_members.current.begin();
      it != way_members.current.end(); ++it)
  {
    for (std::vector< Way_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      ways.push_back(*iit);
  }
  sort(ways.begin(), ways.end());
}


Relation_Geometry_Store::Relation_Geometry_Store
    (const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
     const Statement& query, Resource_Manager& rman,
     double south_, double north_, double west_, double east_)
    : way_geometry_store(0), south(ilat_(south_)), north(ilat_(north_)), west(ilon_(west_)), east(ilon_(east_))
{
  if (relations.empty())
  {
    // Turn off bounding box, because it isn't needed anyway
    north = 0;
    south = 1;
  }
  Request_Context context(&query, rman);

  Ranges< Uint32_Index > node_ranges(south <= north
      ? get_ranges_32(south_, north_, west_, east_) : Ranges< Uint32_Index >::global());

  // Retrieve all nodes referred by the relations.
  auto nodes_by_idx = relation_node_members(context, {}, relations, node_ranges, {}, true);

  // Order node ids by id.
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator it = nodes_by_idx.current.begin();
      it != nodes_by_idx.current.end(); ++it)
  {
    for (std::vector< Node_Skeleton >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::iterator it = nodes_by_idx.attic.begin();
      it != nodes_by_idx.attic.end(); ++it)
  {
    for (std::vector< Attic< Node_Skeleton > >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      nodes.push_back(Node(iit->id, it->first.val(), iit->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());

  Ranges< Uint31_Index > way_ranges(south <= north
      ? calc_parents(node_ranges) : Ranges< Uint31_Index >::global());

  // Retrieve all ways referred by the relations.
  Timeless< Uint31_Index, Way_Skeleton > ways_by_idx_pair
      = relation_way_members(context,
          std::map< Uint31_Index, std::vector< Relation_Skeleton > >(), relations,
          way_ranges, {}, true);
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > ways_by_idx;
  keep_matching_skeletons(ways_by_idx, ways_by_idx_pair.current, ways_by_idx_pair.attic,
      rman.get_desired_timestamp());

  way_geometry_store = new Way_Geometry_Store(ways_by_idx, query, rman);

  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::iterator it = ways_by_idx.begin();
      it != ways_by_idx.end(); ++it)
  {
    for (std::vector< Attic< Way_Skeleton > >::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      ways.push_back(*iit);
  }
  sort(ways.begin(), ways.end());
}


bool Relation_Geometry_Store::matches_bbox(uint32 ll_upper, uint32 ll_lower) const
{
  if (north < south)
    return true;
  uint32 lat(::ilat(ll_upper, ll_lower));
  int32 lon(::ilon(ll_upper, ll_lower));
  return (lat >= south && lat <= north &&
     ((lon >= west && lon <= east)
            || (east < west && (lon >= west || lon <= east))));
}


std::vector< std::vector< Quad_Coord > > Relation_Geometry_Store::get_geometry
    (const Relation_Skeleton& relation) const
{
  std::vector< std::vector< Quad_Coord > > result;
  for (std::vector< Relation_Entry >::const_iterator it = relation.members.begin();
       it != relation.members.end(); ++it)
  {
    if (it->type == Relation_Entry::NODE)
    {
      const Node* node = binary_search_for_id(nodes, it->ref);
      if (node == 0)
        result.push_back(std::vector< Quad_Coord >());
      else if (!matches_bbox(node->index, node->ll_lower_))
        result.push_back(std::vector< Quad_Coord >(1, Quad_Coord(0u, 0u)));
      else
        result.push_back(std::vector< Quad_Coord >(1, Quad_Coord(node->index, node->ll_lower_)));
    }
    else if (it->type == Relation_Entry::WAY)
    {
      const Way_Skeleton* way = binary_search_for_id(ways, Way_Skeleton::Id_Type(it->ref.val()));
      if (way == 0)
        result.push_back(std::vector< Quad_Coord >());
      else
      {
        result.push_back(way_geometry_store->get_geometry(*way));
        if (result.back().empty())
          ;
        else if (result.back().size() == 1)
        {
          if (!matches_bbox(result.back().begin()->ll_upper, result.back().begin()->ll_lower))
            *result.back().begin() = Quad_Coord(0u, 0u);
        }
        else
        {
          bool this_matches = matches_bbox(result.back()[0].ll_upper, result.back()[0].ll_lower);
          bool next_matches = matches_bbox(result.back()[1].ll_upper, result.back()[1].ll_lower);
          if (!this_matches && !next_matches)
            result.back()[0] = Quad_Coord(0u, 0u);
          for (uint i = 1; i < result.back().size() - 1; ++i)
          {
            bool last_matches = this_matches;
            this_matches = next_matches;
            next_matches = matches_bbox(result.back()[i+1].ll_upper, result.back()[i+1].ll_lower);
            if (!last_matches && !this_matches && !next_matches)
              result.back()[i] = Quad_Coord(0u, 0u);
          }
          if (!this_matches && !next_matches)
            result.back()[result.back().size()-1] = Quad_Coord(0u, 0u);
        }
      }
    }
    else if (it->type == Relation_Entry::RELATION)
      result.push_back(std::vector< Quad_Coord >());
  }

  return result;
}
