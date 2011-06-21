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
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

using namespace std;

// not const - sorts its container first and clears it afterwards.
void clear_nodes_to_map(vector< pair< uint32, Uint32_Index > >& id_to_idx,
			Nonsynced_Transaction& transaction)
{
  bool deviations_exist = false;
  
  sort(id_to_idx.begin(), id_to_idx.end());
  
  Random_File< Uint32_Index > random
      (transaction.random_index(osm_base_settings().NODES));

  for (vector< pair< uint32, Uint32_Index > >::const_iterator it(id_to_idx.begin());
      it != id_to_idx.end(); ++it)
  {
    if (!(random.get(it->first) == it->second))
    {
      deviations_exist = true;
      cout<<"Node "<<it->first<<":\tmap = "<<random.get(it->first).val()
          <<",\tbin = "<<it->second.val()<<'\n';
    }
  }
  
  if (!deviations_exist)
    cout<<"For "<<id_to_idx.size()
        <<" nodes their idx in bin matches their idx in map.\n";
      
  id_to_idx.clear();
}

void dump_nodes(string db_dir)
{
  vector< pair< uint32, Uint32_Index > > id_to_idx;
  
  Nonsynced_Transaction transaction(false, false, db_dir, "");
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (transaction.data_index(osm_base_settings().NODES));
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
      it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
  {
    id_to_idx.push_back(make_pair(it.object().id, it.index()));
    if (id_to_idx.size() > 16*1024*1024)
      clear_nodes_to_map(id_to_idx, transaction);
  }
  clear_nodes_to_map(id_to_idx, transaction);
}

// not const - sorts its container first and clears it afterwards.
void clear_ways_to_map(vector< pair< uint32, Uint31_Index > >& id_to_idx,
			Nonsynced_Transaction& transaction)
{
  bool deviations_exist = false;
  
  sort(id_to_idx.begin(), id_to_idx.end());
  
  Random_File< Uint31_Index > random
      (transaction.random_index(osm_base_settings().WAYS));

  for (vector< pair< uint32, Uint31_Index > >::const_iterator it(id_to_idx.begin());
      it != id_to_idx.end(); ++it)
  {
    if (!(random.get(it->first) == it->second))
    {
      deviations_exist = true;
      cout<<"Way "<<it->first<<":\tmap = "<<random.get(it->first).val()
          <<",\tbin = "<<it->second.val()<<'\n';
    }
  }
  
  if (!deviations_exist)
    cout<<"For "<<id_to_idx.size()
        <<" ways their idx in bin matches their idx in map.\n";
      
  id_to_idx.clear();
}

void dump_ways(string db_dir)
{
  vector< pair< uint32, Uint31_Index > > id_to_idx;
  
  Nonsynced_Transaction transaction(false, false, db_dir, "");
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (transaction.data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
      it(ways_db.flat_begin()); !(it == ways_db.flat_end()); ++it)
  {
    id_to_idx.push_back(make_pair(it.object().id, it.index()));
    if (id_to_idx.size() > 16*1024*1024)
      clear_ways_to_map(id_to_idx, transaction);
  }
  clear_ways_to_map(id_to_idx, transaction);
}

// not const - sorts its container first and clears it afterwards.
void clear_relations_to_map(vector< pair< uint32, Uint31_Index > >& id_to_idx,
			Nonsynced_Transaction& transaction)
{
  bool deviations_exist = false;
  
  sort(id_to_idx.begin(), id_to_idx.end());
  
  Random_File< Uint31_Index > random
      (transaction.random_index(osm_base_settings().RELATIONS));

  for (vector< pair< uint32, Uint31_Index > >::const_iterator it(id_to_idx.begin());
      it != id_to_idx.end(); ++it)
  {
    if (!(random.get(it->first) == it->second))
    {
      deviations_exist = true;
      cout<<"Relation "<<it->first<<":\tmap = "<<random.get(it->first).val()
          <<",\tbin = "<<it->second.val()<<'\n';
    }
  }
  
  if (!deviations_exist)
    cout<<"For "<<id_to_idx.size()
        <<" relations their idx in bin matches their idx in map.\n";
      
  id_to_idx.clear();
}

void dump_relations(string db_dir)
{
  vector< pair< uint32, Uint31_Index > > id_to_idx;
  
  Nonsynced_Transaction transaction(false, false, db_dir, "");
  Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
      (transaction.data_index(osm_base_settings().RELATIONS));
  for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
      it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
  {
    id_to_idx.push_back(make_pair(it.object().id, it.index()));
    if (id_to_idx.size() > 16*1024*1024)
      clear_relations_to_map(id_to_idx, transaction);
  }
  clear_relations_to_map(id_to_idx, transaction);
}

int main(int argc, char* args[])
{
  // read command line arguments
  string db_dir;
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(args[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)args[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    ++argpos;
  }
  
  try
  {
    dump_nodes(db_dir);
  }
  catch (File_Error e)
  {
    cerr<<"compare_osm_base_maps: File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  try
  {
    dump_ways(db_dir);
  }
  catch (File_Error e)
  {
    cerr<<"compare_osm_base_maps: File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  try
  {
    dump_relations(db_dir);
  }
  catch (File_Error e)
  {
    cerr<<"compare_osm_base_maps: File error caught: "
    <<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
