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

#ifndef DE__OSM3S___OVERPASS_API__DATA__WAY_GEOMETRY_STORE_H
#define DE__OSM3S___OVERPASS_API__DATA__WAY_GEOMETRY_STORE_H

#include "../core/datatypes.h"
#include "../statements/statement.h"
// #include "abstract_processing.h"
// #include "filenames.h"

#include <map>
#include <vector>


class Way_Geometry_Store
{
public:
  Way_Geometry_Store(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
                     const Statement& query, Resource_Manager& rman);
  Way_Geometry_Store(const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
                     const Statement& query, Resource_Manager& rman);
  
  // return the empty std::vector if the way is not found
  std::vector< Quad_Coord > get_geometry(const Way_Skeleton& way) const;
  
private:
  std::vector< Node > nodes;
};


class Way_Bbox_Geometry_Store : public Way_Geometry_Store
{
public:
  Way_Bbox_Geometry_Store(const map< Uint31_Index, vector< Way_Skeleton > >& ways,
                     const Statement& query, Resource_Manager& rman,
                     double south_, double north_, double west_, double east_);
  Way_Bbox_Geometry_Store(const map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways, uint64 timestamp,
                     const Statement& query, Resource_Manager& rman,
                     double south_, double north_, double west_, double east_);
  
  // return the empty vector if the way is not found
  vector< Quad_Coord > get_geometry(const Way_Skeleton& way) const;
  
private:
  uint32 south;
  uint32 north;
  int32 west;
  int32 east;
  
  bool matches_bbox(uint32 ll_upper, uint32 ll_lower) const;
};


#endif
