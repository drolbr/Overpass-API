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

#include "geometry_from_quad_coords.h"


Opaque_Geometry* make_linestring_way_geom(const std::vector< Quad_Coord >& geometry)
{
  bool is_complete = true;
  for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    is_complete &= (it->ll_upper != 0 || it->ll_lower != 0);

  if (is_complete)
  {
    std::vector< Point_Double > coords;
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
      coords.push_back(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
    return new Linestring_Geometry(coords);
  }
  else
  {
    Partial_Way_Geometry* pw_geom = new Partial_Way_Geometry();
    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    {
      if (it->ll_upper != 0 || it->ll_lower != 0)
        pw_geom->add_point(Point_Double(::lat(it->ll_upper, it->ll_lower), ::lon(it->ll_upper, it->ll_lower)));
      else
        pw_geom->add_point(Point_Double(100., 200.));
    }
    return pw_geom;
  }
  return 0;
}


const Opaque_Geometry& Geometry_From_Quad_Coords::make_way_geom(
    const Way_Skeleton& skel, unsigned int mode, Way_Bbox_Geometry_Store* store)
{
  delete geom;
  geom = 0;

  if (store && (mode & Output_Mode::GEOMETRY))
    geom = make_linestring_way_geom(store->get_geometry(skel));
  else if (store && ((mode & Output_Mode::BOUNDS) || (mode & Output_Mode::CENTER)))
  {
    std::vector< Quad_Coord > geometry = store->get_geometry(skel);

    double min_lat = 100.;
    double max_lat = -100.;
    double min_lon = 200.;
    double max_lon = -200.;

    for (std::vector< Quad_Coord >::const_iterator it = geometry.begin(); it != geometry.end(); ++it)
    {
      double lat = ::lat(it->ll_upper, it->ll_lower);
      min_lat = std::min(min_lat, lat);
      max_lat = std::max(max_lat, lat);
      double lon = ::lon(it->ll_upper, it->ll_lower);
      min_lon = std::min(min_lon, lon);
      max_lon = std::max(max_lon, lon);
    }

    if (mode & Output_Mode::BOUNDS)
      geom = new Bbox_Geometry(min_lat, min_lon, max_lat, max_lon);
    else
      geom = new Point_Geometry((min_lat + max_lat) / 2., (min_lon + max_lon) / 2.);
  }
  else
    geom = new Null_Geometry();

  return *geom;
}


const Opaque_Geometry& Geometry_From_Quad_Coords::make_way_geom(
    const std::vector< Quad_Coord >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds)
{
  delete geom;
  geom = 0;

  if (geometry && !geometry->empty())
    geom = make_linestring_way_geom(*geometry);
  else if (bounds && (bounds->first.ll_upper | bounds->first.ll_lower))
  {
    if (bounds->second)
      geom = new Bbox_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
                           ::lon(bounds->first.ll_upper, bounds->first.ll_lower),
                           ::lat(bounds->second->ll_upper, bounds->second->ll_lower),
                           ::lon(bounds->second->ll_upper, bounds->second->ll_lower));
    else
      geom = new Point_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
                            ::lon(bounds->first.ll_upper, bounds->first.ll_lower));
  }
  else
    geom = new Null_Geometry();

  return *geom;
}


