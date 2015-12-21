#ifndef DE__OSM3S___OVERPASS_API__CORE__GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__CORE__GEOMETRY_H


#include <vector>


struct Point_Double
{
public:
  Point_Double(double lat_, double lon_) : lat(lat_), lon(lon_) {}
  
  double lat;
  double lon;
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
  
  const static Bbox_Double invalid;
};


// All coordinates are always in latitude and longitude
class Opaque_Geometry
{
public:
  virtual ~Opaque_Geometry() {}
  
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
  //virtual const std::vector< std::vector< Point_Double > >* get_multiline_geometry() const { return 0; }
  
  virtual bool has_components() const = 0;
  virtual const std::vector< Opaque_Geometry* >* get_components() const { return 0; }
};


class Null_Geometry : public Opaque_Geometry
{
public:
  Null_Geometry() {}
  
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
};


class Point_Geometry : public Opaque_Geometry
{
public:
  Point_Geometry(double lat_, double lon_) : pt(lat_, lon_) {}
  
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
  
private:
  Point_Double pt;
};


class Bbox_Geometry : public Opaque_Geometry
{
public:
  Bbox_Geometry(double south, double west, double north, double east) : bbox(south, west, north, east) {}
  
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
  
private:
  Bbox_Double bbox;
};


class Linestring_Geometry : public Opaque_Geometry
{
public:
  Linestring_Geometry(const std::vector< Point_Double >& points_) : points(points_), bounds(0) {}
  virtual ~Linestring_Geometry() { delete bounds; }
  
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
  
private:
  std::vector< Point_Double > points;
  mutable Bbox_Double* bounds;
};


class Compound_Geometry : public Opaque_Geometry
{
public:
  Compound_Geometry() : bounds(0) {}
  virtual ~Compound_Geometry()
  {
    delete bounds;
    for (std::vector< Opaque_Geometry* >::iterator it = components.begin(); it != components.end(); ++it)
      delete *it;
  }
  
  virtual bool has_center() const;
  virtual double center_lat() const;
  virtual double center_lon() const;
  
  virtual bool has_bbox() const;
  virtual double south() const;
  virtual double north() const;
  virtual double west() const;
  virtual double east() const;
  
  virtual bool has_line_geometry() const { return false; }
  
  virtual bool has_multiline_geometry() const { return true; }
  //virtual const std::vector< std::vector< Point_Double > >* get_multiline_geometry() const;

  virtual bool has_components() const { return true; }
  virtual const std::vector< Opaque_Geometry* >* get_components() const { return &components; }
  
  void add_component(Opaque_Geometry* component);
  
private:
  std::vector< Opaque_Geometry* > components;
  mutable Bbox_Double* bounds;
  mutable std::vector< std::vector< Point_Double > >* linestrings;
};


#endif
