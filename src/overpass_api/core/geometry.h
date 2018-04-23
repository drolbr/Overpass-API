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

#ifndef DE__OSM3S___OVERPASS_API__CORE__GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__CORE__GEOMETRY_H


#include <cmath>
#include <vector>


struct Point_Double
{
public:
  Point_Double(double lat_, double lon_) : lat(lat_), lon(lon_) {}

  double lat;
  double lon;

  bool operator==(const Point_Double& rhs) const { return lat == rhs.lat && lon == rhs.lon; }
  bool operator!=(const Point_Double& rhs) const { return !(*this == rhs); }
  bool operator<(const Point_Double& rhs) const
  { return lat != rhs.lat ? lat < rhs.lat : lon < rhs.lon; }

  bool epsilon_equal(const Point_Double& rhs) const
  { return fabs(lat - rhs.lat) < 1e-7 && fabs(lon - rhs.lon) < 1e-7; }
};


struct Bbox_Double
{
public:
  Bbox_Double(double south_, double west_, double north_, double east_)
      : south(south_), west(west_), north(north_), east(east_) {}

  bool valid() const
  {
    return (south >= -90.0 && south <= north && north <= 90.0
        && east >= -180.0 && east <= 180.0 && west >= -180.0 && west <= 180.0);
  }

  double center_lat() const;
  double center_lon() const;

  double south, west, north, east;

  bool contains(const Point_Double& point) const;
  bool intersects(const Point_Double& from, const Point_Double& to) const;

  const static Bbox_Double invalid;
};


// All coordinates are always in latitude and longitude
class Opaque_Geometry
{
public:
  virtual ~Opaque_Geometry() {}
  virtual Opaque_Geometry* clone() const = 0;

  virtual bool has_center() const = 0;
  virtual double center_lat() const = 0;
  virtual double center_lon() const = 0;

  // We require for a bounding box the following:
  // Usually, west is smaller than east.
  // If the object passes through all lines of longitude then the bounding box is west -180.0, east 180.0.
  // If the object crosses the date line but doesn't pass all lines of longitude then east is smaller than west.
  // For a single point, south and north are equal, and so are west and east.
  virtual bool has_bbox() const = 0;
  virtual double south() const = 0;
  virtual double north() const = 0;
  virtual double west() const = 0;
  virtual double east() const = 0;

  virtual bool has_line_geometry() const = 0;
  virtual const std::vector< Point_Double >* get_line_geometry() const { return 0; }

  virtual bool has_multiline_geometry() const = 0;
  virtual const std::vector< std::vector< Point_Double > >* get_multiline_geometry() const { return 0; }

  virtual bool has_components() const = 0;
  virtual const std::vector< Opaque_Geometry* >* get_components() const { return 0; }
  virtual std::vector< Opaque_Geometry* >* move_components() { return 0; }

  virtual unsigned int way_size() const = 0;
  virtual bool has_faithful_way_geometry() const = 0;
  virtual bool way_pos_is_valid(unsigned int pos) const = 0;
  virtual double way_pos_lat(unsigned int pos) const = 0;
  virtual double way_pos_lon(unsigned int pos) const = 0;

  virtual bool has_faithful_relation_geometry() const = 0;
  virtual bool relation_pos_is_valid(unsigned int member_pos) const = 0;
  virtual double relation_pos_lat(unsigned int member_pos) const = 0;
  virtual double relation_pos_lon(unsigned int member_pos) const = 0;
  virtual unsigned int relation_way_size(unsigned int member_pos) const = 0;
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const = 0;
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const = 0;
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const = 0;

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const = 0;
};


class Null_Geometry : public Opaque_Geometry
{
public:
  Null_Geometry() {}
  virtual Opaque_Geometry* clone() const { return new Null_Geometry(); }

  virtual bool has_center() const { return false; }
  virtual double center_lat() const { return 0; }
  virtual double center_lon() const { return 0; }

  virtual bool has_bbox() const { return false; }
  virtual double south() const { return 0; }
  virtual double north() const { return 0; }
  virtual double west() const { return 0; }
  virtual double east() const { return 0; }

  virtual bool has_line_geometry() const { return false; }
  virtual bool has_multiline_geometry() const { return false; }
  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const { return false; }
};


class Point_Geometry : public Opaque_Geometry
{
public:
  Point_Geometry(double lat_, double lon_) : pt(lat_, lon_) {}
  virtual Opaque_Geometry* clone() const { return new Point_Geometry(pt.lat, pt.lon); }

  virtual bool has_center() const { return true; }
  virtual double center_lat() const { return pt.lat; }
  virtual double center_lon() const { return pt.lon; }

  virtual bool has_bbox() const { return true; }
  virtual double south() const { return pt.lat; }
  virtual double north() const { return pt.lat; }
  virtual double west() const { return pt.lon; }
  virtual double east() const { return pt.lon; }

  virtual bool has_line_geometry() const { return false; }
  virtual bool has_multiline_geometry() const { return false; }
  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

private:
  Point_Double pt;
};


