#include "geometry.h"


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
  has_coords |= (point.lat < 100.);
  points.push_back(point);
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
