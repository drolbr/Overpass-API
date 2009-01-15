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
    temp<<"select message from conflicts "
	<<"where id in ("<<*it;
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
      cout<<"<p class=\"report-line\">"<<row[0]<<"</p>\n";
      row = mysql_fetch_row(result);
    }
    delete result;
  }
}
