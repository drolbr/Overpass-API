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
#include "../data/collect_items.h"
#include "../data/tilewise_geometry.h"
#include "coord_query.h"


bool Coord_Query_Statement::is_used_ = false;

Generic_Statement_Maker< Coord_Query_Statement > Coord_Query_Statement::statement_maker("coord-query");

Coord_Query_Statement::Coord_Query_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  is_used_ = true;

  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["lat"] = "";
  attributes["lon"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);

  lat = 100.0;
  lon = 200.0;
  if (attributes["lat"] != "" || attributes["lon"] != "")
  {
    lat = atof(attributes["lat"].c_str());
    if ((lat < -90.0) || (lat > 90.0) || (attributes["lat"] == ""))
    {
      std::ostringstream temp;
      temp<<"For the attribute \"lat\" of the element \"coord-query\""
          <<" the only allowed values are floats between -90.0 and 90.0.";
      add_static_error(temp.str());
    }

    lon = atof(attributes["lon"].c_str());
    if ((lon < -180.0) || (lon > 180.0) || (attributes["lon"] == ""))
    {
      std::ostringstream temp;
      temp<<"For the attribute \"lon\" of the element \"coord-query\""
          <<" the only allowed values are floats between -180.0 and 180.0.";
      add_static_error(temp.str());
    }
  }

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
  std::vector< uint64 >::const_iterator it(area_block.coors.begin());
  uint32 lat = ::ilat(ll_index | (((*it)>>32)&0xff), (*it & 0xffffffff));
  int32 lon = ::ilon(ll_index | (((*it)>>32)&0xff), (*it & 0xffffffff));
  while (++it != area_block.coors.end())
  {
    uint32 last_lat = lat;
    int32 last_lon = lon;
    lon = ::ilon(ll_index | (((*it)>>32)&0xff), (*it & 0xffffffff));
    lat = ::ilat(ll_index | (((*it)>>32)&0xff), (*it & 0xffffffff));

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
    else // last_lon == lon
    {
      if (lon == coord_lon &&
          ((last_lat <= coord_lat && coord_lat <= lat) || (lat <= coord_lat && coord_lat <= last_lat)))
        return HIT; // case (2)
      continue; // else: case (1)
    }

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


void register_coord(double lat, double lon,
    std::set< Uint31_Index >& req, std::map< Uint31_Index, std::vector< std::pair< double, double > > >& coord_per_req)
{
  Uint31_Index idx = Uint31_Index(::ll_upper_(lat, lon) & 0xffffff00);
  req.insert(idx);
  coord_per_req[idx].push_back(std::make_pair(lat, lon));
}


struct Closedness_Predicate
{
  bool match(const Way_Skeleton& obj) const { return !obj.nds.empty() && obj.nds.front() == obj.nds.back(); }
  bool match(const Handle< Way_Skeleton >& h) const
  { return !h.object().nds.empty() && h.object().nds.front() == h.object().nds.back(); }
  bool match(const Handle< Attic< Way_Skeleton > >& h) const
  { return !h.object().nds.empty() && h.object().nds.front() == h.object().nds.back(); }
};


void Coord_Query_Statement::execute(Resource_Manager& rman)
{
  if (rman.area_updater())
    rman.area_updater()->flush();

  std::set< Uint31_Index > req;
  std::set< Uint31_Index > node_idxs;
  std::map< Uint31_Index, std::vector< std::pair< double, double > > > coord_per_req;

  const Set* input_set = 0;
  if (lat != 100.0)
  {
    node_idxs.insert(::ll_upper_(lat, lon));
    register_coord(lat, lon, req, coord_per_req);
  }
  else
  {
    input_set = rman.get_set(input);
    if (input_set)
    {
      const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes = input_set->nodes;
      for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator it = nodes.begin();
	  it != nodes.end(); ++it)
      {
        node_idxs.insert(Uint31_Index(it->first.val()));
        for (std::vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
          register_coord(::lat(it->first.val(), it2->ll_lower), ::lon(it->first.val(), it2->ll_lower),
              req, coord_per_req);
      }

      const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& attic_nodes = input_set->attic_nodes;
      for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator it = attic_nodes.begin();
          it != attic_nodes.end(); ++it)
      {
        node_idxs.insert(Uint31_Index(it->first.val()));
        for (std::vector< Attic< Node_Skeleton > >::const_iterator it2 = it->second.begin();
            it2 != it->second.end(); ++it2)
          register_coord(::lat(it->first.val(), it2->ll_lower), ::lon(it->first.val(), it2->ll_lower),
              req, coord_per_req);
      }
    }
  }

  std::map< std::pair< double, double >, std::map< Area::Id_Type, int > > areas_inside;
  std::set< Area::Id_Type > areas_found;

  std::map< Uint31_Index, std::vector< std::pair< double, double > > >::const_iterator coord_block_it = coord_per_req.begin();
  Uint31_Index last_idx = req.empty() ? Uint31_Index(0u) : *req.begin();

  Block_Backend< Uint31_Index, Area_Block, std::set< Uint31_Index >::const_iterator > area_blocks_db
      (rman.get_area_transaction()->data_index(area_settings().AREA_BLOCKS));
  for (Block_Backend< Uint31_Index, Area_Block, std::set< Uint31_Index >::const_iterator >::Discrete_Iterator
      it(area_blocks_db.discrete_begin(req.begin(), req.end()));
      !(it == area_blocks_db.discrete_end()); ++it)
  {
    if (!(it.index() == last_idx))
    {
      last_idx = it.index();

      for (std::map< std::pair< double, double >, std::map< Area::Id_Type, int > >::const_iterator
	  inside_it = areas_inside.begin(); inside_it != areas_inside.end(); ++inside_it)
      {
	for (std::map< Area::Id_Type, int >::const_iterator inside_it2 = inside_it->second.begin();
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

    for (std::vector< std::pair< double, double > >::const_iterator coord_it = coord_block_it->second.begin();
	 coord_it != coord_block_it->second.end(); ++coord_it)
    {
      uint32 ilat((coord_it->first + 91.0)*10000000+0.5);
      int32 ilon(coord_it->second*10000000 + (coord_it->second > 0 ? 0.5 : -0.5));

      int check = check_area_block(it.index().val(), it.object(), ilat, ilon);
      if (check == HIT)
        areas_found.insert(it.object().id);
      else if (check != 0)
      {
        std::map< Area::Id_Type, int >::iterator it2 = areas_inside[*coord_it].find(it.object().id);
        if (it2 != areas_inside[*coord_it].end())
	  it2->second ^= check;
        else
	  areas_inside[*coord_it].insert(std::make_pair(it.object().id, check));
      }
    }
  }

  for (std::map< std::pair< double, double >, std::map< Area::Id_Type, int > >::const_iterator
      inside_it = areas_inside.begin(); inside_it != areas_inside.end(); ++inside_it)
  {
    for (std::map< Area::Id_Type, int >::const_iterator inside_it2 = inside_it->second.begin();
        inside_it2 != inside_it->second.end(); ++inside_it2)
    {
      if (inside_it2->second != 0)
        areas_found.insert(inside_it2->first);
    }
  }
  areas_inside.clear();

  Set into;

  if (!areas_found.empty())
  {
    std::vector< uint32 > req_v;
    for (std::set< Uint31_Index >::const_iterator it = req.begin(); it != req.end(); ++it)
      req_v.push_back(it->val());
    std::vector< uint32 > idx_req_v = ::calc_parents(req_v);
    std::vector< Uint31_Index > idx_req;
    for (std::vector< uint32 >::const_iterator it = idx_req_v.begin(); it != idx_req_v.end(); ++it)
      idx_req.push_back(*it);
    sort(idx_req.begin(), idx_req.end());
    Block_Backend< Uint31_Index, Area_Skeleton, std::vector< Uint31_Index >::const_iterator > area_locations_db
        (rman.get_area_transaction()->data_index(area_settings().AREAS));
    for (Block_Backend< Uint31_Index, Area_Skeleton, std::vector< Uint31_Index >::const_iterator >::Discrete_Iterator
        it = area_locations_db.discrete_begin(idx_req.begin(), idx_req.end());
        !(it == area_locations_db.discrete_end()); ++it)
    {
      if (areas_found.find(it.object().id) != areas_found.end())
        into.areas[it.index()].push_back(it.object());
    }
  }

  std::set< Uint31_Index > way_idxs = calc_parents(node_idxs);
  std::map< Uint31_Index, std::vector< Way_Skeleton > > current_candidates;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_candidates;
  Request_Context context(this, rman);
  collect_items_discrete(context, way_idxs, Closedness_Predicate(), current_candidates, attic_candidates);

  if (lat != 100.0)
  {
    Tilewise_Area_Iterator tai(current_candidates, attic_candidates, *this, rman);
    uint32 idx = ::ll_upper_(lat, lon);
    tai.set_limits(ilat(idx, 0u), ilat(idx, 0u));
    while (!tai.is_end() && tai.get_idx().val() < idx)
      tai.next();
    if (!tai.is_end())
      tai.move_covering_ways(idx, ::ll_lower(lat, lon), into.ways, into.attic_ways);
  }
  else if (input_set)
  {
    Tilewise_Area_Iterator tai(current_candidates, attic_candidates, *this, rman);
    uint32 minlat = 0x7fff0000u;
    uint32 maxlat = 0u;
    for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator it = input_set->nodes.begin();
        it != input_set->nodes.end(); ++it)
    {
      minlat = std::min(minlat, ilat(it->first.val(), 0u));
      maxlat = std::max(maxlat, ilat(it->first.val(), 0u));
    }
    for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator it = input_set->attic_nodes.begin();
        it != input_set->attic_nodes.end(); ++it)
    {
      minlat = std::min(minlat, ilat(it->first.val(), 0u));
      maxlat = std::max(maxlat, ilat(it->first.val(), 0u));
    }
    tai.set_limits(minlat, maxlat);

    std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator cur_it = input_set->nodes.begin();
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator attic_it =
        input_set->attic_nodes.begin();

    while (cur_it != input_set->nodes.end() || attic_it != input_set->attic_nodes.end())
    {
      Uint32_Index idx =
          (attic_it == input_set->attic_nodes.end() ||
              (cur_it != input_set->nodes.end() && !(attic_it->first < cur_it->first))) ? cur_it->first : attic_it->first;
      while (!tai.is_end() && tai.get_idx().val() < idx.val())
        tai.next();
      if (tai.is_end())
        break;

      if (cur_it->first == tai.get_idx())
      {
        for (std::vector< Node_Skeleton >::const_iterator it2 = cur_it->second.begin(); it2 != cur_it->second.end();
            ++it2)
          tai.move_covering_ways(cur_it->first.val(), it2->ll_lower, into.ways, into.attic_ways);
      }
      if (attic_it->first == tai.get_idx())
      {
        for (std::vector< Attic< Node_Skeleton > >::const_iterator it2 = attic_it->second.begin();
            it2 != attic_it->second.end(); ++it2)
          tai.move_covering_ways(attic_it->first.val(), it2->ll_lower, into.ways, into.attic_ways);
      }
      while (cur_it != input_set->nodes.end() && !(tai.get_idx().val() < cur_it->first.val()))
        ++cur_it;
      while (attic_it != input_set->attic_nodes.end() && !(tai.get_idx().val() < attic_it->first.val()))
        ++attic_it;
    }
  }

  transfer_output(rman, into);
  rman.health_check(*this);
}
