#ifndef DE__OSM3S___OVERPASS_API__CORE__GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__CORE__GEOMETRY_H


struct Bbox
{
public:
  Bbox(double south_, double west_, double north_, double east_)
      : south(south_), west(west_), north(north_), east(east_) {}
      
  bool valid() const
  {
    return (south >= -90.0 && south <= north && north <= 90.0
        && east >= -180.0 && east <= 180.0 && west >= -180.0 && west <= 180.0);
  }
  
  double south, west, north, east;
  
  const static Bbox invalid;
};


class Opaque_Geometry
{
public:
  virtual bool has_center() const = 0;
  virtual double center_lat() const = 0;
  virtual double center_lon() const = 0;
};


class Point_Geometry : public Opaque_Geometry
{
public:
  Point_Geometry(double lat_, double lon_) : lat(lat_), lon(lon_) {}
  
  virtual bool has_center() const { return true; }
  virtual double center_lat() const { return lat; }
  virtual double center_lon() const { return lon; }
  
private:
  double lat;
  double lon;
};


#endif
