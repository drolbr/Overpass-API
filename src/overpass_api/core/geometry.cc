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

#include "four_field_index.h"
#include "geometry.h"
#include "index_computations.h"

#include <cmath>
#include <limits>
#include <map>


const Bbox_Double Bbox_Double::invalid(100.0, 200.0, 100.0, 200.0);


double Bbox_Double::center_lat() const
{
  return (south + north) / 2;
}


double Bbox_Double::center_lon() const
{
  if (west <= east)
    return (west + east) / 2;
  else if ((west + east) / 2 <= 0)
    return (west + east) / 2 + 180.0;
  else
    return (west + east) / 2 - 180.0;
}


bool Bbox_Double::contains(const Point_Double& point) const
{
  if (point.lat < south || point.lat > north)
    return false;

  if (east >= west)
    return point.lon >= west && point.lon <= east;

  return point.lon >= west || point.lon <= east;
}


bool Bbox_Double::intersects(const Point_Double& from, const Point_Double& to) const
{
  double from_lon = from.lon;
  double to_lon = to.lon;
  if (from.lon < 0. && to.lon - from.lon > 180.)
    from_lon += 360.;
  if (to.lon < 0. && from.lon - to.lon > 180.)
    to_lon += 360.;

  double delta_from = 0;
  if (from.lat < south)
  {
    if (to.lat < south)
      return false;
    // Otherwise just adjust from.lat and from.lon
    delta_from = (to_lon - from_lon)*(south - from.lat)/(to.lat - from.lat);
  }
  else if (from.lat > north)
  {
    if (to.lat > north)
      return false;
    // Otherwise just adjust from.lat and from.lon
    delta_from = (to_lon - from_lon)*(north - from.lat)/(to.lat - from.lat);
  }

  if (to.lat < south)
    // Adjust to.lat and to.lon
    to_lon += (from_lon - to_lon)*(south - to.lat)/(from.lat - to.lat);
  else if (to.lat > north)
    // Adjust to.lat and to.lon
    to_lon += (from_lon - to_lon)*(north - to.lat)/(from.lat - to.lat);
  from_lon += delta_from;

  // Now we know that both latitudes are between south and north.
  // Thus we only need to check whether the segment touches the bbox in its east-west-extension.
  // Note that the lons have now values between -180.0 and 360.0.
  double min_lon = std::min(from_lon, to_lon);
  double max_lon = std::max(from_lon, to_lon);
  if (west <= east)
  {
    if (max_lon < 180.)
      return min_lon <= east && max_lon >= west;
    else if (min_lon > 180.)
      return min_lon - 360. <= east && max_lon - 360. >= west;

    return min_lon <= east && max_lon - 360. >= west;
  }

  if (max_lon < 180.)
    return min_lon <= east || max_lon >= west;
  else if (min_lon > 180.)
    return min_lon - 360. <= east || max_lon - 360. >= west;

  return true;
}


bool Point_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  return bbox.contains(pt);
}


Bbox_Double* calc_bounds(const std::vector< Point_Double >& points)
{
  double south = 100.0;
  double west = 200.0;
  double north = -100.0;
  double east = -200.0;

  for (std::vector< Point_Double >::const_iterator it = points.begin(); it != points.end(); ++it)
  {
    if (it->lat < 100.)
    {
      south = std::min(south, it->lat);
      west = std::min(west, it->lon);
      north = std::max(north, it->lat);
      east = std::max(east, it->lon);
    }
  }

  if (north == -100.0)
    return new Bbox_Double(Bbox_Double::invalid);
  else if (east - west > 180.0)
    // In this special case we should check whether the bounding box should rather cross the date line
  {
    double wrapped_west = 180.0;
    double wrapped_east = -180.0;

    for (std::vector< Point_Double >::const_iterator it = points.begin(); it != points.end(); ++it)
    {
      if (it->lat < 100.)
      {
        if (it->lon > 0)
	  wrapped_west = std::min(wrapped_west, it->lon);
        else
	  wrapped_east = std::max(wrapped_east, it->lon);
      }
    }

    if (wrapped_west - wrapped_east > 180.0)
      return new Bbox_Double(south, wrapped_west, north, wrapped_east);
    else
      // The points go around the world, hence a bounding box limit doesn't make sense.
      return new Bbox_Double(south, -180.0, north, 180.0);
  }
  else
    return new Bbox_Double(south, west, north, east);
}


double Linestring_Geometry::center_lat() const
{
  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->center_lat();
}


double Linestring_Geometry::center_lon() const
{
  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->center_lon();
}


double Linestring_Geometry::south() const
{
  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->south;
}


double Linestring_Geometry::north() const
{
  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->north;
}


double Linestring_Geometry::west() const
{
  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->west;
}


double Linestring_Geometry::east() const
{
  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->east;
}


bool Linestring_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  for (std::vector< Point_Double >::const_iterator it = points.begin(); it != points.end(); ++it)
  {
    if (bbox.contains(*it))
      return true;
  }

  for (uint i = 1; i < points.size(); ++i)
  {
    if (bbox.intersects(points[i-1], points[i]))
      return true;
  }

  return false;
}


double Partial_Way_Geometry::center_lat() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->center_lat();
}


double Partial_Way_Geometry::center_lon() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->center_lon();
}


double Partial_Way_Geometry::south() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->south;
}


double Partial_Way_Geometry::north() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->north;
}


double Partial_Way_Geometry::west() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->west;
}


double Partial_Way_Geometry::east() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(points);

  return bounds->east;
}


Partial_Way_Geometry::Partial_Way_Geometry(const std::vector< Point_Double >& points_)
    : points(points_), bounds(0), has_coords(false)
{
  for (std::vector< Point_Double >::const_iterator it = points.begin(); it != points.end() && !has_coords; ++it)
    has_coords |= (it->lat < 100.);

  if (has_coords)
  {
    valid_segments.push_back(std::vector< Point_Double >());
    if (!points.empty() && points.front().lat < 100.)
      valid_segments.back().push_back(points.front());
    for (unsigned int i = 1; i < points.size(); ++i)
    {
      if (points[i].lat < 100.)
      {
        if (points[i-1].lat >= 100.)
          valid_segments.push_back(std::vector< Point_Double >());
        valid_segments.back().push_back(points[i]);
      }
    }
  }
}

void Partial_Way_Geometry::add_point(const Point_Double& point)
{
  delete bounds;
  bounds = 0;
  if (point.lat < 100.)
  {
    if (points.empty() || (points.back().lat >= 100.))
      valid_segments.push_back(std::vector< Point_Double >());
    valid_segments.back().push_back(point);
    has_coords = true;
  }
  points.push_back(point);
}


bool Partial_Way_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  for (std::vector< Point_Double >::const_iterator it = points.begin(); it != points.end(); ++it)
  {
    if (it->lat < 100. && bbox.contains(*it))
      return true;
  }

  for (uint i = 1; i < points.size(); ++i)
  {
    if (points[i-1].lat < 100. && points[i].lat < 100. && bbox.intersects(points[i-1], points[i]))
      return true;
  }

  return false;
}


Bbox_Double* calc_bounds(const std::vector< std::vector< Point_Double > >& linestrings)
{
  double south = 100.0;
  double west = 200.0;
  double north = -100.0;
  double east = -200.0;

  for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
      iti != linestrings.end(); ++iti)
  {
    for (std::vector< Point_Double >::const_iterator it = iti->begin(); it != iti->end(); ++it)
    {
      if (it->lat < 100.)
      {
        south = std::min(south, it->lat);
        west = std::min(west, it->lon);
        north = std::max(north, it->lat);
        east = std::max(east, it->lon);
      }
    }
  }

  if (north == -100.0)
    return new Bbox_Double(Bbox_Double::invalid);
  else if (east - west > 180.0)
    // In this special case we should check whether the bounding box should rather cross the date line
  {
    double wrapped_west = 180.0;
    double wrapped_east = -180.0;

    for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
        iti != linestrings.end(); ++iti)
    {
      for (std::vector< Point_Double >::const_iterator it = iti->begin(); it != iti->end(); ++it)
      {
        if (it->lat < 100.)
        {
          if (it->lon > 0)
            wrapped_west = std::min(wrapped_west, it->lon);
          else
            wrapped_east = std::max(wrapped_east, it->lon);
        }
      }
    }

    if (wrapped_west - wrapped_east > 180.0)
      return new Bbox_Double(south, wrapped_west, north, wrapped_east);
    else
      // The points go around the world, hence a bounding box limit doesn't make sense.
      return new Bbox_Double(south, -180.0, north, 180.0);
  }
  else
    return new Bbox_Double(south, west, north, east);
}


Free_Polygon_Geometry::Free_Polygon_Geometry(const std::vector< std::vector< Point_Double > >& linestrings_) : linestrings(linestrings_), bounds(0)
{
  for (std::vector< std::vector< Point_Double > >::iterator it = linestrings.begin(); it != linestrings.end();
      ++it)
  {
    if (it->front() != it->back())
      it->push_back(it->front());
  }
}


double Free_Polygon_Geometry::center_lat() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->center_lat();
}


double Free_Polygon_Geometry::center_lon() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->center_lon();
}


double Free_Polygon_Geometry::south() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->south;
}


double Free_Polygon_Geometry::north() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->north;
}


double Free_Polygon_Geometry::west() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->west;
}


double Free_Polygon_Geometry::east() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->east;
}


void Free_Polygon_Geometry::add_linestring(const std::vector< Point_Double >& linestring)
{
  if (linestring.size() < 2)
    return;

  delete bounds;
  bounds = 0;
  linestrings.push_back(linestring);
  if (linestrings.back().front() != linestrings.back().back())
    linestrings.back().push_back(linestrings.back().front());
}


