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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__RELATION_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__RELATION_UPDATER_H

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


struct Update_Relation_Logger
{
public:
  void insertion(const Relation& relation)
  {
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator it = insert.find(relation.id);
    if (it == insert.end())
      insert.insert(make_pair(relation.id, make_pair< Relation, OSM_Element_Metadata* >(relation, 0)));
    else
      it->second.first = relation;
    
    it = keep.find(relation.id);
    if (it != keep.end())
      keep.erase(it);
  }
  
  void insertion(Relation::Id_Type id, const OSM_Element_Metadata& meta)
  {
    if (insert[id].second)
      delete insert[id].second;
    insert[id].second = new OSM_Element_Metadata(meta);
  }
  
  void deletion(const Uint31_Index& index, const Relation_Skeleton& skel)
  {
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator it = erase.find(skel.id);
    if (it == erase.end())
    {
      it = erase.insert(make_pair(skel.id, make_pair< Relation, OSM_Element_Metadata* >
          (Relation(skel.id.val()), 0))).first;
    }
    else
      it->second.first = Relation(skel.id.val());
    it->second.first.members = skel.members;
    it->second.first.index = index.val();
    it->second.first.node_idxs = skel.node_idxs;
    it->second.first.way_idxs = skel.way_idxs;

    it = keep.find(skel.id);
    if (it != keep.end())
      keep.erase(it);
  }
  
  void keeping(const Uint31_Index& index, const Relation_Skeleton& skel)
  {
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator it = erase.find(skel.id);
    if (it != erase.end())
      return;
    
    it = insert.find(skel.id);
    if (it != insert.end())
      return;
    
    it = keep.find(skel.id);
    if (it == keep.end())
    {
      it = keep.insert(make_pair(skel.id, make_pair< Relation, OSM_Element_Metadata* >
          (Relation(skel.id.val()), 0))).first;
    }
    else
      it->second.first = Relation(skel.id.val());
    it->second.first.members = skel.members;
    it->second.first.index = index.val();
    it->second.first.node_idxs = skel.node_idxs;
    it->second.first.way_idxs = skel.way_idxs;
  }
  
  void deletion(const Tag_Index_Local& index, const Uint32_Index& ref)
  {
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator it = erase.find(ref.val());
    if (it != erase.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
    it = keep.find(ref.val());
    if (it != keep.end())
      it->second.first.tags.push_back(make_pair(index.key, index.value));
  }
  
  void deletion(const Uint31_Index& index,
		const OSM_Element_Metadata_Skeleton< Relation::Id_Type >& meta_skel)
  {
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator it = erase.find(meta_skel.ref);
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
    it = keep.find(meta_skel.ref);
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
  
  void set_delete_meta_data(const vector< OSM_Element_Metadata_Skeleton< Relation::Id_Type > >& meta_to_delete_)
  {
    for (typename vector< OSM_Element_Metadata_Skeleton< Relation::Id_Type > >::const_iterator
        it = meta_to_delete_.begin(); it != meta_to_delete_.end(); ++it)
    {
      OSM_Element_Metadata meta;
      meta.version = it->version;
      meta.timestamp = it->timestamp;
      meta.changeset = it->changeset;
      meta.user_id = it->user_id;
      meta_to_delete.push_back(make_pair(it->ref, meta));
    }
  }
  
  void request_user_names(map< uint32, string >& user_names)
  {
    for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
        it = insert_begin(); it != insert_end(); ++it)
    {
      if (it->second.second)
        user_names[it->second.second->user_id];
    }
    for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
        it = keep_begin(); it != keep_end(); ++it)
    {
      if (it->second.second)
        user_names[it->second.second->user_id];
    }
    for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
        it = erase_begin(); it != erase_end(); ++it)
    {
      if (it->second.second)
        user_names[it->second.second->user_id];
    }
    
