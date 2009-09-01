#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "report_statement.h"

#include <mysql.h>

using namespace std;

void Report_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
}

void Report_Statement::forecast(MYSQL* mysql)
{
  const Set_Forecast& sf_in(declare_read_set(input));
    
  declare_used_time(10 + sf_in.node_count/10 + sf_in.way_count/10 + sf_in.relation_count/10);
  finish_statement_forecast();
    
  display_full();
  display_state();
}

void Report_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  User_Output& out(get_output());
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
    return;
  
  set< int > conflict_ids;
  multiNode_to_multiint_query
      (mysql, "select conflict from node_conflicts where id in",
       "", mit->second.get_nodes(), conflict_ids);
  multiWay_to_multiint_query
      (mysql, "select conflict from way_conflicts where id in",
       "", mit->second.get_ways(), conflict_ids);
  multiRelation_to_multiint_query
      (mysql, "select conflict from relation_conflicts where id in",
       "", mit->second.get_relations(), conflict_ids);
  
  for (set< int >::const_iterator it(conflict_ids.begin());
       it != conflict_ids.end(); )
  {
    ostringstream temp;
    temp<<"select conflicts.message, conflicts.line, conflicts.stack, osm.rule_names.name from conflicts "
	<<"left join osm.rule_bodys on osm.rule_bodys.id = conflicts.rule "
	<<"left join osm.rule_names on osm.rule_names.id = osm.rule_bodys.rule "
	<<"where conflicts.id in ("<<*it;
    unsigned int i(0);
    while (((++it) != conflict_ids.end()) && (i++ < 10000))
      temp<<", "<<*it;
    temp<<')';
    MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
    if (!result)
      return;
	
    MYSQL_ROW row(mysql_fetch_row(result));
    while ((row) && (row[0]))
    {
      if ((row[1]) && (row[2]) && (row[3]))
      {
	out.print("<p class=\"report-line\">In rule ");
	out.print(row[3]);
	out.print("(");
	unsigned int pos(0);
	bool first(true);
	while (row[2][pos])
	{
	  int type(atoi(&row[2][pos]));
	  while ((row[2][pos]) && (isdigit(row[2][pos])))
	    ++pos;
	  while ((row[2][pos]) && (isspace(row[2][pos])))
	    ++pos;
	  int item_id(atoi(&row[2][pos]));
	  while ((row[2][pos]) && (isdigit(row[2][pos])))
	    ++pos;
	  while ((row[2][pos]) && (isspace(row[2][pos])))
	    ++pos;
	  
	  if (first)
	    first = false;
	  else
	    out.print(", ");
	  if (type == NODE)
	  {
	    out.print("node ");
	    out.print(item_id);
	  }
	  if (type == WAY)
	  {
	    out.print("way ");
	    out.print(item_id);
	  }
	  if (type == RELATION)
	  {
	    out.print("relation ");
	    out.print(item_id);
	  }
	}
	out.print("), line ");
	out.print(atoi(row[1]));
	out.print(":<br/>\n");
	out.print(row[0]);
	out.print("</p>\n");
      }
      else
      {
	out.print("<p class=\"report-line\">");
	out.print(row[0]);
	out.print("</p>\n");
      }
      row = mysql_fetch_row(result);
    }
    mysql_free_result(result);
  }
}
