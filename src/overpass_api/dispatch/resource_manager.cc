#include "resource_manager.h"
#include "../core/settings.h"
#include "../statements/statement.h"

using namespace std;

void Resource_Manager::push_reference(const Set& set_)
{
  set_stack.push_back(&set_);
}

void Resource_Manager::pop_reference()
{
  set_stack.pop_back();
}

uint64 eval_set(const Set& set_)
{
  uint64 size(0);
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(set_.nodes.begin()); it != set_.nodes.end(); ++it)
    size += it->second.size()*8 + 64;
  //cerr<<size<<'\t';
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
      it(set_.ways.begin()); it != set_.ways.end(); ++it)
    size += it->second.size()*128 + 64;
  //cerr<<size<<'\t';
  for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
      it(set_.relations.begin()); it != set_.relations.end(); ++it)
    size += it->second.size()*192 + 64;
  //cerr<<size<<'\t';
  for (map< Uint31_Index, vector< Area_Skeleton > >::const_iterator
      it(set_.areas.begin()); it != set_.areas.end(); ++it)
    size += it->second.size()*128 + 64;
  //cerr<<size<<'\t';
  
  return size;
}

void Resource_Manager::health_check(const Statement& stmt)
{
  uint32 elapsed_time(0);
  if (de_osm3s_file_ids::max_allowed_time > 0)
    elapsed_time = time(NULL) - start_time;
  
  //cerr<<stmt.get_name()<<'\t';
  //cerr<<elapsed_time<<'\t'<<de_osm3s_file_ids::max_allowed_time<<'\t';
  
  uint64 size(0);
  for (map< string, Set >::const_iterator it(sets_.begin()); it != sets_.end();
      ++it)
  {
    //cerr<<it->first<<'\t';
    size += eval_set(it->second);
  }
  for (vector< const Set* >::const_iterator it(set_stack.begin());
      it != set_stack.end(); ++it)
  {
    //cerr<<"[Stack]\t";
    size += eval_set(**it);
  }
  //cerr<<'\n';

  if (elapsed_time > de_osm3s_file_ids::max_allowed_time)
  {
    Resource_Error* error = new Resource_Error();
    error->timed_out = true;
    error->stmt_name = stmt.get_name();
    error->line_number = stmt.get_line_number();
    error->size = size;
    error->runtime = elapsed_time;
    
    throw *error;
  }  

  if (size > de_osm3s_file_ids::max_allowed_space)
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
