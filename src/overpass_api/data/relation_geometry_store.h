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

#ifndef DE__OSM3S___OVERPASS_API__DATA__RELATION_GEOMETRY_STORE_H
#define DE__OSM3S___OVERPASS_API__DATA__RELATION_GEOMETRY_STORE_H

#include "way_geometry_store.h"

#include <map>
#include <vector>


class Relation_Geometry_Store
{
public:
  Relation_Geometry_Store
      (const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations,
      const Statement& query, Resource_Manager& rman,
      double south_ = 1., double north_ = 0., double west_ = 0., double east_ = 0.);
  Relation_Geometry_Store
      (const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations,
      const Statement& query, Resource_Manager& rman,
      double south_ = 1., double north_ = 0., double west_ = 0., double east_ = 0.);

  ~Relation_Geometry_Store();

  // return the empty vector if the relation is not found
  std::vector< std::vector< Quad_Coord > > get_geometry(const Relation_Skeleton& relation) const;

private:
  std::vector< Node > nodes;
  std::vector< Way_Skeleton > ways;
  Way_Geometry_Store* way_geometry_store;

  uint32 south;
  uint32 north;
  int32 west;
  int32 east;

  bool matches_bbox(uint32 ll_upper, uint32 ll_lower) const;
};


#endif
