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
#include "union_statement.h"

#include <mysql.h>

using namespace std;

void Union_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
}

void Union_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if (statement->get_result_name() != "")
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Union_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation > relations;
  set< Area > areas;
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->execute(mysql, maps);
    const Set& summand(maps[(*it)->get_result_name()]);
    nodes.insert(summand.get_nodes().begin(), summand.get_nodes().end());
    ways.insert(summand.get_ways().begin(), summand.get_ways().end());
    relations.insert(summand.get_relations().begin(), summand.get_relations().end());
    areas.insert(summand.get_areas().begin(), summand.get_areas().end());
  }
  
  maps[output] = Set(nodes, ways, relations, areas);
}
