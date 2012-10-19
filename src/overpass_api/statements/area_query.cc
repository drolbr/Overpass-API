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

#include "../../template_db/block_backend.h"
#include "area_query.h"
#include "coord_query.h"

using namespace std;

class Area_Constraint : public Query_Constraint
{
  public:
    Area_Constraint(Area_Query_Statement& area_) : area(&area_) {}

    bool delivers_data() { return true; }
    
    bool get_ranges
        (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges);
    void filter(Resource_Manager& rman, Set& into);
    virtual ~Area_Constraint() {}
    
  private:
    Area_Query_Statement* area;
    set< Uint31_Index > area_blocks_req;
};

bool Area_Constraint::get_ranges
    (Resource_Manager& rman, set< pair< Uint32_Index, Uint32_Index > >& ranges)
{
  area->get_ranges(ranges, area_blocks_req, rman);
  return true;
}

void Area_Constraint::filter(Resource_Manager& rman, Set& into)
{
  set< pair< Uint32_Index, Uint32_Index > > range_req;
  if (area_blocks_req.empty())
    area->get_ranges(range_req, area_blocks_req, rman);
  area->collect_nodes(into.nodes, area_blocks_req, rman);
  into.ways.clear();
  into.relations.clear();
  into.areas.clear();
}

//-----------------------------------------------------------------------------

bool Area_Query_Statement::is_used_ = false;

Generic_Statement_Maker< Area_Query_Statement > Area_Query_Statement::statement_maker("area-query");

Area_Query_Statement::Area_Query_Statement
    (int line_number_, const map< string, string >& input_attributes)
    : Statement(line_number_)
{
  is_used_ = true;

  map< string, string > attributes;
  long long submitted_id;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["ref"] = "";
  
  eval_attributes_array(get_name(), attributes, input_attributes);
  
  input = attributes["from"];
  output = attributes["into"];
  submitted_id = atoll(attributes["ref"].c_str());
  if (submitted_id <= 0 && attributes["ref"] != "")
  {
    ostringstream temp;
    temp<<"For the attribute \"ref\" of the element \"area-query\""
    <<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
  else if (submitted_id > 0)
    area_id.push_back(Area_Skeleton::Id_Type(submitted_id));
}

Area_Query_Statement::~Area_Query_Statement()
{
  for (vector< Query_Constraint* >::const_iterator it = constraints.begin();
      it != constraints.end(); ++it)
    delete *it;
}

void Area_Query_Statement::forecast()
{
}


void Area_Query_Statement::get_ranges
    (set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
     set< Uint31_Index >& area_block_req,
     Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Skeleton > area_locations_db
      (rman.get_area_transaction()->data_index(area_settings().AREAS));
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it(area_locations_db.flat_begin());
      !(it == area_locations_db.flat_end()); ++it)
  {
    if (binary_search(area_id.begin(), area_id.end(), it.object().id))
    {
      for (set< uint32 >::const_iterator it2(it.object().used_indices.begin());
          it2 != it.object().used_indices.end(); ++it2)
      {
	area_block_req.insert(Uint31_Index(*it2));
	pair< Uint32_Index, Uint32_Index > range
	    (make_pair(Uint32_Index(*it2), Uint32_Index((*it2) + 0x100)));
	nodes_req.insert(range);
      }
    }
  }
}


void Area_Query_Statement::get_ranges
    (const map< Uint31_Index, vector< Area_Skeleton > >& input_areas,
     set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
     set< Uint31_Index >& area_block_req,
     Resource_Manager& rman)
{
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator it = input_areas.begin();
       it != input_areas.end(); ++it)
  {
    for (vector< Area_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      area_id.push_back(it2->id);
      
      for (set< uint32 >::const_iterator it3(it2->used_indices.begin());
          it3 != it2->used_indices.end(); ++it3)
      {
        area_block_req.insert(Uint31_Index(*it3));
        pair< Uint32_Index, Uint32_Index > range
	    (make_pair(Uint32_Index(*it3), Uint32_Index((*it3) + 0x100)));
        nodes_req.insert(range);
      }
    }
  }
  
  sort(area_id.begin(), area_id.end());
}


