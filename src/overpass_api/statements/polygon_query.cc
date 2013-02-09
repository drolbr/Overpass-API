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
  set< pair< Uint32_Index, Uint32_Index > > node_ranges = polygon->calc_ranges();
  ranges = calc_parents(node_ranges);
  return true;
}


void Polygon_Constraint::filter(Resource_Manager& rman, Set& into)
{
  polygon->collect_nodes(into.nodes);
  
  set< pair< Uint31_Index, Uint31_Index > > ranges;
  get_ranges(rman, ranges);
  
  // pre-process ways to reduce the load of the expensive filter
  {
    set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
    map< Uint31_Index, vector< Way_Skeleton > >::iterator it = into.ways.begin();
    set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_begin = ranges.begin();
    for (; it != into.ways.end() && ranges_it != ranges.end(); )
    {
      if (!(it->first < ranges_it->second))
        ++ranges_it;
      else if (!(it->first < ranges_it->first))
      {
        if ((it->first.val() & 0x80000000) == 0 || (it->first.val() & 0x3) != 0)
          ++it;
        else
        {
          vector< Way_Skeleton > filtered_ways;
          while (!(Uint31_Index(it->first.val() & 0x7fffff00) < ranges_begin->second))
            ++ranges_begin;
          for (vector< Way_Skeleton >::const_iterator it2 = it->second.begin();
               it2 != it->second.end(); ++it2)
          {
            set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it2 = ranges_begin;
            vector< Uint31_Index >::const_iterator it3 = it2->segment_idxs.begin();
            for (; it3 != it2->segment_idxs.end() && ranges_it2 != ranges.end(); )
            {
              if (!(*it3 < ranges_it2->second))
                ++ranges_it2;
              else if (!(*it3 < ranges_it2->first))
              {
                // A relevant index is found; thus the way is relevant.
                filtered_ways.push_back(*it2);
                break;
              }
              else
                ++it3;
            }
          }
          
          filtered_ways.swap(it->second);
          ++it;
        }
      }
      else
      {
        // The index of the way is not in the current set of ranges.
        // Thus it cannot be in the result set.
        it->second.clear();
        ++it;
      }
    }
    for (; it != into.ways.end(); ++it)
      it->second.clear();
  }
  
  // pre-filter relations
  {
    set< pair< Uint31_Index, Uint31_Index > >::const_iterator ranges_it = ranges.begin();
    map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = into.relations.begin();
    for (; it != into.relations.end() && ranges_it != ranges.end(); )
    {
      if (!(it->first < ranges_it->second))
        ++ranges_it;
      else if (!(it->first < ranges_it->first))
        ++it;
      else
      {
        it->second.clear();
        ++it;
      }
    }
    for (; it != into.relations.end(); ++it)
      it->second.clear();
  }
  
  //TODO: filter areas
}


