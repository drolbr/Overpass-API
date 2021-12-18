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

#include <algorithm>
#include <iterator>
#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
#include "../data/collect_members.h"
#include "area_query.h"
#include "coord_query.h"
#include "make_area.h"
#include "polygon_query.h"
#include "recurse.h"


//-----------------------------------------------------------------------------

class Polygon_Constraint : public Query_Constraint
{
  public:
    Query_Filter_Strategy delivers_data(Resource_Manager& rman);

    Polygon_Constraint(Polygon_Query_Statement& polygon_) : polygon(&polygon_) {}
    bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges);
    bool get_ranges
        (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into);
    void filter(const Statement& query, Resource_Manager& rman, Set& into);
    virtual ~Polygon_Constraint() {}

  private:
    Polygon_Query_Statement* polygon;
};


Query_Filter_Strategy Polygon_Constraint::delivers_data(Resource_Manager& rman)
{
  return (polygon && !polygon->covers_large_area()) ? prefer_ranges : ids_useful;
}


bool Polygon_Constraint::get_ranges
    (Resource_Manager& rman, std::set< std::pair< Uint32_Index, Uint32_Index > >& ranges)
{
  ranges = polygon->calc_ranges();
  return true;
}


bool Polygon_Constraint::get_ranges
    (Resource_Manager& rman, std::set< std::pair< Uint31_Index, Uint31_Index > >& ranges)
{
  std::set< std::pair< Uint32_Index, Uint32_Index > > node_ranges = polygon->calc_ranges();
  ranges = calc_parents(node_ranges);
  return true;
}


void Polygon_Constraint::filter(Resource_Manager& rman, Set& into)
{
  polygon->collect_nodes(into.nodes, true);
  if (!into.attic_nodes.empty())
    polygon->collect_nodes(into.attic_nodes, true);

  std::set< std::pair< Uint31_Index, Uint31_Index > > ranges;
  get_ranges(rman, ranges);

  // pre-process ways to reduce the load of the expensive filter
  filter_ways_by_ranges(into.ways, ranges);
  if (!into.attic_ways.empty())
    filter_ways_by_ranges(into.attic_ways, ranges);

  // pre-filter relations
  filter_relations_by_ranges(into.relations, ranges);
  if (!into.attic_relations.empty())
    filter_relations_by_ranges(into.attic_relations, ranges);

  //TODO: filter areas
}


void Polygon_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  //Process ways
  polygon->collect_ways(into.ways, Way_Geometry_Store(into.ways, query, rman), true, query, rman);

  //Process relations

  // Retrieve all nodes referred by the relations.
  std::set< std::pair< Uint32_Index, Uint32_Index > > node_ranges;
  get_ranges(rman, node_ranges);
  std::map< Uint32_Index, std::vector< Node_Skeleton > > node_members
      = relation_node_members(&query, rman, into.relations, node_ranges, {}, true);

  // filter for those nodes that are in one of the areas
  polygon->collect_nodes(node_members, false);

  // Retrieve all ways referred by the relations.
  std::set< std::pair< Uint31_Index, Uint31_Index > > way_ranges;
  get_ranges(rman, way_ranges);
  std::map< Uint31_Index, std::vector< Way_Skeleton > > way_members_
      = relation_way_members(&query, rman, into.relations, way_ranges, {}, true);

  polygon->collect_ways(way_members_, Way_Geometry_Store(way_members_, query, rman), false, query, rman);

  filter_relations_expensive(order_by_id(node_members, Order_By_Node_Id()),
			     order_by_id(way_members_, Order_By_Way_Id()),
			     into.relations);

  //Process ways
  if (!into.attic_ways.empty())
    polygon->collect_ways(into.attic_ways, Way_Geometry_Store(into.attic_ways, query, rman),
			  true, query, rman);

  //Process relations
  if (!into.attic_relations.empty())
  {
    // Retrieve all nodes referred by the relations.
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > node_members
        = relation_node_members(&query, rman, into.attic_relations, node_ranges);

    // filter for those nodes that are in one of the areas
    polygon->collect_nodes(node_members, false);

    // Retrieve all ways referred by the relations.
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > way_members_
        = relation_way_members(&query, rman, into.attic_relations, way_ranges);

    polygon->collect_ways(way_members_, Way_Geometry_Store(way_members_, query, rman),
			  false, query, rman);

    filter_relations_expensive(order_attic_by_id(node_members, Order_By_Node_Id()),
			       order_attic_by_id(way_members_, Order_By_Way_Id()),
			       into.attic_relations);
  }

  //TODO: filter areas
}

