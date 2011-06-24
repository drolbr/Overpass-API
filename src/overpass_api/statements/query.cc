#include <algorithm>
#include <sstream>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "area_query.h"
#include "bbox_query.h"
#include "query.h"

using namespace std;

//-----------------------------------------------------------------------------

const unsigned int QUERY_NODE = 1;
const unsigned int QUERY_WAY = 2;
const unsigned int QUERY_RELATION = 3;
// const unsigned int QUERY_AREA = 4;

void Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  if (attributes["type"] == "node")
    type = QUERY_NODE;
  else if (attributes["type"] == "way")
    type = QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = QUERY_RELATION;
/*  else if (attributes["type"] == "area")
    type = QUERY_AREA;*/
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
/*    temp<<"For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\", \"relation\" or \"area\".";*/
    add_static_error(temp.str());
  }
}

void Query_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  Has_Kv_Statement* has_kv(dynamic_cast<Has_Kv_Statement*>(statement));
  if (has_kv)
  {
    key_values.push_back(make_pair< string, string >
	(has_kv->get_key(), has_kv->get_value()));
    return;
  }
  Area_Query_Statement* area(dynamic_cast<Area_Query_Statement*>(statement));
  Bbox_Query_Statement* bbox(dynamic_cast<Bbox_Query_Statement*>(statement));
  if (area != 0)
  {
    if (type != QUERY_NODE)
    {
      ostringstream temp;
      temp<<"An area-query as substatement is only allowed for queries of type \"node\".";
      add_static_error(temp.str());
      return;
    }
    if ((area_restriction != 0) || (bbox_restriction != 0))
    {
      ostringstream temp;
      temp<<"A query statement may contain at most one area-query or bbox-query "
	  <<"as substatement.";
      add_static_error(temp.str());
      return;
    }
    area_restriction = area;
    return;
  }
  else if (bbox != 0)
  {
    if (type != QUERY_NODE)
    {
      ostringstream temp;
      temp<<"A bbox-query as substatement is only allowed for queries of type \"node\".";
      add_static_error(temp.str());
      return;
    }
    if ((area_restriction != 0) || (bbox_restriction != 0))
    {
      ostringstream temp;
      temp<<"A query statement may contain at most one area-query or bbox-query "
	  <<"as substatement.";
      add_static_error(temp.str());
      return;
    }
    bbox_restriction = bbox;
    return;
  }
  else
    substatement_error(get_name(), statement);
}

