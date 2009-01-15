#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "foreach_statement.h"

#include <mysql.h>

using namespace std;

void Foreach_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
}

void Foreach_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if ((statement->get_name() == "union") ||
       (statement->get_name() == "id-query") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "recurse") ||
       (statement->get_name() == "foreach") ||
       (statement->get_name() == "make-area") ||
       (statement->get_name() == "coord-query") ||
       (statement->get_name() == "print") ||
       (statement->get_name() == "conflict") ||
       (statement->get_name() == "report") ||
       (statement->get_name() == "detect-odd-nodes"))
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Foreach_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  Set base_set(maps[input]);
  
  for (set< Node >::const_iterator oit(base_set.get_nodes().begin());
       oit != base_set.get_nodes().end(); ++oit)
  {
    set< Node > nodes;
    nodes.insert(*oit);
    maps[output] = Set(nodes, set< Way >(), set< Relation >());
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->execute(mysql, maps);
  }
  for (set< Way >::const_iterator oit(base_set.get_ways().begin());
       oit != base_set.get_ways().end(); ++oit)
  {
    set< Way > ways;
    ways.insert(*oit);
    maps[output] = Set(set< Node >(), ways, set< Relation >());
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->execute(mysql, maps);
  }
  for (set< Relation >::const_iterator oit(base_set.get_relations().begin());
       oit != base_set.get_relations().end(); ++oit)
  {
    set< Relation > relations;
    relations.insert(*oit);
    maps[output] = Set(set< Node >(), set< Way >(), relations);
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->execute(mysql, maps);
  }
  
  if (input == output)
    maps[output] = base_set;
}
