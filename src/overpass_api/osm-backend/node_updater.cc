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
#include "node_updater.h"

using namespace std;

Node_Updater::Node_Updater(Transaction& transaction_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true)
{}

Node_Updater::Node_Updater(string db_dir_)
  : update_counter(0), transaction(0),
    external_transaction(false), db_dir(db_dir_)
{}

struct Meta_Comparator_By_Id {
  bool operator()
    (const pair< OSM_Element_Metadata_Skeleton, uint32 >& a,
     const pair< OSM_Element_Metadata_Skeleton, uint32 >& b)
  {
    return (a.first.ref < b.first.ref);
  }
};

struct Meta_Equal_Id {
  bool operator()
    (const pair< OSM_Element_Metadata_Skeleton, uint32 >& a,
     const pair< OSM_Element_Metadata_Skeleton, uint32 >& b)
  {
    return (a.first.ref == b.first.ref);
  }
};

void process_meta_data
  (Transaction& transaction,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& nodes_meta_to_insert,
   const vector< pair< uint32, bool > >& ids_to_modify,
   const map< uint32, vector< uint32 > >& to_delete)
{
  static Meta_Comparator_By_Id meta_comparator_by_id;
  static Meta_Equal_Id meta_equal_id;
  
  map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > > db_to_delete;
  map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > > db_to_insert;
  
  // fill db_to_delete
  for (map< uint32, vector< uint32 > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint31_Index idx(it->first);
    for (vector< uint32 >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      db_to_delete[idx].insert(OSM_Element_Metadata_Skeleton(*it2));
  }

  // keep always the most recent (last) element of all equal elements
  stable_sort
      (nodes_meta_to_insert.begin(), nodes_meta_to_insert.end(), meta_comparator_by_id);
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::iterator nodes_begin
      (unique(nodes_meta_to_insert.rbegin(), nodes_meta_to_insert.rend(), meta_equal_id)
       .base());
  nodes_meta_to_insert.erase(nodes_meta_to_insert.begin(), nodes_begin);
  
  // fill insert
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::const_iterator
      nit = nodes_meta_to_insert.begin();
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != nodes_meta_to_insert.end()) && (it->first == nit->first.ref))
    {
      if (it->second)
	db_to_insert[Uint31_Index(nit->second)].insert(nit->first);
      ++nit;
    }
  }
  
  nodes_meta_to_insert.clear();
  
  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton > user_db
      (transaction.data_index(meta_settings().NODES_META));
  user_db.update(db_to_delete, db_to_insert);
}

