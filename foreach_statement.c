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
#include "user_interface.h"
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

void Foreach_Statement::forecast()
{
  Set_Forecast sf_in(declare_read_set(input));
  if (sf_in.node_count > 0)
  {
    ostringstream temp;
    temp<<"Loop forecast for one of "<<sf_in.node_count<<" nodes:";
    add_sanity_remark(temp.str());
    
    inc_stack();
    Set_Forecast& sf_out(declare_write_set(output));
    sf_out.node_count = 1;
  
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->forecast();
  
    const vector< pair< int, string > >& stack(pending_stack());
    set< string > being_read_only;
    set< string > being_written;
    for (vector< pair< int, string > >::const_iterator it(stack.begin());
	 it != stack.end(); ++it)
    {
      if ((it->first == READ_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_written.find(it->second) == being_written.end())
	  being_read_only.insert(it->second);
      }
      else if ((it->first == WRITE_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_read_only.find(it->second) == being_read_only.end())
	  being_written.insert(it->second);
	else
	{
	  ostringstream temp;
	  temp<<"The data flow of set \""<<it->second
	      <<"\" makes the loop cycles dependend on each other.";
	  add_sanity_error(temp.str());
	  being_written.insert(it->second);
	}
      }
    }
    
    int time_offset(stack_time_offset());
    dec_stack();
    declare_used_time(time_offset * (sf_in.node_count - 1));
  }
  if (sf_in.way_count > 0)
  {
    ostringstream temp;
    temp<<"Loop forecast for one of "<<sf_in.way_count<<" ways:";
    add_sanity_remark(temp.str());
    
    inc_stack();
    Set_Forecast& sf_out(declare_write_set(output));
    sf_out.way_count = 1;
  
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->forecast();
  
    const vector< pair< int, string > >& stack(pending_stack());
    set< string > being_read_only;
    set< string > being_written;
    for (vector< pair< int, string > >::const_iterator it(stack.begin());
	 it != stack.end(); ++it)
    {
      if ((it->first == READ_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_written.find(it->second) == being_written.end())
	  being_read_only.insert(it->second);
      }
      else if ((it->first == WRITE_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_read_only.find(it->second) == being_read_only.end())
	  being_written.insert(it->second);
	else
	{
	  ostringstream temp;
	  temp<<"The data flow of set \""<<it->second
	      <<"\" makes the loop cycles dependend on each other.";
	  add_sanity_error(temp.str());
	  being_written.insert(it->second);
	}
      }
    }
    
    int time_offset(stack_time_offset());
    dec_stack();
    declare_used_time(time_offset * (sf_in.way_count - 1));
  }
  if (sf_in.relation_count > 0)
  {
    ostringstream temp;
    temp<<"Loop forecast for one of "<<sf_in.relation_count<<" relations:";
    add_sanity_remark(temp.str());
    
    inc_stack();
    Set_Forecast& sf_out(declare_write_set(output));
    sf_out.relation_count = 1;
  
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->forecast();
  
    const vector< pair< int, string > >& stack(pending_stack());
    set< string > being_read_only;
    set< string > being_written;
    for (vector< pair< int, string > >::const_iterator it(stack.begin());
	 it != stack.end(); ++it)
    {
      if ((it->first == READ_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_written.find(it->second) == being_written.end())
	  being_read_only.insert(it->second);
      }
      else if ((it->first == WRITE_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_read_only.find(it->second) == being_read_only.end())
	  being_written.insert(it->second);
	else
	{
	  ostringstream temp;
	  temp<<"The data flow of set \""<<it->second
	      <<"\" makes the loop cycles dependend on each other.";
	  add_sanity_error(temp.str());
	  being_written.insert(it->second);
	}
      }
    }
    
    int time_offset(stack_time_offset());
    dec_stack();
    declare_used_time(time_offset * (sf_in.relation_count - 1));
  }
  if (sf_in.area_count > 0)
  {
    ostringstream temp;
    temp<<"Loop forecast for one of "<<sf_in.area_count<<" areas:";
    add_sanity_remark(temp.str());
    
    inc_stack();
    Set_Forecast& sf_out(declare_write_set(output));
    sf_out.area_count = 1;
  
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
      (*it)->forecast();
  
    const vector< pair< int, string > >& stack(pending_stack());
    set< string > being_read_only;
    set< string > being_written;
    for (vector< pair< int, string > >::const_iterator it(stack.begin());
	 it != stack.end(); ++it)
    {
      if ((it->first == READ_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_written.find(it->second) == being_written.end())
	  being_read_only.insert(it->second);
      }
      else if ((it->first == WRITE_FORECAST) || (it->first == UNION_FORECAST))
      {
	if (being_read_only.find(it->second) == being_read_only.end())
	  being_written.insert(it->second);
	else
	{
	  ostringstream temp;
	  temp<<"The data flow of set \""<<it->second
	      <<"\" makes the loop cycles dependend on each other.";
	  add_sanity_error(temp.str());
	  being_written.insert(it->second);
	}
      }
    }
    
    int time_offset(stack_time_offset());
    dec_stack();
    declare_used_time(time_offset * (sf_in.area_count - 1));
  }
  finish_statement_forecast();
    
  display_full();
  display_state();
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
    push_stack(NODE, oit->id);
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
    {
      (*it)->execute(mysql, maps);
      statement_finished(*it);
    }
    pop_stack();
  }
  for (set< Way >::const_iterator oit(base_set.get_ways().begin());
       oit != base_set.get_ways().end(); ++oit)
  {
    set< Way > ways;
    ways.insert(*oit);
    maps[output] = Set(set< Node >(), ways, set< Relation >());
    push_stack(WAY, oit->id);
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
    {
      (*it)->execute(mysql, maps);
      statement_finished(*it);
    }
    pop_stack();
  }
  for (set< Relation >::const_iterator oit(base_set.get_relations().begin());
       oit != base_set.get_relations().end(); ++oit)
  {
    set< Relation > relations;
    relations.insert(*oit);
    maps[output] = Set(set< Node >(), set< Way >(), relations);
    push_stack(RELATION, oit->id);
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
    {
      (*it)->execute(mysql, maps);
      statement_finished(*it);
    }
    pop_stack();
  }
  for (set< Area >::const_iterator oit(base_set.get_areas().begin());
       oit != base_set.get_areas().end(); ++oit)
  {
    set< Area > areas;
    areas.insert(*oit);
    maps[output] = Set(set< Node >(), set< Way >(), set< Relation >(), areas);
    push_stack(AREA, oit->id);
    for (vector< Statement* >::iterator it(substatements.begin());
	 it != substatements.end(); ++it)
    {
      (*it)->execute(mysql, maps);
      statement_finished(*it);
    }
    pop_stack();
  }
  
  if (input == output)
    maps[output] = base_set;
}