void toggle_if_inside(bool& is_inside, bool& on_vertex, bool& on_segment, double border_lat, double border_lon,
    Point_Double pt, Point_Double lhs, Point_Double rhs)
{
  if (pt.lon < -179.9)
  {
    lhs.lon -= lhs.lon > 0 ? 360. : 0.;
    rhs.lon -= rhs.lon > 0 ? 360. : 0.;
  }
  else if (pt.lon > 179.9)
  {
    lhs.lon += lhs.lon < 0 ? 360. : 0.;
    rhs.lon += rhs.lon < 0 ? 360. : 0.;
  }

  if (lhs.lat == rhs.lat)
  {
    if (lhs.lat < pt.lat)
      is_inside ^= ((lhs.lon - pt.lon)*(rhs.lon - pt.lon) < 0);
    else if (lhs.lat == pt.lat)
    {
      if (pt.lon == lhs.lon || pt.lon == rhs.lon)
        on_vertex = true;
      else
        on_segment |= ((lhs.lon - pt.lon)*(rhs.lon - pt.lon) < 0);
    }
    // no else required -- such a segment does not play a role
  }
  else if (lhs.lon == rhs.lon)
  {
    if ((pt.lat == lhs.lat && pt.lon == lhs.lon) || (pt.lat == rhs.lat && pt.lon == rhs.lon))
      on_vertex = true;
    else if (pt.lon == lhs.lon && (pt.lat - lhs.lat)*(pt.lat - rhs.lat) < 0)
      on_segment = true;
    else if (lhs.lon < pt.lon && (lhs.lat < border_lat || rhs.lat < border_lat))
      is_inside = !is_inside;
    // no else required -- such a segment does not play a role
  }
  else if ((lhs.lat - rhs.lat)*(lhs.lon - rhs.lon) < 0)
    // The segment runs from nw to se or se to nw
  {
    if ((pt.lat == lhs.lat && pt.lon == lhs.lon) || (pt.lat == rhs.lat && pt.lon == rhs.lon))
      on_vertex = true;
    else if ((pt.lon - lhs.lon)*(pt.lon - rhs.lon) < 0)
    {
      double isect_lat = lhs.lat + (rhs.lat - lhs.lat)*(pt.lon - lhs.lon)/(rhs.lon - lhs.lon);
      if (isect_lat < pt.lat)
        is_inside = !is_inside;
      else if (isect_lat == pt.lat)
        on_segment = true;
    }
    else if (pt.lon == lhs.lon && rhs.lon < pt.lon)
    {
      if (lhs.lat < pt.lat)
        is_inside = !is_inside;
    }
    else if (pt.lon == rhs.lon && lhs.lon < pt.lon)
    {
      if (rhs.lat < pt.lat)
        is_inside = !is_inside;
    }
    else if ((border_lat - lhs.lat)*(border_lat - rhs.lat) < 0)
    {
      double isect_lon = lhs.lon + (rhs.lon - lhs.lon)*(border_lat - lhs.lat)/(rhs.lat - lhs.lat);
      if (border_lon < isect_lon && isect_lon < pt.lon)
        is_inside = !is_inside;
    }
  }
  else
    // The segment runs from sw to ne or ne to sw
  {
    if ((border_lat - lhs.lat)*(border_lat - rhs.lat) < 0)
    {
      double isect_lon = lhs.lon + (rhs.lon - lhs.lon)*(border_lat - lhs.lat)/(rhs.lat - lhs.lat);
      if ((lhs.lon <= pt.lon || rhs.lon <= pt.lon) && (border_lon < isect_lon))
        is_inside = !is_inside;
    }
    if ((pt.lat == lhs.lat && pt.lon == lhs.lon) || (pt.lat == rhs.lat && pt.lon == rhs.lon))
    {
      on_vertex = true;
      return;
    }

    if ((pt.lon - lhs.lon)*(pt.lon - rhs.lon) < 0)
    {
      double isect_lat = lhs.lat + (rhs.lat - lhs.lat)*(pt.lon - lhs.lon)/(rhs.lon - lhs.lon);
      if (isect_lat < pt.lat)
        is_inside = !is_inside;
      else if (isect_lat == pt.lat)
        on_segment = true;
    }
    else if (pt.lon == lhs.lon && rhs.lon < pt.lon)
    {
      if (lhs.lat < pt.lat)
        is_inside = !is_inside;
    }
    else if (pt.lon == rhs.lon && lhs.lon < pt.lon)
    {
      if (rhs.lat < pt.lat)
        is_inside = !is_inside;
    }
  }
}


bool Free_Polygon_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
      iti != linestrings.end(); ++iti)
  {
    for (std::vector< Point_Double >::const_iterator it = iti->begin(); it != iti->end(); ++it)
    {
      if (bbox.contains(*it))
        return true;
    }
  }

  for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
      iti != linestrings.end(); ++iti)
  {
    for (uint i = 1; i < iti->size(); ++i)
    {
      if (bbox.intersects((*iti)[i-1], (*iti)[i]))
        return true;
    }
  }

  return false;
}


Point_Double interpolation_point(
    double orth_x, double orth_y, double orth_z,
    double lhs_gc_x, double lhs_gc_z, double factor, double lhs_lon)
{
  static const double deg_to_arc = acos(0)/90.;

  double sin_factor = sin(factor);
  double cos_factor = cos(factor);
  double new_x = sin_factor * orth_x + cos_factor * lhs_gc_x;
  double new_y = sin_factor * orth_y;
  double new_z = sin_factor * orth_z + cos_factor * lhs_gc_z;

  double new_lat = asin(new_x)/deg_to_arc;
  double new_lon = atan2(new_y, new_z)/deg_to_arc + lhs_lon;
  if (new_lon < -180.)
    new_lon += 360.;
  else if (new_lon > 180.)
    new_lon -= 360.;

  return Point_Double(new_lat, new_lon);
}


namespace
{
  struct Interpolation_Collector
  {
    Interpolation_Collector(double orth_x, double orth_y, double orth_z,
        double lhs_gc_x, double lhs_gc_z, double dist, double lhs_lon, std::vector< Point_Double >& target);

    void collect_single_point(const Point_Double& pt) { target->push_back(pt); }
    void collect_single_point(double dist);
    void collect_sequence(double from, double to, double max_step);
    void collect_center(double from, double to, int divisor);

    double orth_x;
    double orth_y;
    double orth_z;
    double lhs_gc_x;
    double lhs_gc_z;
    double lhs_lon;
    double center;
    Point_Double ex_pt;
    double ex_gc_pt_lat;
    double acceptable_max_length;
    std::vector< Point_Double >* target;
  };


  Interpolation_Collector::Interpolation_Collector(double orth_x_, double orth_y_, double orth_z_,
      double lhs_gc_x_, double lhs_gc_z_, double dist, double lhs_lon_, std::vector< Point_Double >& target_)
      : orth_x(orth_x_), orth_y(orth_y_), orth_z(orth_z_),
      lhs_gc_x(lhs_gc_x_), lhs_gc_z(lhs_gc_z_), lhs_lon(lhs_lon_),
      center(lhs_gc_x == 0 ? dist : atan(orth_x/lhs_gc_x)),
      ex_pt(interpolation_point(orth_x, orth_y, orth_z, lhs_gc_x, lhs_gc_z,
          std::max(std::min(center, dist), 0.), lhs_lon)),
      ex_gc_pt_lat(ex_pt.lat),
      target(&target_)
  {
    static const double deg_to_arc = acos(0)/90.;
    acceptable_max_length = .0065536*cos(ex_pt.lat * deg_to_arc);

    double bounded_center = std::max(std::min(center, dist), 0.);
    if (acceptable_max_length < .0016384 && center != bounded_center)
    {
      Point_Double ex_gc_pt = interpolation_point(orth_x, orth_y, orth_z,
          lhs_gc_x, lhs_gc_z, center, lhs_lon);
      ex_gc_pt_lat = ex_gc_pt.lat;
    }
    center = bounded_center;
  }


  void Interpolation_Collector::collect_single_point(double dist)
  {
    target->push_back(interpolation_point(orth_x, orth_y, orth_z, lhs_gc_x, lhs_gc_z, dist, lhs_lon));
  }


  void Interpolation_Collector::collect_sequence(double from, double to, double max_step)
  {
    int num_sections = (int)((to - from)/max_step)+1;
    for (int j = 1; j < num_sections; ++j)
      target->push_back(interpolation_point(orth_x, orth_y, orth_z,
          lhs_gc_x, lhs_gc_z, from + (to - from)*j/num_sections, lhs_lon));
  }


  void Interpolation_Collector::collect_center(double from, double to, int divisor)
  {
    static const double deg_to_arc = acos(0)/90.;

    double max_length_threshold = .0065536/divisor;
    if (acceptable_max_length < max_length_threshold && divisor < 65536)
    {
      double dist_ex_s = acos(sqrt(1-1./divisor/divisor)/sin(ex_gc_pt_lat * deg_to_arc));
      double bound_l = std::max(std::min(center - dist_ex_s, to), from);
      double bound_r = std::max(std::min(center + dist_ex_s, to), from);

      collect_sequence(from, bound_l, max_length_threshold*deg_to_arc);
      if (bound_l > 0 && bound_l < to)
        collect_single_point(bound_l);

      collect_center(bound_l, bound_r, divisor*4);

      if (bound_r > 0 && bound_r < to)
        collect_single_point(bound_r);
      collect_sequence(bound_r, to, max_length_threshold*deg_to_arc);
    }
    else
    {
      if (acceptable_max_length < 1e-9)
        acceptable_max_length = 1e-9;
      acceptable_max_length *= deg_to_arc;

      collect_sequence(from, center, acceptable_max_length);
      if (center > from && center < to)
        collect_single_point(center);
      collect_sequence(center, to, acceptable_max_length);
    }
  }
}