void Query_Statement::forecast()
{
/*  Set_Forecast& sf_out(declare_write_set(output));
    
  if (type == QUERY_NODE)
  {
    map< uint, pair< string, string > > key_value_counts;
    for (vector< pair< string, string > >::const_iterator it(key_values.begin());
	 it != key_values.end(); ++it)
    {
      uint count(node_kv_to_count_query(it->first, it->second));
      key_value_counts.insert
	  (make_pair< uint, pair< string, string > >(count, *it));
    }
    uint i(0);
    bool reordered(false);
    for (map< uint, pair< string, string > >::const_iterator it(key_value_counts.begin());
	 it != key_value_counts.end(); ++it)
    {
      reordered |= (it->second != key_values[i]);
      key_values[i++] = it->second;
    }
    if (reordered)
    {
      ostringstream temp;
      temp<<"The clauses of this query have been reordered to improve performance:<br/>\n";
      for (map< uint32, pair< string, string > >::const_iterator it(key_value_counts.begin()); ; )
      {
	temp<<"Has_Kv \""<<it->second.first<<"\" \""<<it->second.second<<"\": "<<it->first
	    <<" results expected.";
	if (++it != key_value_counts.end())
	  temp<<"<br/>\n";
	else
	  break;
      }
      add_sanity_remark(temp.str());
    }
    
    if (key_value_counts.empty())
    {
      sf_out.node_count = 400*1000*1000;
      add_sanity_error("A query with empty conditions is not allowed.");
    }
    else
      sf_out.node_count = key_value_counts.begin()->first;
    declare_used_time(24000 + sf_out.node_count);
  }
  else if (type == QUERY_WAY)
  {
    map< uint, pair< string, string > > key_value_counts;
    for (vector< pair< string, string > >::const_iterator it(key_values.begin());
	 it != key_values.end(); ++it)
    {
      uint count(way_kv_to_count_query(it->first, it->second));
      key_value_counts.insert
	  (make_pair< uint, pair< string, string > >(count, *it));
    }
    uint i(0);
    bool reordered(false);
    for (map< uint, pair< string, string > >::const_iterator it(key_value_counts.begin());
	 it != key_value_counts.end(); ++it)
    {
      reordered |= (it->second != key_values[i]);
      key_values[i++] = it->second;
    }
    if (reordered)
    {
      ostringstream temp;
      temp<<"The clauses of this query have been reordered to improve performance:<br/>\n";
      for (map< uint32, pair< string, string > >::const_iterator it(key_value_counts.begin()); ; )
      {
	temp<<"Has_Kv \""<<it->second.first<<"\" \""<<it->second.second<<"\": "<<it->first
	    <<" results expected.";
	if (++it != key_value_counts.end())
	  temp<<"<br/>\n";
	else
	  break;
      }
      add_sanity_remark(temp.str());
    }
    
    if (key_value_counts.empty())
    {
      sf_out.way_count = 30*1000*1000;
      add_sanity_error("A query with empty conditions is not allowed.");
    }
    else
      sf_out.way_count = key_value_counts.begin()->first;
    
    declare_used_time(90000 + sf_out.way_count);
  }
  else if (type == QUERY_RELATION)
  {
    map< uint, pair< string, string > > key_value_counts;
    for (vector< pair< string, string > >::const_iterator it(key_values.begin());
	 it != key_values.end(); ++it)
    {
      uint count(relation_kv_to_count_query(it->first, it->second));
      key_value_counts.insert
	  (make_pair< uint, pair< string, string > >(count, *it));
    }
    uint i(0);
    bool reordered(false);
    for (map< uint, pair< string, string > >::const_iterator it(key_value_counts.begin());
	 it != key_value_counts.end(); ++it)
    {
      reordered |= (it->second != key_values[i]);
      key_values[i++] = it->second;
    }
    if (reordered)
    {
      ostringstream temp;
      temp<<"The clauses of this query have been reordered to improve performance:<br/>\n";
      for (map< uint32, pair< string, string > >::const_iterator it(key_value_counts.begin()); ; )
      {
	temp<<"Has_Kv \""<<it->second.first<<"\" \""<<it->second.second<<"\": "<<it->first
	    <<" results expected.";
	if (++it != key_value_counts.end())
	  temp<<"<br/>\n";
	else
	  break;
      }
      add_sanity_remark(temp.str());
    }
    
    if (key_value_counts.empty())
    {
      sf_out.relation_count = 100*1000;
      add_sanity_error("A query with empty conditions is not allowed.");
    }
    else
      sf_out.relation_count = key_value_counts.begin()->first;
    declare_used_time(100 + sf_out.relation_count);
  }
  else if (type == QUERY_AREA)
  {
    if (key_value_counts.empty())
    {
      sf_out.area_count = 100*1000;
      add_sanity_error("A query with empty conditions is not allowed.");
    }
    else
      sf_out.area_count = 15;
    declare_used_time(30*1000);
  }
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

vector< uint32 >* Query_Statement::collect_ids
  (const vector< pair< string, string > >& key_values,
   const File_Properties& file_prop, uint32 stopwatch_account,
   Resource_Manager& rman)
{
  if (key_values.empty())
    return new vector< uint32 >();
 
  stopwatch.stop(Stopwatch::NO_DISK);
  Block_Backend< Tag_Index_Global, Uint32_Index > tags_db
      (rman.get_transaction().data_index(&file_prop));
  
  vector< uint32 >* new_ids(new vector< uint32 >());
  vector< pair< string, string > >::const_iterator it = key_values.begin();
  if (it->second != "")
  {
    set< Tag_Index_Global > tag_req;
    Tag_Index_Global idx;
    idx.key = it->first;
    idx.value = it->second;
    tag_req.insert(idx);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
        it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
        !(it2 == tags_db.discrete_end()); ++it2)
      new_ids->push_back(it2.object().val());
  }
  else
  {
    set< pair< Tag_Index_Global, Tag_Index_Global > > range_req;
    pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
    idx_pair.first.key = it->first;
    idx_pair.first.value = "";
    idx_pair.second.key = it->first + (char)0;
    idx_pair.second.value = "";
    range_req.insert(idx_pair);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
        it2(tags_db.range_begin
          (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
        !(it2 == tags_db.range_end()); ++it2)
      new_ids->push_back(it2.object().val());
  }
  stopwatch.add(stopwatch_account, tags_db.read_count());
  stopwatch.stop(stopwatch_account);
  sort(new_ids->begin(), new_ids->end());
  stopwatch.stop(Stopwatch::NO_DISK);
  ++it;
  
  for (; it != key_values.end(); ++it)
  {
    vector< uint32 >* old_ids = new_ids;
    new_ids = new vector< uint32 >();
    if (it->second != "")
    {
      set< Tag_Index_Global > tag_req;
      Tag_Index_Global idx;
      idx.key = it->first;
      idx.value = it->second;
      tag_req.insert(idx);
      for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
          it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
          !(it2 == tags_db.discrete_end()); ++it2)
      {
        if (binary_search(old_ids->begin(), old_ids->end(), it2.object().val()))
	  new_ids->push_back(it2.object().val());
      }
    }
    else
    {
      set< pair< Tag_Index_Global, Tag_Index_Global > > range_req;
      pair< Tag_Index_Global, Tag_Index_Global > idx_pair;
      idx_pair.first.key = it->first;
      idx_pair.first.value = "";
      idx_pair.second.key = it->first + (char)0;
      idx_pair.second.value = "";
      range_req.insert(idx_pair);
      for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
	  it2(tags_db.range_begin
	  (Default_Range_Iterator< Tag_Index_Global >(range_req.begin()),
	   Default_Range_Iterator< Tag_Index_Global >(range_req.end())));
	  !(it2 == tags_db.range_end()); ++it2)
      {
	if (binary_search(old_ids->begin(), old_ids->end(), it2.object().val()))
	  new_ids->push_back(it2.object().val());
      }
    }
    delete(old_ids);
    stopwatch.add(stopwatch_account, tags_db.read_count());
    stopwatch.stop(stopwatch_account);
    sort(new_ids->begin(), new_ids->end());
    stopwatch.stop(Stopwatch::NO_DISK);
    
    rman.health_check(*this);
  }
  
  return new_ids;
}

void Query_Statement::execute(Resource_Manager& rman)
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
  
  if (key_values.empty())
    return;
  
  set< Tag_Index_Global > tag_req;
  Tag_Index_Global idx;
  idx.key = key_values.begin()->first;
  idx.value = key_values.begin()->second;
  tag_req.insert(idx);
  
  if (type == QUERY_NODE)
  {
    vector< uint32 >* ids(collect_ids
        (key_values, *osm_base_settings().NODE_TAGS_GLOBAL,
         Stopwatch::NODE_TAGS_GLOBAL, rman));

    set< pair< Uint32_Index, Uint32_Index > > nodes_req;
    set< Uint31_Index > area_blocks_req;	
    set< Uint32_Index > obj_req;
    set< pair< Uint32_Index, Uint32_Index > > range_req;
    if (area_restriction != 0)
    {
      stopwatch.stop(Stopwatch::NO_DISK);
      area_restriction->get_ranges(nodes_req, area_blocks_req, rman);
      stopwatch.stop(Stopwatch::AREAS);
      for (set< pair< Uint32_Index, Uint32_Index > >::const_iterator
	  it(nodes_req.begin()); it != nodes_req.end(); ++it)
      {
	for (uint32 i(it->first.val()); i < it->second.val(); ++i)
	  obj_req.insert(Uint32_Index(i));
      }
    }
    else if (bbox_restriction != 0)
    {
      vector< pair< uint32, uint32 > >* ranges(bbox_restriction->calc_ranges());
      for (vector< pair< uint32, uint32 > >::const_iterator
	it(ranges->begin()); it != ranges->end(); ++it)
      {
	pair< Uint32_Index, Uint32_Index > range
	    (make_pair(Uint32_Index(it->first), Uint32_Index(it->second)));
	range_req.insert(range);
      }
      delete(ranges);
    }
    else
    {
      stopwatch.stop(Stopwatch::NO_DISK);
      Random_File< Uint32_Index > random
          (rman.get_transaction().random_index(osm_base_settings().NODES));
      for (vector< uint32 >::const_iterator it(ids->begin());
          it != ids->end(); ++it)
	obj_req.insert(random.get(*it));
      stopwatch.stop(Stopwatch::NODES_MAP);
    }
    
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    
    stopwatch.stop(Stopwatch::NO_DISK);
    if (area_restriction != 0)
    {
      area_restriction->collect_nodes
          (nodes_req, area_blocks_req, ids, nodes, stopwatch, rman);
      stopwatch.stop(Stopwatch::NO_DISK);
    }
    else if (bbox_restriction != 0)
    {
      uint nodes_count = 0;
      Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
	  (rman.get_transaction().data_index(osm_base_settings().NODES));
      for (Block_Backend< Uint32_Index, Node_Skeleton >::Range_Iterator
          it(nodes_db.range_begin
             (Default_Range_Iterator< Uint32_Index >(range_req.begin()),
	      Default_Range_Iterator< Uint32_Index >(range_req.end())));
          !(it == nodes_db.range_end()); ++it)
      {
	if (++nodes_count >= 64*1024)
	{
	  nodes_count = 0;
	  rman.health_check(*this);
	}
	
	if (binary_search(ids->begin(), ids->end(), it.object().id))
	{
	  double lat(Node::lat(it.index().val(), it.object().ll_lower));
	  double lon(Node::lon(it.index().val(), it.object().ll_lower));
	  if ((lat >= bbox_restriction->get_south()) &&
	      (lat <= bbox_restriction->get_north()) &&
	      (((lon >= bbox_restriction->get_west()) &&
	       (lon <= bbox_restriction->get_east())) ||
	       ((bbox_restriction->get_east() < bbox_restriction->get_west()) &&
	        ((lon >= bbox_restriction->get_west()) ||
		 (lon <= bbox_restriction->get_east())))))
	    nodes[it.index()].push_back(it.object());
	}
      }
      stopwatch.add(Stopwatch::NODES, nodes_db.read_count());
      stopwatch.stop(Stopwatch::NODES);
    }
    else
    {
      uint nodes_count = 0;
      Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
          (rman.get_transaction().data_index(osm_base_settings().NODES));
      for (Block_Backend< Uint32_Index, Node_Skeleton >::Discrete_Iterator
	  it(nodes_db.discrete_begin(obj_req.begin(), obj_req.end()));
          !(it == nodes_db.discrete_end()); ++it)
      {
	if (++nodes_count >= 64*1024)
	{
	  nodes_count = 0;
	  rman.health_check(*this);
	}
	
	if (binary_search(ids->begin(), ids->end(), it.object().id))
	  nodes[it.index()].push_back(it.object());
      }    
      stopwatch.add(Stopwatch::NODES, nodes_db.read_count());
      stopwatch.stop(Stopwatch::NODES);
    }
  }
  else if (type == QUERY_WAY)
  {
    vector< uint32 >* ids(collect_ids
        (key_values, *osm_base_settings().WAY_TAGS_GLOBAL,
	 Stopwatch::WAY_TAGS_GLOBAL, rman));
	 
    set< Uint31_Index > obj_req;
    {
      stopwatch.stop(Stopwatch::NO_DISK);
      Random_File< Uint31_Index > random
          (rman.get_transaction().random_index(osm_base_settings().WAYS));
      for (vector< uint32 >::const_iterator it(ids->begin());
          it != ids->end(); ++it)
        obj_req.insert(random.get(*it));
      stopwatch.stop(Stopwatch::WAYS_MAP);
    }
    
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    
    stopwatch.stop(Stopwatch::NO_DISK);
    uint ways_count = 0;
    Block_Backend< Uint31_Index, Way_Skeleton > ways_db
        (rman.get_transaction().data_index(osm_base_settings().WAYS));
    for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
        it(ways_db.discrete_begin(obj_req.begin(), obj_req.end()));
        !(it == ways_db.discrete_end()); ++it)
    {
      if (++ways_count >= 64*1024)
      {
	ways_count = 0;
	rman.health_check(*this);
      }
      
      if (binary_search(ids->begin(), ids->end(), it.object().id))
	ways[it.index()].push_back(it.object());
    }    
    stopwatch.add(Stopwatch::WAYS, ways_db.read_count());
    stopwatch.stop(Stopwatch::WAYS);
  }
  else if (type == QUERY_RELATION)
  {
    vector< uint32 >* ids(collect_ids
        (key_values, *osm_base_settings().RELATION_TAGS_GLOBAL,
	 Stopwatch::RELATION_TAGS_GLOBAL, rman));
    
    set< Uint31_Index > obj_req;
    {
      stopwatch.stop(Stopwatch::NO_DISK);
      Random_File< Uint31_Index > random
          (rman.get_transaction().random_index(osm_base_settings().RELATIONS));
      for (vector< uint32 >::const_iterator it(ids->begin());
          it != ids->end(); ++it)
      obj_req.insert(random.get(*it));
      stopwatch.stop(Stopwatch::RELATIONS_MAP);
    }
    
    nodes.clear();
    ways.clear();
    relations.clear();
    areas.clear();
    
    stopwatch.stop(Stopwatch::NO_DISK);
    uint relations_count = 0;
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
        (rman.get_transaction().data_index(osm_base_settings().RELATIONS));
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Discrete_Iterator
        it(relations_db.discrete_begin(obj_req.begin(), obj_req.end()));
        !(it == relations_db.discrete_end()); ++it)
    {
      if (++relations_count >= 64*1024)
      {
	relations_count = 0;
	rman.health_check(*this);
      }
      
      if (binary_search(ids->begin(), ids->end(), it.object().id))
	relations[it.index()].push_back(it.object());
    }    
    stopwatch.add(Stopwatch::RELATIONS, relations_db.read_count());
    stopwatch.stop(Stopwatch::RELATIONS);
  }
  
  stopwatch.report(get_name());  
  rman.health_check(*this);
}

//-----------------------------------------------------------------------------

void Has_Kv_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["v"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  key = attributes["k"];
  value = attributes["v"];
  if (key == "")
  {
    ostringstream temp("");
    temp<<"For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
}

void Has_Kv_Statement::forecast()
{
  // will never be called
}
