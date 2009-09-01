#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "user_interface.h"
#include "make_area_statement.h"

#include <mysql.h>

using namespace std;

void Make_Area_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["pivot"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
  tags = attributes["pivot"];
}

void Make_Area_Statement::forecast(MYSQL* mysql)
{
  Set_Forecast sf_in(declare_read_set(input));
  declare_read_set(tags);
  Set_Forecast& sf_out(declare_write_set(output));
    
  sf_out.area_count = 1;
  declare_used_time(100 + sf_in.node_count + sf_in.way_count);
  finish_statement_forecast();
    
  display_full();
  display_state();
}

void insert_segment
    (map< uint32, set< Line_Segment > >& segments_per_tile,
     const Node& nd1, const Node& nd2)
{
  //force nde to be the eastmost node
  Node const* ndw(&nd1);
  Node const* nde(&nd2);
  if ((nd2.lon < nd1.lon) || ((nd1.lon == nd2.lon) && (nd2.lat < nd1.lat)))
  {
    ndw = &nd2;
    nde = &nd1;
  }
  
  //catch the special case that we pass the longitude -180.0
  if (nde->lon - ndw->lon > 180*10*1000*1000)
  {
    int64 latdiff(ndw->lat - nde->lat);
    int32 londiff((180*10*1000*1000 - nde->lon) + (ndw->lon + 180*10*1000*1000));
    Node intersection
	(0, (int32)(latdiff*(180*10*1000*1000 - nde->lon)/londiff + ndw->lat + 90*10*1000*1000),
	  -180*10*1000*1000);
    insert_segment(segments_per_tile, *ndw, intersection);
    intersection.lon = 180*10*1000*1000;
    insert_segment(segments_per_tile, *nde, intersection);
    return;
  }
  
  //split the segment at tile borders
  //first longitudinal
  vector< pair< int32, int32 > > coords;
  coords.push_back(make_pair(ndw->lat + 90*10*1000*1000, ndw->lon));
  int32 lon_split_number((nde->lon>>16) - (ndw->lon>>16));
  if (lon_split_number > 0)
  {
    int64 latdiff(nde->lat - ndw->lat);
    int32 londiff(nde->lon - ndw->lon);
    for (int32 i(1); i <= lon_split_number; ++i)
    {
      coords.push_back
	  (make_pair(
	   (int32)(latdiff*(((i + (ndw->lon>>16))<<16) - ndw->lon)/londiff + ndw->lat + 90*10*1000*1000),
	   (i + (ndw->lon>>16))<<16));
    }
  }
  coords.push_back(make_pair(nde->lat + 90*10*1000*1000, nde->lon));
  //then latitudinal
  //this includes inserting the segments into their tiles
  vector< pair< int32, int32 > >::const_iterator it(coords.begin());
  pair< int32, int32 > last_coord(*it);
  for (++it; it != coords.end(); ++it)
  {
    int32 lat_split_number((it->first>>20) - (last_coord.first>>20));
    if (lat_split_number > 0)
    {
      int32 latdiff(it->first - last_coord.first);
      int64 londiff(it->second - last_coord.second);
      int32 lastlat(last_coord.first);
      int32 lastlon(last_coord.second);
      for (int32 i(1); i <= lat_split_number; ++i)
      {
	pair< int32, int32 > cur_coord
	    (make_pair(
	     (i + (lastlat>>20))<<20,
	     (int32)(londiff*(((i + (lastlat>>20))<<20) - lastlat)/latdiff + lastlon)));
	pair< set< Line_Segment >::iterator, bool > sp
	    (segments_per_tile
		[ll_idx(last_coord.first - 90*10*1000*1000, last_coord.second) & 0xffffff55].insert
		(Line_Segment(last_coord.first - 90*10*1000*1000, last_coord.second,
		 cur_coord.first - 90*10*1000*1000, cur_coord.second)));
	if (!sp.second)
	  segments_per_tile
	      [ll_idx(last_coord.first - 90*10*1000*1000, last_coord.second) & 0xffffff55].erase
	      (sp.first);
	last_coord = cur_coord;
      }
    }
    else if (lat_split_number < 0)
    {
      int32 latdiff(it->first - last_coord.first);
      int64 londiff(it->second - last_coord.second);
      int32 lastlat(last_coord.first);
      int32 lastlon(last_coord.second);
      for (int32 i(0); i > lat_split_number; --i)
      {
	pair< int32, int32 > cur_coord
	    (make_pair(
	     (i + (lastlat>>20))<<20,
	     (int32)(londiff*(((i + (lastlat>>20))<<20) - lastlat)/latdiff + lastlon)));
	pair< set< Line_Segment >::iterator, bool > sp
	    (segments_per_tile
		[ll_idx(cur_coord.first - 90*10*1000*1000, last_coord.second) & 0xffffff55].insert
	    (Line_Segment(last_coord.first - 90*10*1000*1000, last_coord.second,
	     cur_coord.first - 90*10*1000*1000, cur_coord.second)));
	if (!sp.second)
	  segments_per_tile
	      [ll_idx(cur_coord.first - 90*10*1000*1000, last_coord.second) & 0xffffff55].erase
	      (sp.first);
	last_coord = cur_coord;
      }
    }
    pair< set< Line_Segment >::iterator, bool > sp
	(segments_per_tile
	    [ll_idx(it->first - 90*10*1000*1000, last_coord.second) & 0xffffff55].insert
	(Line_Segment(last_coord.first - 90*10*1000*1000, last_coord.second,
	 it->first - 90*10*1000*1000, it->second)));
    if (!sp.second)
      segments_per_tile
	  [ll_idx(it->first - 90*10*1000*1000, last_coord.second) & 0xffffff55].erase
	  (sp.first);
    last_coord = *it;
  }
}