Opaque_Geometry* make_verbatim_rel_geom(const std::vector< std::vector< Quad_Coord > >& geometry)
{
  bool is_complete = true;
  for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
      it != geometry.end(); ++it)
  {
    if (it->empty())
      is_complete = false;
    else if (it->size() == 1)
      is_complete &= ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0);
    else
    {
      for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        is_complete &= (it2->ll_upper != 0 || it2->ll_lower != 0);
    }
  }

  if (is_complete)
  {
    Compound_Geometry* cp_geom = new Compound_Geometry();
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->empty())
        cp_geom->add_component(new Null_Geometry());
      else if (it->size() == 1)
        cp_geom->add_component(new Point_Geometry(
            ::lat(it->front().ll_upper, it->front().ll_lower),
            ::lon(it->front().ll_upper, it->front().ll_lower)));
      else
      {
        std::vector< Point_Double > coords;
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          coords.push_back(Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
        cp_geom->add_component(new Linestring_Geometry(coords));
      }
    }
    return cp_geom;
  }
  else if (geometry.empty())
    return new Null_Geometry();
  else
  {
    Partial_Relation_Geometry* pr_geom = new Partial_Relation_Geometry();
    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->empty())
        pr_geom->add_placeholder();
      else if (it->size() == 1 && ((*it)[0].ll_upper != 0 || (*it)[0].ll_lower != 0))
        pr_geom->add_point(Point_Double(
              ::lat(it->front().ll_upper, it->front().ll_lower),
              ::lon(it->front().ll_upper, it->front().ll_lower)));
      else
      {
        pr_geom->start_way();
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        {
          if (it2->ll_upper != 0 || it2->ll_lower != 0)
            pr_geom->add_way_point(
                Point_Double(::lat(it2->ll_upper, it2->ll_lower), ::lon(it2->ll_upper, it2->ll_lower)));
          else
            pr_geom->add_way_placeholder();
        }
      }
    }
    return pr_geom;
  }
  return 0;
}


const Opaque_Geometry& Geometry_From_Quad_Coords::make_relation_geom(
    const Relation_Skeleton& skel, unsigned int mode, Relation_Geometry_Store* store)
{
  delete geom;
  geom = 0;

  if (store && (mode & Output_Mode::GEOMETRY))
    geom = make_verbatim_rel_geom(store->get_geometry(skel));
  else if (store && ((mode & Output_Mode::BOUNDS) || (mode & Output_Mode::CENTER)))
  {
    std::vector< std::vector< Quad_Coord > > geometry = store->get_geometry(skel);

    double min_lat = 100.;
    double max_lat = -100.;
    double min_lon = 200.;
    double max_lon = -200.;

    for (std::vector< std::vector< Quad_Coord > >::const_iterator it = geometry.begin();
        it != geometry.end(); ++it)
    {
      if (it->size() == 1)
      {
        double lat = ::lat((*it)[0].ll_upper, (*it)[0].ll_lower);
        min_lat = std::min(min_lat, lat);
        max_lat = std::max(max_lat, lat);
        double lon = ::lon((*it)[0].ll_upper, (*it)[0].ll_lower);
        min_lon = std::min(min_lon, lon);
        max_lon = std::max(max_lon, lon);
      }
      else if (!it->empty())
      {
        for (std::vector< Quad_Coord >::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        {
          double lat = ::lat(it2->ll_upper, it2->ll_lower);
          min_lat = std::min(min_lat, lat);
          max_lat = std::max(max_lat, lat);
          double lon = ::lon(it2->ll_upper, it2->ll_lower);
          min_lon = std::min(min_lon, lon);
          max_lon = std::max(max_lon, lon);
        }
      }
    }

    if (min_lat <= max_lat)
    {
      if (mode & Output_Mode::BOUNDS)
        geom = new Bbox_Geometry(min_lat, min_lon, max_lat, max_lon);
      else
        geom = new Point_Geometry((min_lat + max_lat) / 2., (min_lon + max_lon) / 2.);
    }
    else
      geom = new Null_Geometry();
  }
  else
    geom = new Null_Geometry();

  return *geom;
}


const Opaque_Geometry& Geometry_From_Quad_Coords::make_relation_geom(
    const std::vector< std::vector< Quad_Coord > >* geometry, const std::pair< Quad_Coord, Quad_Coord* >* bounds)
{
  delete geom;
  geom = 0;

  if (geometry)
    geom = make_verbatim_rel_geom(*geometry);
  else if (bounds)
  {
    if (bounds->second)
      geom = new Bbox_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
          ::lon(bounds->first.ll_upper, bounds->first.ll_lower),
          ::lat(bounds->second->ll_upper, bounds->second->ll_lower),
          ::lon(bounds->second->ll_upper, bounds->second->ll_lower));
    else
      geom = new Point_Geometry(::lat(bounds->first.ll_upper, bounds->first.ll_lower),
          ::lon(bounds->first.ll_upper, bounds->first.ll_lower));
  }
  else
    geom = new Null_Geometry();

  return *geom;
}
