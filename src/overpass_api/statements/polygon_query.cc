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
#include <iterator>
#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "../data/geometry.h"
#include "area_query.h"
#include "coord_query.h"
#include "make_area.h"
#include "polygon_query.h"
#include "recurse.h"

using namespace std;

//-----------------------------------------------------------------------------

class Polygon_Constraint : public Query_Constraint
{
  public:
    bool delivers_data(Resource_Manager& rman);
    
    Polygon_Constraint(Polygon_Query_Statement& polygon_) : polygon(&polygon_) {}
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into, uint64 timestamp);
    void filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp);
    virtual ~Polygon_Constraint() {}
    
  private:
    Polygon_Query_Statement* polygon;
};


bool Polygon_Constraint::delivers_data(Resource_Manager& rman)
{
  return false;
}


bool Polygon_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  if (polygon->polygons_from_inputset())
    polygon->convert_inputset(rman);

  ranges = polygon->calc_ranges();
  return true;
}


bool Polygon_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint31_Index, Uint31_Index > >& ranges)
{
  if (polygon->polygons_from_inputset())
    polygon->convert_inputset(rman);

  set< pair< Uint32_Index, Uint32_Index > > node_ranges = polygon->calc_ranges();
  ranges = calc_parents(node_ranges);
  return true;

}


void Polygon_Constraint::filter(Resource_Manager& rman, Set& into, uint64 timestamp)
{
  polygon->collect_nodes(into.nodes, true);
  if (timestamp != NOW)
    polygon->collect_nodes(into.attic_nodes, true);
  
  set< pair< Uint31_Index, Uint31_Index > > ranges;
  get_ranges(rman, ranges);
  
  // pre-process ways to reduce the load of the expensive filter
  filter_ways_by_ranges(into.ways, ranges);
  if (timestamp != NOW)
    filter_ways_by_ranges(into.attic_ways, ranges);
  
  // pre-filter relations
  filter_relations_by_ranges(into.relations, ranges);
  if (timestamp != NOW)
    filter_relations_by_ranges(into.attic_relations, ranges);
  
  //TODO: filter areas
}


