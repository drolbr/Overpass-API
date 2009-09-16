#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "user_interface.h"
#include "query_statement.h"
#include "area_query_statement.h"

#include <mysql.h>

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
  
  Has_Key_Value_Statement* has_kv(dynamic_cast<Has_Key_Value_Statement*>(statement));
  if (has_kv)
  {
    key_values.push_back(make_pair< string, string >
	(has_kv->get_key(), has_kv->get_value()));
    return;
  }
  Area_Query_Statement* area(dynamic_cast<Area_Query_Statement*>(statement));
  if (area)
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
  Bbox_Query_Statement* bbox(dynamic_cast<Bbox_Query_Statement*>(statement));
  if (bbox)
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

void Query_Statement::forecast(MYSQL* mysql)
{
  Set_Forecast& sf_out(declare_write_set(output));
    
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
/*  else if (type == QUERY_AREA)
  {
    if (key_value_counts.empty())
    {
      sf_out.area_count = 100*1000;
      add_sanity_error("A query with empty conditions is not allowed.");
    }
    else
      sf_out.area_count = 15;
    declare_used_time(30*1000);
  }*/
  finish_statement_forecast();
    
  display_full();
  display_state();
}

void Query_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  if (key_values.size() == 0)
    return;
  
  set< Node >& nodes(maps[output].get_nodes_handle());
  set< Way >& ways(maps[output].get_ways_handle());
  set< Relation_ >& relations((maps[output].get_relations_handle()));
  set< Area >& areas((maps[output].get_areas_handle()));
  
  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();
  
  if (type == QUERY_NODE)
  {
    if (area_restriction)
    {
      uint32 part_count(area_restriction->prepare_split(mysql));
      for (uint32 i(0); i < part_count; ++i)
      {
	set< Node > tnodes;
	area_restriction->get_nodes(mysql, tnodes, i);
	kvs_multiNode_to_multiNode_query(key_values.begin(), key_values.end(), tnodes, nodes);
      }
    }
    else if (bbox_restriction)
    {
      uint32 part_count(bbox_restriction->prepare_split(mysql));
      for (uint32 i(0); i < part_count; ++i)
      {
	set< Node > tnodes;
	bbox_restriction->get_nodes(mysql, tnodes, i);
	kvs_multiNode_to_multiNode_query(key_values.begin(), key_values.end(), tnodes, nodes);
      }
    }
    else
    {
      set< uint32 > tnodes;
      node_kv_to_multiint_query(key_values.front().first, key_values.front().second, tnodes);
      set< Node > tnodes2;
      multiint_to_multiNode_query(tnodes, tnodes2);
      kvs_multiNode_to_multiNode_query(++(key_values.begin()), key_values.end(), tnodes2, nodes);
    }
  }
  else if (type == QUERY_WAY)
  {
    set< Way_::Id > tways;
    way_kv_to_multiint_query(key_values.front().first, key_values.front().second, tways);
    set< Way_ > tways2, coord_ways;
    multiint_to_multiWay_query(tways, tways2);
    kvs_multiWay_to_multiWay_query(++(key_values.begin()), key_values.end(), tways2, coord_ways);
    for (set< Way_ >::const_iterator it(coord_ways.begin()); it != coord_ways.end(); ++it)
    {
      Way way(it->head.second);
      way.members = ((*it).data);
      ways.insert(way);
    }
  }
  else if (type == QUERY_RELATION)
  {
    set< Relation_::Id > trelations;
    relation_kv_to_multiint_query(key_values.front().first, key_values.front().second, trelations);
    set< Relation_ > trelations2;
    multiint_to_multiRelation_query(trelations, trelations2);
    kvs_multiRelation_to_multiRelation_query
	(++(key_values.begin()), key_values.end(), trelations2, relations);
  }
}

//-----------------------------------------------------------------------------

void Has_Key_Value_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["k"] = "";
  attributes["v"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  ostringstream temp;
  escape_xml(temp, attributes["k"]);
  key = temp.str();
  temp.str("");
  escape_xml(temp, attributes["v"]);
  value = temp.str();
  if (key == "")
  {
    temp.str("");
    temp<<"For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(temp.str());
  }
}

void Has_Key_Value_Statement::forecast(MYSQL* mysql)
{
  // will never be called
}
