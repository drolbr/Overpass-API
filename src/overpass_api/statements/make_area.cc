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

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../osm-backend/area_updater.h"
#include "make_area.h"
#include "print.h"

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

void Make_Area_Statement::execute(Resource_Manager& rman)
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
  
  // detect pivot element
  map< string, Set >::const_iterator mit(rman.sets().find(pivot));
  if (mit == rman.sets().end())
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    stopwatch.stop(Stopwatch::NO_DISK);
    stopwatch.report(get_name());
    
    return;
  }
  pair< uint32, uint32 > pivot_pair(detect_pivot(mit->second));
  uint32 pivot_type(pivot_pair.first);
  uint32 pivot_id(pivot_pair.second);
  
  stopwatch.stop(Stopwatch::NO_DISK);
  
  if (pivot_type == 0)
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    stopwatch.report(get_name());
    
    return;
  }
  
  //formulate range query to query tags of the pivot
  set< Uint31_Index > coarse_indices;
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  if (pivot_type == NODE)
    coarse_indices.insert(mit->second.nodes.begin()->first.val() & 0xffffff00);
  else if (pivot_type == WAY)
    coarse_indices.insert(mit->second.ways.begin()->first.val() & 0xffffff00);
  else if (pivot_type == RELATION)
    coarse_indices.insert(mit->second.relations.begin()->first.val() & 0xffffff00);
  
  formulate_range_query(range_set, coarse_indices);
  
  // iterate over the result
  vector< pair< string, string > > new_tags;
  File_Properties* file_prop;
  if (pivot_type == NODE)
    file_prop = de_osm3s_file_ids::NODE_TAGS_LOCAL;
  else if (pivot_type == WAY)
    file_prop = de_osm3s_file_ids::WAY_TAGS_LOCAL;
  else if (pivot_type == RELATION)
    file_prop = de_osm3s_file_ids::RELATION_TAGS_LOCAL;
  Block_Backend< Tag_Index_Local, Uint32_Index > items_db(*file_prop, false, false);
  Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
      tag_it(items_db.range_begin
        (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
         Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  for (; !(tag_it == items_db.range_end()); ++tag_it)
  {
    if (tag_it.object().val() == pivot_id)
      new_tags.push_back(make_pair(tag_it.index().key, tag_it.index().value));
  }
  
  if (pivot_type == NODE)
  {
    stopwatch.add(Stopwatch::NODE_TAGS_LOCAL, items_db.read_count());
    stopwatch.stop(Stopwatch::NODE_TAGS_LOCAL);
  }
  else if (pivot_type == WAY)
  {
    stopwatch.add(Stopwatch::WAY_TAGS_LOCAL, items_db.read_count());
    stopwatch.stop(Stopwatch::WAY_TAGS_LOCAL);
  }
  else if (pivot_type == RELATION)
  {
    stopwatch.add(Stopwatch::RELATION_TAGS_LOCAL, items_db.read_count());
    stopwatch.stop(Stopwatch::RELATION_TAGS_LOCAL);
  }
  
  if (pivot_type == WAY)
    pivot_id += 2400000000u;
  else if (pivot_type == RELATION)
    pivot_id += 3600000000u;
  
  mit = rman.sets().find(input);
  if (mit == rman.sets().end())
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    stopwatch.stop(Stopwatch::NO_DISK);
    stopwatch.report(get_name());
    
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
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    stopwatch.stop(Stopwatch::NO_DISK);
    stopwatch.report(get_name());
    
    return;
  }
  
  add_segment_blocks(area_blocks, pivot_id);
  
  set< uint32 > used_indices;
  for (map< Uint31_Index, vector< Area_Block > >::const_iterator
      it(area_blocks.begin()); it != area_blocks.end(); ++it)
    used_indices.insert(it->first.val());
  Area_Location new_location(pivot_id, used_indices);
  new_location.tags = new_tags;
  Uint31_Index new_index(new_location.calc_index());
  
  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();

  if (new_index.val() == 0)
  {
    stopwatch.stop(Stopwatch::NO_DISK); 
    stopwatch.report(get_name());
    return;
  }
  
  Area_Updater& area_updater(rman.area_updater());
  area_updater.set_area(new_index, new_location);
  area_updater.add_blocks(area_blocks);
  stopwatch.stop(Stopwatch::NO_DISK);
  area_updater.commit(stopwatch);
  stopwatch.stop(Stopwatch::NO_DISK);
  
  areas[new_index].push_back(Area_Skeleton(new_location));
  
  stopwatch.stop(Stopwatch::NO_DISK); 
  stopwatch.report(get_name());
  rman.health_check(*this);
}
