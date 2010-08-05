#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "coord_query.h"

using namespace std;

void Coord_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["lat"] = "";
  attributes["lon"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  double lat_d(atof(attributes["lat"].c_str()));
  if ((lat_d < -90.0) || (lat_d > 90.0) || (attributes["lat"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"lat\" of the element \"coord-query\""
	<<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  double lon_d(atof(attributes["lon"].c_str()));
  if ((lon_d < -180.0) || (lon_d > 180.0) || (attributes["lon"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"lon\" of the element \"coord-query\""
	<<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  //lat = (int)(lat_d * 10000000 + 0.5);
  //lon = (int)(lon_d * 10000000 + 0.5);
}

void Coord_Query_Statement::forecast()
{
/*  Set_Forecast& sf_out(declare_write_set(output));
    
  sf_out.area_count = 10;
  declare_used_time(10);
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

int Coord_Query_Statement::check_segment
  (uint32 a_lat, int32 a_lon, uint32 b_lat, int32 b_lon,
   uint32 coord_lat, int32 coord_lon)
{
  uint32 lower_lat(0);
  if (a_lon < b_lon)
    if (a_lat < b_lat)
      lower_lat = ((int64)b_lat - a_lat)*
          (coord_lon - a_lon)/(b_lon - a_lon + 1) + a_lat;
    else
      lower_lat = ((int64)b_lat - a_lat)*
          (coord_lon - a_lon + 1)/(b_lon - a_lon + 1) + a_lat;
  else
    if (a_lat < b_lat)
      lower_lat = ((int64)a_lat - b_lat)*
          (coord_lon - b_lon + 1)/(a_lon - b_lon + 1) + b_lat;
    else
      lower_lat = ((int64)a_lat - b_lat)*
          (coord_lon - b_lon)/(a_lon - b_lon + 1) + b_lat;
  if (lower_lat > coord_lat)
    return 0;
  uint32 upper_lat(0);
  if (a_lon < b_lon)
    if (a_lat < b_lat)
      upper_lat = ((int64)b_lat - a_lat)*
          (coord_lon - a_lon + 1)/(b_lon - a_lon + 1) + a_lat;
    else
      upper_lat = ((int64)b_lat - a_lat)*
          (coord_lon - a_lon)/(b_lon - a_lon + 1) + a_lat;
  else
    if (a_lat < b_lat)
      upper_lat = ((int64)a_lat - b_lat)*
          (coord_lon - b_lon)/(a_lon - b_lon + 1) + b_lat;
    else
      upper_lat = ((int64)a_lat - b_lat)*
          (coord_lon - b_lon + 1)/(a_lon - b_lon + 1) + b_lat;
  if (upper_lat >= coord_lat)
    return HIT;
  return TOGGLE;
}

uint32 Coord_Query_Statement::shifted_lat(uint32 ll_index, uint64 coord)
{
  uint32 lat(0);
  coord |= (((uint64)ll_index)<<32);
  for (uint32 i(0); i < 16; i+=1)
  {
    lat |= (((uint64)0x1<<(31-2*i))&(coord>>32))<<i;
    lat |= (((uint64)0x1<<(31-2*i))&coord)>>(16-i);
  }
  return lat;
}

int32 Coord_Query_Statement::lon(uint32 ll_index, uint64 coord)
{
  int32 lon(0);
  coord |= (((uint64)ll_index)<<32);
  for (uint32 i(0); i < 16; i+=1)
  {
    lon |= (((uint64)0x1<<(30-2*i))&(coord>>32))<<(i+1);
    lon |= (((uint64)0x1<<(30-2*i))&coord)>>(15-i);
  }
  return lon;
}

/*overall strategy: we count the number of intersections of a straight line from
  the coordinates to the southern end of the block. If it is odd, the coordinate
  is inside the area, if not, they are not.
  
  possible cases for a segment:
  - both lons are less than the lon of the coordinate => don't count
  - the segment is north-south and contains the coordinate => is anyway contained
  - the coordinate is on the segment => is anyway contained
  - both lats are equal or bigger than the lats of the coordinate => don't count
  - one lon is less and the other one is bigger than the lon of the coordinate
    => iff the intersection is equal or south of the coordinate's lat, then count
    
    if (end) return
    lon = ...
    bool less(lon < coord_lon);
    old_lon = lon, old_lat = lat
    while (!end)
    {
      lon = ...
      if (less)
        if (lon < coord_lon)
	  continue
	less = false
      else
	if (lon == coord_lon)
	  if (old_lon == coord_lon)
	    if ((old_lat <= coord_lat) && (lat >= coord_lat)) || ...
	      SEGMENT_HIT
	  else
	    check_segment(lat, lon, old_lat, old_lon, coord_lat, coord_lon)
	if (lon >= coord_lon)
	  continue
	less = true
      check_segment(lat, lon, old_lat, old_lon, coord_lat, coord_lon)
      old_lon = lon, old_lat = lat
    }
*/
/* other notes:
  - baseline segments
  - proportion with zero sizes
  - proportion at the boundary
*/
int Coord_Query_Statement::check_area_block
    (uint32 ll_index, const Area_Block& area_block,
     uint32 coord_lat, int32 coord_lon)
{
  if (area_block.coors.size() == 0)
    return 0;
  uint64 coord(area_block.coors.front());
  int32 lat(Node::lat(ll_index | (coord>>32), coord & 0xfffffffful));
  int32 lon(Node::lat(ll_index | (coord>>32), coord & 0xfffffffful));
  //...
}
 
void Coord_Query_Statement::execute(map< string, Set >& maps)
{ 
/*  ostringstream temp;
  temp<<"select id, min_lat, min_lon, max_lat, max_lon from area_segments "
      <<"where ll_idx = "<<(ll_idx(lat, lon) & 0xffffff55);
  
  set< Area > areas;
  maps[output] = Set(set< Node >(), set< Way >(), set< Relation_ >(),
		     multiArea_query(mysql, temp.str(), lat, lon, areas));*/
}
