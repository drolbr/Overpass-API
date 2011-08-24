#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_UPDATER_H

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
  Way_Updater(Transaction& transaction, bool meta);
  
  Way_Updater(string db_dir, bool meta);
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_way
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< uint32 > nds,
       const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Way way;
    way.id = id;
    way.nds = nds;
    way.tags = tags;
    ways_to_insert.push_back(way);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= way.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      ways_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
  }
  
  void set_way(const Way& way,
	       const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(way.id, true));
    ways_to_insert.push_back(way);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= way.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      ways_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
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
  vector< pair< uint32, uint32 > > moved_ways;
  string db_dir;
  
  bool meta;
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > > ways_meta_to_insert;
  map< uint32, string > user_by_id;
  
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
  
  void merge_files(const vector< string >& froms, string into);
};

#endif
