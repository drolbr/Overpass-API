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
#include "../data/abstract_processing.h"
#include "../data/utils.h"
#include "../statements/statement.h"

#include <sstream>


uint64 eval_map(const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes)
{
  uint64 size(0);
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    size += it->second.size()*8 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
    size += it->second.size()*128 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it(relations.begin()); it != relations.end(); ++it)
    size += it->second.size()*192 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& nodes)
{
  uint64 size(0);
  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    size += it->second.size()*16 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
    size += it->second.size()*136 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
      it(relations.begin()); it != relations.end(); ++it)
    size += it->second.size()*200 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Area_Skeleton > >& areas)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator
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


Set* Runtime_Stack_Frame::get_set(const std::string& set_name)
{
  std::map< std::string, Set >::iterator it = sets.find(set_name);
  if (it != sets.end())
    return &it->second;
  
  if (parent)
    return parent->get_set(set_name);
  
  return 0;
}


void Runtime_Stack_Frame::swap_set(const std::string& set_name, Set& set_)
{
  Set& to_swap = sets[set_name];
  size_per_set[set_name] = eval_set(to_swap);
  set_.swap(to_swap);
}


void Runtime_Stack_Frame::clear_sets()
{
  sets.clear();
  size_per_set.clear();
}


void Runtime_Stack_Frame::copy_outward(const std::string& inner_set_name, const std::string& top_set_name)
{
  Set* from = 0;
  
  if (parent)
    from = parent->get_set(inner_set_name);
  
  if (from)
  {
    size_per_set[top_set_name] = eval_set(*from);
    sets[top_set_name] = *from;
  }
  else
  {
    size_per_set[top_set_name] = 0;
    sets[top_set_name];
  }
}


void Runtime_Stack_Frame::move_outward(const std::string& inner_set_name, const std::string& top_set_name)
{
  if (parent)
  {
    size_per_set[top_set_name] = parent->size_per_set[inner_set_name];
    sets[top_set_name].clear();
    parent->swap_set(inner_set_name, sets[top_set_name]);
  }
}


void Runtime_Stack_Frame::clear_inside(const std::string& inner_set_name)
{
  if (parent)
  {
    parent->sets.erase(inner_set_name);
    parent->size_per_set[inner_set_name] = 0;
  }
}


bool Runtime_Stack_Frame::union_inward(const std::string& top_set_name, const std::string& inner_set_name)
{
  bool new_elements_found = false;
  Set* source = get_set(top_set_name);
  
  if (source && parent)
  {
    Set& target = parent->sets[inner_set_name];

    new_elements_found |= indexed_set_union(target.nodes, source->nodes);
    new_elements_found |= indexed_set_union(target.attic_nodes, source->attic_nodes);

    new_elements_found |= indexed_set_union(target.ways, source->ways);
    new_elements_found |= indexed_set_union(target.attic_ways, source->attic_ways);

    new_elements_found |= indexed_set_union(target.relations, source->relations);
    new_elements_found |= indexed_set_union(target.attic_relations, source->attic_relations);

    new_elements_found |= indexed_set_union(target.areas, source->areas);
    new_elements_found |= indexed_set_union(target.deriveds, source->deriveds);
    
    parent->size_per_set[inner_set_name] = eval_set(target);
  }
  
  return new_elements_found;
}


void Runtime_Stack_Frame::substract_from_inward(const std::string& top_set_name, const std::string& inner_set_name)
{
  Set* source = get_set(top_set_name);
  
  if (source && parent)
  {
    Set& target = parent->sets[inner_set_name];

    indexed_set_difference(target.nodes, source->nodes);
    indexed_set_difference(target.ways, source->ways);
    indexed_set_difference(target.relations, source->relations);
    
    indexed_set_difference(target.attic_nodes, source->attic_nodes);
    indexed_set_difference(target.attic_ways, source->attic_ways);
    indexed_set_difference(target.attic_relations, source->attic_relations);
    
    indexed_set_difference(target.areas, source->areas);
    indexed_set_difference(target.deriveds, source->deriveds);
    
    parent->size_per_set[inner_set_name] = eval_set(target);
  }
}


