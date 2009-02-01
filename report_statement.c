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

void Report_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
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
    temp<<"select conflicts.message, conflicts.line, conflicts.stack, rule_names.name from conflicts "
	<<"left join rule_bodys on rule_bodys.id = conflicts.rule "
	<<"left join rule_names on rule_names.id = rule_bodys.rule "
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
	cout<<"<p class=\"report-line\">In rule "<<row[3]<<"(";
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
	    cout<<", ";
	  if (type == NODE)
	    cout<<"node "<<item_id;
	  if (type == WAY)
	    cout<<"way "<<item_id;
	  if (type == RELATION)
	    cout<<"relation "<<item_id;
	}
	cout<<"), line "<<atoi(row[1])<<":<br/>\n"
	    <<row[0]<<"</p>\n";
      }
      else
	cout<<"<p class=\"report-line\">"<<row[0]<<"</p>\n";
      row = mysql_fetch_row(result);
    }
    mysql_free_result(result);
  }
}
