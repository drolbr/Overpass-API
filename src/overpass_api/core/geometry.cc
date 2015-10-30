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
    south = std::min(south, it->lat);
    west = std::min(west, it->lon);
    north = std::max(north, it->lat);
    east = std::max(west, it->lon);
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
      if (it->lon > 0)
	wrapped_west = std::min(wrapped_west, it->lon);
      else
	wrapped_east = std::max(wrapped_east, it->lon);
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


Bbox_Double* calc_bounds(const std::vector< Opaque_Geometry* >& components)
{
  double south = 100.0;
  double west = 200.0;
  double north = -100.0;
  double east = -200.0;
  bool wrapped = false;
  
  for (std::vector< Opaque_Geometry* >::const_iterator it = components.begin(); it != components.end(); ++it)
  {
    south = std::min(south, (*it)->south());
    north = std::max(north, (*it)->north());
    west = std::min(west, (*it)->west());
    east = std::max(east, (*it)->east());
    
    wrapped |= ((*it)->west() < (*it)->east());
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


void Compound_Geometry::add_component(Opaque_Geometry* component)
{ 
  delete bounds;
  bounds = 0;
  components.push_back(component);
}