void interpolate_segment(double lhs_lat, double lhs_lon, double rhs_lat, double rhs_lon,
    std::vector< Point_Double >& target)
{
  static const double deg_to_arc = acos(0)/90.;

  if (fabs(rhs_lat - lhs_lat) < .0065536 && fabs(rhs_lon - lhs_lon) < .0065536)
    target.push_back(Point_Double(rhs_lat, rhs_lon));
  else
  {
    if (fabs(rhs_lon - lhs_lon) > 180.)
      rhs_lon = (lhs_lon < 0 ? rhs_lon - 360. : rhs_lon + 360.);

    if (fabs(rhs_lon - lhs_lon) > 179.999999)
    {
      double pole_lat = (lhs_lat + rhs_lat >= 0 ? 90. : -90.);

      int num_sections = (int)(fabs(pole_lat - lhs_lat)/.0065536)+1;
      for (int j = 1; j < num_sections; ++j)
        target.push_back(Point_Double(lhs_lat + (pole_lat - lhs_lat)*j/num_sections, lhs_lon));
      target.push_back(Point_Double(pole_lat, lhs_lon));

      num_sections = (int)(fabs(rhs_lon - lhs_lon)/.0065536)+1;
      for (int j = 1; j < num_sections; ++j)
        target.push_back(Point_Double(pole_lat, lhs_lon + (rhs_lon - lhs_lon)*j/num_sections));
      target.push_back(Point_Double(pole_lat, rhs_lon));

      num_sections = (int)(fabs(rhs_lat - pole_lat)/.0065536)+1;
      for (int j = 1; j < num_sections; ++j)
        target.push_back(Point_Double(pole_lat + (rhs_lat - pole_lat)*j/num_sections, rhs_lon));
    }
    else
    {
      double lhs_cos = cos(lhs_lat * deg_to_arc);
      double lhs_gc_x = sin(lhs_lat * deg_to_arc);
      double lhs_gc_z = lhs_cos;

      double rhs_cos = cos(rhs_lat * deg_to_arc);
      double rhs_gc_x = sin(rhs_lat * deg_to_arc);
      double rhs_gc_y = sin((rhs_lon - lhs_lon) * deg_to_arc) * rhs_cos;
      double rhs_gc_z = cos((rhs_lon - lhs_lon) * deg_to_arc) * rhs_cos;

      double prod = lhs_gc_x * rhs_gc_x + lhs_gc_z * rhs_gc_z;
      double dist = acos(prod);

      double orth_x = rhs_gc_x - prod * lhs_gc_x;
      double orth_y = rhs_gc_y;
      double orth_z = rhs_gc_z - prod * lhs_gc_z;
      double lg_orth = sqrt(orth_x * orth_x + orth_y * orth_y + orth_z * orth_z);
      orth_x /= lg_orth;
      orth_y /= lg_orth;
      orth_z /= lg_orth;

      Interpolation_Collector collector(orth_x, orth_y, orth_z, lhs_gc_x, lhs_gc_z, dist, lhs_lon, target);
      collector.collect_center(0, dist, 4);
    }

    if (rhs_lon < -180.)
      rhs_lon += 360.;
    else if (rhs_lon > 180.)
      rhs_lon -= 360.;
    target.push_back(Point_Double(rhs_lat, rhs_lon));
  }
}


void add_segment(std::map< uint32, std::vector< unsigned int > >& segments_per_idx,
    const Point_Double& from, const Point_Double& to, unsigned int pos)
{
  uint32 lhs_ilat = ::ilat(from.lat);
  int32 lhs_ilon = ::ilon(from.lon);
  uint32 rhs_ilat = ::ilat(to.lat);
  int32 rhs_ilon = ::ilon(to.lon);

  segments_per_idx[(lhs_ilat & 0xffff0000) | (uint32(lhs_ilon)>>16)].push_back(pos);
  if ((lhs_ilon & 0xffff0000) != (rhs_ilon & 0xffff0000))
    segments_per_idx[(lhs_ilat & 0xffff0000) | (uint32(rhs_ilon)>>16)].push_back(pos);
  if ((lhs_ilat & 0xffff0000) != (rhs_ilat & 0xffff0000))
  {
    segments_per_idx[(rhs_ilat & 0xffff0000) | (uint32(lhs_ilon)>>16)].push_back(pos);
    if ((lhs_ilon & 0xffff0000) != (rhs_ilon & 0xffff0000))
      segments_per_idx[(rhs_ilat & 0xffff0000) | (uint32(rhs_ilon)>>16)].push_back(pos);
  }
}


void replace_segment(std::vector< unsigned int >& segments, unsigned int old_pos, unsigned int new_pos)
{
  for (std::vector< unsigned int >::iterator it = segments.begin(); it != segments.end(); ++it)
  {
    if (*it == old_pos)
      *it = new_pos;
  }
}


void replace_segment(std::map< uint32, std::vector< unsigned int > >& segments_per_idx,
    const Point_Double& from, const Point_Double& via, const Point_Double& to,
    unsigned int old_pos, unsigned int new_pos)
{
  uint32 lhs_ilat = ::ilat(from.lat) & 0xffff0000;
  int32 lhs_ilon = ::ilon(from.lon) & 0xffff0000;
  uint32 rhs_ilat = ::ilat(to.lat) & 0xffff0000;
  int32 rhs_ilon = ::ilon(to.lon) & 0xffff0000;
  uint32 via_ilat = ::ilat(via.lat) & 0xffff0000;
  int32 via_ilon = ::ilon(via.lon) & 0xffff0000;

  // The upper part is the one that possibly crosses the index boundary
  if (via_ilat == lhs_ilat && via_ilon == lhs_ilon)
    ++new_pos;

  replace_segment(segments_per_idx[lhs_ilat | (uint32(lhs_ilon)>>16)], old_pos, new_pos);
  if (lhs_ilon != rhs_ilon)
    replace_segment(segments_per_idx[lhs_ilat | (uint32(rhs_ilon)>>16)], old_pos, new_pos);
  if (lhs_ilat != rhs_ilat)
  {
    replace_segment(segments_per_idx[rhs_ilat | (uint32(lhs_ilon)>>16)], old_pos, new_pos);
    if (lhs_ilon != rhs_ilon)
    {
      if ((via_ilat == lhs_ilat && via_ilon == lhs_ilon) || (via_ilat == rhs_ilat && via_ilon == rhs_ilon))
        replace_segment(segments_per_idx[rhs_ilat | (uint32(rhs_ilon)>>16)], old_pos, new_pos);
      else
      {
        segments_per_idx[lhs_ilat | (uint32(rhs_ilon)>>16)].push_back(new_pos+1);
        segments_per_idx[rhs_ilat | (uint32(lhs_ilon)>>16)].push_back(new_pos+1);
        replace_segment(segments_per_idx[rhs_ilat | (uint32(rhs_ilon)>>16)], old_pos, new_pos+1);
      }
    }
  }

  if (via_ilat == lhs_ilat && via_ilon == lhs_ilon)
    segments_per_idx[lhs_ilat | (uint32(lhs_ilon)>>16)].push_back(new_pos-1);
  else if (via_ilat == rhs_ilat && via_ilon == rhs_ilon)
    segments_per_idx[rhs_ilat | (uint32(rhs_ilon)>>16)].push_back(new_pos+1);
}


