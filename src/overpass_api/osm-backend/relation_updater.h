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

struct Relation_Updater
{
  Relation_Updater(Transaction& transaction, bool meta);
  
  Relation_Updater(string db_dir, bool meta);
  
  void set_id_deleted(uint32 id)
  {
    ids_to_modify.push_back(make_pair(id, false));
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
      OSM_Element_Metadata_Skeleton meta_skel;
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
      OSM_Element_Metadata_Skeleton meta_skel;
      meta_skel.ref= rel.id;
      meta_skel.version = meta->version;
      meta_skel.changeset = meta->changeset;
      meta_skel.timestamp = meta->timestamp;
      meta_skel.user_id = meta->user_id;
      rels_meta_to_insert.push_back(make_pair(meta_skel, 0));
    }
  }
  
  uint32 get_role_id(const string& s);
  
  void update(Osm_Backend_Callback* callback);
  
  void update_moved_idxs(const vector< pair< uint32, uint32 > >& moved_nodes,
			 const vector< pair< uint32, uint32 > >& moved_ways);
  
private:
  uint32 update_counter;
  Transaction* transaction;
  bool external_transaction;
  map< string, uint32 > role_ids;
  uint32 max_role_id;
  uint32 max_written_role_id;
  vector< pair< uint32, bool > > ids_to_modify;
  vector< Relation > rels_to_insert;
  static Pair_Comparator_By_Id pair_comparator_by_id;
  static Pair_Equal_Id pair_equal_id;
  vector< pair< uint32, uint32 > > moved_relations;
  string db_dir;

  bool meta;
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > > rels_meta_to_insert;
  map< uint32, string > user_by_id;
  
  void find_affected_relations(const vector< pair< uint32, uint32 > >& moved_nodes,
			       const vector< pair< uint32, uint32 > >& moved_ways);
  
  void compute_indexes(vector< Relation* >& rels_ptr);
  
  void update_rel_ids(vector< Relation* >& rels_ptr, map< uint32, vector< uint32 > >& to_delete);
  
  void update_members(vector< Relation* >& rels_ptr,
		      const map< uint32, vector< uint32 > >& to_delete);
  
  void flush_roles();
};

#endif
