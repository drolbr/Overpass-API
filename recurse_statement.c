#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "user_interface.h"
#include "recurse_statement.h"

#include <mysql.h>

using namespace std;

const unsigned int RECURSE_WAY_NODE = 1;
const unsigned int RECURSE_RELATION_RELATION = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;
const unsigned int RECURSE_NODE_WAY = 5;

void Recurse_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
  
  if (attributes["type"] == "way-node")
    type = RECURSE_WAY_NODE;
  else if (attributes["type"] == "relation-relation")
    type = RECURSE_RELATION_RELATION;
  else if (attributes["type"] == "relation-way")
    type = RECURSE_RELATION_WAY;
  else if (attributes["type"] == "relation-node")
    type = RECURSE_RELATION_NODE;
  else if (attributes["type"] == "node-way")
    type = RECURSE_NODE_WAY;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"way-node\", \"relation-relation\","
	<<"\"relation-way\" or \"relation-node\".";
    add_static_error(temp.str());
  }
}

void Recurse_Statement::forecast(MYSQL* mysql)
{
  if (input == output)
  {
    Set_Forecast& sf_io(declare_union_set(output));
    
    if (type == RECURSE_WAY_NODE)
    {
      sf_io.node_count += 28*sf_io.way_count;
      declare_used_time(50*sf_io.way_count);
    }
    else if (type == RECURSE_RELATION_RELATION)
    {
      declare_used_time(100*sf_io.relation_count);
      sf_io.relation_count += sf_io.relation_count;
    }
    else if (type == RECURSE_RELATION_WAY)
    {
      sf_io.way_count += 22*sf_io.relation_count;
      declare_used_time(100*sf_io.relation_count);
    }
    else if (type == RECURSE_RELATION_NODE)
    {
      sf_io.node_count += 2*sf_io.relation_count;
      declare_used_time(100*sf_io.relation_count);
    }
    else if (type == RECURSE_NODE_WAY)
    {
      sf_io.way_count += sf_io.node_count/2;
      declare_used_time(sf_io.node_count/1000); //TODO
    }
    
    finish_statement_forecast();

    display_full();
    display_state();
  }
  else
  {
    const Set_Forecast& sf_in(declare_read_set(input));
    Set_Forecast& sf_out(declare_write_set(output));
    
    if (type == RECURSE_WAY_NODE)
    {
      sf_out.node_count += 28*sf_in.way_count;
      declare_used_time(50*sf_in.way_count);
    }
    else if (type == RECURSE_RELATION_RELATION)
    {
      sf_out.relation_count += sf_in.relation_count;
      declare_used_time(100*sf_in.relation_count);
    }
    else if (type == RECURSE_RELATION_WAY)
    {
      sf_out.way_count += 22*sf_in.relation_count;
      declare_used_time(100*sf_in.relation_count);
    }
    else if (type == RECURSE_RELATION_NODE)
    {
      sf_out.node_count += 2*sf_in.relation_count;
      declare_used_time(100*sf_in.relation_count);
    }
    else if (type == RECURSE_NODE_WAY)
    {
      sf_out.way_count += sf_in.node_count/2;
      declare_used_time(sf_in.node_count/1000); //TODO
    }
    
    finish_statement_forecast();
    
    display_full();
    display_state();
  }
}

void multiWay_to_multiint_collect(const set< Way >& source, set< int >& result_set)
{
  for (set< Way >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    for(vector< int >::const_iterator it2(it->members.begin()); it2 != it->members.end(); ++it2)
      result_set.insert(*it2);
  }
}

void multiRelation_to_multiint_collect_relations(const set< Relation >& source, set< int >& result_set)
{
  for (set< Relation >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    for(set< pair< int, int > >::const_iterator it2(it->relation_members.begin()); it2 != it->relation_members.end(); ++it2)
      result_set.insert(it2->first);
  }
}

void multiRelation_to_multiint_collect_ways(const set< Relation >& source, set< int >& result_set)
{
  for (set< Relation >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    for(set< pair< int, int > >::const_iterator it2(it->way_members.begin()); it2 != it->way_members.end(); ++it2)
      result_set.insert(it2->first);
  }
}

void multiRelation_to_multiint_collect_nodes(const set< Relation >& source, set< int >& result_set)
{
  for (set< Relation >::const_iterator it(source.begin()); it != source.end(); ++it)
  {
    for(set< pair< int, int > >::const_iterator it2(it->node_members.begin()); it2 != it->node_members.end(); ++it2)
      result_set.insert(it2->first);
  }
}

void Recurse_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node >* nodes(&(maps[output].get_nodes_handle()));
  set< Way >* ways(&(maps[output].get_ways_handle()));
  set< Relation >* relations(&(maps[output].get_relations_handle()));
  set< Area >* areas(&(maps[output].get_areas_handle()));
  if (input != output)
  {
    nodes->clear();
    ways->clear();
    relations->clear();
    areas->clear();
  }
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
    return;
  
  if (type == RECURSE_WAY_NODE)
  {
    set< int > tnodes;
    multiWay_to_multiint_collect(mit->second.get_ways(), tnodes);
    multiint_to_multiNode_query(tnodes, *nodes);
  }
  else if (type == RECURSE_RELATION_RELATION)
  {
    set< int > rels;
    multiRelation_to_multiint_collect_relations(mit->second.get_relations(), rels);
    multiint_to_multiRelation_query
	(mysql, "select id, ref, role from relation_node_members "
	"where id in", "order by id",
	"select id, ref, role from relation_way_members "
	"where id in", "order by id",
	"select id, ref, role from relation_relation_members "
	"where id in", "order by id", rels, *relations);
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    set< int > tways;
    multiRelation_to_multiint_collect_ways(mit->second.get_relations(), tways);
    multiint_to_multiWay_query(tways, *ways);
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    set< int > tnodes;
    multiRelation_to_multiint_collect_nodes(mit->second.get_relations(), tnodes);
    multiint_to_multiNode_query(tnodes, *nodes);
  }
  else if (type == RECURSE_NODE_WAY)
  {
    multiNode_to_multiWay_query(mit->second.get_nodes(), *ways);
  }
}