//-----------------------------------------------------------------------------


void add_segment_blocks(std::vector< Aligned_Segment >& segments)
{
  std::map< Uint31_Index, std::vector< Aligned_Segment > > by_upper;
  for (std::vector< Aligned_Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    by_upper[it->ll_upper_].push_back(*it);

  /* We use that more northern segments always have bigger indices.
    Thus we can collect each block's end points and add them, if they do not
    cancel out, to the next block further northern.*/
  for (std::map< Uint31_Index, std::vector< Aligned_Segment > >::const_iterator
      it = by_upper.begin(); it != by_upper.end(); ++it)
  {
    std::set< int32 > lons;

    for (std::vector< Aligned_Segment >::const_iterator it2 = it->second.begin();
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
    std::vector< Aligned_Segment >& northern_block(by_upper[northern_ll_upper]);
    for (std::set< int32 >::const_iterator it2 = lons.begin(); it2 != lons.end(); ++it2)
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
  for (std::map< Uint31_Index, std::vector< Aligned_Segment > >::const_iterator it = by_upper.begin();
       it != by_upper.end(); ++it)
  {
    for (std::vector< Aligned_Segment >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      segments.push_back(*it2);
  }
}


Polygon_Query_Statement::Statement_Maker Polygon_Query_Statement::statement_maker;
Polygon_Query_Statement::Criterion_Maker Polygon_Query_Statement::criterion_maker;


Statement* Polygon_Query_Statement::Criterion_Maker::create_criterion(const Token_Node_Ptr& tree_it,
    const std::string& type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  uint line_nr = tree_it->line_col.first;

  if (tree_it->token == ":" && tree_it->rhs)
  {
    std::map< std::string, std::string > attributes;
    attributes["bounds"] = decode_json(tree_it.rhs()->token, error_output);
    attributes["into"] = into;
    return new Polygon_Query_Statement(line_nr, attributes, global_settings);
  }

  return 0;
}


bool covers_large_area(const std::vector< std::pair< double, double > >& edges)
{
  double max_lat = -100.;
  double min_lat = 100.;
  double max_lon = -200.;
  double min_lon = 200.;

  for (std::vector< std::pair< double, double > >::const_iterator it = edges.begin(); it != edges.end(); ++it)
  {
    max_lat = std::max(max_lat, it->first);
    min_lat = std::min(min_lat, it->first);
    max_lon = std::max(max_lon, it->second);
    min_lon = std::min(min_lon, it->second);
  }

  if (max_lat < min_lat || max_lon < min_lon)
    return false;
  return (max_lat - min_lat) * (max_lon - min_lon) > 1.;
}


Polygon_Query_Statement::Polygon_Query_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_)
{
  std::map< std::string, std::string > attributes;

  attributes["into"] = "_";
  attributes["bounds"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  set_output(attributes["into"]);

  //convert bounds
  std::istringstream in(attributes["bounds"]);
  while (in.good())
  {
    double lat, lon;
    in>>lat;
    if (!in.good())
    {
      add_static_error("For the attribute \"bounds\" of the element \"polygon-query\""
          " an even number of float values must be provided.");
      break;
    }
    in>>lon;
    edges.push_back(std::make_pair(lat, lon));
  }

  if (edges.size() < 3)
    add_static_error("For the attribute \"bounds\" of the element \"polygon-query\""
        " at least 3 lat/lon float value pairs must be provided.");

  for (std::vector< std::pair< double, double > >::const_iterator it = edges.begin();
      it != edges.end(); ++it)
  {
    if (it->first < -90.0 || it->first > 90.0)
      add_static_error("For the attribute \"bounds\" of the element \"polygon-query\""
          " the only allowed values for latitude are floats between -90.0 and 90.0.");

    if (it->second < -180.0 || it->second > 180.0)
      add_static_error("For the attribute \"bounds\" of the element \"polygon-query\""
          " the only allowed values for longitude are floats between -180.0 and 180.0.");
  }

  covers_large_area_ = ::covers_large_area(edges);

  for (unsigned int i = 1; i < edges.size(); ++i)
    Area::calc_aligned_segments(segments, edges[i-1].first, edges[i-1].second, edges[i].first, edges[i].second);
  if (!edges.empty())
    Area::calc_aligned_segments(
        segments, edges.back().first, edges.back().second, edges[0].first, edges[0].second);
  std::sort(segments.begin(), segments.end());

  add_segment_blocks(segments);
}


Polygon_Query_Statement::~Polygon_Query_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


std::set< std::pair< Uint32_Index, Uint32_Index > > Polygon_Query_Statement::calc_ranges()
{
  std::set< std::pair< Uint32_Index, Uint32_Index > > result;
  for (std::vector< Aligned_Segment >::const_iterator it = segments.begin(); it != segments.end(); ++it)
    result.insert(std::make_pair(it->ll_upper_, it->ll_upper_ + 0x100));
  return result;
}


template< typename Node_Skeleton >
void Polygon_Query_Statement::collect_nodes(std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
                                            bool add_border)
{
  std::vector< Aligned_Segment >::const_iterator area_it = segments.begin();
  typename std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator nodes_it = nodes.begin();

  uint32 current_idx(0);

  while (area_it != segments.end())
  {
    current_idx = area_it->ll_upper_;

    std::vector< Area_Block > areas;
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
      std::vector< Node_Skeleton > into;
      for (typename std::vector< Node_Skeleton >::const_iterator iit = nodes_it->second.begin();
          iit != nodes_it->second.end(); ++iit)
      {
        uint32 ilat((::lat(nodes_it->first.val(), iit->ll_lower)
            + 91.0)*10000000+0.5);
        int32 ilon(::lon(nodes_it->first.val(), iit->ll_lower)*10000000
            + (::lon(nodes_it->first.val(), iit->ll_lower) > 0 ? 0.5 : -0.5));

        int inside = 0;
        for (std::vector< Area_Block >::const_iterator it = areas.begin();
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
      (std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
       const Way_Geometry_Store& way_geometries,
       bool add_border, const Statement& query, Resource_Manager& rman)
{
  std::map< uint32, std::vector< std::pair< uint32, Way::Id_Type > > > way_coords_to_id;
  for (typename std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    for (typename std::vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      std::vector< Quad_Coord > coords = way_geometries.get_geometry(*it2);
      for (std::vector< Quad_Coord >::const_iterator it3 = coords.begin(); it3 != coords.end(); ++it3)
        way_coords_to_id[it3->ll_upper].push_back(std::make_pair(it3->ll_lower, it2->id));
    }
  }

  std::map< uint32, std::vector< std::pair< uint32, Way::Id_Type > > >::const_iterator nodes_it = way_coords_to_id.begin();

  std::vector< Aligned_Segment >::const_iterator area_it = segments.begin();

  std::map< Uint31_Index, std::vector< Area_Block > > way_segments;
  for (typename std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    for (typename std::vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_way_to_area_blocks(way_geometries.get_geometry(*it2), it2->id.val(), way_segments);
  }

  std::map< Way::Id_Type, bool > ways_inside;

  // Fill node_status with the area related status of each node and segment
  uint32 current_idx(0);
  while (area_it != segments.end())
  {
    current_idx = area_it->ll_upper_;

    std::vector< Area_Block > areas;
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
      std::vector< std::pair< uint32, Way::Id_Type > > into;
      for (std::vector< std::pair< uint32, Way::Id_Type > >::const_iterator iit = nodes_it->second.begin();
          iit != nodes_it->second.end(); ++iit)
      {
        uint32 ilat = ::ilat(nodes_it->first, iit->first);
        int32 ilon = ::ilon(nodes_it->first, iit->first);

        int inside = 0;
        for (std::vector< Area_Block >::const_iterator it2 = areas.begin(); it2 != areas.end();
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
    for (std::vector< Area_Block >::const_iterator sit = way_segments[Uint31_Index(current_idx)].begin();
         sit != way_segments[Uint31_Index(current_idx)].end(); ++sit)
    {
      int inside = 0;
      for (std::vector< Area_Block >::const_iterator it = areas.begin();
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

  std::map< Uint31_Index, std::vector< Way_Skeleton > > result;

  // Mark ways as found that intersect the area border
  for (typename std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    std::vector< Way_Skeleton > cur_result;
    for (typename std::vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
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


void Polygon_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;

  Polygon_Constraint constraint(*this);
  std::set< std::pair< Uint32_Index, Uint32_Index > > ranges;
  constraint.get_ranges(rman, ranges);
  get_elements_from_db< Uint32_Index, Node_Skeleton >(
      into.nodes, into.attic_nodes, ranges, *this, rman);
  constraint.filter(rman, into);
  filter_attic_elements(rman, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);

  transfer_output(rman, into);
  rman.health_check(*this);
}


Query_Constraint* Polygon_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Polygon_Constraint(*this));
  return constraints.back();
}
