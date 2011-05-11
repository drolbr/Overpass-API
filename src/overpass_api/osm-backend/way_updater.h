#ifndef DE_OSM3S__BACKEND__WAY_UPDATER
#define DE_OSM3S__BACKEND__WAY_UPDATER

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

using namespace std;

struct Way_Updater
{
  Way_Updater(Transaction* transaction);
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_way
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< uint32 > nds)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Way way;
    way.id = id;
    way.nds = nds;
    way.tags = tags;
    ways_to_insert.push_back(way);
  }
  
  void set_way(const Way& way)
  {
    ids_to_modify.push_back(make_pair(way.id, true));
    ways_to_insert.push_back(way);
  }
  
  void update(Osm_Backend_Callback* callback, bool partial = false);
  
  void update_moved_idxs
      (Osm_Backend_Callback* callback,
       const vector< pair< uint32, uint32 > >& moved_nodes);
  
  const vector< pair< uint32, uint32 > >& get_moved_ways() const
  {
    return moved_ways;
  }
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Way > ways_to_insert;
  static Way_Comparator_By_Id way_comparator_by_id;
  static Way_Equal_Id way_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  bool map_file_existed_before;
  vector< pair< uint32, uint32 > > moved_ways;
  
  void filter_affected_ways(const vector< Way >& maybe_affected_ways);
  
  void find_affected_ways(const vector< pair< uint32, uint32 > >& moved_nodes);
  
  void compute_indexes();

  void update_way_ids(map< uint32, vector< uint32 > >& to_delete);
  
  void update_members(const map< uint32, vector< uint32 > >& to_delete);
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
       
  void prepare_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
  
  void update_way_tags_local(const vector< Tag_Entry >& tags_to_delete);
  
  void update_way_tags_global(const vector< Tag_Entry >& tags_to_delete);
  
  void merge_files(string from, string into);
};

#endif
