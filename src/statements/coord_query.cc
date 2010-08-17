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

#include "../backend/block_backend.h"
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
  lat = atof(attributes["lat"].c_str());
  if ((lat < -90.0) || (lat > 90.0) || (attributes["lat"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"lat\" of the element \"coord-query\""
	<<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  lon = atof(attributes["lon"].c_str());
  if ((lon < -180.0) || (lon > 180.0) || (attributes["lon"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"lon\" of the element \"coord-query\""
	<<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
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

int32 Coord_Query_Statement::lon_(uint32 ll_index, uint64 coord)
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
*/
int Coord_Query_Statement::check_area_block
    (uint32 ll_index, const Area_Block& area_block,
     uint32 coord_lat, int32 coord_lon)
{
  vector< uint64 >::const_iterator it(area_block.coors.begin());
  bool odd_segs_below(false);
  if (it == area_block.coors.end())
    return 0;
  uint32 lat(shifted_lat(ll_index, *it));
  int32 lon(lon_(ll_index, *it));
  if ((coord_lon == lon) && (coord_lat == lat))
    return HIT;
  bool last_less(lon < coord_lon);
  while (++it != area_block.coors.end())
  {
    uint32 last_lat(lat);
    int32 last_lon(lon);
    lat = shifted_lat(ll_index, *it);
    lon = lon_(ll_index, *it);
    if (last_less)
    {
      if (lon < coord_lon)
	continue;
      last_less = false;
    }
    else
    {
      if (lon == coord_lon)
      {
	if (last_lon == coord_lon)
	{
	  if ((last_lat <= coord_lat) && (lat >= coord_lat))
	    return HIT;
	  if ((lat <= coord_lat) && (last_lat >= coord_lat))
	    return HIT;
	}
	int check(check_segment
	    (last_lat, last_lon, lat, lon, coord_lat, coord_lon));
	if (check == HIT)
	  return HIT;
      }
      if (lon >= coord_lon)
	continue;
      last_less = true;
    }
    int check(check_segment
        (last_lat, last_lon, lat, lon, coord_lat, coord_lon));
    if (check == HIT)
      return HIT;
    if (check != 0)
      odd_segs_below = !odd_segs_below;
  }
  if (odd_segs_below)
    return TOGGLE;
  return 0;
}

void Coord_Query_Statement::execute(map< string, Set >& maps)
{ 
  set< Uint31_Index > req;
  set< uint32 > areas_inside;
  set< uint32 > areas_on_border;
  req.insert(Uint31_Index(Node::ll_upper(lat, lon) & 0xffffff00));

  uint32 ilat((lat + 91.0)*10000000+0.5);
  int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
  
  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (*de_osm3s_file_ids::AREA_BLOCKS, false);
  for (Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      it(area_blocks_db.discrete_begin(req.begin(), req.end()));
      !(it == area_blocks_db.discrete_end()); ++it)
  {
    int check(check_area_block(it.index().val(), it.object(), ilat, ilon));
    if (check == HIT)
      areas_on_border.insert(it.object().id);
    else if (check == TOGGLE)
    {
      if (areas_inside.find(it.object().id) != areas_inside.end())
	areas_inside.erase(it.object().id);
      else
	areas_inside.insert(it.object().id);
    }
  }
/*  cout<<lat<<", "<<lon<<":";
  for (set< uint32 >::const_iterator it(areas_inside.begin()); it != areas_inside.end(); ++it)
    cout<<' '<<*it;
  cout<<"; ";
  for (set< uint32 >::const_iterator it(areas_on_border.begin()); it != areas_on_border.end(); ++it)
    cout<<' '<<*it;
  cout<<'\n';*/
  //cout<<(areas_inside.size() + areas_on_border.size());
}
