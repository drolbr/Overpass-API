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

#include <mysql.h>

using namespace std;

//-----------------------------------------------------------------------------

const unsigned int QUERY_NODE = 1;
const unsigned int QUERY_WAY = 2;
const unsigned int QUERY_RELATION = 3;

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
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
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
  if (type == QUERY_WAY)
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
  if (type == QUERY_RELATION)
  {
    map< int, pair< string, string > > key_value_counts;
    for (vector< pair< string, string > >::const_iterator it(key_values.begin());
	 it != key_values.end(); ++it)
    {
      if (it->second == "")
      {
	ostringstream temp;
	temp<<"select count from relation_tag_counts "
	    <<"left join key_s on key_s.id = relation_tag_counts.id "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, it->first);
	temp<<"\"";
	int count(int_query(mysql, temp.str()));
	key_value_counts.insert
	    (make_pair< int, pair< string, string > >(count, *it));
      }
      else
      {
	ostringstream temp;
	temp<<"select count, spread from relation_tag_counts "
	    <<"left join key_s on key_s.id = relation_tag_counts.id "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, it->first);
	temp<<"\"";
	pair< int, int > count(intint_query(mysql, temp.str()));
	key_value_counts.insert
	    (make_pair< int, pair< string, string > >(count.first*2/(count.second+1), *it));
      }
    }
    unsigned int i(0);
    bool reordered(false);
    for (map< int, pair< string, string > >::const_iterator it(key_value_counts.begin());
	 it != key_value_counts.end(); ++it)
    {
      reordered |= (it->second != key_values[i]);
      key_values[i++] = it->second;
    }
    if (reordered)
    {
      ostringstream temp;
      temp<<"The clauses of this query have been reordered to improve performance:<br/>\n";
      for (map< int, pair< string, string > >::const_iterator it(key_value_counts.begin()); ; )
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
  set< Relation >& relations((maps[output].get_relations_handle()));
  set< Area >& areas((maps[output].get_areas_handle()));
  
  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();
  
  if (type == QUERY_NODE)
  {
    set< uint32 > tnodes;
    node_kv_to_multiint_query(key_values.front().first, key_values.front().second, tnodes);
    set< Node > tnodes2;
    multiint_to_multiNode_query(tnodes, tnodes2);
    kvs_multiNode_to_multiNode_query(++(key_values.begin()), key_values.end(), tnodes2, nodes);
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
    ostringstream temp;
    if (key_values.front().second == "")
    {
      temp<<"select relations.id from relations "
	  <<"left join relation_tags on relations.id = relation_tags.id "
	  <<"left join key_s on key_s.id = relation_tags.key_ "
	  <<"where key_s.key_ = \"";
      escape_xml(temp, key_values.front().first);
      temp<<"\" order by relations.id";
    }
    else
    {
      temp<<"select relations.id from relations "
	  <<"left join relation_tags on relations.id = relation_tags.id "
	  <<"left join key_s on key_s.id = relation_tags.key_ "
	  <<"left join value_s on value_s.id = relation_tags.value_ "
	  <<"where key_s.key_ = \"";
      escape_xml(temp, key_values.front().first);
      temp<<"\" and value_s.value_ = \"";
      escape_xml(temp, key_values.front().second);
      temp<<"\" order by relations.id";
    }
  
    set< uint32 > rels;
    rels = multiint_query(mysql, temp.str(), rels);
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      temp.str("");
      if (key_values[key_count].second == "")
      {
	temp<<"select relation_tags.id from relation_tags "
	    <<"left join key_s on key_s.id = relation_tags.key_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values[key_count].first);
	temp<<"\" and relation_tags.id in";
      }
      else
      {
	temp<<"select relation_tags.id from relation_tags "
	    <<"left join key_s on key_s.id = relation_tags.key_ "
	    <<"left join value_s on value_s.id = relation_tags.value_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values[key_count].first);
	temp<<"\" and value_s.value_ = \"";
	escape_xml(temp, key_values[key_count].second);
	temp<<"\" and relation_tags.id in";
      }
      
      set< uint32 > new_rels;
      rels = multiint_to_multiint_query
	  (mysql, temp.str(), "order by relation_tags.id", rels, new_rels);
      
      ++key_count;
    }
  
    set< Relation > relations;
    relations = multiint_to_multiRelation_query
	(mysql, "select id, ref, role from relation_node_members "
	"where id in", "order by id",
	"select id, ref, role from relation_way_members "
	"where id in", "order by id",
	"select id, ref, role from relation_relation_members "
	"where id in", "order by id", rels, relations);
    maps[output] = Set(set< Node >(), set< Way >(), relations);
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
