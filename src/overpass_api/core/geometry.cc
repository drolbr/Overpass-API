#include "four_field_index.h"
#include "geometry.h"

#include <cmath>


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
  // TODO: correct behaviour over 180°

  double from_lon = from.lon;
  if (from.lat < south)
  {
    if (to.lat < south)
      return false;
    // Otherwise just adjust from.lat and from.lon
    from_lon += (to.lon - from.lon)*(south - from.lat)/(to.lat - from.lat);
  }
  else if (from.lat > north)
  {
    if (to.lat > north)
      return false;
    // Otherwise just adjust from.lat and from.lon
    from_lon += (to.lon - from.lon)*(north - from.lat)/(to.lat - from.lat);
  }

  double to_lon = to.lon;
  if (to.lat < south)
    // Adjust to.lat and to.lon
    to_lon += (from.lon - to.lon)*(south - to.lat)/(from.lat - to.lat);
  else if (to.lat > north)
    // Adjust to.lat and to.lon
    to_lon += (from.lon - to.lon)*(north - to.lat)/(from.lat - to.lat);

  // Now we know that both latitudes are between south and north.
  // Thus we only need to check whether the segment touches the bbox in its east-west-extension.
  if (from_lon < west && to_lon < west)
    return false;
  if (from_lon > east && to_lon > east)
    return false;
  
  return true;
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

  
void Partial_Way_Geometry::add_point(const Point_Double& point)
{
  delete bounds;
  bounds = 0;
  has_coords |= (point.lat < 100.);
  points.push_back(point);
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
  
    double lhs_cos = cos(lhs_lat * deg_to_arc);
    double lhs_gc_x = sin(lhs_lat * deg_to_arc);
    double lhs_gc_z = lhs_cos;
    
    double rhs_cos = cos(rhs_lat * deg_to_arc);
    double rhs_gc_x = sin(rhs_lat * deg_to_arc);
    double rhs_gc_y = sin((rhs_lon - lhs_lon) * deg_to_arc) * rhs_cos;
    double rhs_gc_z = cos((rhs_lon - lhs_lon) * deg_to_arc) * rhs_cos;
    
    double prod = lhs_gc_x * rhs_gc_x + lhs_gc_z * rhs_gc_z;
    
    double orth_x = rhs_gc_x - prod * lhs_gc_x;
    double orth_y = rhs_gc_y;
    double orth_z = rhs_gc_z - prod * lhs_gc_z;
    double lg_orth = sqrt(orth_x * orth_x + orth_y * orth_y + orth_z * orth_z);
    orth_x /= lg_orth;
    orth_y /= lg_orth;
    orth_z /= lg_orth;
    
    double dist = acos(prod);
    double ex_lat = (lhs_gc_x == 0 ? dist : atan(orth_x/lhs_gc_x));
    if (ex_lat > dist)
      ex_lat = dist;
    if (ex_lat < 0)
      ex_lat = 0;
    
    Point_Double ex_pt = interpolation_point(orth_x, orth_y, orth_z,
          lhs_gc_x, lhs_gc_z, ex_lat, lhs_lon);
    double acceptable_max_length = .0065536*cos(ex_pt.lat * deg_to_arc);
    if (acceptable_max_length < .0000256)
      acceptable_max_length = .0000256;
    acceptable_max_length *= deg_to_arc;
    
    int num_sections_l = (int)(ex_lat/acceptable_max_length)+1;
    int num_sections_r = (int)((dist - ex_lat)/acceptable_max_length)+1;
    
    for (int j = 1; j < num_sections_l; ++j)
      target.push_back(interpolation_point(orth_x, orth_y, orth_z,
          lhs_gc_x, lhs_gc_z, ex_lat*j/num_sections_l, lhs_lon));

    if (ex_lat > 0 && ex_lat < dist)
      target.push_back(ex_pt);
      
    for (int j = 1; j < num_sections_r; ++j)
      target.push_back(interpolation_point(orth_x, orth_y, orth_z,
          lhs_gc_x, lhs_gc_z, ex_lat + (dist-ex_lat)*j/num_sections_r, lhs_lon));

    if (rhs_lon < -180.)
      rhs_lon += 360.;
    else if (rhs_lon > 180.)
      rhs_lon -= 360.;
    target.push_back(Point_Double(rhs_lat, rhs_lon));
  }
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
  
  Four_Field_Index four_field_idx;

  std::vector< unsigned int >::const_iterator gap_it = gap_positions.begin();
  for (unsigned int i = 1; i < all_segments.size(); ++i)
  {
    if (*gap_it == i)
    {
      ++gap_it;
      continue;
    }
    
    four_field_idx.add_segment(all_segments[i-1].lat, all_segments[i-1].lon,
        all_segments[i].lat, all_segments[i].lon);
  }

  gap_it = gap_positions.begin();
  for (unsigned int i = 0; i < all_segments.size(); ++i)
  {
    if (*gap_it == i)
    {
      ++gap_it;
      linestrings.push_back(std::vector< Point_Double >());
    }
    
    linestrings.back().push_back(all_segments[i]);
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
  else if (wrapped || east - west > 180.0)
    // In this special case we should check whether the bounding box should rather cross the date line
  {
    double wrapped_west = 180.0;
    double wrapped_east = -180.0;
    
    for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
    {
      if ((*it)->east() <= 0)
      {
	wrapped_east = std::max(wrapped_east, (*it)->west());
	if ((*it)->west() >= 0)
	  wrapped_west = std::min(wrapped_west, (*it)->west());
      }
      else if ((*it)->west() >= 0)
	wrapped_west = std::min(wrapped_west, (*it)->west());
      else
	// The components are too wildly distributed
	return new Bbox_Double(south, -180.0, north, 180.0);
    }
    
    if (wrapped_west - wrapped_east > 180.0)
      return new Bbox_Double(south, wrapped_west, north, wrapped_east);
    else
      // The components are too wildly distributed
      return new Bbox_Double(south, -180.0, north, 180.0);
  }
  else
    return new Bbox_Double(south, west, north, east);
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
