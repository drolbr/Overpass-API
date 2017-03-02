/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resource_manager.h"
#include "../statements/statement.h"

#include <sstream>

using namespace std;

uint count_set(const Set& set_)
{
  uint size(0);
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(set_.nodes.begin()); it != set_.nodes.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(set_.ways.begin()); it != set_.ways.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(set_.relations.begin()); it != set_.relations.end(); ++it)
    size += it->second.size();
  
  for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::const_iterator
      it(set_.attic_nodes.begin()); it != set_.attic_nodes.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Attic< Way_Skeleton > > >::const_iterator
      it(set_.attic_ways.begin()); it != set_.attic_ways.end(); ++it)
    size += it->second.size();
  for (map< Uint31_Index, vector< Attic< Relation_Skeleton > > >::const_iterator
      it(set_.attic_relations.begin()); it != set_.attic_relations.end(); ++it)
    size += it->second.size();

  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
      it(set_.areas.begin()); it != set_.areas.end(); ++it)
    size += it->second.size();
  
  return size;
}


uint64 eval_map(const std::map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  uint64 size(0);
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    size += it->second.size()*8 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, vector< Way_Skeleton > >& ways)
{
  uint64 size(0);
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
    size += it->second.size()*128 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, vector< Relation_Skeleton > >& relations)
{
  uint64 size(0);
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(relations.begin()); it != relations.end(); ++it)
    size += it->second.size()*192 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint32_Index, vector< Attic< Node_Skeleton > > >& nodes)
{
  uint64 size(0);
  for (map< Uint32_Index, vector< Attic< Node_Skeleton > > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    size += it->second.size()*16 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, vector< Attic< Way_Skeleton > > >& ways)
{
  uint64 size(0);
  for (map< Uint31_Index, vector< Attic< Way_Skeleton > > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
    size += it->second.size()*136 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, vector< Attic< Relation_Skeleton > > >& relations)
{
  uint64 size(0);
  for (map< Uint31_Index, vector< Attic< Relation_Skeleton > > >::const_iterator
      it(relations.begin()); it != relations.end(); ++it)
    size += it->second.size()*200 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, vector< Area_Skeleton > >& areas)
{
  uint64 size(0);
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
      it(areas.begin()); it != areas.end(); ++it)
    size += it->second.size()*128 + 64;
  return size;
}


uint64 eval_set(const Set& set_)
{
  return eval_map(set_.nodes) + eval_map(set_.ways) + eval_map(set_.relations)
      + eval_map(set_.attic_nodes) + eval_map(set_.attic_ways)
      + eval_map(set_.attic_relations) + eval_map(set_.areas);
}


void Resource_Manager::push_reference(const Set& set_)
{
  set_stack.push_back(&set_);
  stack_progress.push_back(make_pair(0, count_set(set_)));
  set_stack_sizes.push_back(eval_set(set_));
}


void Resource_Manager::pop_reference()
{
  set_stack.pop_back();
  stack_progress.pop_back();
  set_stack_sizes.pop_back();
}


void Resource_Manager::count_loop()
{
  ++stack_progress.back().first;
}


void Resource_Manager::log_and_display_error(std::string message)
{
  if (error_output)
    error_output->runtime_error(message);
  
  Logger logger(transaction->get_db_dir());
  logger.annotated_log(message);  
}


void Resource_Manager::health_check(const Statement& stmt, uint32 extra_time, uint64 extra_space)
{
  uint32 elapsed_time = 0;
  if (max_allowed_time > 0)
    elapsed_time = time(NULL) - start_time + extra_time;
  
  if (elapsed_time >= last_ping_time + 5)
  {
    if (watchdog)
      watchdog->ping();
    last_ping_time = elapsed_time;

    if (elapsed_time >= last_report_time + 15)
    {
      if (error_output)
      {
        error_output->display_statement_progress
            (elapsed_time, stmt.get_name(), stmt.get_progress(), stmt.get_line_number(),
	     stack_progress);
      }
      last_report_time = elapsed_time;
    }
  }
  
  uint64 size = 0;
  if (max_allowed_space > 0)
  {
    size = extra_space;
    
    for (map< string, Set >::const_iterator it(sets_.begin()); it != sets_.end();
        ++it)
      size += eval_set(it->second);
    for (vector< long long >::const_iterator it = set_stack_sizes.begin();
        it != set_stack_sizes.end(); ++it)
      size += *it;
  }

  if (elapsed_time > max_allowed_time)
  {
    if (error_output)
    {
      error_output->display_statement_progress
          (elapsed_time, stmt.get_name(), stmt.get_progress(), stmt.get_line_number(),
           stack_progress);
    }
    
    Resource_Error* error = new Resource_Error();
    error->timed_out = true;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;
    
    Logger logger(transaction->get_db_dir());
    ostringstream out;
    out<<"Timeout: runtime "<<error->runtime<<" seconds, size "<<error->size<<" bytes, "
        "in line "<<error->line_number<<", statement "<<error->stmt_name;
    logger.annotated_log(out.str());
    
    throw *error;
  }

  if (size > max_allowed_space)
  {
    if (error_output)
    {
      error_output->display_statement_progress
          (elapsed_time, stmt.get_name(), stmt.get_progress(), stmt.get_line_number(),
           stack_progress);
    }
    
    Resource_Error* error = new Resource_Error();
    error->timed_out = false;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;
    
    Logger logger(transaction->get_db_dir());
    ostringstream out;
    out<<"Oversized: runtime "<<error->runtime<<" seconds, size "<<error->size<<" bytes, "
        "in line "<<error->line_number<<", statement "<<error->stmt_name;
    logger.annotated_log(out.str());
    
    throw *error;
  }  
}


void Resource_Manager::start_cpu_timer(uint index)
{
  if (cpu_start_time.size() <= index)
    cpu_start_time.resize(index+1, 0);
  cpu_start_time[index] = clock()/1000;
}


void Resource_Manager::stop_cpu_timer(uint index)
{
  if (cpu_runtime.size() <= index)
    cpu_runtime.resize(index+1, 0);
  if (index < cpu_start_time.size())
    cpu_runtime[index] += clock()/1000 - cpu_start_time[index];
}