void report_missing_node(const Way& way, uint32 node_id, const string& input)
{
  ostringstream temp;
  temp<<"Make-Area: Node "<<node_id<<" referred by way "<<way.id
      <<" is not contained in set \""<<input<<"\".\n";
  runtime_error(temp.str());
}

void insert_bottomlines(map< uint32, set< Line_Segment > >& segments_per_tile)
{
  map< uint32, set< Line_Segment > >::const_iterator tile_it(segments_per_tile.begin());
  while (tile_it != segments_per_tile.end())
  {
    set< int32 > lat_projections;
    for (set< Line_Segment >::const_iterator it(tile_it->second.begin());
	 it != tile_it->second.end(); ++it)
    {
      pair< set< int32 >::iterator, bool > lip(lat_projections.insert(it->west_lon));
      if (!lip.second)
	lat_projections.erase(lip.first);
      lip = lat_projections.insert(it->east_lon);
      if (!lip.second)
	lat_projections.erase(lip.first);
    }
    uint32 cur_tile(tile_it->first);
    if (!lat_projections.empty())
    {
      int32 cur_lat(lat_of_ll(cur_tile) + 16*65536);
      map< uint32, set< Line_Segment > >::iterator upper_tile_it
	  (segments_per_tile.insert(make_pair
	  (ll_idx(cur_lat, tile_it->second.begin()->west_lon) & 0xffffff55,
	   set< Line_Segment >())).first);
      set< int32 >::const_iterator it(lat_projections.begin());
      while (it != lat_projections.end())
      {
	uint32 west_lon(*it);
	upper_tile_it->second.insert(Line_Segment
	    (- 100*10*1000*1000, west_lon, - 100*10*1000*1000, *(++it)));
	++it;
      }
    }
    tile_it = segments_per_tile.upper_bound(cur_tile);
  }
}