class Bbox_Geometry : public Opaque_Geometry
{
public:
  Bbox_Geometry(double south, double west, double north, double east) : bbox(south, west, north, east) {}
  Bbox_Geometry(const Bbox_Double& bbox_) : bbox(bbox_) {}
  virtual Opaque_Geometry* clone() const { return new Bbox_Geometry(bbox); }

  virtual bool has_center() const { return true; }
  virtual double center_lat() const { return bbox.center_lat(); }
  virtual double center_lon() const { return bbox.center_lon(); }

  virtual bool has_bbox() const { return true; }
  virtual double south() const { return bbox.south; }
  virtual double north() const { return bbox.north; }
  virtual double west() const { return bbox.west; }
  virtual double east() const { return bbox.east; }

  virtual bool has_line_geometry() const { return false; }
  virtual bool has_multiline_geometry() const { return false; }
  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const { return false; }

private:
  Bbox_Double bbox;
};


class Linestring_Geometry : public Opaque_Geometry
{
public:
  Linestring_Geometry(const std::vector< Point_Double >& points_) : points(points_), bounds(0) {}
  virtual ~Linestring_Geometry() { delete bounds; }
  virtual Opaque_Geometry* clone() const { return new Linestring_Geometry(points); }

  virtual bool has_center() const { return true; }
  virtual double center_lat() const;
  virtual double center_lon() const;

  virtual bool has_bbox() const { return true; }
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;

  virtual bool has_line_geometry() const { return true; }
  virtual const std::vector< Point_Double >* get_line_geometry() const { return &points; }

  virtual bool has_multiline_geometry() const { return false; }
  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return points.size(); }
  virtual bool has_faithful_way_geometry() const { return true; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return pos < points.size(); }
  virtual double way_pos_lat(unsigned int pos) const { return points[pos].lat; }
  virtual double way_pos_lon(unsigned int pos) const { return points[pos].lon; }

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

private:
  std::vector< Point_Double > points;
  mutable Bbox_Double* bounds;
};


class Partial_Way_Geometry : public Opaque_Geometry
{
public:
  Partial_Way_Geometry() : bounds(0), has_coords(false) {}
  Partial_Way_Geometry(const std::vector< Point_Double >& points_);
  virtual ~Partial_Way_Geometry() { delete bounds; }
  virtual Opaque_Geometry* clone() const { return new Partial_Way_Geometry(points); }

  virtual bool has_center() const { return has_coords; }
  virtual double center_lat() const;
  virtual double center_lon() const;

  virtual bool has_bbox() const { return has_coords; }
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;

  virtual bool has_line_geometry() const { return valid_segments.size() == 1; }
  virtual const std::vector< Point_Double >* get_line_geometry() const
  { return valid_segments.size() == 1 ? &valid_segments.front() : 0; }

  virtual bool has_multiline_geometry() const { return true; }
  virtual const std::vector< std::vector< Point_Double > >* get_multiline_geometry() const
  { return &valid_segments; }

  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return points.size(); }
  virtual bool has_faithful_way_geometry() const { return true; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return pos < points.size() && points[pos].lat < 100.; }
  virtual double way_pos_lat(unsigned int pos) const { return points[pos].lat; }
  virtual double way_pos_lon(unsigned int pos) const { return points[pos].lon; }

  void add_point(const Point_Double& point);

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

private:
  std::vector< Point_Double > points;
  std::vector< std::vector< Point_Double > > valid_segments;
  mutable Bbox_Double* bounds;
  bool has_coords;
};


class Free_Polygon_Geometry : public Opaque_Geometry
{
public:
  Free_Polygon_Geometry() : bounds(0) {}
  Free_Polygon_Geometry(const std::vector< std::vector< Point_Double > >& linestrings_);
  virtual ~Free_Polygon_Geometry() { delete bounds; }
  virtual Opaque_Geometry* clone() const { return new Free_Polygon_Geometry(linestrings); }

  virtual bool has_center() const { return true; }
  virtual double center_lat() const;
  virtual double center_lon() const;

  virtual bool has_bbox() const { return true; }
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;

  virtual bool has_line_geometry() const { return false; }
  virtual bool has_multiline_geometry() const { return true; }
  virtual const std::vector< std::vector< Point_Double > >* get_multiline_geometry() const { return &linestrings; }
  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

  void add_linestring(const std::vector< Point_Double >& linestring);

private:
  std::vector< std::vector< Point_Double > > linestrings;
  mutable Bbox_Double* bounds;
};


class RHR_Polygon_Geometry : public Opaque_Geometry
{
public:
  RHR_Polygon_Geometry(const Free_Polygon_Geometry& rhs);
  virtual ~RHR_Polygon_Geometry() { delete bounds; }
  virtual Opaque_Geometry* clone() const { return new RHR_Polygon_Geometry(linestrings); }

  virtual bool has_center() const { return true; }
  virtual double center_lat() const;
  virtual double center_lon() const;

  virtual bool has_bbox() const { return true; }
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;