void Area_Query_Statement::collect_nodes
    (const set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
     const set< Uint31_Index >& req,
     vector< Node::Id_Type >* ids,
     map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (rman.get_area_transaction()->data_index(area_settings().AREA_BLOCKS));
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (rman.get_transaction()->data_index(osm_base_settings().NODES));
  Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      area_it(area_blocks_db.discrete_begin(req.begin(), req.end()));
  Block_Backend< Uint32_Index, Node_Skeleton >::Range_Iterator
      nodes_it(nodes_db.range_begin(nodes_req.begin(), nodes_req.end()));
  uint32 current_idx(0);
  if (!(area_it == area_blocks_db.discrete_end()))
    current_idx = area_it.index().val();
  while (!(area_it == area_blocks_db.discrete_end()))
  {
    rman.health_check(*this);
    
    map< Area_Skeleton::Id_Type, vector< Area_Block > > areas;
    while ((!(area_it == area_blocks_db.discrete_end())) &&
        (area_it.index().val() == current_idx))
    {
      if (binary_search(area_id.begin(), area_id.end(), area_it.object().id))
	areas[area_it.object().id].push_back(area_it.object());
      ++area_it;
    }
    while ((!(nodes_it == nodes_db.range_end())) &&
        ((nodes_it.index().val() & 0xffffff00) == current_idx))
    {
      if ((ids != 0) &&
	  (!binary_search(ids->begin(), ids->end(), nodes_it.object().id)))
      {
	++nodes_it;
	continue;
      }
      
      uint32 ilat((::lat(nodes_it.index().val(), nodes_it.object().ll_lower)
          + 91.0)*10000000+0.5);
      int32 ilon(::lon(nodes_it.index().val(), nodes_it.object().ll_lower)*10000000
          + (::lon(nodes_it.index().val(), nodes_it.object().ll_lower) > 0
	      ? 0.5 : -0.5));
      for (map< Area_Skeleton::Id_Type, vector< Area_Block > >::const_iterator it = areas.begin();
	   it != areas.end(); ++it)
      {
        int inside = 0;
        for (vector< Area_Block >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
	     ++it2)
        {
	  int check(Coord_Query_Statement::check_area_block(current_idx, *it2, ilat, ilon));
	  if (check == Coord_Query_Statement::HIT)
	  {
	    inside = 1;
	    break;
	  }
	  else if (check != 0)
	    inside ^= check;
        }
        if (inside)
	{
	  nodes[nodes_it.index()].push_back(nodes_it.object());
	  break;
	}
      }
      ++nodes_it;
    }
    current_idx = area_it.index().val();
  }
}

void Area_Query_Statement::collect_nodes
    (map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     const set< Uint31_Index >& req,
     Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (rman.get_area_transaction()->data_index(area_settings().AREA_BLOCKS));
  Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      area_it(area_blocks_db.discrete_begin(req.begin(), req.end()));

  map< Uint32_Index, vector< Node_Skeleton > >::iterator nodes_it = nodes.begin();
  
  uint32 current_idx(0);
  if (!(area_it == area_blocks_db.discrete_end()))
    current_idx = area_it.index().val();
  
  while (!(area_it == area_blocks_db.discrete_end()))
  {
    rman.health_check(*this);
    
    map< Area_Skeleton::Id_Type, vector< Area_Block > > areas;
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
      vector< Node_Skeleton > into;
      for (vector< Node_Skeleton >::const_iterator iit = nodes_it->second.begin();
          iit != nodes_it->second.end(); ++iit)
      {
        uint32 ilat((::lat(nodes_it->first.val(), iit->ll_lower)
            + 91.0)*10000000+0.5);
        int32 ilon(::lon(nodes_it->first.val(), iit->ll_lower)*10000000
            + (::lon(nodes_it->first.val(), iit->ll_lower) > 0 ? 0.5 : -0.5));
        for (map< Area_Skeleton::Id_Type, vector< Area_Block > >::const_iterator it = areas.begin();
	     it != areas.end(); ++it)
        {
          int inside = 0;
          for (vector< Area_Block >::const_iterator it2 = it->second.begin(); it2 != it->second.end();
	       ++it2)
          {
	    int check(Coord_Query_Statement::check_area_block(current_idx, *it2, ilat, ilon));
	    if (check == Coord_Query_Statement::HIT)
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
    current_idx = area_it.index().val();
  }
  while (nodes_it != nodes.end())
  {
    nodes_it->second.clear();
    ++nodes_it;
  }
}

void collect_nodes_from_req
    (const set< pair< Uint32_Index, Uint32_Index > >& req,
     map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     Resource_Manager& rman)
{
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (rman.get_transaction()->data_index(osm_base_settings().NODES));
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Range_Iterator
      it(nodes_db.range_begin
      (Default_Range_Iterator< Uint32_Index >(req.begin()),
       Default_Range_Iterator< Uint32_Index >(req.end())));
      !(it == nodes_db.range_end()); ++it)
    nodes[it.index()].push_back(it.object());    
}

void Area_Query_Statement::execute(Resource_Manager& rman)
{
  map< Uint32_Index, vector< Node_Skeleton > > nodes;
  map< Uint31_Index, vector< Way_Skeleton > > ways;
  map< Uint31_Index, vector< Relation_Skeleton > > relations;
  map< Uint31_Index, vector< Area_Skeleton > > areas;
  
  set< Uint31_Index > req;
  
  set< pair< Uint32_Index, Uint32_Index > > nodes_req;
  if (area_id.empty())
    get_ranges(rman.sets()[input].areas, nodes_req, req, rman);
  else
    get_ranges(nodes_req, req, rman);
  collect_nodes_from_req(nodes_req, nodes, rman);
  
  collect_nodes(nodes, req, rman);
  
  nodes.swap(rman.sets()[output].nodes);
  ways.swap(rman.sets()[output].ways);
  relations.swap(rman.sets()[output].relations);
  areas.swap(rman.sets()[output].areas);
  
  rman.health_check(*this);
}

Query_Constraint* Area_Query_Statement::get_query_constraint()
{
  constraints.push_back(new Area_Constraint(*this));
  return constraints.back();
}
