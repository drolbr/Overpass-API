/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__DATA__GEOMETRY_H
#define DE__OSM3S___OVERPASS_API__DATA__GEOMETRY_H


inline bool segment_intersects_bbox
    (double first_lat, double first_lon, double second_lat, double second_lon,
     double south, double north, double west, double east);

    
//-----------------------------------------------------------------------------

    
inline bool segment_intersects_bbox
    (double first_lat, double first_lon, double second_lat, double second_lon,
     double south, double north, double west, double east)
{
  if (first_lat < south)
  {
    if (second_lat < south)
      return false;
    // Otherwise just adjust first_lat and first_lon
    first_lon += (second_lon - first_lon)*(south - first_lat)/(second_lat - first_lat);
    first_lat = south;
  }
  if (first_lat > north)
  {
    if (second_lat > north)
      return false;
    // Otherwise just adjust first_lat and first_lon
    first_lon += (second_lon - first_lon)*(north - first_lat)/(second_lat - first_lat);
    first_lat = north;
  }

  if (second_lat < south)
  {
    // Adjust second_lat and second_lon
    second_lon += (first_lon - second_lon)*(south - second_lat)/(first_lat - second_lat);
    second_lat = south;
  }
  if (second_lat > north)
  {
    // Adjust second_lat and second_lon
    second_lon += (first_lon - second_lon)*(north - second_lat)/(first_lat - second_lat);
    second_lat = north;
  }

  // Now we know that both latitudes are between south and north.
  // Thus we only need to check whether the segment touches the bbox in its east-west-extension.
  if (first_lon < west && second_lon < west)
    return false;
  if (first_lon > east && second_lon > east)
    return false;
  
  return true;
}


#endif
