#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "user_interface.h"
#include "area_query_statement.h"

#include <mysql.h>

using namespace std;

void Area_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["ref"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  
  area_ref = (uint32)atoll(attributes["ref"].c_str());
  if (area_ref == 0)
  {
    ostringstream temp;
    temp<<"For the attribute \"ref\" of the element \"area-query\""
	<<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
}

const uint8 OUTSIDE(0);
const uint8 BORDER(1);
const uint8 INSIDE(2);

void set_block_status
    (uint32 ll_idx_, const set< Line_Segment >& segments, vector< uint8 >& block_status)
{
  int32 lat(lat_of_ll(ll_idx_));
  int32 lon(lon_of_ll(ll_idx_));
  
  vector< bool > lat_intersections(16+1, false);
  for (set< Line_Segment >::const_iterator it(segments.begin());
       it != segments.end(); ++it)
  {
    int32 max_lat(it->west_lat);
    int32 min_lat(it->east_lat);
    if (it->east_lat > max_lat)
    {
      max_lat = it->east_lat;
      min_lat = it->west_lat;
    }
    
    //treat bottomline segments
    if (max_lat == -100*10*1000*1000)
    {
      if (it->west_lon == lon)
	lat_intersections[0] = !(lat_intersections[0]);
      continue;
    }
    
    //mark the blocks that contain this segment as border
    uint max_lat_level((max_lat - lat)>>16);
    if (max_lat_level > 16-1)
      max_lat_level = 16-1;
    uint min_lat_level((min_lat - lat)>>16);
    for (uint i(min_lat_level); i <= max_lat_level; ++i)
      block_status[i] = BORDER;
    
    //trace where the left border is inside or outside
    if (it->west_lon == lon)
    {
      int32 lat_level((it->west_lat - lat)>>16);
      lat_intersections[lat_level] = !(lat_intersections[lat_level]);
    }
    if (it->east_lon == lon)
    {
      int32 lat_level((it->east_lat - lat)>>16);
      lat_intersections[lat_level] = !(lat_intersections[lat_level]);
    }
  }
  
  //detect the blocks that are entirely inside by investigating lat_intersections
  bool is_inside(false);
  for (uint i(0); i < 16; ++i)
  {
    is_inside = is_inside ^ lat_intersections[i];
    if ((is_inside) && (block_status[i] == OUTSIDE))
      block_status[i] = INSIDE;
  }
}

void deduce_intervals
    (const vector< vector< uint8 > >& block_status, uint32 current_bigblock,
     set< pair< int32, int32 > >& in_inside, set< pair< int32, int32 > >&  in_border)
{
  int32 pending_inside(-1);
  for (uint i(0); i < 256; ++i)
  {
    uint32 x(i & 0x01);
    x |= (i>>1) & 0x02;
    x |= (i>>2) & 0x04;
    x |= (i>>3) & 0x08;
    uint32 y((i>>1) & 0x01);
    y |= (i>>2) & 0x02;
    y |= (i>>3) & 0x04;
    y |= (i>>4) & 0x08;
    
    if (block_status[x][y] == INSIDE)
    {
      if (pending_inside == -1)
	pending_inside = i;
      continue;
    }
    if (pending_inside != -1)
      in_inside.insert(make_pair(current_bigblock + pending_inside, current_bigblock + i-1));
    pending_inside = -1;
    if (block_status[x][y] == BORDER)
      in_border.insert(make_pair(current_bigblock + i, current_bigblock + i));
  }
  if (pending_inside != -1)
    in_inside.insert(make_pair(current_bigblock + pending_inside, current_bigblock + 255));
}

void Area_Query_Statement::indices_of_area(MYSQL* mysql, uint32 area_ref)
{
  if (segments_per_tile.empty())
  {
    ostringstream temp;
    temp<<"select ll_idx, min_lat, min_lon, max_lat, max_lon from area_segments "
	<<"where id = "<<area_ref;
    singleArea_query(mysql, temp.str(), segments_per_tile);
  }
  
  if ((in_inside.empty()) && (in_border.empty()))
  {
    vector< vector< uint8 > > block_status(16, vector< uint8 >(16, OUTSIDE));
    for (map< uint32, set< Line_Segment > >::const_iterator
	 it(segments_per_tile.begin());
	 it != segments_per_tile.end(); )
    {
      uint32 current_bigblock(it->first & 0xffffff00);
      set_block_status(it->first, it->second, block_status[(lon_of_ll(it->first)>>16) & 0xf]);
    
      if (++it == segments_per_tile.end())
      {
	deduce_intervals(block_status, current_bigblock, in_inside, in_border);
	break;
      }
      if ((it->first & 0xffffff00) != current_bigblock)
      {
	deduce_intervals(block_status, current_bigblock, in_inside, in_border);
	block_status = vector< vector< uint8 > >(16, vector< uint8 >(16, OUTSIDE));
      }
    }
  }
}

void Area_Query_Statement::forecast(MYSQL* mysql)
{
  Set_Forecast& sf_out(declare_write_set(output));
  
  indices_of_area(mysql, area_ref);
  
  sf_out.node_count = multiRange_to_count_query(in_inside, in_border);
  declare_used_time(1000 + sf_out.node_count/100000);
  finish_statement_forecast();
  
  display_full();
  display_state();
}

bool Area_Query_Statement::is_contained(const Node& node)
{
  set< Line_Segment > column(segments_per_tile[ll_idx(node.lat, node.lon) & 0xffffff55]);
  bool is_contained(false);
  
  for (set< Line_Segment >::const_iterator it(column.begin()); it != column.end(); ++it)
  {
    if (it->east_lon > node.lon)
    {
      if (it->west_lon < node.lon)
      {
	if ((it->west_lat < node.lat) && (it->east_lat < node.lat))
	  is_contained = !is_contained;
	else if ((it->west_lat < node.lat) || (it->east_lat < node.lat))
	{
	  int rel_lat(((long long)(it->east_lat - it->west_lat))*(node.lon - it->west_lon)/
	      (it->east_lon - it->west_lon) + it->west_lat);
	  if (rel_lat < node.lat)
	    is_contained = !is_contained;
	  else if (rel_lat == node.lat)
	    //We are on a border segment.
	    return true;
	}
	else if ((it->west_lat == node.lat) && (it->east_lat == node.lat))
	  //We are on a horizontal border segment.
	  return true;
      }
      else if (it->west_lon == node.lon)
	//We are north of a node of the border.
	//We can safely count such a segment if and only if the node is
	//on its western end.
      {
	if (it->west_lat < node.lat)
	  is_contained = !is_contained;
	else if (it->west_lat == node.lat)
	  //We have hit a node of the border.
	  return true;
      }
    }
    else if (it->east_lon == node.lon)
    {
      if (it->east_lat == node.lat)
	//We have hit a node of the border.
	return true;
      else if (it->west_lon == it->east_lon)
	//We are on a vertical border segment.
      {
	if ((it->west_lat <= node.lat) && (node.lat <= it->east_lat))
	  return true;
      }
    }
  }
  
  return is_contained;
}

void Area_Query_Statement::get_nodes(MYSQL* mysql, set< Node >& nodes)
{
  indices_of_area(mysql, area_ref);
  set< Node > on_border;
  multiRange_to_multiNode_query(in_inside, in_border, nodes, on_border);
  for (set< Node >::const_iterator it(on_border.begin()); it != on_border.end(); ++it)
  {
    if (is_contained(*it))
      nodes.insert(*it);
  }
}

void Area_Query_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node >* nodes(&(maps[output].get_nodes_handle()));
  set< Way >* ways(&(maps[output].get_ways_handle()));
  set< Relation_ >* relations(&(maps[output].get_relations_handle()));
  set< Area >* areas(&(maps[output].get_areas_handle()));
  nodes->clear();
  ways->clear();
  relations->clear();
  areas->clear();
  
  get_nodes(mysql, *nodes);
}
