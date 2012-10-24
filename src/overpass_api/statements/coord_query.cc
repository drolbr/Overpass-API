/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#include <iomanip>

#include "../../template_db/block_backend.h"
#include "coord_query.h"

using namespace std;

bool Coord_Query_Statement::is_used_ = false;

Generic_Statement_Maker< Coord_Query_Statement > Coord_Query_Statement::statement_maker("coord-query");

Coord_Query_Statement::Coord_Query_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  is_used_ = true;

  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["lat"] = "";
  attributes["lon"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  output = attributes["into"];
  
  lat = 100.0;
  lon = 200.0;
  if (attributes["lat"] != "" || attributes["lon"] != "")
  {
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

uint32 Coord_Query_Statement::shifted_lat(uint32 ll_index, uint64 coord)
{
  uint32 lat(0);
  coord |= (((uint64)ll_index)<<32);
/*  for (uint32 i(0); i < 16; ++i)
  {
    lat |= (((uint64)0x1<<(31-2*i))&(coord>>32))<<i;
    lat |= (((uint64)0x1<<(31-2*i))&coord)>>(16-i);
  }*/
  // manual loop unrolling
  lat |= (((uint64)0x1<<(31-2*0))&(coord>>32))<<0;
  lat |= (((uint64)0x1<<(31-2*0))&coord)>>(16-0);
  lat |= (((uint64)0x1<<(31-2*1))&(coord>>32))<<1;
  lat |= (((uint64)0x1<<(31-2*1))&coord)>>(16-1);
  lat |= (((uint64)0x1<<(31-2*2))&(coord>>32))<<2;
  lat |= (((uint64)0x1<<(31-2*2))&coord)>>(16-2);
  lat |= (((uint64)0x1<<(31-2*3))&(coord>>32))<<3;
  lat |= (((uint64)0x1<<(31-2*3))&coord)>>(16-3);
  lat |= (((uint64)0x1<<(31-2*4))&(coord>>32))<<4;
  lat |= (((uint64)0x1<<(31-2*4))&coord)>>(16-4);
  lat |= (((uint64)0x1<<(31-2*5))&(coord>>32))<<5;
  lat |= (((uint64)0x1<<(31-2*5))&coord)>>(16-5);
  lat |= (((uint64)0x1<<(31-2*6))&(coord>>32))<<6;
  lat |= (((uint64)0x1<<(31-2*6))&coord)>>(16-6);
  lat |= (((uint64)0x1<<(31-2*7))&(coord>>32))<<7;
  lat |= (((uint64)0x1<<(31-2*7))&coord)>>(16-7);
  lat |= (((uint64)0x1<<(31-2*8))&(coord>>32))<<8;
  lat |= (((uint64)0x1<<(31-2*8))&coord)>>(16-8);
  lat |= (((uint64)0x1<<(31-2*9))&(coord>>32))<<9;
  lat |= (((uint64)0x1<<(31-2*9))&coord)>>(16-9);
  lat |= (((uint64)0x1<<(31-2*10))&(coord>>32))<<10;
  lat |= (((uint64)0x1<<(31-2*10))&coord)>>(16-10);
  lat |= (((uint64)0x1<<(31-2*11))&(coord>>32))<<11;
  lat |= (((uint64)0x1<<(31-2*11))&coord)>>(16-11);
  lat |= (((uint64)0x1<<(31-2*12))&(coord>>32))<<12;
  lat |= (((uint64)0x1<<(31-2*12))&coord)>>(16-12);
  lat |= (((uint64)0x1<<(31-2*13))&(coord>>32))<<13;
  lat |= (((uint64)0x1<<(31-2*13))&coord)>>(16-13);
  lat |= (((uint64)0x1<<(31-2*14))&(coord>>32))<<14;
  lat |= (((uint64)0x1<<(31-2*14))&coord)>>(16-14);
  lat |= (((uint64)0x1<<(31-2*15))&(coord>>32))<<15;
  lat |= (((uint64)0x1<<(31-2*15))&coord)>>(16-15);
  return lat;
}

int32 Coord_Query_Statement::lon_(uint32 ll_index, uint64 coord)
{
  int32 lon(0);
  coord |= (((uint64)ll_index)<<32);
/*  for (uint32 i(0); i < 16; ++i)
  {
    lon |= (((uint64)0x1<<(30-2*i))&(coord>>32))<<(i+1);
    lon |= (((uint64)0x1<<(30-2*i))&coord)>>(15-i);
  }*/
  // manual loop unrolling
  lon |= (((uint64)0x1<<(30-2*0))&(coord>>32))<<(0+1);
  lon |= (((uint64)0x1<<(30-2*0))&coord)>>(15-0);
  lon |= (((uint64)0x1<<(30-2*1))&(coord>>32))<<(1+1);
  lon |= (((uint64)0x1<<(30-2*1))&coord)>>(15-1);
  lon |= (((uint64)0x1<<(30-2*2))&(coord>>32))<<(2+1);
  lon |= (((uint64)0x1<<(30-2*2))&coord)>>(15-2);
  lon |= (((uint64)0x1<<(30-2*3))&(coord>>32))<<(3+1);
  lon |= (((uint64)0x1<<(30-2*3))&coord)>>(15-3);
  lon |= (((uint64)0x1<<(30-2*4))&(coord>>32))<<(4+1);
  lon |= (((uint64)0x1<<(30-2*4))&coord)>>(15-4);
  lon |= (((uint64)0x1<<(30-2*5))&(coord>>32))<<(5+1);
  lon |= (((uint64)0x1<<(30-2*5))&coord)>>(15-5);
  lon |= (((uint64)0x1<<(30-2*6))&(coord>>32))<<(6+1);
  lon |= (((uint64)0x1<<(30-2*6))&coord)>>(15-6);
  lon |= (((uint64)0x1<<(30-2*7))&(coord>>32))<<(7+1);
  lon |= (((uint64)0x1<<(30-2*7))&coord)>>(15-7);
  lon |= (((uint64)0x1<<(30-2*8))&(coord>>32))<<(8+1);
  lon |= (((uint64)0x1<<(30-2*8))&coord)>>(15-8);
  lon |= (((uint64)0x1<<(30-2*9))&(coord>>32))<<(9+1);
  lon |= (((uint64)0x1<<(30-2*9))&coord)>>(15-9);
  lon |= (((uint64)0x1<<(30-2*10))&(coord>>32))<<(10+1);
  lon |= (((uint64)0x1<<(30-2*10))&coord)>>(15-10);
  lon |= (((uint64)0x1<<(30-2*11))&(coord>>32))<<(11+1);
  lon |= (((uint64)0x1<<(30-2*11))&coord)>>(15-11);
  lon |= (((uint64)0x1<<(30-2*12))&(coord>>32))<<(12+1);
  lon |= (((uint64)0x1<<(30-2*12))&coord)>>(15-12);
  lon |= (((uint64)0x1<<(30-2*13))&(coord>>32))<<(13+1);
  lon |= (((uint64)0x1<<(30-2*13))&coord)>>(15-13);
  lon |= (((uint64)0x1<<(30-2*14))&(coord>>32))<<(14+1);
  lon |= (((uint64)0x1<<(30-2*14))&coord)>>(15-14);
  lon |= (((uint64)0x1<<(30-2*15))&(coord>>32))<<(15+1);
  lon |= (((uint64)0x1<<(30-2*15))&coord)>>(15-15);
  
  lon ^= 0x80000000;
  return lon;
}

/*overall strategy: we count the number of intersections of a straight line from
  the coordinates to the southern end of the block. If it is odd, the coordinate
  is inside the area, if not, they are not.
*/
int Coord_Query_Statement::check_area_block
    (uint32 ll_index, const Area_Block& area_block,
     uint32 coord_lat, int32 coord_lon)
{
  // An area block is a chain of segments. We consider each
  // segment individually. This falls into different cases, determined by
  // the relative position of the intersection of the segment and a straight
  // line from north to south through the coordinate to test.
  // (1) If the segment intersects north of the coordinates or doesn't intersect
  // at all, we can ignore it.
  // (2) If the segment intersects at exactly the coordinates, the coordinates
  // are on the boundary, hence in our definition part of the area.
  // (3) If the segment intersects south of us, it toggles whether we are inside
  // or outside the area
  // (4) A special case is if one endpoint is the intersection point. We then toggle
  // only either the western or the eastern side. We are part of the area if in the
  // end the western or eastern side have an odd state.
  int state = 0;
  vector< uint64 >::const_iterator it(area_block.coors.begin());
  uint32 lat = shifted_lat(ll_index, *it);
  int32 lon = lon_(ll_index, *it);
  while (++it != area_block.coors.end())
  {
    uint32 last_lat = lat;
    int32 last_lon = lon;
    lon = lon_(ll_index, *it);
    lat = shifted_lat(ll_index, *it);
    
    if (last_lon < lon)
    {
      if (lon < coord_lon)
	continue; // case (1)
      else if (last_lon > coord_lon)
	continue; // case (1)
      else if (lon == coord_lon)
      {
	if (lat < coord_lat)
	  state ^= TOGGLE_WEST; // case (4)
	else if (lat == coord_lat)
	  return HIT; // case (2)
	// else: case (1)
	continue;
      }
      else if (last_lon == coord_lon)
      {
	if (last_lat < coord_lat)
	  state ^= TOGGLE_EAST; // case (4)
	else if (last_lat == coord_lat)
	  return HIT; // case (2)
	// else: case (1)
	continue;
      }
    }
    else if (last_lon > lon)
    {
      if (lon > coord_lon)
	continue; // case (1)
      else if (last_lon < coord_lon)
	continue; // case (1)
      else if (lon == coord_lon)
      {
	if (lat < coord_lat)
	  state ^= TOGGLE_EAST; // case (4)
	else if (lat == coord_lat)
	  return HIT; // case (2)
	// else: case (1)
	continue;
      }
      else if (last_lon == coord_lon)
      {
	if (last_lat < coord_lat)
	  state ^= TOGGLE_WEST; // case (4)
	else if (last_lat == coord_lat)
	  return HIT; // case (2)
	// else: case (1)
	continue;
      }
    }
    else // last_lon == lon should normally not happen and can be safely ignored
      continue; // otherwise.
    
    uint32 intersect_lat = lat +
        ((int64)coord_lon - lon)*((int64)last_lat - lat)/((int64)last_lon - lon);
    if (coord_lat > intersect_lat)
      state ^= (TOGGLE_EAST | TOGGLE_WEST); // case (3)
    else if (coord_lat == intersect_lat)
      return HIT; // case (2)
    // else: case (1)
  }
  return state;
}

void Coord_Query_Statement::execute(Resource_Manager& rman)
{ 
  if (rman.area_updater())
    rman.area_updater()->flush();
  
  set< Uint31_Index > req;
  map< Uint31_Index, vector< pair< double, double > > > coord_per_req;
  
  map< pair< double, double >, map< Area::Id_Type, int > > areas_inside;
  set< Area::Id_Type > areas_found;
  
  if (lat != 100.0)
  {
    Uint31_Index idx = Uint31_Index(::ll_upper_(lat, lon) & 0xffffff00);
    req.insert(idx);
    coord_per_req[idx].push_back(make_pair(lat, lon));
  }
  else
  {
    const map< Uint32_Index, vector< Node_Skeleton > >& nodes = rman.sets()[input].nodes;
    for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = nodes.begin();
	 it != nodes.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
	double lat = ::lat(it->first.val(), it2->ll_lower);
	double lon = ::lon(it->first.val(), it2->ll_lower);
        Uint31_Index idx = Uint31_Index(::ll_upper_(lat, lon) & 0xffffff00);
        req.insert(idx);
        coord_per_req[idx].push_back(make_pair(lat, lon));
      }
    }
  }

  map< Uint31_Index, vector< pair< double, double > > >::const_iterator coord_block_it = coord_per_req.begin();
  Uint31_Index last_idx = req.empty() ? Uint31_Index(0u) : *req.begin();
  
  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (rman.get_area_transaction()->data_index(area_settings().AREA_BLOCKS));
  for (Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      it(area_blocks_db.discrete_begin(req.begin(), req.end()));
      !(it == area_blocks_db.discrete_end()); ++it)
  {
    if (!(it.index() == last_idx))
    {
      last_idx = it.index();
      
      for (map< pair< double, double >, map< Area::Id_Type, int > >::const_iterator
	  inside_it = areas_inside.begin(); inside_it != areas_inside.end(); ++inside_it)
      {
	for (map< Area::Id_Type, int >::const_iterator inside_it2 = inside_it->second.begin();
	     inside_it2 != inside_it->second.end(); ++inside_it2)
	{
	  if (inside_it2->second != 0)
	    areas_found.insert(inside_it2->first);
	}
      }
      areas_inside.clear();
      
      while (coord_block_it != coord_per_req.end() && coord_block_it->first < it.index())
        ++coord_block_it;
      if (coord_block_it == coord_per_req.end())
        break;
    }

    for (vector< pair< double, double > >::const_iterator coord_it = coord_block_it->second.begin();
	 coord_it != coord_block_it->second.end(); ++coord_it)
    {
      uint32 ilat((coord_it->first + 91.0)*10000000+0.5);
      int32 ilon(coord_it->second*10000000 + (coord_it->second > 0 ? 0.5 : -0.5));
  
      int check = check_area_block(it.index().val(), it.object(), ilat, ilon);
      if (check == HIT)
        areas_found.insert(it.object().id);
      else if (check != 0)
      {
        map< Area::Id_Type, int >::iterator it2 = areas_inside[*coord_it].find(it.object().id);
        if (it2 != areas_inside[*coord_it].end())
	  it2->second ^= check;
        else
	  areas_inside[*coord_it].insert(make_pair(it.object().id, check));
      }
    }
  }
  
  for (map< pair< double, double >, map< Area::Id_Type, int > >::const_iterator
      inside_it = areas_inside.begin(); inside_it != areas_inside.end(); ++inside_it)
  {
    for (map< Area::Id_Type, int >::const_iterator inside_it2 = inside_it->second.begin();
        inside_it2 != inside_it->second.end(); ++inside_it2)
    {
      if (inside_it2->second != 0)
        areas_found.insert(inside_it2->first);
    }
  }
  areas_inside.clear();
  
  map< Uint32_Index, vector< Node_Skeleton > >& nodes
      (rman.sets()[output].nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways
      (rman.sets()[output].ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations
      (rman.sets()[output].relations);
  map< Uint31_Index, vector< Area_Skeleton > >& areas
      (rman.sets()[output].areas);
  
  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();

  Block_Backend< Uint31_Index, Area_Skeleton > area_locations_db
      (rman.get_area_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it(area_locations_db.flat_begin());
      !(it == area_locations_db.flat_end()); ++it)
  {
    if (areas_found.find(it.object().id) != areas_found.end())
      areas[it.index()].push_back(it.object());
//     else if ((areas_inside.find(it.object().id) != areas_inside.end())
//         && (areas_inside[it.object().id] != 0))
//       areas[it.index()].push_back(it.object());
  }

  rman.health_check(*this);
}
