#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>

#include "foreach.h"

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
  
  if ((statement->get_name() == "area-query") ||
       (statement->get_name() == "bbox-query") ||
       (statement->get_name() == "coord-query") ||
       (statement->get_name() == "foreach") ||
       (statement->get_name() == "id-query") ||
       (statement->get_name() == "item") ||
       (statement->get_name() == "make-area") ||
       (statement->get_name() == "print") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "recurse") ||
       (statement->get_name() == "union")/* ||
       (statement->get_name() == "conflict") ||
       (statement->get_name() == "report") ||
       (statement->get_name() == "detect-odd-nodes")*/)
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Foreach_Statement::forecast()
{
/*  Set_Forecast sf_in(declare_read_set(input));
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
      (*it)->forecast(mysql);
  
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
      (*it)->forecast(mysql);
  
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
      (*it)->forecast(mysql);
  
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
      (*it)->forecast(mysql);
  
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
  display_state();*/
}

void Foreach_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();
  
  Set base_set(rman.sets()[input]);
  rman.push_reference(base_set);
  
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(base_set.nodes.begin()); it != base_set.nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].nodes[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
          it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
    it(base_set.ways.begin()); it != base_set.ways.end(); ++it)
  {
    for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].ways[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
    it(base_set.relations.begin()); it != base_set.relations.end(); ++it)
  {
    for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].relations[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
    it(base_set.areas.begin()); it != base_set.areas.end(); ++it)
  {
    for (vector< Area_Skeleton >::const_iterator it2(it->second.begin());
    it2 != it->second.end(); ++it2)
    {
      rman.count_loop();
      rman.sets()[output].nodes.clear();
      rman.sets()[output].ways.clear();
      rman.sets()[output].relations.clear();
      rman.sets()[output].areas.clear();
      rman.sets()[output].areas[it->first].push_back(*it2);
      stopwatch.stop(Stopwatch::NO_DISK);
      for (vector< Statement* >::iterator it(substatements.begin());
      it != substatements.end(); ++it)
      {
	(*it)->execute(rman);
	stopwatch.sum((*it)->stopwatch);
      }
      stopwatch.skip();
    }
  }
  
  if (input == output)
    rman.sets()[output] = base_set;
  
  stopwatch.stop(Stopwatch::NO_DISK);
  stopwatch.report(get_name());
  rman.pop_reference();
  rman.health_check(*this);
}
