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

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"
#include "node_updater.h"
#include "tags_updater.h"

using namespace std;


Update_Node_Logger::~Update_Node_Logger()
{
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = insert.begin();
      it != insert.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = keep.begin();
      it != keep.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator it = erase.begin();
      it != erase.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
}


Node_Updater::Node_Updater(Transaction& transaction_, bool meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_)
{}

Node_Updater::Node_Updater(string db_dir_, bool meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(true), db_dir(db_dir_), meta(meta_)
{
  partial_possible = !file_exists
      (db_dir + 
       osm_base_settings().NODES->get_file_name_trunk() +
       osm_base_settings().NODES->get_data_suffix() +
       osm_base_settings().NODES->get_index_suffix());
}

void Node_Updater::update(Osm_Backend_Callback* callback, bool partial,
			  Update_Node_Logger* update_logger)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  map< uint32, vector< Node::Id_Type > > to_delete;
  callback->update_started();
  update_node_ids(to_delete, (update_logger != 0));
  callback->update_ids_finished();
  update_coords(to_delete, update_logger);
  callback->update_coords_finished();
  
  if (update_logger && meta)
  {
    for (vector< pair< OSM_Element_Metadata_Skeleton< Node::Id_Type >, uint32 > >::const_iterator
        it = nodes_meta_to_insert.begin(); it != nodes_meta_to_insert.end(); ++it)
    {
      OSM_Element_Metadata meta;
      meta.version = it->first.version;
      meta.timestamp = it->first.timestamp;
      meta.changeset = it->first.changeset;
      meta.user_id = it->first.user_id;
      meta.user_name = user_by_id[it->first.user_id];
      update_logger->insertion(it->first.ref, meta);
    }
  }
  
  Node_Comparator_By_Id node_comparator_by_id;
  Node_Equal_Id node_equal_id;
  
  vector< Tag_Entry< Node::Id_Type > > tags_to_delete;
  prepare_delete_tags(*transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL),
		      tags_to_delete, to_delete);
  callback->prepare_delete_tags_finished();
  vector< Node* > nodes_ptr = sort_elems_to_insert
      (nodes_to_insert, node_comparator_by_id, node_equal_id);
  update_tags_local(*transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL),
		    nodes_ptr, ids_to_modify, tags_to_delete, update_logger);
  callback->tags_local_finished();
  update_tags_global(*transaction->data_index(osm_base_settings().NODE_TAGS_GLOBAL),
		     nodes_ptr, ids_to_modify, tags_to_delete);
  callback->tags_global_finished();
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(nodes_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().NODES_META),
		      nodes_meta_to_insert, ids_to_modify, to_delete, update_logger);
    process_user_data(*transaction, user_by_id, idxs_by_id);
    
    if (update_logger)
    {
      stable_sort(nodes_meta_to_delete.begin(), nodes_meta_to_delete.begin());
      nodes_meta_to_delete.erase(unique(nodes_meta_to_delete.begin(), nodes_meta_to_delete.end()),
				 nodes_meta_to_delete.end());
      update_logger->set_delete_meta_data(nodes_meta_to_delete);
      nodes_meta_to_delete.clear();
    }
  }
  callback->update_finished();
  
  ids_to_modify.clear();
  nodes_to_insert.clear();

  if (!external_transaction)
    delete transaction;
  
  if (partial_possible && !partial && (update_counter > 0))
  {
    callback->partial_started();

    vector< string > froms;
    for (uint i = 0; i < update_counter % 16; ++i)
    {
      string from(".0a");
      from[2] += i;
      froms.push_back(from);
    }
    merge_files(froms, "");
    
    if (update_counter >= 256)
      merge_files(vector< string >(1, ".2"), ".1");
    if (update_counter >= 16)
    {
      vector< string > froms;
      for (uint i = 0; i < update_counter/16 % 16; ++i)
      {
	string from(".1a");
	from[2] += i;
	froms.push_back(from);
      }
      merge_files(froms, ".1");
      
      merge_files(vector< string >(1, ".1"), "");
    }
    update_counter = 0;
    callback->partial_finished();
  }
  else if (partial_possible && partial)
  {
    string to(".0a");
    to[2] += update_counter % 16;
    rename_referred_file(db_dir, "", to, *osm_base_settings().NODES);
    rename_referred_file(db_dir, "", to, *osm_base_settings().NODE_TAGS_LOCAL);
    rename_referred_file(db_dir, "", to, *osm_base_settings().NODE_TAGS_GLOBAL);
    if (meta)
      rename_referred_file(db_dir, "", to, *meta_settings().NODES_META);
    
    ++update_counter;
    if (update_counter % 16 == 0)
    {
      callback->partial_started();
      
      string to(".1a");
      to[2] += (update_counter/16-1) % 16;
      
      vector< string > froms;
      for (uint i = 0; i < 16; ++i)
      {
	string from(".0a");
	from[2] += i;
	froms.push_back(from);
      }
      merge_files(froms, to);
      callback->partial_finished();
    }
    if (update_counter % 256 == 0)
    {
      callback->partial_started();
      
      vector< string > froms;
      for (uint i = 0; i < 16; ++i)
      {
	string from(".1a");
	from[2] += i;
	froms.push_back(from);
      }
      merge_files(froms, ".2");
      callback->partial_finished();
    }
  }
}