bool try_intersect(const Point_Double& lhs_from, const Point_Double& lhs_to,
    const Point_Double& rhs_from, const Point_Double& rhs_to, Point_Double& isect)
{
  double rfmlt_lat = rhs_from.lat - lhs_to.lat;
  double rfmlt_lon = rhs_from.lon - lhs_to.lon;

  //The two segments are connected by a vertex
  if (!rfmlt_lat && !rfmlt_lon)
    return false;
  if (lhs_from == rhs_to || lhs_from == rhs_from || lhs_to == rhs_to)
    return false;

  double lfmlt_lat = lhs_from.lat - lhs_to.lat;
  double lfmlt_lon = lhs_from.lon - lhs_to.lon;
  double rfmrt_lat = rhs_from.lat - rhs_to.lat;
  double rfmrt_lon = rhs_from.lon - rhs_to.lon;
  double det = lfmlt_lat * rfmrt_lon - rfmrt_lat * lfmlt_lon;

  if (det == 0)
  {
    // Segments on parallel but distinct beams
    if (lfmlt_lat * rfmlt_lon - lfmlt_lon * rfmlt_lat != 0)
      return false;

    if (fabs(rfmlt_lat) > fabs(rfmlt_lon))
    {
      if (lhs_from.lat < lhs_to.lat)
      {
        if (rhs_from.lat < rhs_to.lat)
        {
          // Segments are non-overlapping
          if (lhs_to.lat < rhs_from.lat || rhs_to.lat < lhs_from.lat)
            return false;

          if (lhs_from.lat < rhs_from.lat)
            isect = rhs_from;
          else
            isect = lhs_from;
        }
        else
        {
          // Segments are non-overlapping
          if (lhs_to.lat < rhs_to.lat || rhs_from.lat < lhs_from.lat)
            return false;

          if (lhs_from.lat < rhs_to.lat)
            isect = rhs_to;
          else
            isect = lhs_from;
        }
      }
      else
      {
        if (rhs_from.lat < rhs_to.lat)
        {
          // Segments are non-overlapping
          if (lhs_from.lat < rhs_from.lat || rhs_to.lat < lhs_to.lat)
            return false;

          if (lhs_to.lat < rhs_from.lat)
            isect = rhs_from;
          else
            isect = lhs_to;
        }
        else
        {
          // Segments are non-overlapping
          if (lhs_from.lat < rhs_to.lat || rhs_from.lat < lhs_to.lat)
            return false;

          if (lhs_to.lat < rhs_to.lat)
            isect = rhs_to;
          else
            isect = lhs_to;
        }
      }
    }
    else
    {
      if (lhs_from.lon < lhs_to.lon)
      {
        if (rhs_from.lon < rhs_to.lon)
        {
          // Segments are non-overlapping
          if (lhs_to.lon < rhs_from.lon || rhs_to.lon < lhs_from.lon)
            return false;

          if (lhs_from.lon < rhs_from.lon)
            isect = rhs_from;
          else
            isect = lhs_from;
        }
        else
        {
          // Segments are non-overlapping
          if (lhs_to.lon < rhs_to.lon || rhs_from.lon < lhs_from.lon)
            return false;

          if (lhs_from.lon < rhs_to.lon)
            isect = rhs_to;
          else
            isect = lhs_from;
        }
      }
      else
      {
        if (rhs_from.lon < rhs_to.lon)
        {
          // Segments are non-overlapping
          if (lhs_from.lon < rhs_from.lon || rhs_to.lon < lhs_to.lon)
            return false;

          if (lhs_to.lon < rhs_from.lon)
            isect = rhs_from;
          else
            isect = lhs_to;
        }
        else
        {
          // Segments are non-overlapping
          if (lhs_from.lon < rhs_to.lon || rhs_from.lon < lhs_to.lon)
            return false;

          if (lhs_to.lon < rhs_to.lon)
            isect = rhs_to;
          else
            isect = lhs_to;
        }
      }
    }
    return true;
  }

  double x = (rfmrt_lon * rfmlt_lat - rfmrt_lat * rfmlt_lon)/det;
  double y = (lfmlt_lat * rfmlt_lon - lfmlt_lon * rfmlt_lat)/det;

  if (x <= -1e-7 || x >= 1+1e-7 || y <= -1e-7 || y >= 1+1e-7)
    return false;

  isect.lat = lhs_to.lat + x * lfmlt_lat;
  isect.lon = lhs_to.lon + x * lfmlt_lon;
  return true;
}


struct Idx_Per_Point_Double
{
  Idx_Per_Point_Double(const Point_Double& pt_, unsigned int idx_, unsigned int remote_idx_)
      : pt(pt_), idx(idx_), remote_idx(remote_idx_) {}

  Point_Double pt;
  unsigned int idx;
  unsigned int remote_idx;

  bool operator<(const Idx_Per_Point_Double& rhs) const
  {
    if (pt.lat != rhs.pt.lat)
      return pt.lat < rhs.pt.lat;
    if (pt.lon != rhs.pt.lon)
      return pt.lon < rhs.pt.lon;

    return idx < rhs.idx;
  }
};


struct Line_Divertion
{
  Line_Divertion() : to_idx(std::numeric_limits< uint >::max()),
      to_remote_idx(std::numeric_limits< uint >::max()) {}
  Line_Divertion(const Idx_Per_Point_Double& rhs) : to_idx(rhs.idx), to_remote_idx(rhs.remote_idx) {}

  unsigned int to_idx;
  unsigned int to_remote_idx;
};


void split_segments(
    std::vector< Point_Double >& all_segments,
    std::vector< unsigned int >& gap_positions,
    std::map< uint32, std::vector< unsigned int > >& segments_per_idx)
{
  for (std::map< uint32, std::vector< unsigned int > >::iterator idx_it = segments_per_idx.begin();
      idx_it != segments_per_idx.end(); ++idx_it)
  {
    Point_Double isect(100, 0);
    for (unsigned int i = 1; i < idx_it->second.size(); ++i)
    {
      for (unsigned int j = 0; j < i; ++j)
      {
        if (try_intersect(all_segments[idx_it->second[j]], all_segments[idx_it->second[j]+1],
              all_segments[idx_it->second[i]], all_segments[idx_it->second[i]+1], isect))
        {
          uint32 lhs_ilat = ::ilat(isect.lat);
          int32 lhs_ilon = ::ilon(isect.lon);

          if (((lhs_ilat & 0xffff0000) | (uint32(lhs_ilon)>>16)) == idx_it->first)
            // Ensure that the same intersection is processed only in one index
          {
            // Avoid rounding artifacts
            for (unsigned int k = 0; k < idx_it->second.size(); ++k)
            {
              if (isect.epsilon_equal(all_segments[idx_it->second[k]]))
              {
                isect.lat = all_segments[idx_it->second[k]].lat;
                isect.lon = all_segments[idx_it->second[k]].lon;
              }
              if (isect.epsilon_equal(all_segments[idx_it->second[k]+1]))
              {
                isect.lat = all_segments[idx_it->second[k]+1].lat;
                isect.lon = all_segments[idx_it->second[k]+1].lon;
              }
            }

            if (isect != all_segments[idx_it->second[j]] && isect != all_segments[idx_it->second[j]+1])
            {
              all_segments.push_back(all_segments[idx_it->second[j]]);
              all_segments.push_back(isect);
              all_segments.push_back(all_segments[idx_it->second[j]+1]);
              gap_positions.insert(std::lower_bound(
                  gap_positions.begin(), gap_positions.end(), idx_it->second[j]+1), idx_it->second[j]+1);
              gap_positions.push_back(all_segments.size());

              replace_segment(segments_per_idx, all_segments[idx_it->second[j]], isect,
                  all_segments[idx_it->second[j]+1], idx_it->second[j], all_segments.size()-3);
            }

            if (isect != all_segments[idx_it->second[i]] && isect != all_segments[idx_it->second[i]+1])
            {
              all_segments.push_back(all_segments[idx_it->second[i]]);
              all_segments.push_back(isect);
              all_segments.push_back(all_segments[idx_it->second[i]+1]);
              gap_positions.insert(std::lower_bound(
                  gap_positions.begin(), gap_positions.end(), idx_it->second[i]+1), idx_it->second[i]+1);
              gap_positions.push_back(all_segments.size());

              replace_segment(segments_per_idx, all_segments[idx_it->second[i]], isect,
                  all_segments[idx_it->second[i]+1], idx_it->second[i], all_segments.size()-3);
            }
          }

          //TODO: check and track common segments
        }
      }
    }
  }
}


unsigned int line_divertion_idx(const Idx_Per_Point_Double& rhs)
{
  return rhs.idx*2 + ((int)rhs.remote_idx - (int)rhs.idx - 1)/2;
}


void collect_divertions(const std::vector< Point_Double >& all_segments,
    uint32 idx, const std::vector< unsigned int >& segments,
    std::vector< Line_Divertion >& divertions)
{
  std::vector< Idx_Per_Point_Double > pos_per_pt;

  for (std::vector< unsigned int >::const_iterator seg_it = segments.begin();
      seg_it != segments.end(); ++seg_it)
  {
    uint32 lhs_ilat = ::ilat(all_segments[*seg_it].lat);
    int32 lhs_ilon = ::ilon(all_segments[*seg_it].lon);
    if (((lhs_ilat & 0xffff0000) | (uint32(lhs_ilon)>>16)) == idx)
      pos_per_pt.push_back(Idx_Per_Point_Double(all_segments[*seg_it], *seg_it, *seg_it+1));

    uint32 rhs_ilat = ::ilat(all_segments[*seg_it+1].lat);
    int32 rhs_ilon = ::ilon(all_segments[*seg_it+1].lon);
    if (((rhs_ilat & 0xffff0000) | (uint32(rhs_ilon)>>16)) == idx)
      pos_per_pt.push_back(Idx_Per_Point_Double(all_segments[*seg_it+1], *seg_it+1, *seg_it));
  }

  if (pos_per_pt.empty())
    return;

  std::sort(pos_per_pt.begin(), pos_per_pt.end());

  unsigned int same_since = 0;
  unsigned int i = 0;
  while (i <= pos_per_pt.size())
  {
    if (i == pos_per_pt.size() || pos_per_pt[i].pt != pos_per_pt[same_since].pt)
    {
      if (i - same_since == 2)
      {
        divertions[line_divertion_idx(pos_per_pt[same_since])] = Line_Divertion(pos_per_pt[same_since+1]);
        divertions[line_divertion_idx(pos_per_pt[same_since+1])] = Line_Divertion(pos_per_pt[same_since]);
      }
      else
      {
        std::vector< std::pair< double, unsigned int > > line_per_gradient;
        for (unsigned int j = same_since; j < i; ++j)
          line_per_gradient.push_back(std::make_pair(
              atan2(all_segments[pos_per_pt[j].remote_idx].lon - all_segments[pos_per_pt[j].idx].lon,
                  all_segments[pos_per_pt[j].remote_idx].lat - all_segments[pos_per_pt[j].idx].lat),
              j));

        std::sort(line_per_gradient.begin(), line_per_gradient.end());

        for (unsigned int j = 0; j < line_per_gradient.size(); j += 2)
        {
          if (j+3 < line_per_gradient.size()
              && line_per_gradient[j+1].first == line_per_gradient[j+2].first)
          {
            divertions[line_divertion_idx(pos_per_pt[line_per_gradient[j].second])]
                = Line_Divertion(pos_per_pt[line_per_gradient[j+3].second]);
            divertions[line_divertion_idx(pos_per_pt[line_per_gradient[j+1].second])]
                = Line_Divertion(pos_per_pt[line_per_gradient[j+2].second]);
            divertions[line_divertion_idx(pos_per_pt[line_per_gradient[j+2].second])]
                = Line_Divertion(pos_per_pt[line_per_gradient[j+1].second]);
            divertions[line_divertion_idx(pos_per_pt[line_per_gradient[j+3].second])]
                = Line_Divertion(pos_per_pt[line_per_gradient[j].second]);
            j += 2;
          }
          else
          {
            divertions[line_divertion_idx(pos_per_pt[line_per_gradient[j].second])]
                = Line_Divertion(pos_per_pt[line_per_gradient[j+1].second]);
            divertions[line_divertion_idx(pos_per_pt[line_per_gradient[j+1].second])]
                = Line_Divertion(pos_per_pt[line_per_gradient[j].second]);
          }
        }
      }

      same_since = i;
    }
    ++i;
  }
}


