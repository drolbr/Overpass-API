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
#include "recurse_statement.h"

#include <mysql.h>

using namespace std;

const unsigned int RECURSE_WAY_NODE = 1;
const unsigned int RECURSE_RELATION_RELATION = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;

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
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"way-node\", \"relation-relation\","
	<<"\"relation-way\" or \"relation-node\".";
    add_static_error(Error(temp.str(), current_line_number()));
  }
}

void Recurse_Statement::add_statement(Statement* statement)
{
  substatement_error(get_name(), statement);
}

void Recurse_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation > relations;
  set< Area > areas;
  if (input == output)
  {
    nodes = maps[output].get_nodes();
    ways = maps[output].get_ways();
    relations = maps[output].get_relations();
    areas = maps[output].get_areas();
  }
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
  {
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  
  if (type == RECURSE_WAY_NODE)
  {
    set< int > tnodes;
    tnodes = multiWay_to_multiint_query
	(mysql, "select way_members.ref from ways "
	"left join way_members on way_members.id = ways.id "
	"where ways.id in", "order by ways.id", mit->second.get_ways(), tnodes);
    
    nodes = multiint_to_multiNode_query
	(mysql, "select id, lat, lon from nodes where id in", "order by id", tnodes, nodes);
  }
  else if (type == RECURSE_RELATION_RELATION)
  {
    set< int > rels;
    rels = multiRelation_to_multiint_query
	(mysql, "select relation_relation_members.ref from relations "
	"left join relation_relation_members on relation_relation_members.id = relations.id "
	"where relations.id in", "order by relations.id", mit->second.get_relations(), rels);
    
    relations = multiint_to_multiRelation_query
	(mysql, "select id, ref, role from relation_node_members "
	"where id in", "order by id",
	"select id, ref, role from relation_way_members "
	"where id in", "order by id",
	"select id, ref, role from relation_relation_members "
	"where id in", "order by id", rels, relations);
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    set< int > tways;
    tways = multiRelation_to_multiint_query
	(mysql, "select relation_way_members.ref from relations "
	"left join relation_way_members on relation_way_members.id = relations.id "
	"where relations.id in", "order by relations.id", mit->second.get_relations(), tways);
    
    ways = multiint_to_multiWay_query
	(mysql, "select ways.id, way_members.count, way_members.ref from ways "
	"left join way_members on way_members.id = ways.id "
	"where ways.id in", "order by ways.id", tways, ways);
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    set< int > tnodes;
    tnodes = multiRelation_to_multiint_query
	(mysql, "select relation_node_members.ref from relations "
	"left join relation_node_members on relation_node_members.id = relations.id "
	"where relations.id in", "order by relations.id", mit->second.get_relations(), tnodes);
    
    nodes = multiint_to_multiNode_query
	(mysql, "select id, lat, lon from nodes where id in", "order by id", tnodes, nodes);
  }
  
  maps[output] = Set(nodes, ways, relations, areas);
}
