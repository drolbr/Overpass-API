#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

using namespace std;

struct Output_Sorter
{
  vector< string > output_per_index;
  uint32 last_index;
  
  Output_Sorter() : last_index(0) {}

  void sort_and_output_if_index_changed(uint32 index)
  {
    if (index != last_index)
    {
      sort(output_per_index.begin(), output_per_index.end());
      for (vector< string >::const_iterator it(output_per_index.begin());
          it != output_per_index.end(); ++it)
        cout<<*it;
      output_per_index.clear();
      last_index = index;
    }
  }
  
  ~Output_Sorter()
  {
    sort(output_per_index.begin(), output_per_index.end());
    for (vector< string >::const_iterator it(output_per_index.begin());
        it != output_per_index.end(); ++it)
      cout<<*it;
  }
};

struct Output_Sorter_Kv
{
  vector< string > output_per_index;
  pair< string, string > last_index;
  
  Output_Sorter_Kv() {}
  
  void sort_and_output_if_index_changed(const pair< string, string >& index)
  {
    if (index != last_index)
    {
      sort(output_per_index.begin(), output_per_index.end());
      for (vector< string >::const_iterator it(output_per_index.begin());
          it != output_per_index.end(); ++it)
        cout<<*it;
      output_per_index.clear();
      last_index = index;
    }
  }
  
  ~Output_Sorter_Kv()
  {
    sort(output_per_index.begin(), output_per_index.end());
    for (vector< string >::const_iterator it(output_per_index.begin());
        it != output_per_index.end(); ++it)
      cout<<*it;
  }
};

void dump_nodes(uint32 pattern_size)
{
  Output_Sorter output_sorter;
  
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (*de_osm3s_file_ids::NODES, false);
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
      it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed(it.index().val());
    ostringstream buf;
    buf<<it.object().id<<'\t'<<setprecision(10)
	<<Node::lat(it.index().val(), it.object().ll_lower)<<'\t'
	<<Node::lon(it.index().val(), it.object().ll_lower)<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_node_tags_local(uint32 pattern_size)
{
  Output_Sorter output_sorter;
  
  Block_Backend< Tag_Index_Local, Uint32_Index > nodes_local_db
      (*de_osm3s_file_ids::NODE_TAGS_LOCAL, false);
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
      it(nodes_local_db.flat_begin());
      !(it == nodes_local_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed(it.index().index);
    ostringstream buf;
    buf<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_node_tags_global(uint32 pattern_size)
{
  Output_Sorter_Kv output_sorter;
  
  Block_Backend< Tag_Index_Global, Uint32_Index > nodes_global_db
  (*de_osm3s_file_ids::NODE_TAGS_GLOBAL, false);
  for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
      it(nodes_global_db.flat_begin());
      !(it == nodes_global_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed
        (make_pair(it.index().key, it.index().value));
    ostringstream buf;
    buf<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_ways(uint32 pattern_size)
{
  Output_Sorter output_sorter;
  
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (*de_osm3s_file_ids::WAYS, false);
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
      it(ways_db.flat_begin()); !(it == ways_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed(it.index().val());
    ostringstream buf;
    buf<<hex<<it.index().val()<<dec
        <<'\t'<<it.object().id<<'\t';
    for (uint i(0); i < it.object().nds.size(); ++i)
      buf<<it.object().nds[i]<<' ';
    buf<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  } 
}

void dump_way_tags_local(uint32 pattern_size)
{
  Output_Sorter output_sorter;
  
  Block_Backend< Tag_Index_Local, Uint32_Index > ways_local_db
      (*de_osm3s_file_ids::WAY_TAGS_LOCAL, false);
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
      it(ways_local_db.flat_begin());
      !(it == ways_local_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed(it.index().index);
    ostringstream buf;
    buf<<hex<<it.index().index<<dec
        <<'\t'<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_way_tags_global(uint32 pattern_size)
{
  Output_Sorter_Kv output_sorter;
  
  Block_Backend< Tag_Index_Global, Uint32_Index > ways_global_db
      (*de_osm3s_file_ids::WAY_TAGS_GLOBAL, false);
  for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
      it(ways_global_db.flat_begin());
      !(it == ways_global_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed
        (make_pair(it.index().key, it.index().value));
    ostringstream buf;
    buf<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_relations(uint32 pattern_size)
{
  Output_Sorter output_sorter;
  
  map< uint32, string > roles;
  Block_Backend< Uint32_Index, String_Object > roles_db
      (*de_osm3s_file_ids::RELATION_ROLES, true);
  for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
      it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
    roles[it.index().val()] = it.object().val();
  
  Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
      (*de_osm3s_file_ids::RELATIONS, false);
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
      it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed(it.index().val());
    ostringstream buf;
    buf<<hex<<it.index().val()<<dec
        <<'\t'<<it.object().id<<'\t';
    for (uint i(0); i < it.object().members.size(); ++i)
      buf<<it.object().members[i].ref<<' '
          <<it.object().members[i].type<<' '
          <<roles[it.object().members[i].role]<<' ';
    buf<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_relation_tags_local(uint32 pattern_size)
{
  Output_Sorter output_sorter;
  
  Block_Backend< Tag_Index_Local, Uint32_Index > relations_local_db
  (*de_osm3s_file_ids::RELATION_TAGS_LOCAL, false);
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
    it(relations_local_db.flat_begin());
  !(it == relations_local_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed(it.index().index);
    ostringstream buf;
    buf<<hex<<it.index().index<<dec
        <<'\t'<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

void dump_relation_tags_global(uint32 pattern_size)
{
  Output_Sorter_Kv output_sorter;
  
  Block_Backend< Tag_Index_Global, Uint32_Index > relations_global_db
  (*de_osm3s_file_ids::RELATION_TAGS_GLOBAL, false);
  for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
    it(relations_global_db.flat_begin());
  !(it == relations_global_db.flat_end()); ++it)
  {
    output_sorter.sort_and_output_if_index_changed
        (make_pair(it.index().key, it.index().value));
    ostringstream buf;
    buf<<it.object().val()<<'\t'
        <<it.index().key<<'\t'<<it.index().value<<'\n';
    output_sorter.output_per_index.push_back(buf.str());
  }
}

int main(int argc, char* args[])
{
  // read command line arguments
  string db_dir;
  uint32 pattern_size = 0;
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(args[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)args[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
      set_basedir(db_dir);
    }
    else if (!(strncmp(args[argpos], "--pattern_size=", 15)))
      pattern_size = atol(((string)args[argpos]).substr(15).c_str());
    ++argpos;
  }
  
  if (pattern_size == 0)
  {
    cerr<<"Pattern size must be nonzero.\n";
    return -1;
  }
  
  try
  {
    dump_nodes(pattern_size);
    dump_node_tags_local(pattern_size);
    dump_node_tags_global(pattern_size);
    dump_ways(pattern_size);
    dump_way_tags_local(pattern_size);
    dump_way_tags_global(pattern_size);
    dump_relations(pattern_size);
    dump_relation_tags_local(pattern_size);
    dump_relation_tags_global(pattern_size);
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
