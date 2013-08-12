/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
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
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = insert.find(node.id);
    if (it == insert.end())
      insert.insert(make_pair(node.id, make_pair(node, (OSM_Element_Metadata*)0)));
    else
      it->second.first = node;
  }
  
  void insertion(Node::Id_Type id, const OSM_Element_Metadata& meta)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = insert.find(id);
    if (it != insert.end())
    {
      if (it->second.second)
        delete it->second.second;
      it->second.second = new OSM_Element_Metadata(meta);
    }
  }
  
  void deletion(const Uint32_Index& index, const Node_Skeleton& skel)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = erase.find(skel.id);
    if (it == erase.end())
    {
      erase.insert(make_pair(skel.id, make_pair< Node, OSM_Element_Metadata* >
          (Node(skel.id.val(), index.val(), skel.ll_lower), 0)));
    }
    else
      it->second.first = Node(skel.id.val(), index.val(), skel.ll_lower);
  }
  
  void keeping(const Uint32_Index& index, const Node_Skeleton& skel)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = keep.find(skel.id);
    if (it == keep.end())
    {
      keep.insert(make_pair(skel.id, make_pair< Node, OSM_Element_Metadata* >
          (Node(skel.id.val(), index.val(), skel.ll_lower), 0)));
    }
    else
      it->second.first = Node(skel.id.val(), index.val(), skel.ll_lower);
  }
  
  void deletion(const Tag_Index_Local& index, const Node::Id_Type& ref)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = erase.find(ref);
    if (it != erase.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
  }
  
  void keeping(const Tag_Index_Local& index, const Node::Id_Type& ref)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = keep.find(ref);
    if (it != keep.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
  }
  
  void deletion(const Uint31_Index& index,
		const OSM_Element_Metadata_Skeleton< Node::Id_Type >& meta_skel)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = erase.find(meta_skel.ref);
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
  
  void keeping(const Uint31_Index& index,
	       const OSM_Element_Metadata_Skeleton< Node::Id_Type >& meta_skel)
  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator it = keep.find(meta_skel.ref);
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
  
  void set_delete_meta_data(const vector< OSM_Element_Metadata_Skeleton< Node::Id_Type > >& meta_to_delete_)
  {
    for (vector< OSM_Element_Metadata_Skeleton< Node::Id_Type > >::const_iterator
        it = meta_to_delete_.begin(); it != meta_to_delete_.end(); ++it)
    {
      OSM_Element_Metadata meta;
      meta.version = it->version;
      meta.timestamp = it->timestamp;
      meta.changeset = it->changeset;
      meta.user_id = it->user_id;
      meta_to_delete.push_back(make_pair(it->ref, meta));
      meta_to_delete_sorted = false;
    }
  }
  
  void request_user_names(map< uint32, string >& user_names)
  {
    for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
        it = insert_begin(); it != insert_end(); ++it)
    {
      if (it->second.second)
        user_names[it->second.second->user_id];
    }
    for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
        it = keep_begin(); it != keep_end(); ++it)
    {
      if (it->second.second)
        user_names[it->second.second->user_id];
    }
    for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
        it = erase_begin(); it != erase_end(); ++it)
    {
      if (it->second.second)
        user_names[it->second.second->user_id];
    }
    
    for (vector< pair< Node::Id_Type, OSM_Element_Metadata > >::const_iterator
        it = meta_to_delete.begin(); it != meta_to_delete.end(); ++it)
      user_names[it->second.user_id];
  }

  void set_user_names(map< uint32, string >& user_names)
  {
    for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator
        it = insert_begin(); it != insert_end(); ++it)
    {
      if (it->second.second)
        it->second.second->user_name = user_names[it->second.second->user_id];
    }  
    for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator
        it = keep_begin(); it != keep_end(); ++it)
    {
      if (it->second.second)
        it->second.second->user_name = user_names[it->second.second->user_id];
    }  
    for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator
        it = erase_begin(); it != erase_end(); ++it)
    {
      if (it->second.second)
        it->second.second->user_name = user_names[it->second.second->user_id];
    }  
    
    for (vector< pair< Node::Id_Type, OSM_Element_Metadata > >::iterator
        it = meta_to_delete.begin(); it != meta_to_delete.end(); ++it)
      it->second.user_name = user_names[it->second.user_id];
  }

  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator insert_begin() const
  { return insert.begin(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator insert_end() const
  { return insert.end(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator keep_begin() const
  { return keep.begin(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator keep_end() const
  { return keep.end(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator erase_begin() const
  { return erase.begin(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator erase_end() const
  { return erase.end(); }

  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator insert_begin()
  { return insert.begin(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator insert_end()
  { return insert.end(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator keep_begin()
  { return keep.begin(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator keep_end()
  { return keep.end(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator erase_begin()
  { return erase.begin(); }
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator erase_end()
  { return erase.end(); }

  const Node* get_erased(Node::Id_Type ref) const;
  const Node* get_inserted(Node::Id_Type ref) const;
  
  const OSM_Element_Metadata* get_erased_meta(Node::Id_Type ref)
  {
    if (!meta_to_delete_sorted)
    {
      sort(meta_to_delete.begin(), meta_to_delete.end());
      meta_to_delete_sorted = true;
    }
    return binary_pair_search< Node::Id_Type, OSM_Element_Metadata >(meta_to_delete, ref);    
  }
  
  ~Update_Node_Logger();
  
private:
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > > insert;
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > > keep;
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > > erase;
  
  vector< pair< Node::Id_Type, OSM_Element_Metadata > > meta_to_delete;
  bool meta_to_delete_sorted;
};


inline const Node* Update_Node_Logger::get_erased(Node::Id_Type ref) const
{
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = erase.find(ref);
  if (it != erase.end())
    return &it->second.first;
  
  it = keep.find(ref);
  if (it != keep.end())
    return &it->second.first;
  
  return 0;
}


inline const Node* Update_Node_Logger::get_inserted(Node::Id_Type ref) const
{
  map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = insert.find(ref);
  if (it != insert.end())
    return &it->second.first;
  
  it = keep.find(ref);
  if (it != keep.end())
    return &it->second.first;
  
  return 0;
}


struct Node_Updater
{
  Node_Updater(Transaction& transaction, bool meta);
  
  Node_Updater(string db_dir, bool meta);
  
  void set_id_deleted(Node::Id_Type id, const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(id, false));
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton< Node::Id_Type > meta_skel(id, *meta);
      nodes_meta_to_delete.push_back(meta_skel);
    }
  }
  
  
  void set_node(const Node& node, const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(node.id, true));
    nodes_to_insert.push_back(node);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton< Node::Id_Type > meta_skel(node.id, *meta);
      nodes_meta_to_insert.push_back(make_pair(meta_skel, node.index));
    }
  }
  
  void update(Osm_Backend_Callback* callback, bool partial,
			  Update_Node_Logger* update_logger);
  
  const vector< pair< Node::Id_Type, Uint32_Index > >& get_moved_nodes() const
  {
    return moved_nodes;
  }
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  bool partial_possible;
  static Node_Comparator_By_Id node_comparator_by_id;
  static Node_Equal_Id node_equal_id;
  string db_dir;

  vector< pair< Node::Id_Type, bool > > ids_to_modify;
  vector< Node > nodes_to_insert;
  vector< pair< Node::Id_Type, Uint32_Index > > moved_nodes;

  bool meta;
  vector< pair< OSM_Element_Metadata_Skeleton< Node::Id_Type >, uint32 > > nodes_meta_to_insert;
  vector< OSM_Element_Metadata_Skeleton< Node::Id_Type > > nodes_meta_to_delete;
  map< uint32, string > user_by_id;
  
  void update_node_ids(map< uint32, vector< Node::Id_Type > >& to_delete, bool record_minuscule_moves);
  
  void update_coords(const map< uint32, vector< Node::Id_Type > >& to_delete,
		     Update_Node_Logger* update_logger);
  
  void merge_files(const vector< string >& froms, string into);
};


#endif
