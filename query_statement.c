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
    map< int, pair< string, string > > key_value_counts;
    for (vector< pair< string, string > >::const_iterator it(key_values.begin());
	 it != key_values.end(); ++it)
    {
      if (it->second == "")
      {
	ostringstream temp;
	temp<<"select count from node_tag_counts "
	    <<"left join key_s on key_s.id = node_tag_counts.id "
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
	temp<<"select count, spread from node_tag_counts "
	    <<"left join key_s on key_s.id = node_tag_counts.id "
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
      sf_out.node_count = 400*1000*1000;
      add_sanity_error("A query with empty conditions is not allowed.");
    }
    else
      sf_out.node_count = key_value_counts.begin()->first;
    declare_used_time(24000 + sf_out.node_count);
  }
  if (type == QUERY_WAY)
  {
    map< int, pair< string, string > > key_value_counts;
    for (vector< pair< string, string > >::const_iterator it(key_values.begin());
	 it != key_values.end(); ++it)
    {
      if (it->second == "")
      {
	ostringstream temp;
	temp<<"select count from way_tag_counts "
	    <<"left join key_s on key_s.id = way_tag_counts.id "
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
	temp<<"select count, spread from way_tag_counts "
	    <<"left join key_s on key_s.id = way_tag_counts.id "
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
  
  if (type == QUERY_NODE)
  {
    if (key_values.size() == 1)
    {
      ostringstream temp;
      temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	  <<"left join node_tags on nodes.id = node_tags.id ";
      if (key_values.front().second == "")
      {
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values.front().first);
	temp<<"\" "
	    <<"order by node_tags.id";
      }
      else
      {
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values.front().first);
	temp<<"\" and value_s.value_ = \"";
	escape_xml(temp, key_values.front().second);
	temp<<"\" order by node_tags.id";
      }
      
      set< Node > nodes;
      maps[output] = Set(multiNode_query(mysql, temp.str(), nodes),
			 set< Way >(), set< Relation >());
    }
    else
    {
      ostringstream temp;
      temp<<"select node_tags.id from node_tags ";
      if (key_values.front().second == "")
      {
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values.front().first);
	temp<<"\" order by node_tags.id";
      }
      else
      {
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values.front().first);
	temp<<"\" and value_s.value_ = \"";
	escape_xml(temp, key_values.front().second);
	temp<<"\" order by node_tags.id";
      }
    
      set< int > tnodes;
      tnodes = multiint_query(mysql, temp.str(), tnodes);
    
      unsigned int key_count(1);
      while (key_count < key_values.size()-1)
      {
	temp.str("");
	if (key_values[key_count].second == "")
	{
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"where key_s.key_ = \"";
	  escape_xml(temp, key_values[key_count].first);
	  temp<<"\" and node_tags.id in";
	}
	else
	{
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"left join value_s on value_s.id = node_tags.value_ "
	      <<"where key_s.key_ = \"";
	  escape_xml(temp, key_values[key_count].first);
	  temp<<"\" and value_s.value_ = \"";
	  escape_xml(temp, key_values[key_count].second);
	  temp<<"\" and node_tags.id in";
	}
      
	set< int > new_nodes;
	tnodes = multiint_to_multiint_query
	    (mysql, temp.str(), "order by node_tags.id", tnodes, new_nodes);
      
	++key_count;
      }
    
      temp.str("");
      if (key_values[key_count].second == "")
      {
	temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	    <<"left join node_tags on nodes.id = node_tags.id "
	    <<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values[key_count].first);
	temp<<"\" and node_tags.id in";
      }
      else
      {
	temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	    <<"left join node_tags on nodes.id = node_tags.id "
	    <<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values[key_count].first);
	temp<<"\" and value_s.value_ = \"";
	escape_xml(temp, key_values[key_count].second);
	temp<<"\" and node_tags.id in";
      }
      
      set< Node > nodes;
      maps[output] = Set(multiint_to_multiNode_query
	  (mysql, temp.str(), "order by nodes.id", tnodes, nodes),
	  set< Way >(), set< Relation >());
    }
  }
  else if (type == QUERY_WAY)
  {
    ostringstream temp;
    if (key_values.front().second == "")
    {
      temp<<"select ways.id from ways "
	  <<"left join way_tags on ways.id = way_tags.id "
	  <<"left join key_s on key_s.id = way_tags.key_ "
	  <<"where key_s.key_ = \"";
      escape_xml(temp, key_values.front().first);
      temp<<"\" order by ways.id";
    }
    else
    {
      temp<<"select ways.id from ways "
	  <<"left join way_tags on ways.id = way_tags.id "
	  <<"left join key_s on key_s.id = way_tags.key_ "
	  <<"left join value_s on value_s.id = way_tags.value_ "
	  <<"where key_s.key_ = \"";
      escape_xml(temp, key_values.front().first);
      temp<<"\" and value_s.value_ = \"";
      escape_xml(temp, key_values.front().second);
      temp<<"\" order by ways.id";
    }
  
    set< int > tways;
    tways = multiint_query(mysql, temp.str(), tways);
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      temp.str("");
      if (key_values[key_count].second == "")
      {
	temp<<"select way_tags.id from way_tags "
	    <<"left join key_s on key_s.id = way_tags.key_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values[key_count].first);
	temp<<"\" and way_tags.id in";
      }
      else
      {
	temp<<"select way_tags.id from way_tags "
	    <<"left join key_s on key_s.id = way_tags.key_ "
	    <<"left join value_s on value_s.id = way_tags.value_ "
	    <<"where key_s.key_ = \"";
	escape_xml(temp, key_values[key_count].first);
	temp<<"\" and value_s.value_ = \"";
	escape_xml(temp, key_values[key_count].second);
	temp<<"\" and way_tags.id in";
      }
      
      set< int > new_ways;
      tways = multiint_to_multiint_query(mysql, temp.str(), "order by way_tags.id", tways, new_ways);
      
      ++key_count;
    }
    
    set< Way > ways;
    ways = multiint_to_multiWay_query
	(mysql, "select id, count, ref from way_members "
	"where id in", "order by id", tways, ways);
    maps[output] = Set(set< Node >(), ways, set< Relation >());
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
  
    set< int > rels;
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
      
      set< int > new_rels;
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
