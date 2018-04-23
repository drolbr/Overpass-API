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

#ifndef DE__OSM3S___OVERPASS_API__DATA__GEOMETRY_FROM_QUAD_COORDS_H
#define DE__OSM3S___OVERPASS_API__DATA__GEOMETRY_FROM_QUAD_COORDS_H


#include "../data/relation_geometry_store.h"
#include "../data/way_geometry_store.h"


struct Geometry_From_Quad_Coords
{
  Geometry_From_Quad_Coords() : geom(0) {}
  ~Geometry_From_Quad_Coords() { delete geom; }

  const Opaque_Geometry& make_way_geom(const Way_Skeleton& skel, unsigned int mode, Way_Bbox_Geometry_Store* store);
  const Opaque_Geometry& make_relation_geom(
      const Relation_Skeleton& skel, unsigned int mode, Relation_Geometry_Store* store);

  const Opaque_Geometry& make_way_geom(
      const std::vector< Quad_Coord >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds);
  const Opaque_Geometry& make_relation_geom(
      const std::vector< std::vector< Quad_Coord > >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds);

private:
  Opaque_Geometry* geom;
};


#endif
