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
#include "id_query_statement.h"

#include <mysql.h>

using namespace std;

void Id_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  
  if (attributes["type"] == "node")
    type = NODE;
  else if (attributes["type"] == "way")
    type = WAY;
  else if (attributes["type"] == "relation")
    type = RELATION;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"id-query\""
	<<" the only allowed values are \"node\", \"way\" or \"relation\".";
    add_static_error(temp.str());
  }
  
  ref = (unsigned int)atol(attributes["ref"].c_str());
  if (ref == 0)
  {
    ostringstream temp;
    temp<<"For the attribute \"ref\" of the element \"id-query\""
	<<" the only allowed values are positive integers.";
    add_static_error(temp.str());
  }
}

void Id_Query_Statement::forecast(MYSQL* mysql)
{
  Set_Forecast& sf_out(declare_write_set(output));
  
  if (type == NODE)
  {
    sf_out.node_count = 1;
    declare_used_time(400);
  }
  else if (type == WAY)
  {
    sf_out.way_count = 1;
    declare_used_time(300);
  }
  if (type == RELATION)
  {
    sf_out.relation_count = 1;
    declare_used_time(200);
  }
    
  finish_statement_forecast();
    
  display_full();
  display_state();
}

void Id_Query_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  if (ref == 0)
    return;
  
  set< Node >& nodes(maps[output].get_nodes_handle());
  set< Way >& ways(maps[output].get_ways_handle());
  set< Relation >& relations((maps[output].get_relations_handle()));
  set< Area >& areas((maps[output].get_areas_handle()));
  
  nodes.clear();
  ways.clear();
  relations.clear();
  areas.clear();
  
  if (type == NODE)
  {
    set< uint32 > source;
    source.insert(ref);
    multiint_to_multiNode_query(source, nodes);
    
    return;
  }
  else if (type == WAY)
  {
    set< uint32 > source;
    source.insert(ref);
    multiint_to_multiWay_query(source, ways);
    
    return;
  }
  else if (type == RELATION)
  {
    set< uint32 > source;
    source.insert(ref);
    multiint_to_multiRelation_query(source, relations);
    
    return;
  }
  
  ostringstream temp;
  if (type == RELATION)
    temp<<"select relations.id, relation_node_members.ref, relation_node_members.role from relations "
	<<"left join relation_node_members on relations.id = relation_node_members.id "
	<<"where relations.id = "<<ref;
  else
    return;
  
  MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
  
  if (!result)
    return;
  
  if (type == RELATION)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    if ((row) && (row[0]))
    {
      Relation relation(atoi(row[0]));
      while ((row) && (row[1]))
      {
	if (row[2])
	  relation.node_members.insert
	      (make_pair< int, int >(atoi(row[1]), atoi(row[2])));
	else
	  relation.node_members.insert
	      (make_pair< int, int >(atoi(row[1]), 0));
	row = mysql_fetch_row(result);
      }
      
      temp.str("");
      temp<<"select ref, role from relation_way_members "
	  <<"where relation_way_members.id = "<<ref;
      mysql_free_result(result);
      result = mysql_query_wrapper(mysql, temp.str());
      if (!result)
	return;
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	if (row[1])
	  relation.way_members.insert
	      (make_pair< int, int >(atoi(row[0]), atoi(row[1])));
	else
	  relation.way_members.insert
	      (make_pair< int, int >(atoi(row[0]), 0));
	row = mysql_fetch_row(result);
      }
      
      temp.str("");
      temp<<"select ref, role from relation_relation_members "
	  <<"where relation_relation_members.id = "<<ref;
      mysql_free_result(result);
      result = mysql_query_wrapper(mysql, temp.str());
      if (!result)
	return;
      row = mysql_fetch_row(result);
      while ((row) && (row[0]))
      {
	if (row[1])
	  relation.relation_members.insert
	      (make_pair< int, int >(atoi(row[0]), atoi(row[1])));
	else
	  relation.relation_members.insert
	      (make_pair< int, int >(atoi(row[0]), 0));
	row = mysql_fetch_row(result);
      }
      
      relations.insert(relation);
    }
  }
  mysql_free_result(result);
  
  maps[output] = Set(nodes, ways, relations);
}