void Polygon_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  {
    //Process ways
  
    // Retrieve all nodes referred by the ways.
    map< Uint32_Index, vector< Node_Skeleton > > way_members_
        = way_members(&query, rman, into.ways);
  
    // Order node ids by id.
    vector< pair< Uint32_Index, const Node_Skeleton* > > way_members_by_id;
    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = way_members_.begin();
        it != way_members_.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        way_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    Order_By_Node_Id order_by_node_id;
    sort(way_members_by_id.begin(), way_members_by_id.end(), order_by_node_id);
    
    polygon->collect_ways(into.ways, way_members_, way_members_by_id, rman);
  }
  {
    //Process relations
    
    // Retrieve all nodes referred by the relations.
    set< pair< Uint32_Index, Uint32_Index > > node_ranges;
    get_ranges(rman, node_ranges);
    
    map< Uint32_Index, vector< Node_Skeleton > > node_members
        = relation_node_members(&query, rman, into.relations, &node_ranges);
  
    // filter for those nodes that are in one of the areas
    polygon->collect_nodes(node_members);
  
    // Order node ids by id.
    vector< pair< Uint32_Index, const Node_Skeleton* > > node_members_by_id;
    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = node_members.begin();
        it != node_members.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        node_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    Order_By_Node_Id order_by_node_id;
    sort(node_members_by_id.begin(), node_members_by_id.end(), order_by_node_id);
    
    // Retrieve all ways referred by the relations.
    set< pair< Uint31_Index, Uint31_Index > > way_ranges;
    get_ranges(rman, way_ranges);
    
    map< Uint31_Index, vector< Way_Skeleton > > way_members_
        = relation_way_members(&query, rman, into.relations, &way_ranges);
  
    // Filter for those ways that are in one of the areas
    // Retrieve all nodes referred by the ways.
    map< Uint32_Index, vector< Node_Skeleton > > node_members_
        = way_members(&query, rman, way_members_);

    // Order node ids by id.
    vector< pair< Uint32_Index, const Node_Skeleton* > > way_node_members_by_id;
    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator it = node_members_.begin();
        it != node_members_.end(); ++it)
    {
      for (vector< Node_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        way_node_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    sort(way_node_members_by_id.begin(), way_node_members_by_id.end(), order_by_node_id);
    
    polygon->collect_ways(way_members_, node_members_, way_node_members_by_id, rman);
    
    // Order way ids by id.
    vector< pair< Uint31_Index, const Way_Skeleton* > > way_members_by_id;
    for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = way_members_.begin();
        it != way_members_.end(); ++it)
    {
      for (vector< Way_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
        way_members_by_id.push_back(make_pair(it->first, &*iit));
    }
    Order_By_Way_Id order_by_way_id;
    sort(way_members_by_id.begin(), way_members_by_id.end(), order_by_way_id);
    
    for (map< Uint31_Index, vector< Relation_Skeleton > >::iterator it = into.relations.begin();
        it != into.relations.end(); ++it)
    {
      vector< Relation_Skeleton > local_into;
      for (vector< Relation_Skeleton >::const_iterator iit = it->second.begin();
          iit != it->second.end(); ++iit)
      {
        for (vector< Relation_Entry >::const_iterator nit = iit->members.begin();
            nit != iit->members.end(); ++nit)
        {
          if (nit->type == Relation_Entry::NODE)
          {
            const pair< Uint32_Index, const Node_Skeleton* >* second_nd =
                binary_search_for_pair_id(node_members_by_id, nit->ref);
            if (second_nd)
            {
              local_into.push_back(*iit);
              break;
            }
          }
          else if (nit->type == Relation_Entry::WAY)
          {
            const pair< Uint31_Index, const Way_Skeleton* >* second_nd =
                binary_search_for_pair_id(way_members_by_id, nit->ref32());
            if (second_nd)
            {
              local_into.push_back(*iit);
              break;
            }
          }
        }
      }
      it->second.swap(local_into);
    }
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
  }
  while (nodes_it != nodes.end())
  {
    nodes_it->second.clear();
    ++nodes_it;
  }
}


void Polygon_Query_Statement::collect_ways
      (map< Uint31_Index, vector< Way_Skeleton > >& ways,
       map< Uint32_Index, vector< Node_Skeleton > >& way_members_,
       vector< pair< Uint32_Index, const Node_Skeleton* > > way_members_by_id,
       Resource_Manager& rman)
{
  vector< Node > nodes;
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = way_members_.begin();
       it != way_members_.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      nodes.push_back(Node(it2->id, it->first.val(), it2->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());

  map< Node::Id_Type, uint > node_status;
  map< Way::Id_Type, bool > ways_inside;
  
  vector< Aligned_Segment >::const_iterator area_it = segments.begin();
  map< Uint32_Index, vector< Node_Skeleton > >::iterator nodes_it = way_members_.begin();
  
  map< Uint31_Index, vector< Area_Block > > way_segments;
  for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_way_to_area_blocks(*it2, nodes, it2->id.val(), way_segments);
  }
      
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
    while (nodes_it != way_members_.end() && nodes_it->first.val() < current_idx)
    {
      nodes_it->second.clear();
      ++nodes_it;
    }
    while (nodes_it != way_members_.end() &&
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
            inside = Coord_Query_Statement::HIT;
            break;
          }
          else if (check != 0)
            inside ^= check;
        }
        node_status[iit->id] = inside;
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
        if (intersects_inner(*sit, *it))
        {
          ways_inside[Way::Id_Type(sit->id)] = true;
          break;
        }
        has_inner_points(*sit, *it, inside);
      }
      if (inside)
        ways_inside[Way::Id_Type(sit->id)] = true;
    }
  }

  map< Uint31_Index, vector< Way_Skeleton > > result;

  // Mark ways as found that intersect the area border
  for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    vector< Way_Skeleton > cur_result;
    for (vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (ways_inside[it2->id])
      {
        cur_result.push_back(*it2);
        it2->id = Way::Id_Type(0u);
      }
    }
    result[it->first].swap(cur_result);
  }

  // Mark ways as found that have an inner node as a member
  for (map< Uint31_Index, vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    vector< Way_Skeleton >& cur_result = result[it->first];
    for (vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (it2->id == Way::Id_Type(0u))
        continue;
      for (vector< Node::Id_Type >::const_iterator nit = it2->nds.begin(); nit != it2->nds.end(); ++nit)
      {
        if (node_status[*nit] & (Coord_Query_Statement::TOGGLE_EAST | Coord_Query_Statement::TOGGLE_WEST))
        {
          cur_result.push_back(*it2);
          it2->id = Way::Id_Type(0u);
          break;
        }
      }
    }
  }
  
  result.swap(ways);
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
