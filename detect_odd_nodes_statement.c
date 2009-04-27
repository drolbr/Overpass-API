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
#include "detect_odd_nodes_statement.h"

#include <mysql.h>

using namespace std;

void Detect_Odd_Nodes_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
}

void Detect_Odd_Nodes_Statement::forecast(MYSQL* mysql)
{
  Set_Forecast sf_in(declare_read_set(input));
  Set_Forecast& sf_out(declare_write_set(output));
    
  sf_out.node_count = sf_in.way_count;
  declare_used_time(1 + sf_in.way_count/10);
  finish_statement_forecast();
    
  display_full();
  display_state();
}

void Detect_Odd_Nodes_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation_ > relations;
  set< Area > areas;
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
  {
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  const set< Node >& in_nodes(mit->second.get_nodes());
  
  set< int > node_parity_control;
  for (set< Way >::const_iterator it(mit->second.get_ways().begin());
       it != mit->second.get_ways().end(); ++it)
  {
    vector< uint32 >::const_iterator iit(it->members.begin());
    if (iit != it->members.end())
    {
      if (node_parity_control.find(*iit) != node_parity_control.end())
	node_parity_control.erase(*iit);
      else
	node_parity_control.insert(*iit);
      
      iit = --(it->members.end());
      if (node_parity_control.find(*iit) != node_parity_control.end())
	node_parity_control.erase(*iit);
      else
	node_parity_control.insert(*iit);
    }
  }
  if (node_parity_control.size() > 0)
  {
    for (set< int >::const_iterator it(node_parity_control.begin());
	 it != node_parity_control.end(); ++it)
    {
      set< Node >::const_iterator nit(in_nodes.find(Node(*it, 0, 0)));
      if (nit != in_nodes.end())
	nodes.insert(*nit);
      else
      {
	ostringstream temp;
	temp<<"Error in detect-odd-nodes: Node "<<*it
	    <<" is not contained in set \""<<input<<"\".\n";
	runtime_error(temp.str(), cout);
      }
    }
  }
  
  maps[output] = Set(nodes, ways, relations, areas);
}
