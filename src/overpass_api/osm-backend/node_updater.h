#ifndef DE_OSM3S__OSM_BACKEND__NODE_UPDATER
#define DE_OSM3S__OSM_BACKEND__NODE_UPDATER

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"

using namespace std;

struct Node_Updater
{
  Node_Updater();
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
  }
  
  void set_node
      (uint32 id, uint32 lat, uint32 lon,
       const vector< pair< string, string > >& tags)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Node node;
    node.id = id;
    node.ll_upper_ = Node::ll_upper(lat, lon);
    node.ll_lower_ = Node::ll_lower(lat, lon);
    node.tags = tags;
    nodes_to_insert.push_back(node);
  }
  
  void set_node(const Node& node)
  {
    ids_to_modify.push_back(make_pair(node.id, true));
    nodes_to_insert.push_back(node);
  }
  
  void update(Osm_Backend_Callback* callback, bool partial = false);
  
  const vector< pair< uint32, uint32 > >& get_moved_nodes() const
  {
    return moved_nodes;
  }
  
private:
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Node > nodes_to_insert;
  static Node_Comparator_By_Id node_comparator_by_id;
  static Node_Equal_Id node_equal_id;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  uint32 update_counter;
  bool map_file_existed_before;
  vector< pair< uint32, uint32 > > moved_nodes;
  
  void update_node_ids(map< uint32, vector< uint32 > >& to_delete);
  
  void update_coords(const map< uint32, vector< uint32 > >& to_delete);
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
       
  void update_node_tags_local(const vector< Tag_Entry >& tags_to_delete);
  
  void update_node_tags_global(const vector< Tag_Entry >& tags_to_delete);
  
  void merge_files(string from, string into);
};

#endif
