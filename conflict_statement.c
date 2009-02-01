#include <iomanip>
#include <iostream>
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
#include "conflict_statement.h"
#include "item_statement.h"

#include <mysql.h>

using namespace std;

void Conflict_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
}

void Conflict_Statement::add_statement(Statement* statement, string text)
{
  message.push_back(text);
  
  Item_Statement* item(dynamic_cast<Item_Statement*>(statement));
  if (item)
    items.push_back(item->get_result_name());
  else
    substatement_error(get_name(), statement);
}

void Conflict_Statement::add_final_text(string text)
{
  message.push_back(text);
}

void Conflict_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  //temporary
  unsigned int size(items.size());
  if (message.size() < items.size()+1)
    size = message.size()-1;
  ostringstream temp;
  for (unsigned int i(0); i < size; ++i)
  {
    temp<<message[i];
    const Set& item_set(maps[items[i]]);
    for (set< Node >::const_iterator it(item_set.get_nodes().begin());
	 it != item_set.get_nodes().end(); )
    {
      temp<<"<a href=\"http://www.openstreetmap.org/?"
	  <<"lat="<<setprecision(12)<<((double)(it->lat))/10000000<<'&'
	  <<"lon="<<setprecision(12)<<((double)(it->lon))/10000000<<'&'
	  <<"zoom=16\">node "<<it->id<<"</a>";
      if (++it != item_set.get_nodes().end())
	temp<<", ";
    }
    for (set< Way >::const_iterator it(item_set.get_ways().begin());
	 it != item_set.get_ways().end(); )
    {
      temp<<"way "<<it->id;
      if (++it != item_set.get_ways().end())
	temp<<", ";
    }
    for (set< Relation >::const_iterator it(item_set.get_relations().begin());
	 it != item_set.get_relations().end(); )
    {
      temp<<"relation "<<it->id;
      if (++it != item_set.get_relations().end())
	temp<<", ";
    }
  }
  temp<<message[size];
  string complete_message(temp.str());
  
  const Set& base_set(maps[input]);
  int conflict_id(int_query(mysql, "select max(id) from conflicts")+1);
  int rule_id(get_rule_id());
  ostringstream stack;
  for (vector< pair< int, int > >::const_iterator it(get_stack().begin());
       it != get_stack().end(); ++it)
    stack<<it->first<<' '<<it->second<<' ';
  
  temp.str("");
  temp<<"insert conflicts values ("<<conflict_id<<", "
      <<rule_id<<", "
      <<this->get_line_number()<<", '"
      <<stack.str()<<"', '";
  escape_insert(temp, complete_message);
  temp<<"')";
  mysql_query(mysql, temp.str().c_str());
  
  for (set< Node >::const_iterator it(base_set.get_nodes().begin());
       it != base_set.get_nodes().end(); ++it)
  {
    temp.str("");
    temp<<"insert node_conflicts values ("<<it->id<<", "<<conflict_id<<')';
    mysql_query(mysql, temp.str().c_str());
  }
  for (set< Way >::const_iterator it(base_set.get_ways().begin());
       it != base_set.get_ways().end(); ++it)
  {
    temp.str("");
    temp<<"insert way_conflicts values ("<<it->id<<", "<<conflict_id<<')';
    mysql_query(mysql, temp.str().c_str());
  }
  for (set< Relation >::const_iterator it(base_set.get_relations().begin());
       it != base_set.get_relations().end(); ++it)
  {
    temp.str("");
    temp<<"insert relation_conflicts values ("<<it->id<<", "<<conflict_id<<')';
    mysql_query(mysql, temp.str().c_str());
  }
  
}
