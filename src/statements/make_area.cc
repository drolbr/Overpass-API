#include <algorithm>
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
#include "../backend/random_file.h"
#include "make_area.h"

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
  pivot = attributes["pivot"];
}

void Make_Area_Statement::forecast()
{
/*  Set_Forecast sf_in(declare_read_set(input));
  declare_read_set(tags);
  Set_Forecast& sf_out(declare_write_set(output));
    
  sf_out.area_count = 1;
  declare_used_time(100 + sf_in.node_count + sf_in.way_count);
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

pair< uint32, uint32 > Make_Area_Statement::detect_pivot(const Set& pivot)
{
  uint32 pivot_id(0), pivot_type(0);
  map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      nit(pivot.nodes.begin());
  while ((pivot_id == 0) && (nit != pivot.nodes.end()))
  {
    if (nit->second.size() > 0)
    {
      pivot_id = nit->second.front().id;
      pivot_type = NODE;
    }
    ++nit;
  }
  map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      wit(pivot.ways.begin());
  while ((pivot_id == 0) && (wit != pivot.ways.end()))
  {
    if (wit->second.size() > 0)
    {
      pivot_id = wit->second.front().id;
      pivot_type = WAY;
    }
    ++wit;
  }
  map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      rit(pivot.relations.begin());
  while ((pivot_id == 0) && (rit != pivot.relations.end()))
  {
    if (rit->second.size() > 0)
    {
      pivot_id = rit->second.front().id;
      pivot_type = RELATION;
    }
    ++rit;
  }
  
  return make_pair< uint32, uint32 >(pivot_type, pivot_id);
}

uint32 Make_Area_Statement::check_node_parity(const Set& pivot)
{
  set< uint32 > node_parity_control;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(pivot.ways.begin()); it != pivot.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      if (it2->nds.size() < 2)
	continue;
      pair< set< uint32 >::iterator, bool > npp(node_parity_control.insert
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
  return 0;
}

pair< uint32, uint32 > Make_Area_Statement::create_area_blocks
    (map< Uint31_Index, vector< Area_Block > >& areas,
     uint32 id, const Set& pivot)
{
  vector< Node > nodes;
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
    it(pivot.nodes.begin()); it != pivot.nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      nodes.push_back(Node(it2->id, it->first.val(), it2->ll_lower));
  }
  sort(nodes.begin(), nodes.end(), Node_Comparator_By_Id());
  
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(pivot.ways.begin()); it != pivot.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (it2->nds.size() < 2)
	continue;
      uint32 cur_idx(0);
      vector< uint64 > cur_polyline;
      for (vector< uint32 >::const_iterator it3(it2->nds.begin());
          it3 != it2->nds.end(); ++it3)
      {
	Node* node(binary_search_for_id(nodes, *it3));
	if (node == 0)
	  return make_pair< uint32, uint32 >(*it3, it2->id);
	if ((node->ll_upper_ & 0xffffff00) != cur_idx)
	{
	  if (cur_idx != 0)
	  {
	    areas[cur_idx].push_back(Area_Block(id, cur_polyline));
	    
	    vector< Aligned_Segment > aligned_segments;
	    Area::calc_aligned_segments
	        (aligned_segments, cur_polyline.back(),
		 ((uint64)node->ll_upper_<<32) | node->ll_lower_);
	    cur_polyline.clear();
	    for (vector< Aligned_Segment >::const_iterator
	        it(aligned_segments.begin()); it != aligned_segments.end();
	        ++it)
	    {
	      cur_polyline.push_back(((uint64)it->ll_upper_<<32)
	        | it->ll_lower_a);
	      cur_polyline.push_back(((uint64)it->ll_upper_<<32)
	        | it->ll_lower_b);
	      areas[it->ll_upper_].push_back(Area_Block(id, cur_polyline));
	      cur_polyline.clear();
	    }
	  }
	  cur_idx = (node->ll_upper_ & 0xffffff00);
	}
	cur_polyline.push_back(((uint64)node->ll_upper_<<32) | node->ll_lower_);
      }
      if (cur_idx != 0)
        areas[cur_idx].push_back(Area_Block(id, cur_polyline));
    }
  }
  return make_pair< uint32, uint32 >(0, 0);
}

uint32 Make_Area_Statement::shifted_lat(uint32 ll_index, uint64 coord)
{
  uint32 lat(0);
  coord |= (((uint64)ll_index)<<32);
  for (uint32 i(0); i < 16; i+=1)
  {
    lat |= (((uint64)0x1<<(31-2*i))&(coord>>32))<<i;
    lat |= (((uint64)0x1<<(31-2*i))&coord)>>(16-i);
  }
  return lat;
}

int32 Make_Area_Statement::lon_(uint32 ll_index, uint64 coord)
{
  int32 lon(0);
  coord |= (((uint64)ll_index)<<32);
  for (uint32 i(0); i < 16; i+=1)
  {
    lon |= (((uint64)0x1<<(30-2*i))&(coord>>32))<<(i+1);
    lon |= (((uint64)0x1<<(30-2*i))&coord)>>(15-i);
  }
  return lon;
}

void Make_Area_Statement::add_segment_blocks
    (map< Uint31_Index, vector< Area_Block > >& area_blocks, uint32 id)
{
  /* We use that more northern segments always have bigger indices.
    Thus we can collect each block's end points and add them, if they do not
    cancel out, to the next block further northern.*/
  for (map< Uint31_Index, vector< Area_Block > >::const_iterator
    it(area_blocks.begin()); it != area_blocks.end(); ++it)
  {
    set< int32 > lons;
    
    for (vector< Area_Block >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      if (it2->coors.empty())
	continue;
      
      const uint64& ll_front(it2->coors.front());
      int32 lon_front(lon_(ll_front>>32, ll_front & 0xffffffffull));
      uint32 lat_front(shifted_lat(ll_front>>32, ll_front & 0xffffffffull));
      const uint64& ll_back(it2->coors.back());
      int32 lon_back(lon_(ll_back>>32, ll_back & 0xffffffffull));
      uint32 lat_back(shifted_lat(ll_back>>32, ll_back & 0xffffffffull));
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
    
    uint32 current_idx(it->first.val());
    
    // calc lat
    uint32 lat(shifted_lat(it->first.val(), 0) + 16*65536);
    int32 lon(lon_(it->first.val(), 0));
    uint32 northern_ll_upper(Node::ll_upper(lat, lon));
    // insert lons
    vector< Area_Block >& northern_block(area_blocks[northern_ll_upper]);
    for (set< int32 >::const_iterator it2(lons.begin()); it2 != lons.end(); ++it2)
    {
      int32 from(*it2);
      ++it2;
      int32 to(*it2);
      vector< uint64 > coors;
      coors.push_back
          ((((uint64)Node::ll_upper(lat, from))<<32) | Node::ll_lower(lat, from));
      coors.push_back
          ((((uint64)Node::ll_upper(lat, to))<<32) | Node::ll_lower(lat, to));
      Area_Block new_block(id, coors);
      northern_block.push_back(new_block);
    }
    
    it = area_blocks.find(Uint31_Index(current_idx));
  }
}

void Make_Area_Statement::execute(map< string, Set >& maps)
{
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(maps[output].nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways(maps[output].ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(maps[output].relations);
  
  // detect pivot element
  map< string, Set >::const_iterator mit(maps.find(pivot));
  if (mit == maps.end())
    return;
  pair< uint32, uint32 > pivot_pair(detect_pivot(mit->second));
  uint32 pivot_type(pivot_pair.first);
  uint32 pivot_id(pivot_pair.second);
  
  if (pivot_type == 0)
    return;
  
  if (pivot_type == WAY)
    pivot_id += 2400000000u;
  else if (pivot_type == RELATION)
    pivot_id += 3600000000u;
  
  mit = maps.find(input);
  if (mit == maps.end())
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    //areas.clear();
    
    return;
  }
  
  // check node parity
  uint32 odd_id(check_node_parity(mit->second));
  if (odd_id != 0)
  {
    ostringstream temp;
    temp<<"make-area: Node "<<odd_id
        <<" is contained in an odd number of ways.\n";
    runtime_remark(temp.str());
  }
  
  // create area blocks
  map< Uint31_Index, vector< Area_Block > > area_blocks;
  pair< uint32, uint32 > odd_pair
    (create_area_blocks(area_blocks, pivot_id, mit->second));
  if (odd_pair.first != 0)
  {
    ostringstream temp;
    temp<<"make-area: Node "<<odd_pair.first
        <<" referred by way "<<odd_pair.second
        <<" is not contained in set \""<<input<<"\".\n";
    runtime_remark(temp.str());
  }
  
  if ((odd_id != 0) || (odd_pair.first != 0))
    return;
  
  add_segment_blocks(area_blocks, pivot_id);
  
/*  cout<<pivot_id<<": ";*/
  set< uint32 > used_indices;
  for (map< Uint31_Index, vector< Area_Block > >::const_iterator
      it(area_blocks.begin()); it != area_blocks.end(); ++it)
  {
    used_indices.insert(it->first.val());
/*    cout<<it->first.val()<<' ';*/
  }
/*  cout<<'\n';*/
  Area_Location new_location(pivot_id, used_indices);
  Uint31_Index new_index(new_location.calc_index());
  
  if (new_index.val() == 0)
    return;

  map< Uint31_Index, set< Area_Location > > location_to_delete;
  map< Uint31_Index, set< Area_Location > > location_to_insert;
  set< Uint31_Index > blocks_req;
  
  Block_Backend< Uint31_Index, Area_Location > area_locations_db
      (*de_osm3s_file_ids::AREAS, true);
  for (Block_Backend< Uint31_Index, Area_Location >::Flat_Iterator
      it(area_locations_db.flat_begin());
      !(it == area_locations_db.flat_end()); ++it)
  {
    if (it.object().id == pivot_id)
    {
/*      cout<<"d "<<pivot_id;*/
      for (set< uint32 >::const_iterator it2(it.object().used_indices.begin());
          it2 != it.object().used_indices.end(); ++it2)
      {
/*	cout<<' '<<hex<<*it2<<dec;*/
        blocks_req.insert(*it2);
      }
/*      cout<<'\n';*/
      location_to_delete[it.index()].insert(it.object());
    }
  }
  location_to_insert[new_index].insert(new_location);
  area_locations_db.update(location_to_delete, location_to_insert);
  
  map< Uint31_Index, set< Area_Block > > db_to_delete;
  map< Uint31_Index, set< Area_Block > > db_to_insert;

  Block_Backend< Uint31_Index, Area_Block > area_blocks_db
      (*de_osm3s_file_ids::AREA_BLOCKS, true);
  
  for (Block_Backend< Uint31_Index, Area_Block >::Discrete_Iterator
      it(area_blocks_db.discrete_begin(blocks_req.begin(), blocks_req.end()));
      !(it == area_blocks_db.discrete_end()); ++it)
  {
    if (it.object().id == pivot_id)
      db_to_delete[it.index()].insert(it.object());
  }
  
  for (map< Uint31_Index, vector< Area_Block > >::const_iterator
      it(area_blocks.begin()); it != area_blocks.end(); ++it)
  {
    for (vector< Area_Block >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      db_to_insert[it->first].insert(*it2);
  }
  
  area_blocks_db.update(db_to_delete, db_to_insert);
  
  /*  set< Node > nodes;
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
  uint32 previous_area(uint32_query(mysql, temp.str()));
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
  maps[output] = Set(nodes, ways, relations, areas);*/
}
