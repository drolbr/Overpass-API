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

using namespace std;

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

void Node_Updater::update(Osm_Backend_Callback* callback, bool partial)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  map< uint32, vector< uint32 > > to_delete;
  callback->update_started();
  update_node_ids(to_delete);
  callback->update_ids_finished();
  update_coords(to_delete);
  callback->update_coords_finished();
  
  vector< Tag_Entry > tags_to_delete;
  prepare_delete_tags(tags_to_delete, to_delete);
  callback->prepare_delete_tags_finished();
  update_node_tags_local(tags_to_delete);
  callback->tags_local_finished();
  update_node_tags_global(tags_to_delete);
  callback->tags_global_finished();
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(nodes_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().NODES_META),
		      nodes_meta_to_insert, ids_to_modify, to_delete);
    process_user_data(*transaction, user_by_id, idxs_by_id);
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
    (map< uint32, vector< uint32 > >& to_delete)
{
  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ids_to_modify.begin(), ids_to_modify.end(), pair_comparator_by_id);
  vector< pair< uint32, bool > >::iterator modi_begin
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
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    Uint32_Index index(random.get(it->first));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	random.put(it->first, Uint32_Index(nit->ll_upper));
	if ((index.val() > 0) && (index.val() != nit->ll_upper))
	  moved_nodes.push_back(make_pair(it->first, index.val()));
      }
      ++nit;
    }
  }
  sort(moved_nodes.begin(), moved_nodes.end());
}

void Node_Updater::update_coords(const map< uint32, vector< uint32 > >& to_delete)
{
  map< Uint32_Index, set< Node_Skeleton > > db_to_delete;
  map< Uint32_Index, set< Node_Skeleton > > db_to_insert;
  
  for (map< uint32, vector< uint32 > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint32_Index idx(it->first);
    for (vector< uint32 >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    db_to_delete[idx].insert(Node_Skeleton(*it2, 0));
  }
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	Uint32_Index idx(nit->ll_upper);
	db_to_insert[idx].insert(Node_Skeleton(*nit));
      }
      ++nit;
    }
  }
  
  Block_Backend< Uint32_Index, Node_Skeleton > node_db
      (transaction->data_index(osm_base_settings().NODES));
  node_db.update(db_to_delete, db_to_insert);
}

void Node_Updater::prepare_delete_tags
      (vector< Tag_Entry >& tags_to_delete,
       const map< uint32, vector< uint32 > >& to_delete)
{
  // make indices appropriately coarse
  map< uint32, set< uint32 > > to_delete_coarse;
  for (map< uint32, vector< uint32 > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    set< uint32 >& handle(to_delete_coarse[it->first & 0xffffff00]);
    for (vector< uint32 >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
    {
      handle.insert(*it2);
    }
  }
  
  // formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  for (map< uint32, set< uint32 > >::const_iterator
      it(to_delete_coarse.begin()); it != to_delete_coarse.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->first;
    lower.key = "";
    lower.value = "";
    upper.index = it->first + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, Uint32_Index > nodes_db
      (transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL));
  Tag_Index_Local current_index;
  Tag_Entry node_tag_entry;
  current_index.index = 0xffffffff;
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
      it(nodes_db.range_begin
         (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
          Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
      !(it == nodes_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!node_tag_entry.ids.empty()))
	tags_to_delete.push_back(node_tag_entry);
      current_index = it.index();
      node_tag_entry.index = it.index().index;
      node_tag_entry.key = it.index().key;
      node_tag_entry.value = it.index().value;
      node_tag_entry.ids.clear();
    }
    
    set< uint32 >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
      node_tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!node_tag_entry.ids.empty()))
    tags_to_delete.push_back(node_tag_entry);
}

void Node_Updater::update_node_tags_local(const vector< Tag_Entry >& tags_to_delete)
{
  map< Tag_Index_Local, set< Uint32_Index > > db_to_delete;
  map< Tag_Index_Local, set< Uint32_Index > > db_to_insert;
  
  for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
      it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Local index;
    index.index = it->index;
    index.key = it->key;
    index.value = it->value;
    
    set< Uint32_Index > node_ids;
    for (vector< uint32 >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
    node_ids.insert(*it2);
    
    db_to_delete[index] = node_ids;
  }
  
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	Tag_Index_Local index;
	index.index = nit->ll_upper & 0xffffff00;
	
	for (vector< pair< string, string > >::const_iterator it2(nit->tags.begin());
	    it2 != nit->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(nit->id);
	  db_to_delete[index];
	}
      }
      ++nit;
    }
  }
  
  Block_Backend< Tag_Index_Local, Uint32_Index > node_db
      (transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL));
  node_db.update(db_to_delete, db_to_insert);
}

void Node_Updater::update_node_tags_global(const vector< Tag_Entry >& tags_to_delete)
{
  map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
  map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
  
  for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
      it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Global index;
    index.key = it->key;
    index.value = it->value;
    
    set< Uint32_Index > node_ids;
    for (vector< uint32 >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
    db_to_delete[index].insert(*it2);
  }
  
  vector< Node >::const_iterator nit(nodes_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != nodes_to_insert.end()) && (it->first == nit->id))
    {
      if (it->second)
      {
	Tag_Index_Global index;
	
	for (vector< pair< string, string > >::const_iterator it2(nit->tags.begin());
	    it2 != nit->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(nit->id);
	  db_to_delete[index];
	}
      }
      ++nit;
    }
  }
  
  Block_Backend< Tag_Index_Global, Uint32_Index > node_db
      (transaction->data_index(osm_base_settings().NODE_TAGS_GLOBAL));
  node_db.update(db_to_delete, db_to_insert);
}

void Node_Updater::merge_files(const vector< string >& froms, string into)
{
  Transaction_Collection from_transactions(false, false, db_dir, froms);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  ::merge_files< Uint32_Index, Node_Skeleton >
      (from_transactions, into_transaction, *osm_base_settings().NODES);
  ::merge_files< Tag_Index_Local, Uint32_Index >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_LOCAL);
  ::merge_files< Tag_Index_Global, Uint32_Index >
      (from_transactions, into_transaction, *osm_base_settings().NODE_TAGS_GLOBAL);
  if (meta)
  {
    ::merge_files< Uint31_Index, OSM_Element_Metadata_Skeleton >
        (from_transactions, into_transaction, *meta_settings().NODES_META);
  }
}