void assemble_linestrings(const std::vector< Point_Double >& all_segments,
    std::vector< Line_Divertion >& divertions, std::vector< std::vector< Point_Double > >& linestrings)
{
  // the function changes divertions because divertions is no longer needed after the function

  unsigned int pos = 0;
  unsigned int count = 0;
  for (unsigned int i = 0; i < divertions.size(); i += 2)
    count += (divertions[i+1].to_idx == std::numeric_limits< uint >::max());

  //Invariant: count is always equal to the number of i with divertions[2*i+1] == uint::max()
  while (count < all_segments.size())
  {
    while (divertions[2*pos].to_idx == std::numeric_limits< uint >::max())
    {
      ++pos;
      if (pos == all_segments.size()-1)
        pos = 0;
    }

    linestrings.push_back(std::vector< Point_Double >());
    std::vector< Point_Double >& cur_target = linestrings.back();

    int dir = 1;
    while (divertions[2*pos + dir].to_idx != std::numeric_limits< uint >::max())
    {
      if (cur_target.size() >= 2 && all_segments[pos].epsilon_equal(cur_target[cur_target.size()-2]))
        // Remove pairs of equal segments
        cur_target.pop_back();
      else if (cur_target.empty() || !all_segments[pos].epsilon_equal(cur_target.back()))
        cur_target.push_back(all_segments[pos]);

      Line_Divertion& old_div = divertions[2*(int)pos + dir];
      Line_Divertion& divertion = divertions[2*(int)pos + (3*dir-1)/2];
      dir = divertion.to_remote_idx - divertion.to_idx;
      pos = divertion.to_idx;

      ++count;
      old_div.to_idx = std::numeric_limits< uint >::max();
    }

    ++pos;
    if (pos == all_segments.size()-1)
      pos = 0;

    if (cur_target.size() <= 2)
      linestrings.pop_back();
    else
      cur_target.push_back(cur_target.front());
  }
}


struct RHR_Polygon_Area_Oracle : Area_Oracle
{
  RHR_Polygon_Area_Oracle(
      const std::vector< Point_Double >& all_segments_,
      const std::map< uint32, std::vector< unsigned int > >& segments_per_idx_)
      : all_segments(&all_segments_), segments_per_idx(&segments_per_idx_) {}

  virtual void build_area(bool sw_corner_inside, int32 value, bool* se_corner_inside, bool* nw_corner_inside);
  virtual Area_Oracle::point_status get_point_status(int32 value, double lat, double lon);

private:
  const std::vector< Point_Double >* all_segments;
  const std::map< uint32, std::vector< unsigned int > >* segments_per_idx;
  std::set< uint32 > inside_corners;
};


void RHR_Polygon_Area_Oracle::build_area(
    bool sw_corner_inside, int32 value, bool* se_corner_inside, bool* nw_corner_inside)
{
  std::map< uint32, std::vector< unsigned int > >::const_iterator spi_it = segments_per_idx->find(value);
  if (spi_it == segments_per_idx->end())
    return;

  if (sw_corner_inside)
    inside_corners.insert(value);

  if (::lon(uint32(value)<<16) > -179.99)
  {
    if (nw_corner_inside)
    {
      *nw_corner_inside = sw_corner_inside;

      for (std::vector< unsigned int >::const_iterator seg_it = spi_it->second.begin();
          seg_it != spi_it->second.end(); ++seg_it)
      {
        int32 lhs_ilon = ::ilon((*all_segments)[*seg_it].lon) & 0xffff0000;
        int32 rhs_ilon = ::ilon((*all_segments)[*seg_it+1].lon) & 0xffff0000;

        if ((lhs_ilon < (value<<16) && rhs_ilon == (value<<16))
            || (lhs_ilon == (value<<16) && rhs_ilon < (value<<16)))
        {
          double isect_lat = (*all_segments)[*seg_it].lat
              + ((*all_segments)[*seg_it+1].lat - (*all_segments)[*seg_it].lat)
                  *(::lon(uint32(value)<<16) - (*all_segments)[*seg_it].lon)
                  /((*all_segments)[*seg_it+1].lon - (*all_segments)[*seg_it].lon);
          if ((::ilat(isect_lat) & 0xffff0000) == (value & 0xffff0000))
            *nw_corner_inside = !*nw_corner_inside;
        }
      }
    }

    if (se_corner_inside)
    {
      *se_corner_inside = sw_corner_inside;

      for (std::vector< unsigned int >::const_iterator seg_it = spi_it->second.begin();
          seg_it != spi_it->second.end(); ++seg_it)
      {
        uint32 lhs_ilat = ::ilat((*all_segments)[*seg_it].lat) & 0xffff0000;
        uint32 rhs_ilat = ::ilat((*all_segments)[*seg_it+1].lat) & 0xffff0000;

        if ((lhs_ilat < (value & 0xffff0000) && rhs_ilat == (value & 0xffff0000))
            || (lhs_ilat == (value & 0xffff0000) && rhs_ilat < (value & 0xffff0000)))
        {
          double isect_lon = (*all_segments)[*seg_it].lon
              + ((*all_segments)[*seg_it+1].lon - (*all_segments)[*seg_it].lon)
                  *(::lat(value & 0xffff0000) - (*all_segments)[*seg_it].lat)
                  /((*all_segments)[*seg_it+1].lat - (*all_segments)[*seg_it].lat);
          if ((::ilon(isect_lon) & 0xffff0000) == (uint32(value)<<16))
            *se_corner_inside = !*se_corner_inside;
        }
      }
    }
  }
  else
  {
    if (nw_corner_inside)
    {
      *nw_corner_inside = sw_corner_inside;

      for (std::vector< unsigned int >::const_iterator seg_it = spi_it->second.begin();
          seg_it != spi_it->second.end(); ++seg_it)
      {
        double lhs_lon = (*all_segments)[*seg_it].lon;
        lhs_lon -= lhs_lon > 0 ? 360. : 0.;
        double rhs_lon = (*all_segments)[*seg_it+1].lon;
        rhs_lon -= rhs_lon > 0 ? 360. : 0.;

        if (::lon(uint32(value)<<16 > -180.))
        {
          int32 lhs_ilon = ::ilon(lhs_lon) & 0xffff0000;
          int32 rhs_ilon = ::ilon(rhs_lon) & 0xffff0000;

          if ((lhs_ilon < (value<<16) && rhs_ilon == (value<<16))
              || (lhs_ilon == (value<<16) && rhs_ilon < (value<<16)))
          {
            double isect_lat = (*all_segments)[*seg_it].lat
                + ((*all_segments)[*seg_it+1].lat - (*all_segments)[*seg_it].lat)
                    *(::lon(uint32(value)<<16) - (*all_segments)[*seg_it].lon)
                    /((*all_segments)[*seg_it+1].lon - (*all_segments)[*seg_it].lon);
            if ((::ilat(isect_lat) & 0xffff0000) == (value & 0xffff0000))
              *nw_corner_inside = !*nw_corner_inside;
          }
        }
        else
        {
          if ((lhs_lon <= -180. && rhs_lon > -180.) || (rhs_lon <= -180. && lhs_lon > -180.))
          {
            double isect_lat = (*all_segments)[*seg_it].lat
                + ((*all_segments)[*seg_it+1].lat - (*all_segments)[*seg_it].lat)
                    *(-180. - (*all_segments)[*seg_it].lon)
                    /((*all_segments)[*seg_it+1].lon - (*all_segments)[*seg_it].lon);
            if ((::ilat(isect_lat) & 0xffff0000) == (value & 0xffff0000))
              *nw_corner_inside = !*nw_corner_inside;
          }
        }
      }
    }

    if (se_corner_inside)
    {
      *se_corner_inside = sw_corner_inside;

      for (std::vector< unsigned int >::const_iterator seg_it = spi_it->second.begin();
          seg_it != spi_it->second.end(); ++seg_it)
      {
        uint32 lhs_ilat = ::ilat((*all_segments)[*seg_it].lat) & 0xffff0000;
        uint32 rhs_ilat = ::ilat((*all_segments)[*seg_it+1].lat) & 0xffff0000;

        if ((lhs_ilat < (value & 0xffff0000) && rhs_ilat == (value & 0xffff0000))
            || (lhs_ilat == (value & 0xffff0000) && rhs_ilat < (value & 0xffff0000)))
        {
          double lhs_lon = (*all_segments)[*seg_it].lon;
          lhs_lon -= lhs_lon > 0 ? 360. : 0.;
          double rhs_lon = (*all_segments)[*seg_it+1].lon;
          rhs_lon -= rhs_lon > 0 ? 360. : 0.;

          double isect_lon = (*all_segments)[*seg_it].lon
              + ((*all_segments)[*seg_it+1].lon - (*all_segments)[*seg_it].lon)
                  *(::lat(value & 0xffff0000) - (*all_segments)[*seg_it].lat)
                  /((*all_segments)[*seg_it+1].lat - (*all_segments)[*seg_it].lat);
          if (isect_lon >= -180. && (::ilon(isect_lon) & 0xffff0000) == (uint32(value)<<16))
            *se_corner_inside = !*se_corner_inside;
        }
      }
    }
  }
}


