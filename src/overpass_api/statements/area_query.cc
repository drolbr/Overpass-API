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

#include "../../template_db/block_backend.h"
#include "../core/index_computations.h"
#include "../data/collect_members.h"
#include "../data/tilewise_geometry.h"
#include "area_query.h"
#include "coord_query.h"
#include "make_area.h"
#include "recurse.h"


class Area_Constraint : public Query_Constraint
{
  public:
    Area_Constraint(Area_Query_Statement& area_) : area(&area_) {}

    Query_Filter_Strategy delivers_data(Resource_Manager& rman);

    bool get_ranges(Resource_Manager& rman, Ranges< Uint32_Index >& ranges);
    bool get_ranges(Resource_Manager& rman, Ranges< Uint31_Index >& ranges);

    void filter(Resource_Manager& rman, Set& into);
    void filter(const Statement& query, Resource_Manager& rman, Set& into);
    virtual ~Area_Constraint() {}

  private:
    Area_Query_Statement* area;
};


Ranges< Uint32_Index > copy_discrete_to_area_ranges(const std::set< Uint31_Index >& area_blocks_req)
{
  Ranges< Uint32_Index > result;
  for (std::set< Uint31_Index >::const_iterator it = area_blocks_req.begin(); it != area_blocks_req.end(); ++it)
    result.push_back(Uint32_Index(it->val()), Uint32_Index((it->val()) + 0x100));
  result.sort();
  return result;
}


bool Area_Constraint::get_ranges
    (Resource_Manager& rman, Ranges< Uint32_Index >& ranges)
{
  std::set< Uint31_Index > area_blocks_req;
  if (area->areas_from_input())
  {
    const Set* input = rman.get_set(area->get_input());
    if (!input)
      return true;

    area->get_ranges(input->ways, input->areas, area_blocks_req, rman);
    ranges = way_covered_indices(input->ways, input->attic_ways);
  }
  else
  {
    area->get_ranges(area_blocks_req, rman);
    ranges = Ranges< Uint32_Index >();
  }

  ranges.union_(copy_discrete_to_area_ranges(area_blocks_req)).swap(ranges);

  return true;
}


bool Area_Constraint::get_ranges
    (Resource_Manager& rman, Ranges< Uint31_Index >& ranges)
{
  Ranges< Uint32_Index > node_ranges;
  this->get_ranges(rman, node_ranges);
  ranges = calc_parents(node_ranges);
  return true;
}