void Node_Updater::update_node_ids
    (map< uint32, vector< Node::Id_Type > >& to_delete, bool record_minuscule_moves)
{
  static Pair_Comparator_By_Id< Node::Id_Type, bool > pair_comparator_by_id;
  static Pair_Equal_Id< Node::Id_Type, bool > pair_equal_id;

  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ids_to_modify.begin(), ids_to_modify.end(), pair_comparator_by_id);
  vector< pair< Node::Id_Type, bool > >::iterator modi_begin
      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id).base());
  ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
  stable_sort
      (nodes_to_insert.begin(), nodes_to_insert.end(), node_comparator_by_id);
  vector< Node >::iterator nodes_begin
      (unique(nodes_to_insert.rbegin(), nodes_to_insert.rend(), node_equal_id)
       .base());
  nodes_to_insert.erase(nodes_to_insert.begin(), nodes_begin);
  
  Random_File< Uint32_Index > random
      (transaction->random_index(osm_base_settings().NODES));
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< Node::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    Uint32_Index index(random.get(it->first.val()));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	random.put(it->first.val(), Uint32_Index(nit->index));
	if ((index.val() > 0) &&
	    (index.val() != nit->index || record_minuscule_moves))
	  moved_nodes.push_back(make_pair(it->first, index));
      }
      ++nit;
    }
  }
  sort(moved_nodes.begin(), moved_nodes.end());
}

void Node_Updater::update_coords(const map< uint32, vector< Node::Id_Type > >& to_delete,
				 Update_Node_Logger* update_logger)
{
  map< Uint32_Index, set< Node_Skeleton > > db_to_delete;
  map< Uint32_Index, set< Node_Skeleton > > db_to_insert;
  
  for (map< uint32, vector< Node::Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint32_Index idx(it->first);
    for (vector< Node::Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    db_to_delete[idx].insert(Node_Skeleton(*it2, 0));
  }
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< Node::Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	Uint32_Index idx(nit->index);
	db_to_insert[idx].insert(Node_Skeleton(*nit));
	if (update_logger)
          update_logger->insertion(*nit);	  
      }
      ++nit;
    }
  }
  
  Block_Backend< Uint32_Index, Node_Skeleton > node_db
      (transaction->data_index(osm_base_settings().NODES));
  if (update_logger)
    node_db.update(db_to_delete, db_to_insert, *update_logger);
  else
    node_db.update(db_to_delete, db_to_insert);
}


void Node_Updater::merge_files(const vector< string >& froms, string into)
{
  Transaction_Collection from_transactions(false, false, db_dir, froms);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  ::merge_files< Uint32_Index, Node_Skeleton >
      (from_transactions, into_transaction, *osm_base_settings().NODES);
  ::merge_files< Tag_Index_Local, Node::Id_Type >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_LOCAL);
  ::merge_files< Tag_Index_Global, Node::Id_Type >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
  if (meta)
  {
    ::merge_files< Uint31_Index, OSM_Element_Metadata_Skeleton< Node::Id_Type > >
        (from_transactions, into_transaction, *meta_settings().NODES_META);
  }
}
