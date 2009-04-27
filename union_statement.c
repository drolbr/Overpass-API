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
#include "user_interface.h"
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

void Union_Statement::forecast(MYSQL* mysql)
{
  int node_count(0);
  int way_count(0);
  int relation_count(0);
  int area_count(0);
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->forecast(mysql);
    const Set_Forecast& sf_in(declare_read_set((*it)->get_result_name()));
    node_count += sf_in.node_count;
    way_count += sf_in.way_count;
    relation_count += sf_in.relation_count;
    area_count += sf_in.area_count;
  }
  
  Set_Forecast& sf_out(declare_write_set(output));
  
  sf_out.node_count = node_count;
  sf_out.way_count = way_count;
  sf_out.relation_count = relation_count;
  sf_out.area_count = area_count;
  declare_used_time(0);
  finish_statement_forecast();
    
  display_full();
  display_state();
}

void Union_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation_ > relations;
  set< Area > areas;
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->execute(mysql, maps);
    statement_finished(*it);
    
    const Set& summand(maps[(*it)->get_result_name()]);
    nodes.insert(summand.get_nodes().begin(), summand.get_nodes().end());
    ways.insert(summand.get_ways().begin(), summand.get_ways().end());
    relations.insert(summand.get_relations().begin(), summand.get_relations().end());
    areas.insert(summand.get_areas().begin(), summand.get_areas().end());
  }
  
  maps[output] = Set(nodes, ways, relations, areas);
}