Area_Oracle::point_status RHR_Polygon_Area_Oracle::get_point_status(int32 value, double lat, double lon)
{
  if (value == 1)
    return 1;

  std::map< uint32, std::vector< unsigned int > >::const_iterator spi_it = segments_per_idx->find(value);
  if (spi_it == segments_per_idx->end())
    return 0;

  double border_lon = std::max(::lon(value<<16), -180.);
  double border_lat = ::lat(value & 0xffff0000);

  bool on_vertex = false;
  bool on_segment = false;
  bool is_inside = (inside_corners.find(value) != inside_corners.end());
  // is_inside is now true iff the sw corner is inside the area

  for (std::vector< unsigned int >::const_iterator seg_it = spi_it->second.begin();
      seg_it != spi_it->second.end(); ++seg_it)
    toggle_if_inside(is_inside, on_vertex, on_segment, border_lat, border_lon, Point_Double(lat, lon),
        (*all_segments)[*seg_it], (*all_segments)[*seg_it+1]);

  if (on_vertex)
    return 0x20 + 2*is_inside;
  if (on_segment)
    return 0x10 + 2*is_inside;

  return 2*is_inside;
}


bool strictly_west_of(double lhs, double rhs)
{
  if (lhs - rhs > 180.)
    return lhs - 360. < rhs;

  return lhs < rhs;
}


bool weakly_west_of(double lhs, double rhs)
{
  if (lhs - rhs > 180.)
    return lhs - 360. <= rhs;

  return lhs <= rhs;
}


RHR_Polygon_Geometry::RHR_Polygon_Geometry(const Free_Polygon_Geometry& rhs) : bounds(0)
{
  std::vector< std::vector< Point_Double > > input(*rhs.get_multiline_geometry());

  std::vector< Point_Double > all_segments;
  std::vector< unsigned int > gap_positions;

  gap_positions.push_back(0);
  for (std::vector< std::vector< Point_Double > >::const_iterator iti = input.begin(); iti != input.end(); ++iti)
  {
    all_segments.push_back((*iti)[0]);
    for (unsigned int i = 1; i < iti->size(); ++i)
      interpolate_segment((*iti)[i-1].lat, (*iti)[i-1].lon, (*iti)[i].lat, (*iti)[i].lon, all_segments);
    gap_positions.push_back(all_segments.size());
  }

  std::map< uint32, std::vector< unsigned int > > segments_per_idx;
  std::vector< unsigned int >::const_iterator gap_it = gap_positions.begin();
  for (unsigned int i = 0; i < all_segments.size(); ++i)
  {
    if (*gap_it == i)
      ++gap_it;
    else
      add_segment(segments_per_idx, all_segments[i-1], all_segments[i], i-1);
  }
  split_segments(all_segments, gap_positions, segments_per_idx);

  // divertions[2*i - 2] tells which segment follows the segment from all_segments[i+1] to all_segments[i]
  // divertions[2*i - 1] tells which segment follows the segment from all_segments[i] to all_segments[i+1]
  std::vector< Line_Divertion > divertions(2*all_segments.size());

  // fill the gaps with invalid following segments
  for (unsigned int i = 1; i < gap_positions.size(); ++i)
  {
    divertions[gap_positions[i]*2 - 2] = Line_Divertion();
    divertions[gap_positions[i]*2 - 1] = Line_Divertion();
  }

  // compute the real segment connections
  for (std::map< uint32, std::vector< unsigned int > >::const_iterator idx_it = segments_per_idx.begin();
      idx_it != segments_per_idx.end(); ++idx_it)
    collect_divertions(all_segments, idx_it->first, idx_it->second, divertions);

  assemble_linestrings(all_segments, divertions, linestrings);

  RHR_Polygon_Area_Oracle area_oracle(all_segments, segments_per_idx);
  Four_Field_Index four_field_idx(&area_oracle);

  for (std::map< uint32, std::vector< unsigned int > >::const_iterator idx_it = segments_per_idx.begin();
      idx_it != segments_per_idx.end(); ++idx_it)
    four_field_idx.add_point(::lat(idx_it->first | 0x8000), ::lon(idx_it->first<<16 | 0x8000), idx_it->first);

  four_field_idx.compute_inside_parts();

  for (std::vector< std::vector< Point_Double > >::iterator lstr_it = linestrings.begin();
      lstr_it != linestrings.end(); ++lstr_it)
  {
    if (lstr_it->size() > 2)
    {
      if (strictly_west_of((*lstr_it)[0].lon, (*lstr_it)[1].lon))
      {
        if (weakly_west_of((*lstr_it)[1].lon, (*lstr_it)[2].lon))
        {
          if (four_field_idx.get_point_status((*lstr_it)[1].lat, (*lstr_it)[1].lon) & 0x3)
            std::reverse(lstr_it->begin(), lstr_it->end());
        }
        else
        {
          if (bool(four_field_idx.get_point_status((*lstr_it)[1].lat, (*lstr_it)[1].lon) & 0x3)
              ^ (((*lstr_it)[1].lat - (*lstr_it)[0].lat)/((*lstr_it)[1].lon -(*lstr_it)[0].lon)
                  < ((*lstr_it)[2].lat -(*lstr_it)[1].lat)/((*lstr_it)[2].lon -(*lstr_it)[1].lon)))
            std::reverse(lstr_it->begin(), lstr_it->end());
        }
      }
      else if (strictly_west_of((*lstr_it)[2].lon, (*lstr_it)[1].lon))
      {
        if (!(four_field_idx.get_point_status((*lstr_it)[1].lat, (*lstr_it)[1].lon) & 0x3))
          std::reverse(lstr_it->begin(), lstr_it->end());
      }
      else if ((*lstr_it)[0].lon == (*lstr_it)[1].lon)
      {
        if (bool(four_field_idx.get_point_status((*lstr_it)[1].lat, (*lstr_it)[1].lon) & 0x3)
            ^ ((*lstr_it)[0].lat < (*lstr_it)[1].lat))
          std::reverse(lstr_it->begin(), lstr_it->end());
      }
      else if ((*lstr_it)[2].lon == (*lstr_it)[1].lon)
      {
        if (bool(four_field_idx.get_point_status((*lstr_it)[1].lat, (*lstr_it)[1].lon) & 0x3)
            ^ ((*lstr_it)[1].lat < (*lstr_it)[2].lat))
          std::reverse(lstr_it->begin(), lstr_it->end());
      }
      else
      {
        if (bool(four_field_idx.get_point_status((*lstr_it)[1].lat, (*lstr_it)[1].lon) & 0x3)
            ^ (((*lstr_it)[1].lat -(*lstr_it)[0].lat)/((*lstr_it)[1].lon -(*lstr_it)[0].lon)
                < ((*lstr_it)[2].lat -(*lstr_it)[1].lat)/((*lstr_it)[2].lon -(*lstr_it)[1].lon)))
          std::reverse(lstr_it->begin(), lstr_it->end());
      }
    }
  }
}


double RHR_Polygon_Geometry::center_lat() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->center_lat();
}


double RHR_Polygon_Geometry::center_lon() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->center_lon();
}


double RHR_Polygon_Geometry::south() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->south;
}


double RHR_Polygon_Geometry::north() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->north;
}


double RHR_Polygon_Geometry::west() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->west;
}


double RHR_Polygon_Geometry::east() const
{
  if (!bounds)
    bounds = calc_bounds(linestrings);

  return bounds->east;
}


bool RHR_Polygon_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
      iti != linestrings.end(); ++iti)
  {
    for (std::vector< Point_Double >::const_iterator it = iti->begin(); it != iti->end(); ++it)
    {
      if (bbox.contains(*it))
        return true;
    }
  }

  for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
      iti != linestrings.end(); ++iti)
  {
    for (uint i = 1; i < iti->size(); ++i)
    {
      if (bbox.intersects((*iti)[i-1], (*iti)[i]))
        return true;
    }
  }

  bool is_inside = false;
  bool on_vertex = false;
  bool on_segment = false;
  Point_Double bbox_center(bbox.center_lat(), bbox.center_lon());

  for (std::vector< std::vector< Point_Double > >::const_iterator iti = linestrings.begin();
      iti != linestrings.end(); ++iti)
  {
    for (uint i = 1; i < iti->size(); ++i)
      toggle_if_inside(is_inside, on_vertex, on_segment, -90., -180.,
          bbox_center, (*iti)[i-1], (*iti)[i]);
  }
  return (is_inside || on_vertex || on_segment);
}


Opaque_Geometry* Compound_Geometry::clone() const
{
  std::vector< Opaque_Geometry* > cloned;
  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
    cloned.push_back((*it)->clone());
  return new Compound_Geometry(cloned);
}


Bbox_Double* calc_bounds(const std::vector< Opaque_Geometry* >& components)
{
  double south = 100.0;
  double west = 200.0;
  double north = -100.0;
  double east = -200.0;
  bool wrapped = false;

  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
  {
    if ((*it)->has_bbox())
    {
      south = std::min(south, (*it)->south());
      north = std::max(north, (*it)->north());
      west = std::min(west, (*it)->west());
      east = std::max(east, (*it)->east());

      wrapped |= ((*it)->east() < (*it)->west());
    }
  }

  if (north == -100.0)
    return new Bbox_Double(Bbox_Double::invalid);
  if (!wrapped && east - west <= 180.)
    return new Bbox_Double(south, west, north, east);

  west = 200.;
  east = -200.;
  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
  {
    if (!(*it)->has_bbox())
      continue;

    if ((*it)->east() <= 0)
    {
      east = std::max(east, (*it)->east());
      if ((*it)->west() >= 0)
        west = std::min(west, (*it)->west());
    }
    else if ((*it)->west() >= 0)
      west = std::min(west, (*it)->west());
    else
      // The components are too wildly distributed
      return new Bbox_Double(south, -180.0, north, 180.0);
  }

  if (west - east > 180.)
    return new Bbox_Double(south, west, north, east);

  return new Bbox_Double(south, -180.0, north, 180.0);
}


