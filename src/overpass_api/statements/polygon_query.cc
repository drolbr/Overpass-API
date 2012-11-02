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

#include <algorithm>
#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/geometry.h"
#include "coord_query.h"
#include "polygon_query.h"
#include "recurse.h"

using namespace std;

//-----------------------------------------------------------------------------

class Polygon_Constraint : public Query_Constraint
{
  public:
    bool delivers_data();
    
    Polygon_Constraint(Polygon_Query_Statement& polygon_) : polygon(&polygon_) {}
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into);
    void filter(const Statement& query, Resource_Manager& rman, Set& into);
    virtual ~Polygon_Constraint() {}
    
  private:
    Polygon_Query_Statement* polygon;
};


bool Polygon_Constraint::delivers_data()
{
  return false;
}


bool Polygon_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges = polygon->calc_ranges();
  return true;
}


bool Polygon_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  return false;

//   set< pair< Uint32_Index, Uint32_Index > > node_ranges = polygon->calc_ranges();
//   ranges = calc_parents(node_ranges);
//   return true;
}


void Polygon_Constraint::filter(Resource_Manager& rman, Set& into)
{
  polygon->collect_nodes(into.nodes);
  
  into.ways.clear();
  into.relations.clear();
  //TODO: filter areas
}


void Polygon_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  into.ways.clear();
  into.relations.clear();
  //TODO: filter areas
}

//-----------------------------------------------------------------------------


