#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_UPDATER_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

using namespace std;

struct Node_Updater
{
  Node_Updater(Transaction& transaction, bool meta);
  
  Node_Updater(string db_dir, bool meta);
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_node
      (uint32 id, uint32 lat, uint32 lon,
       const vector< pair< string, string > >& tags,
       const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Node node;
    node.id = id;
    node.ll_upper = Node::ll_upper_(lat, lon);
    node.ll_lower_ = Node::ll_lower(lat, lon);
    node.tags = tags;
    nodes_to_insert.push_back(node);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= node.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      nodes_meta_to_insert.push_back(make_pair(meta_skel, node.ll_upper));
    }
  }
  
  void set_node(const Node& node, const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(node.id, true));
    nodes_to_insert.push_back(node);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= node.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      nodes_meta_to_insert.push_back(make_pair(meta_skel, node.ll_upper));
    }
  }
  
  void update(Osm_Backend_Callback* callback, bool partial = false);
  
  const vector< pair< uint32, uint32 > >& get_moved_nodes() const
  {
    return moved_nodes;
  }
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Node > nodes_to_insert;
  static Node_Comparator_By_Id node_comparator_by_id;
  static Node_Equal_Id node_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  vector< pair< uint32, uint32 > > moved_nodes;
  string db_dir;

  bool meta;
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > > nodes_meta_to_insert;
  map< uint32, string > user_by_id;
  
  void update_node_ids(map< uint32, vector< uint32 > >& to_delete);
  
  void update_coords(const map< uint32, vector< uint32 > >& to_delete);
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
       
  void update_node_tags_local(const vector< Tag_Entry >& tags_to_delete);
  
  void update_node_tags_global(const vector< Tag_Entry >& tags_to_delete);
  
  void merge_files(const vector< string >& froms, string into);
};

#endif