void Make_Area_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation_ > relations;
  set< Area > areas;
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
  {
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  const set< Node >& in_nodes(mit->second.get_nodes());
  const set< Way >& in_ways(mit->second.get_ways());
  bool data_is_valid(true);
  
  //detect pivot element
  mit = maps.find(tags);
  int pivot_id(0), pivot_type(0);
  if (mit != maps.end())
  {
    if (mit->second.get_nodes().begin() != mit->second.get_nodes().end())
    {
      pivot_id = (mit->second.get_nodes().begin())->id;
      pivot_type = NODE;
    }
    else if (mit->second.get_ways().begin() != mit->second.get_ways().end())
    {
      pivot_id = (mit->second.get_ways().begin())->id;
      pivot_type = WAY;
    }
    else if (mit->second.get_relations().begin() != mit->second.get_relations().end())
    {
      pivot_id = (mit->second.get_relations().begin())->head;
      pivot_type = RELATION;
    }
  }
  
  //check whether the area already exists
  ostringstream temp;
  temp<<"select id from areas where pivot = "<<pivot_id
      <<" and pivot_type = "<<pivot_type;
  int previous_area(int_query(mysql, temp.str()));
  if (previous_area)
  {
    set< uint32 > previous_ways;
    temp.str("");
    temp<<"select way from area_ways where id = "<<previous_area;
    multiint_query(mysql, temp.str(), previous_ways);
    
    set< Way >::const_iterator it(in_ways.begin());
    set< uint32 >::const_iterator it2(previous_ways.begin());
    while ((it != in_ways.end()) && (it2 != previous_ways.end()) && ((it++)->id == (uint32)*(it2++)))
      ;
    if ((it == in_ways.end()) && (it2 == previous_ways.end()))
    {
      temp.str("");
      temp<<"Make-Area: The pivot "<<types_lowercase[pivot_type]<<' '<<pivot_id
	  <<" is already referred by area "<<previous_area<<" made with the same set of ways.\n";
      runtime_remark(temp.str());
      
      ostringstream stack;
      for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
	   it != get_stack().end(); ++it)
	stack<<it->first<<' '<<it->second<<' ';
      temp.str("");
      temp<<"insert area_origins values ("<<previous_area<<", "
	  <<get_rule_id()<<", "
	  <<this->get_line_number()<<", '"
	  <<stack.str()<<"')";
      mysql_query(mysql, temp.str().c_str());
      
      Area area(previous_area);
      areas.insert(area);
      maps[output] = Set(nodes, ways, relations, areas);
      return;
    }
    
    temp.str("");
    temp<<"Make-Area: The pivot "<<types_lowercase[pivot_type]<<' '<<pivot_id
	<<" is already referred by area "<<previous_area<<" made from a different set of ways.\n";
    runtime_error(temp.str());
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  
  //check node parity
  set< uint32 > node_parity_control;
  for (set< Way >::const_iterator it(in_ways.begin());
       it != in_ways.end(); ++it)
  {
    if (it->members.size() < 2)
      continue;
    pair< set< uint32 >::iterator, bool > npp(node_parity_control.insert
	(it->members.front()));
    if (!npp.second)
      node_parity_control.erase(npp.first);
    npp = node_parity_control.insert
    	(it->members.back());
    if (!npp.second)
      node_parity_control.erase(npp.first);
  }
  if (node_parity_control.size() > 0)
  {
    ostringstream temp;
    temp<<"Make-Area: Node "<<*(node_parity_control.begin())
	<<" is contained in an odd number of ways.\n";
    runtime_error(temp.str());
    data_is_valid = false;
  }
  
  //split and collect segments into their respective tiles
  map< uint32, set< Line_Segment > > segments_per_tile;
  for (set< Way >::const_iterator it(in_ways.begin());
       it != in_ways.end(); ++it)
  {
    vector< uint32 >::const_iterator nit(it->members.begin());
    if (nit == it->members.end())
      continue;
    set< Node >::const_iterator nit1(in_nodes.find(Node(*nit, 0, 0)));
    
    //protect against missing nodes: ensure that nit1 refers to a valid node
    while ((nit1 == in_nodes.end()) && (nit != it->members.end()))
    {
      report_missing_node(*it, *nit, input);
      data_is_valid = false;
      if (++nit == it->members.end())
	break;
      nit1 = in_nodes.find(Node(*nit, 0, 0));
    }
    if (nit == it->members.end())
      continue;
    
    for (++nit; nit != it->members.end(); ++nit)
    {
      set< Node >::const_iterator nit2(in_nodes.find(Node(*nit, 0, 0)));
      if (nit2 == in_nodes.end())
      {
	report_missing_node(*it, *nit, input);
	data_is_valid = false;
      }
      else
      {
	insert_segment(segments_per_tile, *nit1, *nit2);
	nit1 = nit2;
      }
    }
  }
  
  if (!data_is_valid)
  {
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  
  insert_bottomlines(segments_per_tile);
  
  uint32 area_id(pivot_id);
  if (pivot_type == WAY)
    area_id += 2400*1000*1000;
  else if (pivot_type == RELATION)
    area_id += 3600*1000*1000;
  Area area(area_id);
  
  temp.str("");
  temp<<"insert areas values ("<<area.id<<", "<<pivot_id<<", "<<pivot_type<<')';
  mysql_query(mysql, temp.str().c_str());
  
  ofstream area_ways_out("/tmp/db_area_area_ways.tsv");
  if (!area_ways_out)
    throw File_Error(0, "/tmp/db_area_area_ways.tsv", "make_area_statement:1");
  for (set< Way >::const_iterator it(in_ways.begin());
       it != in_ways.end(); ++it)
    area_ways_out<<area.id<<'\t'<<it->id<<'\n';
  area_ways_out.close();
  mysql_query(mysql, "load data local infile '/tmp/db_area_area_ways.tsv' into table area_ways");
  
  ofstream area_segments_out("/tmp/db_area_area_segments.tsv");
  if (!area_segments_out)
    throw File_Error(0, "/tmp/db_area_area_segments.tsv", "make_area_statement:2");
  for (map< uint32, set< Line_Segment > >::const_iterator sit(segments_per_tile.begin());
       sit != segments_per_tile.end(); ++sit)
  {
    for (set< Line_Segment >::const_iterator it(sit->second.begin());
	 it != sit->second.end(); ++it)
      area_segments_out<<area.id<<'\t'<<sit->first<<'\t'
	  <<it->west_lat<<'\t'<<it->west_lon<<'\t'
	  <<it->east_lat<<'\t'<<it->east_lon<<'\n';
  }
  area_segments_out.close();
  mysql_query(mysql, "load data local infile '/tmp/db_area_area_segments.tsv' into table area_segments");
  
  temp.str("");
  vector< vector< pair< string, string > > > tags;
  if (pivot_type == NODE)
  {
    set< Node >::const_iterator it(mit->second.get_nodes().begin());
    multiNode_to_kvs_query(mit->second.get_nodes(), it, tags);
  }
  else if (pivot_type == WAY)
  {
    set< Way >::const_iterator it(mit->second.get_ways().begin());
    multiWay_to_kvs_query(mit->second.get_ways(), it, tags);
  }
  else if (pivot_type == RELATION)
  {
    set< Relation_ >::const_iterator it(mit->second.get_relations().begin());
    multiRelation_to_kvs_query(mit->second.get_relations(), it, tags);
  }
  vector< pair< string, string > >::const_iterator tit(tags.begin()->begin());
  temp<<"insert into area_tags values ";
  while (tit != tags.begin()->end())
  {
    ostringstream temp2;
    temp2<<"select id from key_s where key_s.key_ = '";
    escape_insert(temp2, tit->first);
    temp2<<"'";
    int key_id(int_query(mysql, temp2.str()));
    if (key_id == 0)
    {
      key_id = int_query
	  (mysql, "select max(id) from key_s") + 1;
      temp2.str("");
      temp2<<"insert into key_s values "
	  <<"("<<key_id<<", '";
      escape_insert(temp2, tit->first);
      temp2<<"')";
      mysql_query(mysql, temp2.str().c_str());
    }
      
    temp2.str("");
    temp2<<"select id from value_s where value_s.value_ = '";
    escape_insert(temp2, tit->second);
    temp2<<"'";
    int value_id(int_query(mysql, temp2.str()));
    if (value_id == 0)
    {
      value_id = int_query
	  (mysql, "select max(id) from value_s") + 1;
      temp2.str("");
      temp2<<"insert into value_s values "
	  <<"("<<value_id<<", '";
      escape_insert(temp2, tit->second);
      temp2<<"')";	
      mysql_query(mysql, temp2.str().c_str());
    }
      
    temp<<"("<<area.id<<", "<<key_id<<", "<<value_id<<")";
    if (++tit != tags.begin()->end())
      temp<<", ";
  }
  mysql_query(mysql, temp.str().c_str());
  
  ostringstream stack;
  for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
       it != get_stack().end(); ++it)
    stack<<it->first<<' '<<it->second<<' ';
  temp.str("");
  temp<<"insert area_origins values ("<<area.id<<", "
      <<get_rule_id()<<", "
      <<this->get_line_number()<<", '"
      <<stack.str()<<"')";
  mysql_query(mysql, temp.str().c_str());
  
  areas.insert(area);
  maps[output] = Set(nodes, ways, relations, areas);
}