  virtual bool has_line_geometry() const { return false; }
  virtual bool has_multiline_geometry() const { return true; }
  virtual const std::vector< std::vector< Point_Double > >* get_multiline_geometry() const { return &linestrings; }
  virtual bool has_components() const { return false; }

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return false; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos) const { return 0; }
  virtual unsigned int relation_way_size(unsigned int member_pos) const { return 0; }
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const { return false; }
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const { return 0; }
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const { return 0; }

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

  void add_linestring(const std::vector< Point_Double >& linestring);

private:
  RHR_Polygon_Geometry(const std::vector< std::vector< Point_Double > >& linestrings_)
      : linestrings(linestrings_), bounds(0) {}

  std::vector< std::vector< Point_Double > > linestrings;
  mutable Bbox_Double* bounds;
};


class Compound_Geometry : public Opaque_Geometry
{
public:
  Compound_Geometry() : bounds(0) {}
  Compound_Geometry(const std::vector< Opaque_Geometry* >& components_) : components(components_), bounds(0) {}
  virtual ~Compound_Geometry()
  {
    delete bounds;
    for (std::vector< Opaque_Geometry* >::iterator it = components.begin(); it != components.end(); ++it)
      delete *it;
  }
  virtual Opaque_Geometry* clone() const;

  virtual bool has_center() const;
  virtual double center_lat() const;
  virtual double center_lon() const;

  virtual bool has_bbox() const;
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;

  virtual bool has_line_geometry() const { return false; }

  virtual bool has_multiline_geometry() const { return false; }

  virtual bool has_components() const { return true; }
  virtual const std::vector< Opaque_Geometry* >* get_components() const { return &components; }
  virtual std::vector< Opaque_Geometry* >* move_components() { return &components; }

  void add_component(Opaque_Geometry* component);

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return true; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const;
  virtual double relation_pos_lat(unsigned int member_pos) const;
  virtual double relation_pos_lon(unsigned int member_pos) const;
  virtual unsigned int relation_way_size(unsigned int member_pos) const;
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const;
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const;
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const;

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

private:
  std::vector< Opaque_Geometry* > components;
  mutable Bbox_Double* bounds;
};


class Partial_Relation_Geometry : public Opaque_Geometry
{
public:
  Partial_Relation_Geometry() : bounds(0), has_coords(false) {}
  Partial_Relation_Geometry(const std::vector< Opaque_Geometry* >& components_)
      : components(components_), bounds(0), has_coords(false)
  {
    for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin();
        it != components.end() && !has_coords; ++it)
    {
      Point_Geometry* pt = dynamic_cast< Point_Geometry* >(*it);
      has_coords |= (bool)pt;
      Partial_Way_Geometry* way = dynamic_cast< Partial_Way_Geometry* >(*it);
      if (way)
        has_coords |= way->has_center();
    }
  }
  virtual ~Partial_Relation_Geometry()
  {
    delete bounds;
    for (std::vector< Opaque_Geometry* >::iterator it = components.begin(); it != components.end(); ++it)
      delete *it;
  }
  virtual Opaque_Geometry* clone() const;

  virtual bool has_center() const;
  virtual double center_lat() const;
  virtual double center_lon() const;

  virtual bool has_bbox() const;
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;

  virtual bool has_line_geometry() const { return false; }

  virtual bool has_multiline_geometry() const { return false; }

  virtual bool has_components() const { return true; }
  virtual const std::vector< Opaque_Geometry* >* get_components() const { return &components; }
  virtual std::vector< Opaque_Geometry* >* move_components() { return &components; }

  void add_placeholder();
  void add_point(const Point_Double& point);
  void start_way();
  void add_way_point(const Point_Double& point);
  void add_way_placeholder();

  virtual unsigned int way_size() const { return 0; }
  virtual bool has_faithful_way_geometry() const { return false; }
  virtual bool way_pos_is_valid(unsigned int pos) const { return false; }
  virtual double way_pos_lat(unsigned int pos) const { return 0; }
  virtual double way_pos_lon(unsigned int pos) const { return 0; }

  virtual bool has_faithful_relation_geometry() const { return true; }
  virtual bool relation_pos_is_valid(unsigned int member_pos) const;
  virtual double relation_pos_lat(unsigned int member_pos) const;
  virtual double relation_pos_lon(unsigned int member_pos) const;
  virtual unsigned int relation_way_size(unsigned int member_pos) const;
  virtual bool relation_pos_is_valid(unsigned int member_pos, unsigned int nd_pos) const;
  virtual double relation_pos_lat(unsigned int member_pos, unsigned int nd_pos) const;
  virtual double relation_pos_lon(unsigned int member_pos, unsigned int nd_pos) const;

  virtual bool relevant_to_bbox(const Bbox_Double& bbox) const;

private:
  std::vector< Opaque_Geometry* > components;
  mutable Bbox_Double* bounds;
  bool has_coords;
};


double length(const Opaque_Geometry& geometry);

Opaque_Geometry* make_trace(const Opaque_Geometry& geometry);

Opaque_Geometry* make_hull(const Opaque_Geometry& geometry);

double great_circle_dist(double lat1, double lon1, double lat2, double lon2);


#endif