void process_user_data(Transaction& transaction, map< uint32, string >& user_by_id)
{
  map< Uint32_Index, set< User_Data > > db_to_delete;
  map< Uint32_Index, set< User_Data > > db_to_insert;
  
  for (map< uint32, string >::const_iterator it = user_by_id.begin();
      it != user_by_id.end(); ++it)
  {
    User_Data user_data;
    user_data.id = it->first;
    db_to_delete[Uint32_Index(it->first & 0xffffff00)].insert(user_data);
  }
  for (map< uint32, string >::const_iterator it = user_by_id.begin();
      it != user_by_id.end(); ++it)
  {
    User_Data user_data;
    user_data.id = it->first;
    user_data.name = it->second;
    db_to_insert[Uint32_Index(it->first & 0xffffff00)].insert(user_data);
  }
  user_by_id.clear();
  
  Block_Backend< Uint32_Index, User_Data > user_db
      (transaction.data_index(meta_settings().USER_DATA));
  user_db.update(db_to_delete, db_to_insert);
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
  process_meta_data(*transaction, nodes_meta_to_insert, ids_to_modify, to_delete);
  process_user_data(*transaction, user_by_id);
  callback->update_finished();
  
  ids_to_modify.clear();
  nodes_to_insert.clear();

  if (!external_transaction)
    delete transaction;
  
  if (!external_transaction && !partial && (update_counter > 0))
  {
    callback->partial_started();
    if (update_counter >= 64)
      merge_files(".1", ".0");
    if (update_counter >= 8)
      merge_files(".0", "");
    update_counter = 0;
    callback->partial_finished();
  }
  else if (!external_transaction && partial/* && !map_file_existed_before*/)
  {
    if (++update_counter % 8 == 0)
    {
      callback->partial_started();
      merge_files("", ".0");
      callback->partial_finished();
    }
    if (update_counter % 64 == 0)
    {
      callback->partial_started();
      merge_files(".0", ".1");
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
      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id)
  .base());
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

void Node_Updater::merge_files(string from, string into)
{
  Nonsynced_Transaction from_transaction(false, false, db_dir, from);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  {
    map< Uint32_Index, set< Node_Skeleton > > db_to_delete;
    map< Uint32_Index, set< Node_Skeleton > > db_to_insert;
    
    uint32 item_count(0);
    Block_Backend< Uint32_Index, Node_Skeleton > from_db
        (from_transaction.data_index(osm_base_settings().NODES));
    for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
        it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
    {
      db_to_insert[it.index()].insert(it.object());
      if (++item_count >= 4*1024*1024)
      {
	Block_Backend< Uint32_Index, Node_Skeleton > into_db
	    (into_transaction.data_index(osm_base_settings().NODES));
	into_db.update(db_to_delete, db_to_insert);
	db_to_insert.clear();
	item_count = 0;
      }
    }
    
    Block_Backend< Uint32_Index, Node_Skeleton > into_db
        (into_transaction.data_index(osm_base_settings().NODES));
    into_db.update(db_to_delete, db_to_insert);
  }
  remove((from_transaction.get_db_dir()
      + osm_base_settings().NODES->get_file_name_trunk() + from 
      + osm_base_settings().NODES->get_data_suffix()
      + osm_base_settings().NODES->get_index_suffix()).c_str());
  remove((from_transaction.get_db_dir()
      + osm_base_settings().NODES->get_file_name_trunk() + from 
      + osm_base_settings().NODES->get_data_suffix()).c_str());
  {
    map< Tag_Index_Local, set< Uint32_Index > > db_to_delete;
    map< Tag_Index_Local, set< Uint32_Index > > db_to_insert;
    
    uint32 item_count(0);
    Block_Backend< Tag_Index_Local, Uint32_Index > from_db
        (from_transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
        it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
    {
      db_to_insert[it.index()].insert(it.object());
      if (++item_count >= 4*1024*1024)
      {
	Block_Backend< Tag_Index_Local, Uint32_Index > into_db
	    (into_transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
	into_db.update(db_to_delete, db_to_insert);
	db_to_insert.clear();
	item_count = 0;
      }
    }
    
    Block_Backend< Tag_Index_Local, Uint32_Index > into_db
        (into_transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
    into_db.update(db_to_delete, db_to_insert);
  }
  remove((from_transaction.get_db_dir()
      + osm_base_settings().NODE_TAGS_LOCAL->get_file_name_trunk() + from 
      + osm_base_settings().NODE_TAGS_LOCAL->get_data_suffix()
      + osm_base_settings().NODE_TAGS_LOCAL->get_index_suffix()).c_str());
  remove((from_transaction.get_db_dir()
      + osm_base_settings().NODE_TAGS_LOCAL->get_file_name_trunk() + from 
      + osm_base_settings().NODE_TAGS_LOCAL->get_data_suffix()).c_str());
  {
    map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
    map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
    
    uint32 item_count(0);
    Block_Backend< Tag_Index_Global, Uint32_Index > from_db
        (from_transaction.data_index(osm_base_settings().NODE_TAGS_GLOBAL));
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
        it(from_db.flat_begin()); !(it == from_db.flat_end()); ++it)
    {
      db_to_insert[it.index()].insert(it.object());
      if (++item_count >= 4*1024*1024)
      {
	Block_Backend< Tag_Index_Global, Uint32_Index > into_db
	    (into_transaction.data_index(osm_base_settings().NODE_TAGS_GLOBAL));
	into_db.update(db_to_delete, db_to_insert);
	db_to_insert.clear();
	item_count = 0;
      }
    }
    
    Block_Backend< Tag_Index_Global, Uint32_Index > into_db
        (into_transaction.data_index(osm_base_settings().NODE_TAGS_GLOBAL));
    into_db.update(db_to_delete, db_to_insert);
  }
  remove((from_transaction.get_db_dir()
      + osm_base_settings().NODE_TAGS_GLOBAL->get_file_name_trunk() + from 
      + osm_base_settings().NODE_TAGS_GLOBAL->get_data_suffix()
      + osm_base_settings().NODE_TAGS_GLOBAL->get_index_suffix()).c_str());
  remove((from_transaction.get_db_dir()
      + osm_base_settings().NODE_TAGS_GLOBAL->get_file_name_trunk() + from 
      + osm_base_settings().NODE_TAGS_GLOBAL->get_data_suffix()).c_str());
}
