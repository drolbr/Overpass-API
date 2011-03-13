#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"

using namespace std;

/**
 * Tests the library node_updater with a sample OSM file
 */

// void show_mem_status()
// {
//   ostringstream proc_file_name_("");
//   proc_file_name_<<"/proc/"<<getpid()<<"/stat";
//   ifstream stat(proc_file_name_.str().c_str());
//   while (stat.good())
//   {
//     string line;
//     getline(stat, line);
//     cerr<<line;
//   }
//   cerr<<'\n';
// }

int main(int argc, char* args[])
{
  try
  {
    cerr<<time(NULL)<<' ';
    
    // formulate range query
    set< pair< Tag_Index_Global, Tag_Index_Global > > range_set;
    Tag_Index_Global lower, upper;
    lower.key = "addr:housenumber";
    lower.value = "";
    upper.key = "addr:housenumber ";
    upper.key[16] = 1;
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
    
    // get ids of nodes with this tag
    vector< uint32 > node_ids;
    Block_Backend< Tag_Index_Global, Uint32_Index > node_tags_db
	(*de_osm3s_file_ids::NODE_TAGS_GLOBAL, false);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Range_Iterator
	 it(node_tags_db.range_begin
	     (Default_Range_Iterator< Tag_Index_Global >(range_set.begin()),
	      Default_Range_Iterator< Tag_Index_Global >(range_set.end())));
	 !(it == node_tags_db.range_end()); ++it)
      node_ids.push_back(it.object().val());
    
    cerr<<time(NULL)<<' ';
    sort(node_ids.begin(), node_ids.end());
    cerr<<time(NULL)<<' ';
    
    // get indexes of nodes
    map< uint32, vector< uint32 > > node_idxs;
    Random_File< Uint32_Index > random(*de_osm3s_file_ids::NODES, false);
    for (vector< uint32 >::const_iterator it(node_ids.begin());
	 it != node_ids.end(); ++it)
      node_idxs[random.get(*it).val()].push_back(*it);

    cerr<<time(NULL)<<' ';
    // get node_skeletons - formulate discrete iterator
    set< Uint32_Index > req;
    for (map< uint32, vector< uint32 > >::const_iterator it(node_idxs.begin());
	it != node_idxs.end(); ++it)
      req.insert(it->first);
    cerr<<time(NULL)<<' ';

    // get node_skeletons
    Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
	(*de_osm3s_file_ids::NODES, false);
    for (Block_Backend< Uint32_Index, Node_Skeleton >::Discrete_Iterator
	 it(nodes_db.discrete_begin(req.begin(), req.end()));
	 !(it == nodes_db.discrete_end()); ++it)
    {
      cout<<dec<<it.object().id<<'\t'<<hex
	  <<it.index().val()<<'\t'<<it.object().ll_lower<<'\n';
    }
    
/*    Block_Backend< Tag_Index_Global, Uint32_Index > nodes_global_db
	(de_osm3s_file_ids::NODE_TAGS_GLOBAL, false);
    set< Tag_Index_Global > req;
    Tag_Index_Global req_n;
    req_n.key = "attribution";
    req_n.value = "Natural Resources Canada";
    req.insert(req_n);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Discrete_Iterator
	 it(nodes_global_db.discrete_begin(req.begin(), req.end()));
	 !(it == nodes_global_db.discrete_end()); ++it)
    {
      cout<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';*/
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