void Runtime_Stack_Frame::move_all_inward()
{
  if (parent)
  {
    for (std::map< std::string, Set >::iterator it = sets.begin(); it != sets.end(); ++it)
      parent->swap_set(it->first, it->second);
  }
}


void Runtime_Stack_Frame::move_all_inward_except(const std::string& set_name)
{
  if (parent)
  {
    for (std::map< std::string, Set >::iterator it = sets.begin(); it != sets.end(); ++it)
    {
      if (it->first != set_name)
        parent->swap_set(it->first, it->second);
    }
  }
}


uint64 Runtime_Stack_Frame::total_size()
{
  uint64 result = 0;
  
  for (std::map< std::string, uint64 >::const_iterator it = size_per_set.begin(); it != size_per_set.end(); ++it)
    result += it->second;
  
  if (parent)
    return result + parent->total_size();
  
  return result;
}


std::vector< std::pair< uint, uint > > Runtime_Stack_Frame::stack_progress() const
{
  std::vector< std::pair< uint, uint > > result;
  if (parent)
    parent->stack_progress().swap(result);
  
  result.push_back(std::make_pair(loop_count, loop_size));
  
  return result;
}


Resource_Manager::Resource_Manager(
    Transaction& transaction_, Parsed_Query* global_settings_, Watchdog_Callback* watchdog_,
    Error_Output* error_output_)
      : transaction(&transaction_), error_output(error_output_),
        area_transaction(0), area_updater_(0),
        watchdog(watchdog_), global_settings(global_settings_), global_settings_owned(false),
	start_time(time(NULL)), last_ping_time(0), last_report_time(0),
	max_allowed_time(0), max_allowed_space(0)
{
  if (!global_settings)
  {
    global_settings = new Parsed_Query();
    global_settings_owned = true;
  }
  
  runtime_stack.push_back(new Runtime_Stack_Frame());
}


Resource_Manager::Resource_Manager(
    Transaction& transaction_, Parsed_Query& global_settings_, Error_Output* error_output_,
    Transaction& area_transaction_, Watchdog_Callback* watchdog_, Area_Usage_Listener* area_updater__)
    : transaction(&transaction_), error_output(error_output_),
      area_transaction(&area_transaction_), area_updater_(area_updater__),
      watchdog(watchdog_), global_settings(&global_settings_), global_settings_owned(false),
      start_time(time(NULL)), last_ping_time(0), last_report_time(0),
      max_allowed_time(0), max_allowed_space(0)
{
  runtime_stack.push_back(new Runtime_Stack_Frame());
}


const Set* Resource_Manager::get_set(const std::string& set_name)
{
  if (runtime_stack.empty())
    return 0;
  
  return runtime_stack.back()->get_set(set_name);
}


void Resource_Manager::swap_set(const std::string& set_name, Set& set_)
{
  if (runtime_stack.empty())
    runtime_stack.push_back(new Runtime_Stack_Frame());
  
  sort(set_);
  runtime_stack.back()->swap_set(set_name, set_);
}


void Resource_Manager::clear_sets()
{
  if (!runtime_stack.empty())
    runtime_stack.back()->clear_sets();
}


  
void Resource_Manager::push_stack_frame()
{
  runtime_stack.push_back(new Runtime_Stack_Frame(runtime_stack.empty() ? 0 : runtime_stack.back()));
}


void Resource_Manager::copy_outward(const std::string& inner_set_name, const std::string& top_set_name)
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->copy_outward(inner_set_name, top_set_name);
}


void Resource_Manager::move_outward(const std::string& inner_set_name, const std::string& top_set_name)
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->move_outward(inner_set_name, top_set_name);
}


void Resource_Manager::clear_inside(const std::string& inner_set_name)
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->clear_inside(inner_set_name);
}