bool Compound_Geometry::has_center() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->valid();
}


double Compound_Geometry::center_lat() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->center_lat();
}


double Compound_Geometry::center_lon() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->center_lon();
}


bool Compound_Geometry::has_bbox() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->valid();
}


double Compound_Geometry::south() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->south;
}


double Compound_Geometry::north() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->north;
}


double Compound_Geometry::west() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->west;
}


double Compound_Geometry::east() const
{
  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->east;
}


bool Compound_Geometry::relation_pos_is_valid(unsigned int member_pos) const
{
  return (member_pos < components.size() && components[member_pos]
      && components[member_pos]->has_center());
}


double Compound_Geometry::relation_pos_lat(unsigned int member_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->center_lat();
  return 0;
}


double Compound_Geometry::relation_pos_lon(unsigned int member_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->center_lon();
  return 0;
}


unsigned int Compound_Geometry::relation_way_size(unsigned int member_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->way_size();
  return 0;
}


bool Compound_Geometry::relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const
{
  return (member_pos < components.size() && components[member_pos]
      && components[member_pos]->way_pos_is_valid(nd_pos));
}


double Compound_Geometry::relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->way_pos_lat(nd_pos);
  return 0;
}


double Compound_Geometry::relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->way_pos_lon(nd_pos);
  return 0;
}


void Compound_Geometry::add_component(Opaque_Geometry* component)
{
  delete bounds;
  bounds = 0;
  components.push_back(component);
}


bool Compound_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
  {
    if ((*it)->relevant_to_bbox(bbox))
      return true;
  }

  return false;
}


Opaque_Geometry* Partial_Relation_Geometry::clone() const
{
  std::vector< Opaque_Geometry* > cloned;
  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
    cloned.push_back((*it)->clone());
  return new Partial_Relation_Geometry(cloned);
}


bool Partial_Relation_Geometry::has_center() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->valid();
}


double Partial_Relation_Geometry::center_lat() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->center_lat();
}


double Partial_Relation_Geometry::center_lon() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->center_lon();
}


bool Partial_Relation_Geometry::has_bbox() const
{
  if (!has_coords)
    return false;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->valid();
}


double Partial_Relation_Geometry::south() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->south;
}


double Partial_Relation_Geometry::north() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->north;
}


double Partial_Relation_Geometry::west() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->west;
}


double Partial_Relation_Geometry::east() const
{
  if (!has_coords)
    return 0;

  if (!bounds)
    bounds = calc_bounds(components);

  return bounds->east;
}


bool Partial_Relation_Geometry::relation_pos_is_valid(unsigned int member_pos) const
{
  return (member_pos < components.size() && components[member_pos]
      && components[member_pos]->has_center());
}


double Partial_Relation_Geometry::relation_pos_lat(unsigned int member_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->center_lat();
  return 0;
}


double Partial_Relation_Geometry::relation_pos_lon(unsigned int member_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->center_lon();
  return 0;
}


unsigned int Partial_Relation_Geometry::relation_way_size(unsigned int member_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->way_size();
  return 0;
}


bool Partial_Relation_Geometry::relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const
{
  return (member_pos < components.size() && components[member_pos]
      && components[member_pos]->way_pos_is_valid(nd_pos));
}


double Partial_Relation_Geometry::relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->way_pos_lat(nd_pos);
  return 0;
}


double Partial_Relation_Geometry::relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const
{
  if (member_pos < components.size() && components[member_pos])
    return components[member_pos]->way_pos_lon(nd_pos);
  return 0;
}


void Partial_Relation_Geometry::add_placeholder()
{
  components.push_back(new Null_Geometry());
}


void Partial_Relation_Geometry::add_point(const Point_Double& point)
{
  delete bounds;
  bounds = 0;
  if (point.lat < 100.)
  {
    has_coords = true;
    components.push_back(new Point_Geometry(point.lat, point.lon));
  }
  else
    components.push_back(new Null_Geometry());
}


void Partial_Relation_Geometry::start_way()
{
  components.push_back(new Partial_Way_Geometry());
}


void Partial_Relation_Geometry::add_way_point(const Point_Double& point)
{
  delete bounds;
  bounds = 0;

  Partial_Way_Geometry* geom = dynamic_cast< Partial_Way_Geometry* >(components.back());
  if (geom)
  {
    has_coords = true;
    geom->add_point(point);
  }
}


void Partial_Relation_Geometry::add_way_placeholder()
{
  Partial_Way_Geometry* geom = dynamic_cast< Partial_Way_Geometry* >(components.back());
  if (geom)
    geom->add_point(Point_Double(100., 200.));
}


bool Partial_Relation_Geometry::relevant_to_bbox(const Bbox_Double& bbox) const
{
  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
  {
    if ((*it)->relevant_to_bbox(bbox))
      return true;
  }

  return false;
}


double great_circle_dist(double lat1, double lon1, double lat2, double lon2)
{
  if (lat1 == lat2 && lon1 == lon2)
    return 0;
  static const double deg_to_arc = acos(0)/90.;
  double scalar_prod = cos((lat2-lat1)*deg_to_arc)
      + cos(lat1*deg_to_arc)*cos(lat2*deg_to_arc)*(cos((lon2-lon1)*deg_to_arc) - 1);
  if (scalar_prod > 1)
    scalar_prod = 1;
  static const double arc_to_meter = 10*1000*1000/acos(0);
  return acos(scalar_prod)*arc_to_meter;
}


double length(const Opaque_Geometry& geometry)
{
  double result = 0;

  if (geometry.has_components())
  {
    const std::vector< Opaque_Geometry* >* components = geometry.get_components();
    for (std::vector< Opaque_Geometry* >::const_iterator it = components->begin(); it != components->end(); ++it)
      result += (*it ? length(**it) : 0);
  }
  else if (geometry.has_line_geometry())
  {
    const std::vector< Point_Double >* line_geometry = geometry.get_line_geometry();
    for (unsigned int i = 1; i < line_geometry->size(); ++i)
      result += great_circle_dist((*line_geometry)[i-1].lat, (*line_geometry)[i-1].lon,
          (*line_geometry)[i].lat, (*line_geometry)[i].lon);
  }
  else if (geometry.has_faithful_way_geometry())
  {
    for (unsigned int i = 1; i < geometry.way_size(); ++i)
      result += (geometry.way_pos_is_valid(i-1) && geometry.way_pos_is_valid(i) ?
          great_circle_dist(geometry.way_pos_lat(i-1), geometry.way_pos_lon(i-1),
              geometry.way_pos_lat(i), geometry.way_pos_lon(i)) : 0);
  }

  return result;
}


void collect_components(
    std::vector< Point_Double >& nodes, std::vector< std::vector< Point_Double > >& linestrings,
    const Opaque_Geometry& geometry)
{
  if (geometry.has_components())
  {
    const std::vector< Opaque_Geometry* >& components = *geometry.get_components();
    for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
    {
      if (*it)
        collect_components(nodes, linestrings, **it);
    }
  }
  else if (geometry.has_line_geometry())
    linestrings.push_back(*geometry.get_line_geometry());
  else if (geometry.has_multiline_geometry())
  {
    const std::vector< std::vector< Point_Double > >& lstrs = *geometry.get_multiline_geometry();
    for (std::vector< std::vector< Point_Double > >::const_iterator it = lstrs.begin(); it != lstrs.end(); ++it)
      linestrings.push_back(*it);
  }
  else if (geometry.has_center())
    nodes.push_back(Point_Double(geometry.center_lat(), geometry.center_lon()));
}


struct Linestring_Geometry_Ptr
{
  Linestring_Geometry_Ptr(Linestring_Geometry* ptr_) : ptr(ptr_) {}
  bool operator<(Linestring_Geometry_Ptr rhs) const;

private:
  Linestring_Geometry* ptr;
};


bool Linestring_Geometry_Ptr::operator<(Linestring_Geometry_Ptr rhs_) const
{
  if (!rhs_.ptr)
    return false;
  if (!ptr)
    return true;

  const std::vector< Point_Double >& lhs = *ptr->get_line_geometry();
  const std::vector< Point_Double >& rhs = *rhs_.ptr->get_line_geometry();

  if (lhs.size() != rhs.size())
    return lhs.size() < rhs.size();

  uint l_start = 0;
  uint l_factor = 1;
  if (lhs.back() < lhs.front())
  {
    l_start = lhs.size()-1;
    l_factor = -1;
  }

  uint r_start = 0;
  uint r_factor = 1;
  if (rhs.back() < rhs.front())
  {
    r_start = rhs.size()-1;
    r_factor = -1;
  }

  for (int i = 0; i < (int)lhs.size(); ++i)
  {
    if (lhs[l_start + l_factor*i] != rhs[r_start + r_factor*i])
      return lhs[l_start + l_factor*i] < rhs[r_start + r_factor*i];
  }
  return false;
}


