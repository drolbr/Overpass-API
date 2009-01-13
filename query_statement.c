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
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"type\" of the element \"query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
    add_static_error(Error(temp.str(), current_line_number()));
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
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	    <<"order by node_tags.id";
      else
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	    <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	    <<"order by node_tags.id";
      
      set< Node > nodes;
      maps[output] = Set(multiNode_query(mysql, temp.str(), nodes),
			 set< Way >(), set< Relation >());
    }
    else
    {
      ostringstream temp;
      temp<<"select node_tags.id from node_tags ";
      if (key_values.front().second == "")
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	    <<"order by node_tags.id";
      else
	temp<<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	    <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	    <<"order by node_tags.id";
    
      set< int > tnodes;
      tnodes = multiint_query(mysql, temp.str(), tnodes);
    
      unsigned int key_count(1);
      while (key_count < key_values.size()-1)
      {
	temp.str("");
	if (key_values[key_count].second == "")
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and node_tags.id in";
	else
	  temp<<"select node_tags.id from node_tags "
	      <<"left join key_s on key_s.id = node_tags.key_ "
	      <<"left join value_s on value_s.id = node_tags.value_ "
	      <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	      <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	      <<"and node_tags.id in";
      
	set< int > new_nodes;
	tnodes = multiint_to_multiint_query
	    (mysql, temp.str(), "order by node_tags.id", tnodes, new_nodes);
      
	++key_count;
      }
    
      temp.str("");
      if (key_values[key_count].second == "")
	temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	    <<"left join node_tags on nodes.id = node_tags.id "
	    <<"left join key_s on key_s.id = node_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and node_tags.id in";
      else
	temp<<"select nodes.id, nodes.lat, nodes.lon from nodes "
	    <<"left join node_tags on nodes.id = node_tags.id "
	    <<"left join key_s on key_s.id = node_tags.key_ "
	    <<"left join value_s on value_s.id = node_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	    <<"and node_tags.id in";
      
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
      temp<<"select ways.id from ways "
	  <<"left join way_tags on ways.id = way_tags.id "
	  <<"left join key_s on key_s.id = way_tags.key_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"order by ways.id";
    else
      temp<<"select ways.id from ways "
	  <<"left join way_tags on ways.id = way_tags.id "
	  <<"left join key_s on key_s.id = way_tags.key_ "
	  <<"left join value_s on value_s.id = way_tags.value_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	  <<"order by ways.id";
  
    set< int > tways;
    tways = multiint_query(mysql, temp.str(), tways);
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      temp.str("");
      if (key_values[key_count].second == "")
	temp<<"select way_tags.id from way_tags "
	    <<"left join key_s on key_s.id = way_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and way_tags.id in";
      else
	temp<<"select way_tags.id from way_tags "
	    <<"left join key_s on key_s.id = way_tags.key_ "
	    <<"left join value_s on value_s.id = way_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	    <<"and way_tags.id in";
      
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
      temp<<"select relations.id from relations "
	  <<"left join relation_tags on relations.id = relation_tags.id "
	  <<"left join key_s on key_s.id = relation_tags.key_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"order by relations.id";
    else
      temp<<"select relations.id from relations "
	  <<"left join relation_tags on relations.id = relation_tags.id "
	  <<"left join key_s on key_s.id = relation_tags.key_ "
	  <<"left join value_s on value_s.id = relation_tags.value_ "
	  <<"where key_s.key_ = \""<<key_values.front().first<<"\" "
	  <<"and value_s.value_ = \""<<key_values.front().second<<"\" "
	  <<"order by relations.id";
  
    set< int > rels;
    rels = multiint_query(mysql, temp.str(), rels);
    
    unsigned int key_count(1);
    while (key_count < key_values.size())
    {
      temp.str("");
      if (key_values[key_count].second == "")
	temp<<"select relation_tags.id from relation_tags "
	    <<"left join key_s on key_s.id = relation_tags.key_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and relation_tags.id in";
      else
	temp<<"select relation_tags.id from relation_tags "
	    <<"left join key_s on key_s.id = relation_tags.key_ "
	    <<"left join value_s on value_s.id = relation_tags.value_ "
	    <<"where key_s.key_ = \""<<key_values[key_count].first<<"\" "
	    <<"and value_s.value_ = \""<<key_values[key_count].second<<"\" "
	    <<"and relation_tags.id in";
      
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
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"k\" of the element \"has-kv\""
	<<" the only allowed values are non-empty strings.";
    add_static_error(Error(temp.str(), current_line_number()));
  }
}

void Has_Key_Value_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  substatement_error(get_name(), statement);
}