void Area_Constraint::filter(Resource_Manager& rman, Set& into)
{
  Ranges< Uint31_Index > ranges;
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


template< typename Node_Skeleton >
std::map< Uint32_Index, std::vector< Node_Skeleton > > nodes_contained_in(
    const Set* potential_areas, bool accept_border, const Statement& stmt, Resource_Manager& rman,
    const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes)
{
  if (!potential_areas)
    return std::map< Uint32_Index, std::vector< Node_Skeleton > >();

  Tilewise_Const_Area_Iterator tai(potential_areas->ways, potential_areas->attic_ways, stmt, rman);
  std::map< Uint32_Index, std::vector< Node_Skeleton > > result;

  for (typename std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator iit = nodes.begin();
      iit != nodes.end(); ++iit)
  {
    while (!tai.is_end() && tai.get_idx().val() < iit->first.val())
      tai.next();
    if (tai.is_end() || iit->first.val() < tai.get_idx().val())
      continue;

    std::vector< Node_Skeleton >& result_block = result[iit->first];

    for (typename std::vector< Node_Skeleton >::const_iterator it = iit->second.begin(); it != iit->second.end(); ++it)
    {
      Tilewise_Area_Iterator::Relative_Position relpos = tai.rel_position(iit->first.val(), it->ll_lower, true);
//       std::cout<<"Id "<<it->id.val()<<' '<<relpos<<'\n';
      if ((accept_border && relpos != Tilewise_Area_Iterator::outside)
          || relpos == Tilewise_Area_Iterator::inside)
        result_block.push_back(*it);
    }
  }

//   while (!tai.is_end())
//   {
//     const std::map< const Way_Skeleton*, Tilewise_Area_Iterator::Index_Block >& way_blocks = tai.get_obj();
//     for (std::map< const Way_Skeleton*, Tilewise_Area_Iterator::Index_Block >::const_iterator bit = way_blocks.begin();
//         bit != way_blocks.end(); ++bit)
//     {
//       std::cout<<"Index "<<std::hex<<tai.get_idx().val()
//           <<" ("<<std::dec<<lat(tai.get_idx().val(), 0u)<<' '<<lon(tai.get_idx().val(), 0u)<<") "
//           <<std::dec<<bit->first->id.val()<<": "<<bit->second.sw_is_inside;
//       for (std::vector< Tilewise_Area_Iterator::Entry >::const_iterator it = bit->second.segments.begin();
//           it != bit->second.segments.end(); ++it)
//         std::cout<<" ("<<it->ilat_west<<' '<<it->ilon_west<<' '<<it->ilat_east<<' '<<it->ilon_east<<')';
//       std::cout<<'\n';
//     }
//     for (uint i = 0; i < 16; ++i)
//     {
//       std::cout<<"    ";
//       for (uint j = 0; j < 16; ++j)
//         std::cout<<tai.rel_position(tai.get_idx().val(), ll_lower(0xf000 - i*0x1000, j*0x1000));
//       std::cout<<'\n';
//     }
//     tai.next();
//   }

  return result;
}


std::map< Uint31_Index, std::vector< Way_Skeleton > > ways_contained_in(
    const Set* potential_areas, const Statement& stmt, Resource_Manager& rman,
    const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  if (!potential_areas)
    return std::map< Uint31_Index, std::vector< Way_Skeleton > >();

  Tilewise_Const_Area_Iterator tai(potential_areas->ways, potential_areas->attic_ways, stmt, rman);
  std::map< Uint31_Index, std::vector< Way_Skeleton > > result;

  Tilewise_Way_Iterator twi(ways, std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >(), stmt, rman);
  while (!twi.is_end())
  {
    while (!tai.is_end() && tai.get_idx() < twi.get_idx())
      tai.next();
    if (tai.is_end() || twi.get_idx() < tai.get_idx())
    {
      twi.next();
      continue;
    }

    const std::map< Tilewise_Way_Iterator::Status_Ref< Way_Skeleton >*, Tilewise_Way_Iterator::Index_Block >& obj = twi.get_current_obj();
    for (std::map< Tilewise_Way_Iterator::Status_Ref< Way_Skeleton >*, Tilewise_Way_Iterator::Index_Block >::const_iterator it = obj.begin();
        it != obj.end(); ++it)
    {
      if (it->first->status == Tilewise_Area_Iterator::outside)
      {
        it->first->status = tai.rel_position(it->second.segments, true);
        if (it->first->status != Tilewise_Area_Iterator::outside)
          result[it->first->idx].push_back(*it->first->skel);
      }
    }

    twi.next();
  }

  return result;
}


std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > ways_contained_in(
    const Set* potential_areas, const Statement& stmt, Resource_Manager& rman,
    const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways)
{
  if (!potential_areas)
    return std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >();

  Tilewise_Const_Area_Iterator tai(potential_areas->ways, potential_areas->attic_ways, stmt, rman);
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > result;

  Tilewise_Way_Iterator twi(std::map< Uint31_Index, std::vector< Way_Skeleton > >(), ways, stmt, rman);
  while (!twi.is_end())
  {
    while (!tai.is_end() && tai.get_idx() < twi.get_idx())
      tai.next();
    if (tai.is_end() || twi.get_idx() < tai.get_idx())
    {
      twi.next();
      continue;
    }

    const std::map< Tilewise_Way_Iterator::Status_Ref< Attic< Way_Skeleton > >*, Tilewise_Way_Iterator::Index_Block >& obj = twi.get_attic_obj();
    for (std::map< Tilewise_Way_Iterator::Status_Ref< Attic< Way_Skeleton > >*, Tilewise_Way_Iterator::Index_Block >::const_iterator
        it = obj.begin(); it != obj.end(); ++it)
    {
      if (it->first->status == Tilewise_Area_Iterator::outside)
      {
        it->first->status = tai.rel_position(it->second.segments, true);
        if (it->first->status != Tilewise_Area_Iterator::outside)
          result[it->first->idx].push_back(*it->first->skel);
      }
    }

    twi.next();
  }

  return result;
}


void Area_Constraint::filter(const Statement& query, Resource_Manager& rman, Set& into)
{
  std::set< Uint31_Index > area_blocks_req;
  const Set* input = rman.get_set(area->get_input());
  if (area->areas_from_input())
  {
    if (input)
      area->get_ranges(input->ways, input->areas, area_blocks_req, rman);
  }
  else
    area->get_ranges(area_blocks_req, rman);

  //Process nodes
  {
    std::map< Uint32_Index, std::vector< Node_Skeleton > > nodes_in_wr_areas
        = nodes_contained_in(input, true, query, rman, into.nodes);
    indexed_set_difference(into.nodes, nodes_in_wr_areas);
    area->collect_nodes(into.nodes, area_blocks_req, true, rman);
    indexed_set_union(into.nodes, nodes_in_wr_areas);
  }

  //Process ways
  {
    std::map< Uint31_Index, std::vector< Way_Skeleton > > ways_in_wr_areas
        = ways_contained_in(input, query, rman, into.ways);
    indexed_set_difference(into.ways, ways_in_wr_areas);
    area->collect_ways(Way_Geometry_Store(into.ways, query, rman),
        into.ways, area_blocks_req, false, query, rman);
    indexed_set_union(into.ways, ways_in_wr_areas);
  }

  //Process relations
  Request_Context context(&query, rman);

  // Retrieve all nodes referred by the relations.
  Ranges< Uint32_Index > node_ranges;
  get_ranges(rman, node_ranges);
  std::map< Uint32_Index, std::vector< Node_Skeleton > > current_node_members;
  std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > attic_node_members;
  relation_node_members(context, into.relations, into.attic_relations, node_ranges, {}, true)
      .swap(current_node_members, attic_node_members);

  // filter for those nodes that are in one of the areas
  {
    std::map< Uint32_Index, std::vector< Node_Skeleton > > nodes_in_wr_areas
        = nodes_contained_in(input, false, query, rman, current_node_members);
    indexed_set_difference(current_node_members, nodes_in_wr_areas);
    area->collect_nodes(current_node_members, area_blocks_req, false, rman);
    indexed_set_union(current_node_members, nodes_in_wr_areas);
  }

  // Retrieve all ways referred by the relations.
  Ranges< Uint31_Index > way_ranges;
  get_ranges(rman, way_ranges);
  std::map< Uint31_Index, std::vector< Way_Skeleton > > current_way_members;
  std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > attic_way_members;
  relation_way_members(context, into.relations, into.attic_relations, way_ranges, {}, true)
      .swap(current_way_members, attic_way_members);

  // Filter for those ways that are in one of the areas
  {
    std::map< Uint31_Index, std::vector< Way_Skeleton > > ways_in_wr_areas
        = ways_contained_in(input, query, rman, current_way_members);
    indexed_set_difference(current_way_members, ways_in_wr_areas);
    area->collect_ways(Way_Geometry_Store(current_way_members, query, rman),
        current_way_members, area_blocks_req, false, query, rman);
    indexed_set_union(current_way_members, ways_in_wr_areas);
  }

  filter_relations_expensive(order_by_id(current_node_members, Order_By_Node_Id()),
			     order_by_id(current_way_members, Order_By_Way_Id()),
			     into.relations);

  //Process nodes
  if (!into.attic_nodes.empty())
  {
    std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > nodes_in_wr_areas
        = nodes_contained_in(input, true, query, rman, into.attic_nodes);
    indexed_set_difference(into.attic_nodes, nodes_in_wr_areas);
    area->collect_nodes(into.attic_nodes, area_blocks_req, true, rman);
    indexed_set_union(into.attic_nodes, nodes_in_wr_areas);
  }

  //Process ways
  if (!into.attic_ways.empty())
  {
    std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > ways_in_wr_areas
        = ways_contained_in(input, query, rman, into.attic_ways);
    indexed_set_difference(into.attic_ways, ways_in_wr_areas);
    area->collect_ways(Way_Geometry_Store(into.attic_ways, query, rman),
        into.attic_ways, area_blocks_req, false, query, rman);
    indexed_set_union(into.attic_ways, ways_in_wr_areas);
  }

  //Process relations
  if (!into.attic_relations.empty())
  {
    // filter for those nodes that are in one of the areas
    {
      std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > > nodes_in_wr_areas
          = nodes_contained_in(input, false, query, rman, attic_node_members);
      indexed_set_difference(attic_node_members, nodes_in_wr_areas);
      area->collect_nodes(attic_node_members, area_blocks_req, false, rman);
      indexed_set_union(attic_node_members, nodes_in_wr_areas);
    }

    // Filter for those ways that are in one of the areas
    {
      std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > > ways_in_wr_areas
          = ways_contained_in(input, query, rman, attic_way_members);
      indexed_set_difference(attic_way_members, ways_in_wr_areas);
      area->collect_ways(Way_Geometry_Store(attic_way_members, query, rman),
          attic_way_members, area_blocks_req, false, query, rman);
      indexed_set_union(attic_way_members, ways_in_wr_areas);
    }

    filter_relations_expensive(order_attic_by_id(attic_node_members, Order_By_Node_Id()),
			       order_attic_by_id(attic_way_members, Order_By_Way_Id()),
			       into.attic_relations);
  }

  //TODO: filter areas
}


//-----------------------------------------------------------------------------

bool Area_Query_Statement::is_used_ = false;


Area_Query_Statement::Statement_Maker Area_Query_Statement::statement_maker;
Area_Query_Statement::Criterion_Maker Area_Query_Statement::criterion_maker;


Statement* Area_Query_Statement::Criterion_Maker::create_criterion(const Token_Node_Ptr& input_tree,
    const std::string& type, const std::string& into,
    Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output)
{
  Token_Node_Ptr tree_it = input_tree;
  uint line_nr = tree_it->line_col.first;
  std::string from = "_";
  std::string ref;

  if (tree_it->token == ":" && tree_it->rhs)
  {
    ref = tree_it.rhs()->token;
    tree_it = tree_it.lhs();
  }

  if (tree_it->token == "." && tree_it->rhs)
    from = tree_it.rhs()->token;

  std::map< std::string, std::string > attributes;
  attributes["from"] = from;
  attributes["into"] = into;
  attributes["ref"] = ref;
  return new Area_Query_Statement(line_nr, attributes, global_settings);
}


Area_Query_Statement::Area_Query_Statement
    (int line_number_, const std::map< std::string, std::string >& input_attributes, Parsed_Query& global_settings)
    : Output_Statement(line_number_), area_blocks_req_filled(false)
{
  is_used_ = true;

  std::map< std::string, std::string > attributes;

  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["ref"] = "";

  eval_attributes_array(get_name(), attributes, input_attributes);

  input = attributes["from"];
  set_output(attributes["into"]);
  submitted_id = atoll(attributes["ref"].c_str());
  if (submitted_id <= 0 && attributes["ref"] != "")
  {
    std::ostringstream temp;
    temp<<"For the attribute \"ref\" of the element \"area-query\""
    <<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
  else if (submitted_id > 0)
    area_id.push_back(Area_Skeleton::Id_Type(submitted_id));
}

Area_Query_Statement::~Area_Query_Statement()
{
  for (std::vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}


unsigned int Area_Query_Statement::count_ranges(Resource_Manager& rman)
{
  if (!area_blocks_req_filled)
    fill_ranges(rman);

  return area_blocks_req.size();
}


void Area_Query_Statement::get_ranges(std::set< Uint31_Index >& area_blocks_req, Resource_Manager& rman)
{
  if (!area_blocks_req_filled)
    fill_ranges(rman);

  area_blocks_req = this->area_blocks_req;
}


void Area_Query_Statement::fill_ranges(Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Skeleton > area_locations_db
      (rman.get_area_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it(area_locations_db.flat_begin());
      !(it == area_locations_db.flat_end()); ++it)
  {
    if (binary_search(area_id.begin(), area_id.end(), it.object().id))
    {
      for (std::vector< uint32 >::const_iterator it2(it.object().used_indices.begin());
          it2 != it.object().used_indices.end(); ++it2)
        area_blocks_req.insert(Uint31_Index(*it2));
    }
  }
  area_blocks_req_filled = true;
}


void Area_Query_Statement::get_ranges
    (const std::map< Uint31_Index, std::vector< Way_Skeleton > >& input_ways,
     const std::map< Uint31_Index, std::vector< Area_Skeleton > >& input_areas,
     std::set< Uint31_Index >& area_blocks_req,
     Resource_Manager& rman)
{
  way_areas_id.clear();
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator it = input_ways.begin();
       it != input_ways.end(); ++it)
  {
    for (std::vector< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      if (!it2->nds.empty() && it2->nds.front() == it2->nds.back())
        way_areas_id.push_back(it2->id);
    }
  }
  std::sort(way_areas_id.begin(), way_areas_id.end());

  area_id.clear();
  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator it = input_areas.begin();
       it != input_areas.end(); ++it)
  {
    for (std::vector< Area_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      area_id.push_back(it2->id);

      for (std::vector< uint32 >::const_iterator it3(it2->used_indices.begin());
          it3 != it2->used_indices.end(); ++it3)
        area_blocks_req.insert(Uint31_Index(*it3));
    }
  }
  std::sort(area_id.begin(), area_id.end());
}


Query_Filter_Strategy Area_Constraint::delivers_data(Resource_Manager& rman)
{
  if (!area->areas_from_input())
    return (area->count_ranges(rman) < 12) ? prefer_ranges : ids_useful;
  else
  {
    const Set* input = rman.get_set(area->get_input());
    if (!input)
      return prefer_ranges;

    // Count the indicies of the input areas
    int counter = 0;

    for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator it = input->areas.begin();
         it != input->areas.end(); ++it)
    {
      for (std::vector< Area_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
        counter += it2->used_indices.size();
    }

    return (counter <= 12) ? prefer_ranges : ids_useful;
  }
}


template< typename Node_Skeleton >
void Area_Query_Statement::collect_nodes
    (std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
     const std::set< Uint31_Index >& req, bool add_border,
     Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Block, std::set< Uint31_Index >::const_iterator > area_blocks_db
      (rman.get_area_transaction()->data_index(area_settings().AREA_BLOCKS));
  Block_Backend< Uint31_Index, Area_Block, std::set< Uint31_Index >::const_iterator >::Discrete_Iterator
      area_it(area_blocks_db.discrete_begin(req.begin(), req.end()));

  typename std::map< Uint32_Index, std::vector< Node_Skeleton > >::iterator nodes_it = nodes.begin();

  uint32 loop_count = 0;
  uint32 current_idx(0);
  while (!(area_it == area_blocks_db.discrete_end()))
  {
    current_idx = area_it.index().val();
    if (loop_count > 1024*1024)
    {
      rman.health_check(*this);
      loop_count = 0;
    }

    std::map< Area_Skeleton::Id_Type, std::vector< Area_Block > > areas;
    while ((!(area_it == area_blocks_db.discrete_end())) &&
        (area_it.index().val() == current_idx))
    {
      if (binary_search(area_id.begin(), area_id.end(), area_it.object().id))
	areas[area_it.object().id].push_back(area_it.object());
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
        for (std::map< Area_Skeleton::Id_Type, std::vector< Area_Block > >::const_iterator it = areas.begin();
	     it != areas.end(); ++it)
        {
          int inside = 0;
          for (std::vector< Area_Block >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
	       ++it2)
          {
            ++loop_count;

	    int check(Coord_Query_Statement::check_area_block(current_idx, *it2, ilat, ilon));
	    if (check == Coord_Query_Statement::HIT && add_border)
	    {
	      inside = 1;
	      break;
	    }
	    else if (check != 0)
	      inside ^= check;
          }
          if (inside)
	  {
	    into.push_back(*iit);
	    break;
	  }
        }
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


const int HIT = 1;
const int INTERSECT = 8;


inline int ordered_intersects_inner
    (uint32 lat_a0, uint32 lon_a0, uint32 lat_a1, uint32 lon_a1,
     uint32 lat_b0, uint32 lon_b0, uint32 lat_b1, uint32 lon_b1)
{
  double det = ((double(lat_a1) - lat_a0)*(double(lon_b1) - lon_b0) - (double(lat_b1) - lat_b0)*(double(lon_a1) - lon_a0));
  if (det != 0)
  {
    double lon =
	double(lat_b0 - lat_a0
	    + double(lon_a0)/(double(lon_a1) - lon_a0)*(double(lat_a1) - lat_a0)
	    - double(lon_b0)/(double(lon_b1) - lon_b0)*(double(lat_b1) - lat_b0))
	/det*(double(lon_a1) - lon_a0)*(double(lon_b1) - lon_b0);
    if (lon_a0 < lon && lon < lon_a1 && lon_b0 <= lon && lon <= lon_b1)
      return (((lat_a0 != lat_b0 || lon_a0 != lon_b0) && (lat_a1 != lat_b1 || lon_a1 != lon_b1)) ?
          INTERSECT : 0);
  }
  else if ((fabs(lat_a0 - (double(lon_a0) - lon_b0)/(double(lon_b1) - lon_b0)
                *(double(lat_b1) - lat_b0) - lat_b0) <= 1)
	|| (fabs(lat_a1 - (double(lon_a1) - lon_b0)/(double(lon_b1) - lon_b0)
                *(double(lat_b1) - lat_b0) - lat_b0) <= 1))
    return HIT;

  return 0;
}


inline int ordered_a_intersects_inner
    (uint32 lat_a0, uint32 lon_a0, uint32 lat_a1, uint32 lon_a1,
     uint32 lat_b0, uint32 lon_b0, uint32 lat_b1, uint32 lon_b1)
{
  if (lon_b0 < lon_b1)
  {
    if (lon_a0 < lon_b1 && lon_b0 < lon_a1)
      return ordered_intersects_inner(lat_a0, lon_a0, lat_a1, lon_a1, lat_b0, lon_b0, lat_b1, lon_b1);
  }
  else if (lon_b1 < lon_b0)
  {
    if (lon_a0 < lon_b0 && lon_b1 < lon_a1)
      return ordered_intersects_inner(lat_a0, lon_a0, lat_a1, lon_a1, lat_b1, lon_b1, lat_b0, lon_b0);
  }
  else // lon_b0 == lon_b1
  {
    if (lon_a0 < lon_b0 && lon_b0 < lon_a1)
    {
      double lat = (double(lon_b0) - lon_a0)/(double(lon_a1) - lon_a0)*(double(lat_a1) - lat_a0) + lat_a0;
      return (((lat_b0 < lat && lat < lat_b1) || (lat_b1 < lat && lat < lat_b0)) ?
          INTERSECT : 0);
    }
  }

  return 0;
}


inline int longitude_a_intersects_inner
    (uint32 lat_a0, uint32 lon_a, uint32 lat_a1,
     uint32 lat_b0, uint32 lon_b0, uint32 lat_b1, uint32 lon_b1)
{
  if (lon_b0 < lon_b1)
  {
    if (lon_a < lon_b1 && lon_b0 < lon_a)
    {
      double lat = (double(lon_a) - lon_b0)/(double(lon_b1) - lon_b0)*(double(lat_b1) - lat_b0) + lat_b0;
      return (((lat_a0 < lat && lat < lat_a1) || (lat_a1 < lat && lat < lat_a0)) ? INTERSECT : 0);
    }
  }
  else if (lon_b1 < lon_b0)
  {
    if (lon_a < lon_b0 && lon_b1 < lon_a)
    {
      double lat = (double(lon_a) - lon_b0)/(double(lon_b1) - lon_b0)*(double(lat_b1) - lat_b0) + lat_b0;
      return (((lat_a0 < lat && lat < lat_a1) || (lat_a1 < lat && lat < lat_a0)) ? INTERSECT : 0);
    }
  }
  else // lon_b0 == lon_b1
  {
    if (lon_a != lon_b0)
      return 0;
    if (lat_a0 < lat_a1)
    {
      if (lat_b0 < lat_b1)
        return ((lat_a0 < lat_b1 && lat_b0 < lat_a1) ? HIT : 0);
      else
        return ((lat_a0 < lat_b0 && lat_b1 < lat_a1) ? HIT : 0);
    }
    else
    {
      if (lat_b0 < lat_b1)
        return ((lat_a1 < lat_b1 && lat_b0 < lat_a0) ? HIT : 0);
      else
        return ((lat_a1 < lat_b0 && lat_b1 < lat_a0) ? HIT : 0);
    }
  }

  return 0;
}


int intersects_inner(const Area_Block& string_a, const Area_Block& string_b)
{
  std::vector< std::pair< uint32, uint32 > > coords_a;
  for (std::vector< uint64 >::const_iterator it = string_a.coors.begin(); it != string_a.coors.end(); ++it)
    coords_a.push_back(std::make_pair(::ilat(uint32((*it>>32)&0xff), uint32(*it & 0xffffffffull)),
				 ::ilon(uint32((*it>>32)&0xff), uint32(*it & 0xffffffffull))));

  std::vector< std::pair< uint32, uint32 > > coords_b;
  for (std::vector< uint64 >::const_iterator it = string_b.coors.begin(); it != string_b.coors.end(); ++it)
    coords_b.push_back(std::make_pair(::ilat(uint32((*it>>32)&0xff), uint32(*it & 0xffffffffull)),
				 ::ilon(uint32((*it>>32)&0xff), uint32(*it & 0xffffffffull))));

  for (std::vector< std::pair< uint32, uint32 > >::size_type i = 0; i < coords_a.size()-1; ++i)
  {
    if (coords_a[i].second < coords_a[i+1].second)
    {
      for (std::vector< std::pair< uint32, uint32 > >::size_type j = 0; j < coords_b.size()-1; ++j)
      {
	int result = ordered_a_intersects_inner
	    (coords_a[i].first, coords_a[i].second, coords_a[i+1].first, coords_a[i+1].second,
	     coords_b[j].first, coords_b[j].second, coords_b[j+1].first, coords_b[j+1].second);
        if (result)
	  return result;
      }
    }
    else if (coords_a[i+1].second < coords_a[i].second)
    {
      for (std::vector< std::pair< uint32, uint32 > >::size_type j = 0; j < coords_b.size()-1; ++j)
      {
	int result = ordered_a_intersects_inner
	    (coords_a[i+1].first, coords_a[i+1].second, coords_a[i].first, coords_a[i].second,
	     coords_b[j].first, coords_b[j].second, coords_b[j+1].first, coords_b[j+1].second);
        if (result)
          return result;
      }
    }
    else
      for (std::vector< std::pair< uint32, uint32 > >::size_type j = 0; j < coords_b.size()-1; ++j)
      {
	int result = longitude_a_intersects_inner
	    (coords_a[i].first, coords_a[i].second, coords_a[i+1].first,
	     coords_b[j].first, coords_b[j].second, coords_b[j+1].first, coords_b[j+1].second);
        if (result)
          return result;
      }
  }

  return 0;
}


void has_inner_points(const Area_Block& string_a, const Area_Block& string_b, int& inside)
{
  std::vector< std::pair< uint32, uint32 > > coords_a;
  for (std::vector< uint64 >::const_iterator it = string_a.coors.begin(); it != string_a.coors.end(); ++it)
    coords_a.push_back(std::make_pair(::ilat(uint32((*it>>32)&0xff), uint32(*it & 0xffffffffull)),
                                 ::ilon(uint32((*it>>32)&0xff), uint32(*it & 0xffffffffull))));

  // Check additionally the middle of the segment to also get segments
  // that run through the area
  for (std::vector< std::pair< uint32, uint32 > >::size_type i = 0; i < coords_a.size()-1; ++i)
  {
    uint32 ilat = (coords_a[i].first + coords_a[i+1].first)/2;
    uint32 ilon = (coords_a[i].second + coords_a[i+1].second)/2 + 0x80000000u;
    int check = Coord_Query_Statement::check_area_block(0, string_b, ilat, ilon);
    if (check & Coord_Query_Statement::HIT)
      inside = check;
    else if (check)
      inside ^= check;
  }
}


template< typename Way_Skeleton >
void Area_Query_Statement::collect_ways
      (const Way_Geometry_Store& way_geometries,
       std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
       const std::set< Uint31_Index >& req, bool add_border,
       const Statement& query, Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Block, std::set< Uint31_Index >::const_iterator > area_blocks_db
      (rman.get_area_transaction()->data_index(area_settings().AREA_BLOCKS));
  Block_Backend< Uint31_Index, Area_Block, std::set< Uint31_Index >::const_iterator >::Discrete_Iterator
      area_it(area_blocks_db.discrete_begin(req.begin(), req.end()));

  std::map< Way::Id_Type, bool > ways_inside;

  std::map< Uint31_Index, std::vector< Area_Block > > way_segments;
  for (typename std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways.begin(); it != ways.end(); ++it)
  {
    for (typename std::vector< Way_Skeleton >::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      add_way_to_area_blocks(way_geometries.get_geometry(*it2), it2->id.val(), way_segments);
  }

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

  // Fill node_status with the area related status of each node and segment
  uint32 loop_count = 0;
  uint32 current_idx(0);
  while (!(area_it == area_blocks_db.discrete_end()))
  {
    current_idx = area_it.index().val();
    if (loop_count > 64*1024)
    {
      rman.health_check(*this);
      loop_count = 0;
    }

    std::map< Area_Skeleton::Id_Type, std::vector< Area_Block > > areas;
    while ((!(area_it == area_blocks_db.discrete_end())) &&
        (area_it.index().val() == current_idx))
    {
      if (binary_search(area_id.begin(), area_id.end(), area_it.object().id))
	areas[area_it.object().id].push_back(area_it.object());
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
        for (std::map< Area_Skeleton::Id_Type, std::vector< Area_Block > >::const_iterator it = areas.begin();
	     it != areas.end(); ++it)
        {
          int inside = 0;
          for (std::vector< Area_Block >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
	       ++it2)
          {
            ++loop_count;

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
      }
      ++nodes_it;
    }

    // check segments
    for (std::vector< Area_Block >::const_iterator sit = way_segments[Uint31_Index(current_idx)].begin();
	 sit != way_segments[Uint31_Index(current_idx)].end(); ++sit)
    {
      std::map< Area::Id_Type, int > area_status;
      for (std::map< Area_Skeleton::Id_Type, std::vector< Area_Block > >::const_iterator it = areas.begin();
	   it != areas.end(); ++it)
      {
	if (ways_inside[Way::Id_Type(sit->id)])
	  break;
        for (std::vector< Area_Block >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
	     ++it2)
	{
	  // If an area segment intersects this way segment in the inner of the way,
          // the way is contained in the area.
          // The endpoints are properly handled via the point-in-area test
          // Check additionally the middle of the segment to also get segments
          // that run through the area
          int intersect = intersects_inner(*sit, *it2);
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
              area_status[it2->id] = Coord_Query_Statement::HIT;
            break;
          }
          has_inner_points(*sit, *it2, area_status[it2->id]);
	}
      }
      for (std::map< Area::Id_Type, int >::const_iterator it = area_status.begin(); it != area_status.end(); ++it)
      {
        if ((it->second && (!(it->second & Coord_Query_Statement::HIT))) ||
            (it->second && add_border))
          ways_inside[Way::Id_Type(sit->id)] = true;
      }
    }
  }

  std::map< Uint31_Index, std::vector< Way_Skeleton > > result;

  // Mark ways as found that intersect the area border
  for (typename std::map< Uint31_Index, std::vector< Way_Skeleton > >::iterator it = ways.begin();
       it != ways.end(); ++it)
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


void Area_Query_Statement::execute(Resource_Manager& rman)
{
  Set into;

  Area_Constraint constraint(*this);
  Ranges< Uint32_Index > ranges;
  constraint.get_ranges(rman, ranges);
  get_elements_from_db< Uint32_Index, Node_Skeleton >(ranges, *this, rman)
      .swap(into.nodes, into.attic_nodes);
  constraint.filter(rman, into);

  Request_Context context(this, rman);
  filter_attic_elements(context, rman.get_desired_timestamp(), into.nodes, into.attic_nodes);
  constraint.filter(*this, rman, into);

  transfer_output(rman, into);
  rman.health_check(*this);
}


Query_Constraint* Area_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Area_Constraint(*this));
  return constraints.back();
}