void add_segment_blocks(vector< Aligned_Segment >& segments)
{
  map< Uint31_Index, vector< Aligned_Segment > > by_upper;
  for (vector< Aligned_Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    by_upper[it->ll_upper_].push_back(*it);
  
  /* We use that more northern segments always have bigger indices.
    Thus we can collect each block's end points and add them, if they do not
    cancel out, to the next block further northern.*/
  for (map< Uint31_Index, vector< Aligned_Segment > >::const_iterator
      it = by_upper.begin(); it != by_upper.end(); ++it)
  {
    set< int32 > lons;
    
    for (vector< Aligned_Segment >::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2)
    {
      int32 lon_front(ilon(it->first.val() | (it2->ll_lower_a>>32), (it2->ll_lower_a) & 0xffffffffull));
      int32 lon_back(ilon(it->first.val() | (it2->ll_lower_b>>32), (it2->ll_lower_b) & 0xffffffffull));
      if (lons.find(lon_front) == lons.end())
	lons.insert(lon_front);
      else
	lons.erase(lon_front);
      if (lons.find(lon_back) == lons.end())
	lons.insert(lon_back);
      else
	lons.erase(lon_back);
    }
    
    if (lons.empty())
      continue;
    
    // calc lat
    uint32 lat = ilat(it->first.val(), 0) + 16*65536;
    int32 lon = ilon(it->first.val(), 0);
    uint32 northern_ll_upper = ::ll_upper_(lat, lon);
    
    // insert lons
    vector< Aligned_Segment >& northern_block(by_upper[northern_ll_upper]);
    for (set< int32 >::const_iterator it2 = lons.begin(); it2 != lons.end(); ++it2)
    {
      int32 from(*it2);
      ++it2;
      int32 to(*it2);
      Aligned_Segment segment;
      segment.ll_upper_ = northern_ll_upper;
      segment.ll_lower_a = (((uint64)(::ll_upper_(lat, from)) & 0xff)<<32) | ::ll_lower(lat, from);
      segment.ll_lower_b = (((uint64)(::ll_upper_(lat, to)) & 0xff)<<32) | ::ll_lower(lat, to);
      
      northern_block.push_back(segment);
    }
  }

  segments.clear();
  for (map< Uint31_Index, vector< Aligned_Segment > >::const_iterator it = by_upper.begin();
       it != by_upper.end(); ++it)
  {
    for (vector< Aligned_Segment >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      segments.push_back(*it2);
  }
}


Generic_Statement_Maker< Polygon_Query_Statement > Polygon_Query_Statement::statement_maker("polygon-query");


Polygon_Query_Statement::Polygon_Query_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["bounds"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  output = attributes["into"];
  
  //convert bounds
  istringstream in(attributes["bounds"]);
  double first_lat, first_lon;
  if (in.good())
    in>>first_lat>>first_lon;
  double last_lat = first_lat;
  double last_lon = first_lon;
  while (in.good())
  {
    double lat, lon;
    in>>lat>>lon;
    
    Area::calc_aligned_segments(segments, last_lat, last_lon, lat, lon);
    
    last_lat = lat;
    last_lon = lon;
  }
  Area::calc_aligned_segments(segments, last_lat, last_lon, first_lat, first_lon);
  sort(segments.begin(), segments.end());

  add_segment_blocks(segments);
}


Polygon_Query_Statement::~Polygon_Query_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


set< pair< Uint32_Index, Uint32_Index > > Polygon_Query_Statement::calc_ranges()
{
  set< pair< Uint32_Index, Uint32_Index > > result;
  for (vector< Aligned_Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    result.insert(make_pair(it->ll_upper_, it->ll_upper_ + 0x100));
  return result;
}


void Polygon_Query_Statement::forecast()
{
}


void Polygon_Query_Statement::collect_nodes(map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  vector< Aligned_Segment >::const_iterator area_it = segments.begin();
  map< Uint32_Index, vector< Node_Skeleton > >::iterator nodes_it = nodes.begin();
  
  uint32 current_idx(0);
  if (area_it != segments.end())
    current_idx = area_it->ll_upper_;
  
  while (area_it != segments.end())
  {
    vector< Area_Block > areas;
    while (area_it != segments.end() && area_it->ll_upper_ == current_idx)
    {
      Area_Block block;
      block.coors.push_back(area_it->ll_lower_a);
      block.coors.push_back(area_it->ll_lower_b);
      areas.push_back(block);
      ++area_it;
    }
    
    while (nodes_it != nodes.end() && nodes_it->first.val() < current_idx)
    {
      nodes_it->second.clear();
      ++nodes_it;
    }
    while (nodes_it != nodes.end() &&
        (nodes_it->first.val() & 0xffffff00) == current_idx)
    {
      vector< Node_Skeleton > into;
      for (vector< Node_Skeleton >::const_iterator iit = nodes_it->second.begin();
          iit != nodes_it->second.end(); ++iit)
      {
        uint32 ilat((::lat(nodes_it->first.val(), iit->ll_lower)
            + 91.0)*10000000+0.5);
        int32 ilon(::lon(nodes_it->first.val(), iit->ll_lower)*10000000
            + (::lon(nodes_it->first.val(), iit->ll_lower) > 0 ? 0.5 : -0.5));
	
        int inside = 0;
        for (vector< Area_Block >::const_iterator it = areas.begin();
	     it != areas.end(); ++it)
        {
	  int check(Coord_Query_Statement::check_area_block(current_idx, *it, ilat, ilon));
	  if (check == Coord_Query_Statement::HIT)
	  {
	    inside = 1;
	    break;
	  }
	  else if (check != 0)
	    inside ^= check;
        }
        if (inside)
	  into.push_back(*iit);
      }
      nodes_it->second.swap(into);      
      ++nodes_it;
    }
    
    current_idx = area_it->ll_upper_;
  }
  while (nodes_it != nodes.end())
  {
    nodes_it->second.clear();
    ++nodes_it;
  }
}


void Polygon_Query_Statement::execute(Resource_Manager& rman)
{
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

  set< pair< Uint32_Index, Uint32_Index > > nodes_req = calc_ranges();

  collect_items_range(this, rman, *osm_base_settings().NODES, nodes_req,
		      Trivial_Predicate< Node_Skeleton >(), nodes);
  
  collect_nodes(nodes);
  
  rman.health_check(*this);
}

Query_Constraint* Polygon_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Polygon_Constraint(*this));
  return constraints.back();
}
