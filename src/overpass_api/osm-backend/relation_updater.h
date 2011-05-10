#ifndef DE_OSM3S__BACKEND__RELATION_UPDATER
#define DE_OSM3S__BACKEND__RELATION_UPDATER

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

struct Relation_Updater
{
  Relation_Updater(Transaction* transaction);
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_relation
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< Relation_Entry >& members)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Relation rel;
    rel.id = id;
    rel.members = members;
    rel.tags = tags;
    rels_to_insert.push_back(rel);
  }
  
  void set_relation(const Relation& rel)
  {
    ids_to_modify.push_back(make_pair(rel.id, true));
    rels_to_insert.push_back(rel);
  }
  
  uint32 get_role_id(const string& s);
  
  void update(Osm_Backend_Callback* callback);
  
  void update_moved_idxs(const vector< pair< uint32, uint32 > >& moved_nodes,
			 const vector< pair< uint32, uint32 > >& moved_ways);
  
private:
  Transaction* transaction;
  map< string, uint32 > role_ids;
  uint32 max_written_role_id;
  uint32 max_role_id;
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Relation > rels_to_insert;
  static Relation_Comparator_By_Id rel_comparator_by_id;
  static Relation_Equal_Id rel_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  bool map_file_existed_before;
  vector< pair< uint32, uint32 > > moved_relations;
  
  void find_affected_relations(const vector< pair< uint32, uint32 > >& moved_nodes,
			       const vector< pair< uint32, uint32 > >& moved_ways);
  
  void compute_indexes();
  
  void update_rel_ids(map< uint32, vector< uint32 > >& to_delete);
  
  void update_members(const map< uint32, vector< uint32 > >& to_delete);
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
       
  void prepare_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
       
  void update_rel_tags_local(const vector< Tag_Entry >& tags_to_delete);
  
  void update_rel_tags_global(const vector< Tag_Entry >& tags_to_delete);

  void flush_roles();
};

#endif
