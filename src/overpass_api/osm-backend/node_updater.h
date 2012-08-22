/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

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

struct Update_Node_Logger
{
public:
  void insertion(const Node& node)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = insert.find(node.id);
    if (it == insert.end())
      insert.insert(make_pair(node.id, make_pair< Node, OSM_Element_Metadata* >(node, 0)));
    else
      it->second.first = node;
  }
  
  void insertion(uint32 id, const OSM_Element_Metadata& meta)
  {
    if (insert[id].second)
      delete insert[id].second;
    insert[id].second = new OSM_Element_Metadata(meta);
  }
  
  void deletion(const Uint32_Index& index, const Node_Skeleton& skel)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = erase.find(skel.id);
    if (it == erase.end())
    {
      erase.insert(make_pair(skel.id, make_pair< Node, OSM_Element_Metadata* >
          (Node(skel.id, index.val(), skel.ll_lower), 0)));
    }
    else
      it->second.first = Node(skel.id, index.val(), skel.ll_lower);
  }
  
  void keeping(const Uint32_Index& index, const Node_Skeleton& skel)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = keep.find(skel.id);
    if (it == keep.end())
    {
      keep.insert(make_pair(skel.id, make_pair< Node, OSM_Element_Metadata* >
          (Node(skel.id, index.val(), skel.ll_lower), 0)));
    }
    else
      it->second.first = Node(skel.id, index.val(), skel.ll_lower);
  }
  
  void deletion(const Tag_Index_Local& index, const Uint32_Index& ref)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = erase.find(ref.val());
    if (it != erase.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
  }
  
  void keeping(const Tag_Index_Local& index, const Uint32_Index& ref)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = keep.find(ref.val());
    if (it != keep.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
  }
  
  void deletion(const Uint31_Index& index, const OSM_Element_Metadata_Skeleton& meta_skel)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = erase.find(meta_skel.ref);
    if (it != erase.end())
    {
      if (it->second.second)
        delete it->second.second;
      OSM_Element_Metadata* meta = new OSM_Element_Metadata();
      meta->version = meta_skel.version;
      meta->timestamp = meta_skel.timestamp;
      meta->changeset = meta_skel.changeset;
      meta->user_id = meta_skel.user_id;
      it->second.second = meta;
    }
  }
  
  void keeping(const Uint31_Index& index, const OSM_Element_Metadata_Skeleton& meta_skel)
  {
    map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator it = keep.find(meta_skel.ref);
    if (it != keep.end())
    {
      if (it->second.second)
        delete it->second.second;
      OSM_Element_Metadata* meta = new OSM_Element_Metadata();
      meta->version = meta_skel.version;
      meta->timestamp = meta_skel.timestamp;
      meta->changeset = meta_skel.changeset;
      meta->user_id = meta_skel.user_id;
      it->second.second = meta;
    }
  }
  
  map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator insert_begin() const
  { return insert.begin(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator insert_end() const
  { return insert.end(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator keep_begin() const
  { return keep.begin(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator keep_end() const
  { return keep.end(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator erase_begin() const
  { return erase.begin(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator erase_end() const
  { return erase.end(); }

  map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator insert_begin()
  { return insert.begin(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator insert_end()
  { return insert.end(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator keep_begin()
  { return keep.begin(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator keep_end()
  { return keep.end(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator erase_begin()
  { return erase.begin(); }
  map< uint32, pair< Node, OSM_Element_Metadata* > >::iterator erase_end()
  { return erase.end(); }

  ~Update_Node_Logger();
  
private:
  map< uint32, pair< Node, OSM_Element_Metadata* > > insert;
  map< uint32, pair< Node, OSM_Element_Metadata* > > keep;
  map< uint32, pair< Node, OSM_Element_Metadata* > > erase;
};


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
      meta_skel.ref = node.id;
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
      meta_skel.ref = node.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      nodes_meta_to_insert.push_back(make_pair(meta_skel, node.ll_upper));
    }
  }
  
  void update(Osm_Backend_Callback* callback, bool partial,
			  Update_Node_Logger* update_logger);
  
  const vector< pair< uint32, uint32 > >& get_moved_nodes() const
  {
    return moved_nodes;
  }
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  bool partial_possible;
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
  
  void update_node_ids(map< uint32, vector< uint32 > >& to_delete, bool record_minuscule_moves);
  
  void update_coords(const map< uint32, vector< uint32 > >& to_delete,
		     Update_Node_Logger* update_logger);
  
  void prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete);
       
  void update_node_tags_local(const vector< Tag_Entry >& tags_to_delete,
		     Update_Node_Logger* update_logger);
  
  void update_node_tags_global(const vector< Tag_Entry >& tags_to_delete);
  
  void merge_files(const vector< string >& froms, string into);
};


// make indices appropriately coarse
map< uint32, set< uint32 > > collect_coarse(const map< uint32, vector< uint32 > >& elems_by_idx);

// formulate range query
set< pair< Tag_Index_Local, Tag_Index_Local > > make_range_set(const map< uint32, set< uint32 > >& coarse);


#endif
