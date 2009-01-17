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

const unsigned int ID_QUERY_NODE = 1;
const unsigned int ID_QUERY_WAY = 2;
const unsigned int ID_QUERY_RELATION = 3;

void Id_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["type"] = "";
  attributes["ref"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  
  if (attributes["type"] == "node")
    type = ID_QUERY_NODE;
  else if (attributes["type"] == "way")
    type = ID_QUERY_WAY;
  else if (attributes["type"] == "relation")
    type = ID_QUERY_RELATION;
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

void Id_Query_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  if (ref == 0)
    return;
  
  ostringstream temp;
  if (type == ID_QUERY_NODE)
    temp<<"select id, lat, lon from nodes where id = "<<ref;
  else if (type == ID_QUERY_WAY)
    temp<<"select ways.id, way_members.count, way_members.ref "
	<<"from ways left join way_members on ways.id = way_members.id "
	<<"where ways.id = "<<ref;
  else if (type == ID_QUERY_RELATION)
    temp<<"select relations.id, relation_node_members.ref, relation_node_members.role from relations "
	<<"left join relation_node_members on relations.id = relation_node_members.id "
	<<"where relations.id = "<<ref;
  else
    return;
  
  set< Node > nodes;
  set< Way > ways;
  set< Relation > relations;
  
  MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
  
  if (!result)
    return;
  
  if (type == ID_QUERY_NODE)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    if ((row) && (row[0]))
    {
      if ((row[1]) && (row[2]))
	nodes.insert(Node(atoi(row[0]), atoi(row[1]), atoi(row[2])));
    }
  }
  else if (type == ID_QUERY_WAY)
  {
    MYSQL_ROW row(mysql_fetch_row(result));
    if ((row) && (row[0]))
    {
      Way way(atoi(row[0]));
      way.members.reserve(16);
      while ((row) && (row[1]) && (row[2]))
      {
	unsigned int count((unsigned int)atol(row[1]));
	if (way.members.size() < count)
	  way.members.resize(count);
	way.members[count-1] = atoi(row[2]);
	row = mysql_fetch_row(result);
      }
      ways.insert(way);
    }
  }
  else if (type == ID_QUERY_RELATION)
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
      delete result;
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
      delete result;
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
  delete result;
  
  maps[output] = Set(nodes, ways, relations);
}
