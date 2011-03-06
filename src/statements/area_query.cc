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
#include "area_query.h"
#include "coord_query.h"

using namespace std;

void Area_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  long long submitted_id;
  
  attributes["into"] = "_";
  attributes["ref"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  submitted_id = atoll(attributes["ref"].c_str());
  if (submitted_id <= 0)
  {
    ostringstream temp;
    temp<<"For the attribute \"ref\" of the element \"area-query\""
	<<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
  area_id = submitted_id;
}

void Area_Query_Statement::forecast()
{
/*  Set_Forecast& sf_out(declare_write_set(output));
    
  sf_out.area_count = 10;
  declare_used_time(10);
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

void Area_Query_Statement::get_ranges
    (set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
     set< Uint31_Index >& area_block_req)
{
  Block_Backend< Uint31_Index, Area_Skeleton > area_locations_db
      (*de_osm3s_file_ids::AREAS, true);
  for (Block_Backend< Uint31_Index, Area_Skeleton >::Flat_Iterator
      it(area_locations_db.flat_begin());
      !(it == area_locations_db.flat_end()); ++it)
  {
    if (it.object().id == area_id)
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

void Area_Query_Statement::collect_nodes
    (const set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
     const set< Uint31_Index >& req,
     vector< uint32 >* ids,
     map< Uint32_Index, vector< Node_Skeleton > >& nodes,
     Stopwatch& stopwatch,
     Resource_Manager& rman)
{
  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (*de_osm3s_file_ids::AREA_BLOCKS, true);
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (*de_osm3s_file_ids::NODES, true);
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
    
    vector< Area_Block > areas;
    while ((!(area_it == area_blocks_db.discrete_end())) &&
        (area_it.index().val() == current_idx))
    {
      if (area_it.object().id == area_id)
	areas.push_back(area_it.object());
      ++area_it;
    }
    stopwatch.add(Stopwatch::AREA_BLOCKS, area_blocks_db.read_count());
    stopwatch.stop(Stopwatch::AREA_BLOCKS);
    while ((!(nodes_it == nodes_db.range_end())) &&
        ((nodes_it.index().val() & 0xffffff00) == current_idx))
    {
      if ((ids != 0) &&
	  (!binary_search(ids->begin(), ids->end(), nodes_it.object().id)))
      {
	++nodes_it;
	continue;
      }
      
      bool inside(false);
      uint32 ilat((Node::lat(nodes_it.index().val(), nodes_it.object().ll_lower)
          + 91.0)*10000000+0.5);
      int32 ilon(Node::lon(nodes_it.index().val(), nodes_it.object().ll_lower)*10000000
          + (Node::lon(nodes_it.index().val(), nodes_it.object().ll_lower) > 0
	      ? 0.5 : -0.5));
      stopwatch.add(Stopwatch::NODES, nodes_db.read_count());
      stopwatch.stop(Stopwatch::NODES);
      for (vector< Area_Block >::const_iterator it(areas.begin());
          it != areas.end(); ++it)
      {
	int check(Coord_Query_Statement::check_area_block
	    (current_idx, *it, ilat, ilon));
	if (check == Coord_Query_Statement::HIT)
	{
	  nodes[nodes_it.index()].push_back(nodes_it.object());
	  inside = false;
	  break;
	}
	else if (check == Coord_Query_Statement::TOGGLE)
	  inside = !inside;
      }
      if (inside)
	nodes[nodes_it.index()].push_back(nodes_it.object());
      stopwatch.stop(Stopwatch::NO_DISK);
      ++nodes_it;
    }
    current_idx = area_it.index().val();
  }
}

void Area_Query_Statement::execute(Resource_Manager& rman)
{ 
  stopwatch.start();

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
  
  set< Uint31_Index > req;
  set< pair< Uint32_Index, Uint32_Index > > nodes_req;
  get_ranges(nodes_req, req);
  
  collect_nodes(nodes_req, req, NULL, nodes, stopwatch, rman);

  stopwatch.stop(Stopwatch::NO_DISK);
  stopwatch.report(get_name());
  rman.health_check(*this);
}
