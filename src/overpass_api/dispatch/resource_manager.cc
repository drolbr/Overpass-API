#include "resource_manager.h"
#include "../statements/statement.h"

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
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
      it(set_.areas.begin()); it != set_.areas.end(); ++it)
    size += it->second.size();
  
  return size;
}

void Resource_Manager::push_reference(const Set& set_)
{
  set_stack.push_back(&set_);
  stack_progress.push_back(make_pair(0, count_set(set_)));
}

void Resource_Manager::pop_reference()
{
  set_stack.pop_back();
  stack_progress.pop_back();
}

void Resource_Manager::count_loop()
{
  ++stack_progress.back().first;
}

uint64 eval_set(const Set& set_)
{
  uint64 size(0);
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(set_.nodes.begin()); it != set_.nodes.end(); ++it)
    size += it->second.size()*8 + 64;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(set_.ways.begin()); it != set_.ways.end(); ++it)
    size += it->second.size()*128 + 64;
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(set_.relations.begin()); it != set_.relations.end(); ++it)
    size += it->second.size()*192 + 64;
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
      it(set_.areas.begin()); it != set_.areas.end(); ++it)
    size += it->second.size()*128 + 64;
  
  return size;
}

void Resource_Manager::health_check(const Statement& stmt)
{
  uint32 elapsed_time = 0;
  if (max_allowed_time > 0)
    elapsed_time = time(NULL) - start_time;
  
  if (elapsed_time >= last_report_time + 5)
  {
    if (watchdog)
      watchdog->ping();
    last_ping_time += 5;

    if (elapsed_time >= last_report_time + 60)
    {
      if (error_output)
      {
        error_output->display_statement_progress
            (elapsed_time, stmt.get_name(), stmt.get_line_number(), stack_progress);
      }
      last_report_time += 60;
    }
  }
  
  uint64 size = 0;
  if (max_allowed_space > 0)
  {
    for (map< string, Set >::const_iterator it(sets_.begin()); it != sets_.end();
        ++it)
      size += eval_set(it->second);
    for (vector< const Set* >::const_iterator it(set_stack.begin());
        it != set_stack.end(); ++it)
      size += eval_set(**it);
  }

  if (elapsed_time > max_allowed_time)
  {
    Resource_Error* error = new Resource_Error();
    error->timed_out = true;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;
    
    throw *error;
  }

  if (size > max_allowed_space)
  {
    Resource_Error* error = new Resource_Error();
    error->timed_out = false;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;
    
    throw *error;
  }  
}