bool Resource_Manager::union_inward(const std::string& top_set_name, const std::string& inner_set_name)
{
  if (runtime_stack.empty())
    return false;
  
  return runtime_stack.back()->union_inward(top_set_name, inner_set_name);
}


void Resource_Manager::substract_from_inward(const std::string& top_set_name, const std::string& inner_set_name)
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->substract_from_inward(top_set_name, inner_set_name);
}


void Resource_Manager::move_all_inward()
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->move_all_inward();
}


void Resource_Manager::move_all_inward_except(const std::string& set_name)
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->move_all_inward_except(set_name);
}


void Resource_Manager::pop_stack_frame()
{
  if (!runtime_stack.empty())
  {
    delete runtime_stack.back();
    runtime_stack.pop_back();
  }
}


uint count_set(const Set& set_)
{
  uint size(0);
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator
      it(set_.nodes.begin()); it != set_.nodes.end(); ++it)
    size += it->second.size();
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
      it(set_.ways.begin()); it != set_.ways.end(); ++it)
    size += it->second.size();
  for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it(set_.relations.begin()); it != set_.relations.end(); ++it)
    size += it->second.size();

  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator
      it(set_.attic_nodes.begin()); it != set_.attic_nodes.end(); ++it)
    size += it->second.size();
  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator
      it(set_.attic_ways.begin()); it != set_.attic_ways.end(); ++it)
    size += it->second.size();
  for (std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
      it(set_.attic_relations.begin()); it != set_.attic_relations.end(); ++it)
    size += it->second.size();

  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator
      it(set_.areas.begin()); it != set_.areas.end(); ++it)
    size += it->second.size();

  return size;
}


void Resource_Manager::count_loop()
{
  if (runtime_stack.empty())
    return;
  
  runtime_stack.back()->count_loop();
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
	     runtime_stack.back()->stack_progress());
      }
      last_report_time = elapsed_time;
    }
  }

  uint64 size = 0;
  if (max_allowed_space > 0)
  {
    size = extra_space;
    if (!runtime_stack.empty())
      size += runtime_stack.back()->total_size();
  }

  if (elapsed_time > max_allowed_time)
  {
    if (error_output)
    {
      error_output->display_statement_progress
          (elapsed_time, stmt.get_name(), stmt.get_progress(), stmt.get_line_number(),
           runtime_stack.back()->stack_progress());
    }

    Resource_Error* error = new Resource_Error();
    error->timed_out = true;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;

    Logger logger(transaction->get_db_dir());
    std::ostringstream out;
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
           runtime_stack.back()->stack_progress());
    }

    Resource_Error* error = new Resource_Error();
    error->timed_out = false;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;

    Logger logger(transaction->get_db_dir());
    std::ostringstream out;
    out<<"Oversized: runtime "<<error->runtime<<" seconds, size "<<error->size<<" bytes, "
        "in line "<<error->line_number<<", statement "<<error->stmt_name;
    logger.annotated_log(out.str());

    throw *error;
  }
}


uint64 Resource_Manager::get_desired_timestamp() const
{
  return runtime_stack.empty() ? NOW : runtime_stack.back()->get_desired_timestamp();
}


uint64 Resource_Manager::get_diff_from_timestamp() const
{
  return runtime_stack.empty() ? NOW : runtime_stack.back()->get_diff_from_timestamp();
}


uint64 Resource_Manager::get_diff_to_timestamp() const
{
  return runtime_stack.empty() ? NOW : runtime_stack.back()->get_diff_to_timestamp();
}


void Resource_Manager::set_desired_timestamp(uint64 timestamp)
{
  if (!runtime_stack.empty())
    runtime_stack.back()->set_desired_timestamp(timestamp);
}


void Resource_Manager::set_diff_from_timestamp(uint64 timestamp)
{
  if (!runtime_stack.empty())
    runtime_stack.back()->set_diff_from_timestamp(timestamp);
}


void Resource_Manager::set_diff_to_timestamp(uint64 timestamp)
{
  if (!runtime_stack.empty())
    runtime_stack.back()->set_diff_to_timestamp(timestamp);
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