void Polygon_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into, uint64 timestamp)
{
  //Process ways  
  polygon->collect_ways(into.ways, Way_Geometry_Store(into.ways, query, rman), true, query, rman);
  
  //Process relations
  
  // Retrieve all nodes referred by the relations.
  set< pair< Uint32_Index, Uint32_Index > > node_ranges;
  get_ranges(rman, node_ranges);
  map< Uint32_Index, vector< Node_Skeleton > > node_members
      = relation_node_members(&query, rman, into.relations, &node_ranges);
  
  // filter for those nodes that are in one of the areas
  polygon->collect_nodes(node_members, false);
  
  // Retrieve all ways referred by the relations.
  set< pair< Uint31_Index, Uint31_Index > > way_ranges;
  get_ranges(rman, way_ranges);  
  map< Uint31_Index, vector< Way_Skeleton > > way_members_
      = relation_way_members(&query, rman, into.relations, &way_ranges);
  
  polygon->collect_ways(way_members_, Way_Geometry_Store(way_members_, query, rman), false, query, rman);
  
  filter_relations_expensive(order_by_id(node_members, Order_By_Node_Id()),
			     order_by_id(way_members_, Order_By_Way_Id()),
			     into.relations);
  
  if (timestamp != NOW)
  {
    //Process ways  
    polygon->collect_ways(into.attic_ways, Way_Geometry_Store(into.attic_ways, timestamp, query, rman),
			  true, query, rman);
  
    //Process relations
  
    // Retrieve all nodes referred by the relations.
    map< Uint32_Index, vector< Attic< Node_Skeleton > > > node_members
        = relation_node_members(&query, rman, into.attic_relations, timestamp, &node_ranges);
  
    // filter for those nodes that are in one of the areas
    polygon->collect_nodes(node_members, false);
  
    // Retrieve all ways referred by the relations.
    map< Uint31_Index, vector< Attic< Way_Skeleton > > > way_members_
        = relation_way_members(&query, rman, into.attic_relations, timestamp, &way_ranges);
  
    polygon->collect_ways(way_members_, Way_Geometry_Store(way_members_, timestamp, query, rman),
			  false, query, rman);
  
    filter_relations_expensive(order_attic_by_id(node_members, Order_By_Node_Id()),
			       order_attic_by_id(way_members_, Order_By_Way_Id()),
			       into.attic_relations);
  }
  
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
    (int line_number_, const map< string, string >& input_attributes, Query_Constraint* bbox_limitation)
    : Output_Statement(line_number_)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["bounds"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  set_output(attributes["into"]);

  input = attributes["from"];
  has_bounds = (attributes["bounds"] != "");

  if (attributes["bounds"] != "")
    convert_bounds(attributes["bounds"]);

}


Polygon_Query_Statement::~Polygon_Query_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

void Polygon_Query_Statement::convert_bounds(string bounds)
{
  //convert bounds
  istringstream in(bounds);
  vector<double> v = vector<double>(istream_iterator<double>(in), istream_iterator<double>());
  if (v.size() % 2)
  {
    ostringstream temp;
    temp<<"For the attribute \"bounds\" of the element \"polygon-query\""
        <<" an even number of float values must be provided.";
    add_static_error(temp.str());
    return;
  }
  else if (v.size() / 2 < 3)
  {
    ostringstream temp;
    temp<<"For the attribute \"bounds\" of the element \"polygon-query\""
        <<" at least 3 lat/lon float value pairs must be provided.";
    add_static_error(temp.str());
    return;
  }
  else
  {
    for(vector<double>::iterator it = v.begin(); it != v.end(); ++it) {
      double lat = *it;
      double lon = *++it;

      if ((lat < -90.0) || (lat > 90.0))
      {
        ostringstream temp;
        temp<<"For the attribute \"bounds\" of the element \"polygon-query\""
            <<" the only allowed values for latitude are floats between -90.0 and 90.0.";
        add_static_error(temp.str());
        return;
      }

      if ((lon < -180.0) || (lon > 180.0))
      {
        ostringstream temp;
        temp<<"For the attribute \"bounds\" of the element \"polygon-query\""
            <<" the only allowed values for longitude are floats between -180.0 and 180.0.";
        add_static_error(temp.str());
        return;
      }
    }
  }
  vector<double>::iterator it = v.begin();
  double first_lat, first_lon;

  first_lat = *it++;
  first_lon = *it++;

  double last_lat = first_lat;
  double last_lon = first_lon;

  while (it != v.end())
  {
    double lat, lon;
    lat = *it++;
    lon = *it++;
    
    Area::calc_aligned_segments(segments, last_lat, last_lon, lat, lon);
    
    last_lat = lat;
    last_lon = lon;
  }
  Area::calc_aligned_segments(segments, last_lat, last_lon, first_lat, first_lon);
  sort(segments.begin(), segments.end());

  add_segment_blocks(segments);
}


set< pair< Uint32_Index, Uint32_Index > > Polygon_Query_Statement::calc_ranges()
{
  set< pair< Uint32_Index, Uint32_Index > > result;
  for (vector< Aligned_Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    result.insert(make_pair(it->ll_upper_, it->ll_upper_ + 0x100));
  return result;
}


template< typename Node_Skeleton >
void Polygon_Query_Statement::collect_nodes(map< Uint32_Index, vector< Node_Skeleton > >& nodes,
                                            bool add_border)
{
  vector< Aligned_Segment >::const_iterator area_it = segments.begin();
  typename map< Uint32_Index, vector< Node_Skeleton > >::iterator nodes_it = nodes.begin();
  
  uint32 current_idx(0);
  
  while (area_it != segments.end())
  {    
    current_idx = area_it->ll_upper_;
    
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
      for (typename vector< Node_Skeleton >::const_iterator iit = nodes_it->second.begin();
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
	  if (check == Coord_Query_Statement::HIT && add_border)
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
  }
  while (nodes_it != nodes.end())
  {
    nodes_it->second.clear();
    ++nodes_it;
  }
}


template< typename Way_Skeleton >
void Polygon_Query_Statement::collect_ways
      (map< Uint31_Index, vector< Way_Skeleton > >& ways,
       const Way_Geometry_Store& way_geometries,
       bool add_border, const Statement& query, Resource_Manager& rman)
{
  map< uint32, vector< pair< uint32, Way::Id_Type > > > way_coords_to_id;
  for (typename map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    for (typename vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      vector< Quad_Coord > coords = way_geometries.get_geometry(*it2);
      for (vector< Quad_Coord >::const_iterator it3 = coords.begin(); it3 != coords.end(); ++it3)
        way_coords_to_id[it3->ll_upper].push_back(make_pair(it3->ll_lower, it2->id));
    }
  }
  
  map< uint32, vector< pair< uint32, Way::Id_Type > > >::const_iterator nodes_it = way_coords_to_id.begin();

  vector< Aligned_Segment >::const_iterator area_it = segments.begin();
  
  map< Uint31_Index, vector< Area_Block > > way_segments;
  for (typename map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    for (typename vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_way_to_area_blocks(way_geometries.get_geometry(*it2), it2->id.val(), way_segments);
  }
      
  map< Way::Id_Type, bool > ways_inside;
  
  // Fill node_status with the area related status of each node and segment
  uint32 current_idx(0);
  while (area_it != segments.end())
  {
    current_idx = area_it->ll_upper_;
    
    vector< Area_Block > areas;
    while (area_it != segments.end() && area_it->ll_upper_ == current_idx)
    {
      Area_Block block;
      block.coors.push_back(area_it->ll_lower_a);
      block.coors.push_back(area_it->ll_lower_b);
      areas.push_back(block);
      ++area_it;
    }
        
    // check nodes
    while (nodes_it != way_coords_to_id.end() && nodes_it->first < current_idx)
      ++nodes_it;
    while (nodes_it != way_coords_to_id.end() &&
        (nodes_it->first & 0xffffff00) == current_idx)
    {
      vector< pair< uint32, Way::Id_Type > > into;
      for (vector< pair< uint32, Way::Id_Type > >::const_iterator iit = nodes_it->second.begin();
          iit != nodes_it->second.end(); ++iit)
      {
        uint32 ilat = ::ilat(nodes_it->first, iit->first);
        int32 ilon = ::ilon(nodes_it->first, iit->first);
        
        int inside = 0;
        for (vector< Area_Block >::const_iterator it2 = areas.begin(); it2 != areas.end();
             ++it2)
        {
          int check(Coord_Query_Statement::check_area_block(current_idx, *it2, ilat, ilon));
          if (check == Coord_Query_Statement::HIT)
          {
            inside = Coord_Query_Statement::HIT;
            break;
          }
          else
            inside ^= check;
        }
        if (inside & (Coord_Query_Statement::TOGGLE_EAST | Coord_Query_Statement::TOGGLE_WEST))
          ways_inside[iit->second] = true;
      }
      ++nodes_it;
    }
    
    // check segments
    for (vector< Area_Block >::const_iterator sit = way_segments[Uint31_Index(current_idx)].begin();
         sit != way_segments[Uint31_Index(current_idx)].end(); ++sit)
    {
      int inside = 0;
      for (vector< Area_Block >::const_iterator it = areas.begin();
           it != areas.end(); ++it)
      {
        // If an area segment intersects this way segment in the inner of the way,
        // the way is contained in the area.
        // The endpoints are properly handled via the point-in-area test
        // Check additionally the middle of the segment to also get segments
        // that run through the area
        int intersect = intersects_inner(*sit, *it);
        if (intersect == Coord_Query_Statement::INTERSECT)
        {
          ways_inside[Way::Id_Type(sit->id)] = true;
          break;
        }
        else if (intersect == Coord_Query_Statement::HIT)
        {
          if (add_border)
            ways_inside[Way::Id_Type(sit->id)] = true;
          else
            inside = 0;
          break;
        }
        has_inner_points(*sit, *it, inside);
      }
      if ((inside && (!(inside & Coord_Query_Statement::HIT))) ||
          (inside && add_border))
        ways_inside[Way::Id_Type(sit->id)] = true;
    }
  }

  map< Uint31_Index, vector< Way_Skeleton > > result;

  // Mark ways as found that intersect the area border
  for (typename map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    vector< Way_Skeleton > cur_result;
    for (typename vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (ways_inside[it2->id])
      {
        cur_result.push_back(*it2);
        it2->id = Way::Id_Type(0u);
      }
    }
    result[it->first].swap(cur_result);
  }
  
  result.swap(ways);
}

// taken over from make_area.cc
Node::Id_Type Polygon_Query_Statement::check_node_parity(const Set& pivot)
{
  set< Node::Id_Type > node_parity_control;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(pivot.ways.begin()); it != pivot.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      if (it2->nds.size() < 2)
    continue;
      pair< set< Node::Id_Type >::iterator, bool > npp(node_parity_control.insert
          (it2->nds.front()));
      if (!npp.second)
    node_parity_control.erase(npp.first);
      npp = node_parity_control.insert
          (it2->nds.back());
      if (!npp.second)
    node_parity_control.erase(npp.first);
    }
  }
  if (node_parity_control.size() > 0)
    return *(node_parity_control.begin());
  return Node::Id_Type(0ull);
}

void Polygon_Query_Statement::convert_inputset(Resource_Manager& rman)
{
  map< string, Set >::const_iterator mit = rman.sets().find(get_input());
  if (mit == rman.sets().end())
    return;

  segments.clear();

  // check node parity
  Node::Id_Type odd_id(check_node_parity(mit->second));
  if (!(odd_id == Node::Id_Type(0ull)))
  {
    ostringstream temp;
    temp<<"make-area: Node "<<odd_id.val()
            <<" is contained in an odd number of ways.\n";
    runtime_remark(temp.str());
  }
  /*
  // create area blocks
  map< Uint31_Index, vector< Area_Block > > area_blocks;
  pair< Node::Id_Type, Way::Id_Type > odd_pair
    (create_area_blocks(area_blocks, pivot_id, mit->second));
  if (!(odd_pair.first == Node::Id_Type(0ull)))
  {
    ostringstream temp;
    temp<<"make-area: Node "<<odd_pair.first.val()
        <<" referred by way "<<odd_pair.second.val()
        <<" is not contained in set \""<<input<<"\".\n";
    runtime_remark(temp.str());
  }
   */

  vector< Node > nodes;
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(mit->second.nodes.begin()); it != mit->second.nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      nodes.push_back(Node(it2->id.val(), it->first.val(), it2->ll_lower));
  }

  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());

  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(mit->second.ways.begin()); it != mit->second.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      vector< Quad_Coord > coord = make_geometry(*it2, nodes);
      vector< Quad_Coord >::const_iterator it4(coord.begin());

      if (it4 == coord.end())
        continue;

      double last_lat = ::lat(it4->ll_upper, it4->ll_lower);
      double last_lon = ::lon(it4->ll_upper, it4->ll_lower);

      it4++;

      while (it4 != coord.end())
      {
        double lat, lon;

        lat = ::lat(it4->ll_upper, it4->ll_lower);;
        lon = ::lon(it4->ll_upper, it4->ll_lower);

        Area::calc_aligned_segments(segments, last_lat, last_lon, lat, lon);

        last_lat = lat;
        last_lon = lon;

        it4++;
      }
   }
  }
  sort(segments.begin(), segments.end());
  add_segment_blocks(segments);
}

void Polygon_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;
  
  Polygon_Constraint constraint(*this);
  set< pair< Uint32_Index, Uint32_Index > > ranges;
  constraint.get_ranges(rman, ranges);
  get_elements_by_id_from_db< Uint32_Index, Node_Skeleton >
      (into.nodes, into.attic_nodes,
       vector< Node::Id_Type >(), false, rman.get_desired_timestamp(), ranges, *this, rman,
       *osm_base_settings().NODES, *attic_settings().NODES);  
  constraint.filter(rman, into, rman.get_desired_timestamp());
  filter_attic_elements(rman, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);

  transfer_output(rman, into);
  rman.health_check(*this);
}


Query_Constraint* Polygon_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Polygon_Constraint(*this));
  return constraints.back();
}