Opaque_Geometry* make_trace(const Opaque_Geometry& geometry)
{
  std::vector< Point_Double > nodes;
  std::vector< std::vector< Point_Double > > linestrings;
  collect_components(nodes, linestrings, geometry);

  std::sort(nodes.begin(), nodes.end());
  nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

  std::map< Point_Double, uint > coord_count;
  for (std::vector< Point_Double >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    ++coord_count[*it];

  for (std::vector< std::vector< Point_Double > >::const_iterator lit = linestrings.begin();
      lit != linestrings.end(); ++lit)
  {
    if (lit->empty())
      continue;
    coord_count[lit->front()] += 2;
    coord_count[lit->back()] += 2;
    for (uint i = 1; i < lit->size() - 1; ++i)
      ++coord_count[(*lit)[i]];
  }

  Compound_Geometry* result = new Compound_Geometry();

  for (std::vector< Point_Double >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    result->add_component(new Point_Geometry(it->lat, it->lon));

  std::set< Linestring_Geometry_Ptr > lstrs;
  for (std::vector< std::vector< Point_Double > >::const_iterator lit = linestrings.begin();
      lit != linestrings.end(); ++lit)
  {
    if (lit->empty())
      continue;

    std::vector< Point_Double > points;
    points.push_back(lit->front());
    for (uint i = 1; i < lit->size() - 1; ++i)
    {
      if (coord_count[(*lit)[i]] > 1)
      {
        points.push_back((*lit)[i]);

        Linestring_Geometry* lstr = new Linestring_Geometry(points);
        if (lstrs.insert(Linestring_Geometry_Ptr(lstr)).second)
          result->add_component(lstr);
        else
          delete lstr;

        points.clear();
      }
      points.push_back((*lit)[i]);
    }
    points.push_back(lit->back());
    Linestring_Geometry* lstr = new Linestring_Geometry(points);
    if (lstrs.insert(Linestring_Geometry_Ptr(lstr)).second)
      result->add_component(lstr);
    else
      delete lstr;
  }

  return result;
}


void collect_components(std::vector< Point_Double >& nodes, const Opaque_Geometry& geometry)
{
  if (geometry.has_components())
  {
    const std::vector< Opaque_Geometry* >& components = *geometry.get_components();
    for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
    {
      if (*it)
        collect_components(nodes, **it);
    }
  }
  else if (geometry.has_line_geometry())
  {
    const std::vector< Point_Double >& lstrs = *geometry.get_line_geometry();
    for (std::vector< Point_Double >::const_iterator it = lstrs.begin(); it != lstrs.end(); ++it)
      nodes.push_back(*it);
  }
  else if (geometry.has_multiline_geometry())
  {
    const std::vector< std::vector< Point_Double > >& lstrs = *geometry.get_multiline_geometry();
    for (std::vector< std::vector< Point_Double > >::const_iterator lit = lstrs.begin(); lit != lstrs.end(); ++lit)
    {
      for (std::vector< Point_Double >::const_iterator it = lit->begin(); it != lit->end(); ++it)
        nodes.push_back(*it);
    }
  }
  else if (geometry.has_center())
    nodes.push_back(Point_Double(geometry.center_lat(), geometry.center_lon()));
}


struct Point_Double_By_Lon
{
  bool operator()(const Point_Double& lhs, const Point_Double& rhs)
  {
    if (lhs.lon != rhs.lon)
      return lhs.lon < rhs.lon;
    return lhs.lat < rhs.lat;
  }
};


struct Spherical_Vector
{
  Spherical_Vector() : x(0), y(0), z(0) {}

  Spherical_Vector(const Point_Double& pt) : x(sin(pt.lat*deg_to_arc()))
  {
    double cos_lat = cos(pt.lat*deg_to_arc());
    y = cos_lat*sin(pt.lon*deg_to_arc());
    z = cos_lat*cos(pt.lon*deg_to_arc());
  }

  Spherical_Vector(const Spherical_Vector& lhs, const Spherical_Vector& rhs)
      : x(lhs.y*rhs.z - lhs.z*rhs.y), y(lhs.z*rhs.x - lhs.x*rhs.z), z(lhs.x*rhs.y - lhs.y*rhs.x)
  {
    double length = sqrt(x*x + y*y + z*z);
    x /= length;
    y /= length;
    z /= length;
  }

  double x;
  double y;
  double z;

  double operator==(const Spherical_Vector& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
  double operator*(const Spherical_Vector& rhs) const { return x*rhs.x + y*rhs.y + z*rhs.z; }

  static double deg_to_arc()
  {
    static const double result = acos(0)/90.;
    return result;
  }
};


struct Proto_Hull
{
  struct Hull_Segment
  {
    Hull_Segment(const Point_Double& pt) : ll_pt(pt), s_pt(pt) {}
    Hull_Segment(const Point_Double& pt, const Spherical_Vector& s) : ll_pt(pt), s_pt(s) {}

    Point_Double ll_pt;
    Spherical_Vector s_pt;
    Spherical_Vector edge;
  };

  Proto_Hull(const Point_Double& s, const Point_Double& w, const Point_Double& n, const Point_Double& e);
  void enhance(const Point_Double& rhs);
  std::vector< Point_Double > get_line_geometry() const;

private:
  std::vector< Hull_Segment > segments;
};


Proto_Hull::Proto_Hull(const Point_Double& s, const Point_Double& w, const Point_Double& n, const Point_Double& e)
{
  if (w.lon != e.lon)
  {
    segments.push_back(Hull_Segment(w));
    segments.push_back(Hull_Segment(e));
  }
  else if (n.lat != s.lat)
  {
    segments.push_back(Hull_Segment(s));
    segments.push_back(Hull_Segment(n));
  }

  if (!segments.empty())
  {
    segments[1].edge = Spherical_Vector(segments[0].s_pt, segments[1].s_pt);
    segments[0].edge = Spherical_Vector(segments[1].s_pt, segments[0].s_pt);
  }
}


void Proto_Hull::enhance(const Point_Double& rhs)
{
  Spherical_Vector s_pt(rhs);

  std::vector< Hull_Segment >::iterator from_it = segments.begin();
  while (from_it != segments.end() && from_it->edge*s_pt < 1e-8)
    ++from_it;

  if (from_it != segments.end() && from_it->edge*s_pt >= 1e-8)
  {
    if (segments.begin()->edge*s_pt >= 0)
    {
      from_it = segments.end();
      --from_it;
      while (from_it != segments.begin() && from_it->edge*s_pt >= 0)
        --from_it;
      ++from_it;
      segments.erase(from_it, segments.end());

      std::vector< Hull_Segment >::iterator to_it = segments.begin();
      while (to_it != segments.end() && to_it->edge*s_pt >= 0)
        ++to_it;
      --to_it;
      from_it = segments.begin();

      to_it->edge = Spherical_Vector(s_pt, to_it->s_pt);
      if (from_it == to_it)
        from_it = segments.insert(from_it, Hull_Segment(rhs, s_pt));
      else
      {
        ++from_it;
        from_it = segments.erase(from_it, to_it);
        --from_it;
        *from_it = Hull_Segment(rhs, s_pt);
      }
    }
    else
    {
      std::vector< Hull_Segment >::iterator to_it = from_it;
      while (to_it != segments.end() && to_it->edge*s_pt >= 0)
        ++to_it;
      --to_it;
      while (from_it != segments.begin() && from_it->edge*s_pt >= 0)
        --from_it;
      ++from_it;

      to_it->edge = Spherical_Vector(s_pt, to_it->s_pt);
      if (from_it == to_it)
        from_it = segments.insert(from_it, Hull_Segment(rhs, s_pt));
      else
      {
        ++from_it;
        from_it = segments.erase(from_it, to_it);
        --from_it;
        *from_it = Hull_Segment(rhs, s_pt);
      }
    }

    if (from_it == segments.begin())
      from_it->edge = Spherical_Vector(segments.back().s_pt, s_pt);
    else
      from_it->edge = Spherical_Vector((from_it-1)->s_pt, s_pt);
  }
}


std::vector< Point_Double > Proto_Hull::get_line_geometry() const
{
  std::vector< Point_Double > result;
  result.reserve(segments.size());
  for (std::vector< Hull_Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    result.push_back(it->ll_pt);
  return result;
}


Opaque_Geometry* make_hull(const Opaque_Geometry& geometry)
{
  std::vector< Point_Double > nodes;
  collect_components(nodes, geometry);

  std::sort(nodes.begin(), nodes.end(), Point_Double_By_Lon());
  nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

  if (nodes.empty())
    return new Null_Geometry();
  if (nodes.size() == 1)
    return new Point_Geometry(nodes.front().lat, nodes.front().lon);
  if (nodes.size() == 2)
  {
    nodes.push_back(nodes.front());
    return new Linestring_Geometry(nodes);
  }

  Point_Double min_lat(100., 0.);
  Point_Double max_lat(-100., 0.);
  for (std::vector< Point_Double >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
  {
    if (it->lat < min_lat.lat)
      min_lat = *it;
    if (it->lat > max_lat.lat)
      max_lat = *it;
  }

  double max_lon_delta = nodes.front().lon + 360. - nodes.back().lon;
  Point_Double west_end = nodes.front();
  Point_Double east_end = nodes.back();
  for (uint i = 1; i < nodes.size(); ++i)
  {
    if (max_lon_delta < nodes[i].lon - nodes[i-1].lon)
    {
      max_lon_delta = nodes[i].lon - nodes[i-1].lon;
      west_end = nodes[i];
      east_end = nodes[i-1];
    }
  }

  Proto_Hull proto_hull(min_lat, west_end, max_lat, east_end);
  for (std::vector< Point_Double >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    proto_hull.enhance(*it);

  std::vector< Point_Double > line_geom = proto_hull.get_line_geometry();
  if (line_geom.size() < 3)
    return new Linestring_Geometry(line_geom);

  return new RHR_Polygon_Geometry(Free_Polygon_Geometry(
      std::vector< std::vector< Point_Double > >(1, line_geom)));
}