    for (vector< pair< Relation::Id_Type, OSM_Element_Metadata > >::const_iterator
        it = meta_to_delete.begin(); it != meta_to_delete.end(); ++it)
      user_names[it->second.user_id];
  }
  
  void set_user_names(map< uint32, string >& user_names)
  {
    for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator
        it = insert_begin(); it != insert_end(); ++it)
    {
      if (it->second.second)
        it->second.second->user_name = user_names[it->second.second->user_id];
    }  
    for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator
        it = keep_begin(); it != keep_end(); ++it)
    {
      if (it->second.second)
        it->second.second->user_name = user_names[it->second.second->user_id];
    }  
    for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator
        it = erase_begin(); it != erase_end(); ++it)
    {
      if (it->second.second)
        it->second.second->user_name = user_names[it->second.second->user_id];
    }  
    
    for (vector< pair< Relation::Id_Type, OSM_Element_Metadata > >::iterator
        it = meta_to_delete.begin(); it != meta_to_delete.end(); ++it)
      it->second.user_name = user_names[it->second.user_id];
  }

  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator insert_begin() const
  { return insert.begin(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator insert_end() const
  { return insert.end(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator keep_begin() const
  { return keep.begin(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator keep_end() const
  { return keep.end(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator erase_begin() const
  { return erase.begin(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator erase_end() const
  { return erase.end(); }

  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator insert_begin()
  { return insert.begin(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator insert_end()
  { return insert.end(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator keep_begin()
  { return keep.begin(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator keep_end()
  { return keep.end(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator erase_begin()
  { return erase.begin(); }
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator erase_end()
  { return erase.end(); }
  
  const OSM_Element_Metadata* get_erased_meta(Relation::Id_Type ref) const
  { return binary_pair_search(meta_to_delete, ref); }

  ~Update_Relation_Logger();
  
private:
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > > insert;
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > > keep;
  map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > > erase;
  vector< pair< Relation::Id_Type, OSM_Element_Metadata > > meta_to_delete;
};


struct Relation_Updater
{
  Relation_Updater(Transaction& transaction, bool meta);
  
  Relation_Updater(string db_dir, bool meta);
  
  void set_id_deleted(Relation::Id_Type id, const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(id, false));
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton< Relation::Id_Type > meta_skel;
      meta_skel.ref = id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      rels_meta_to_delete.push_back(meta_skel);
    }
  }
  
  void set_relation
      (uint32 id, uint32 lat, uint32 lon, const vector< pair< string, string > >& tags,
       const vector< Relation_Entry >& members,
       const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(id, true));
    
    Relation rel;
    rel.id = id;
    rel.members = members;
    rel.tags = tags;
    rels_to_insert.push_back(rel);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton< Relation::Id_Type > meta_skel;
      meta_skel.ref= rel.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      rels_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
  }
  
  void set_relation(const Relation& rel,
		    const OSM_Element_Metadata* meta = 0)
  {
    ids_to_modify.push_back(make_pair(rel.id, true));
    rels_to_insert.push_back(rel);
    if (meta)
    {
      user_by_id[meta->user_id] = meta->user_name;
      OSM_Element_Metadata_Skeleton< Relation::Id_Type > meta_skel;
      meta_skel.ref= rel.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      rels_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
  }
  
  uint32 get_role_id(const string& s);
  vector< string > get_roles();
  
  void update(Osm_Backend_Callback* callback, Update_Relation_Logger* update_logger);
  
  void update_moved_idxs(const vector< pair< Node::Id_Type, Uint32_Index > >& moved_nodes,
			 const vector< pair< Way::Id_Type, Uint31_Index > >& moved_ways,
			 Update_Relation_Logger* update_logger);
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  map< string, uint32 > role_ids;
  uint32 max_role_id;
  uint32 max_written_role_id;
  vector< pair< Relation::Id_Type, bool > > ids_to_modify;
  vector< Relation > rels_to_insert;
  vector< pair< Relation::Id_Type, uint32 > > moved_relations;
  string db_dir;

  bool meta;
  vector< pair< OSM_Element_Metadata_Skeleton< Relation::Id_Type >, uint32 > > rels_meta_to_insert;
  vector< OSM_Element_Metadata_Skeleton< Relation::Id_Type > > rels_meta_to_delete;
  map< uint32, string > user_by_id;
  
  void find_affected_relations(const vector< pair< Node::Id_Type, Uint32_Index > >& moved_nodes,
			       const vector< pair< Way::Id_Type, Uint31_Index > >& moved_ways,
			       Update_Relation_Logger* update_logger);
  
  void compute_indexes(vector< Relation* >& rels_ptr);
  
  void update_rel_ids(vector< Relation* >& rels_ptr, map< uint32, vector< Relation::Id_Type > >& to_delete);
  
  void update_members(vector< Relation* >& rels_ptr,
		      const map< uint32, vector< Relation::Id_Type > >& to_delete,
		      Update_Relation_Logger* update_logger);
  
  void flush_roles();
  void load_roles();
};

#endif
