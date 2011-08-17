#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <sys/stat.h>
#include <cstdio>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"
#include "way_updater.h"

using namespace std;

Way_Updater::Way_Updater(Transaction& transaction_, bool meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), meta(meta_)
{}

Way_Updater::Way_Updater(string db_dir_, bool meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), db_dir(db_dir_), meta(meta_)
{}

void Way_Updater::update(Osm_Backend_Callback* callback, bool partial)
{
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  map< uint32, vector< uint32 > > to_delete;
  callback->update_started();
  compute_indexes();
  callback->compute_indexes_finished();
  update_way_ids(to_delete);
  callback->update_ids_finished();
  update_members(to_delete);
  callback->update_coords_finished();
  
  vector< Tag_Entry > tags_to_delete;
  prepare_delete_tags(tags_to_delete, to_delete);
  callback->prepare_delete_tags_finished();
  update_way_tags_local(tags_to_delete);
  callback->tags_local_finished();
  update_way_tags_global(tags_to_delete);
  callback->tags_global_finished();
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(ways_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().WAYS_META),
		      ways_meta_to_insert, ids_to_modify, to_delete);
    process_user_data(*transaction, user_by_id, idxs_by_id);
  }
  callback->update_finished();
  
  ids_to_modify.clear();
  ways_to_insert.clear();
  
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

void collect_new_indexes
    (const vector< Way >& ways_to_insert, map< uint32, uint32 >& new_index_by_id)
{
  for (vector< Way >::const_iterator it = ways_to_insert.begin();
      it != ways_to_insert.end(); ++it)
    new_index_by_id[it->id] = it->index;
}

void Way_Updater::update_moved_idxs
    (Osm_Backend_Callback* callback, 
     const vector< pair< uint32, uint32 > >& moved_nodes)
{
  ids_to_modify.clear();
  ways_to_insert.clear();
  ways_meta_to_insert.clear();
  user_by_id.clear();
  
/*  if (!map_file_existed_before)
    return;*/
  
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  map< uint32, vector< uint32 > > to_delete;
  find_affected_ways(moved_nodes);
  update_way_ids(to_delete);
  if (meta)
  {
    map< uint32, uint32 > new_index_by_id;
    collect_new_indexes(ways_to_insert, new_index_by_id);
    collect_old_meta_data(*transaction->data_index(meta_settings().WAYS_META), to_delete,
		          new_index_by_id, ways_meta_to_insert);
  }
  update_members(to_delete);
  
  vector< Tag_Entry > tags_to_delete;
  prepare_tags(tags_to_delete, to_delete);
  update_way_tags_local(tags_to_delete);
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(ways_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().WAYS_META), ways_meta_to_insert,
		      ids_to_modify, to_delete);
    process_user_data(*transaction, user_by_id, idxs_by_id);
  }
  
  //show_mem_status();
  
  ids_to_modify.clear();
  ways_to_insert.clear();
  ways_meta_to_insert.clear();
  user_by_id.clear();
  
  if (!external_transaction)
    delete transaction;
}

void filter_affected_ways(Transaction& transaction, 
			  vector< pair< uint32, bool > >& ids_to_modify,
			  vector< Way >& ways_to_insert,
			  const vector< Way >& maybe_affected_ways)
{
  // retrieve the indices of the referred nodes
  map< uint32, uint32 > used_nodes;
  for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
      wit != maybe_affected_ways.end(); ++wit)
  {
    for (vector< uint32 >::const_iterator nit(wit->nds.begin());
        nit != wit->nds.end(); ++nit)
      used_nodes[*nit] = 0;
  }
  Random_File< Uint32_Index > node_random
      (transaction.random_index(osm_base_settings().NODES));
  for (map< uint32, uint32 >::iterator it(used_nodes.begin());
      it != used_nodes.end(); ++it)
    it->second = node_random.get(it->first).val();
  for (vector< Way >::const_iterator wit(maybe_affected_ways.begin());
      wit != maybe_affected_ways.end(); ++wit)
  {
    vector< uint32 > nd_idxs;
    for (vector< uint32 >::const_iterator nit(wit->nds.begin());
        nit != wit->nds.end(); ++nit)
      nd_idxs.push_back(used_nodes[*nit]);
    
    uint32 index(Way::calc_index(nd_idxs));
    if (wit->index != index)
    {
      ids_to_modify.push_back(make_pair(wit->id, true));
      ways_to_insert.push_back(*wit);
      ways_to_insert.back().index = index;
    }
  }
}

void Way_Updater::find_affected_ways
    (const vector< pair< uint32, uint32 > >& moved_nodes)
{
  vector< Way > maybe_affected_ways;
  
  set< Uint31_Index > req;
  {
    vector< uint32 > moved_node_idxs;
    for (vector< pair< uint32, uint32 > >::const_iterator
        it(moved_nodes.begin()); it != moved_nodes.end(); ++it)
      moved_node_idxs.push_back(it->second);
    vector< uint32 > affected_way_idxs = calc_parents(moved_node_idxs);
    for (vector< uint32 >::const_iterator it = affected_way_idxs.begin();
        it != affected_way_idxs.end(); ++it)
      req.insert(Uint31_Index(*it));
  }
  
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (transaction->data_index(osm_base_settings().WAYS));
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
      it(ways_db.discrete_begin(req.begin(), req.end()));
      !(it == ways_db.discrete_end()); ++it)
  {
    const Way_Skeleton& way(it.object());
    bool is_affected(false);
    for (vector< uint32 >::const_iterator it3(way.nds.begin());
        it3 != way.nds.end(); ++it3)
    {
      if (binary_search(moved_nodes.begin(), moved_nodes.end(),
	make_pair(*it3, 0), pair_comparator_by_id))
      {
	is_affected = true;
	break;
      }
    }
    if (is_affected)
      maybe_affected_ways.push_back(Way(way.id, it.index().val(), way.nds));
    if (maybe_affected_ways.size() >= 512*1024)
    {
      filter_affected_ways(*transaction, ids_to_modify, ways_to_insert, maybe_affected_ways);
      maybe_affected_ways.clear();
    }
  }
  
  filter_affected_ways(*transaction, ids_to_modify, ways_to_insert, maybe_affected_ways);
  maybe_affected_ways.clear();
}

void Way_Updater::compute_indexes()
{
  static Meta_Comparator_By_Id meta_comparator_by_id;
  static Meta_Equal_Id meta_equal_id;
  
  // process the ways itself
  // keep always the most recent (last) element of all equal elements
  stable_sort(ways_to_insert.begin(), ways_to_insert.end(), way_comparator_by_id);
  vector< Way >::iterator ways_begin
      (unique(ways_to_insert.rbegin(), ways_to_insert.rend(), way_equal_id).base());
  ways_to_insert.erase(ways_to_insert.begin(), ways_begin);
  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ways_meta_to_insert.begin(), ways_meta_to_insert.end(), meta_comparator_by_id);
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::iterator meta_begin
      (unique(ways_meta_to_insert.rbegin(), ways_meta_to_insert.rend(), meta_equal_id).base());
  ways_meta_to_insert.erase(ways_meta_to_insert.begin(), meta_begin);
  
  // retrieve the indices of the referred nodes
  map< uint32, uint32 > used_nodes;
  for (vector< Way >::const_iterator wit(ways_to_insert.begin());
      wit != ways_to_insert.end(); ++wit)
  {
    for (vector< uint32 >::const_iterator nit(wit->nds.begin());
        nit != wit->nds.end(); ++nit)
      used_nodes[*nit] = 0;
  }
  Random_File< Uint32_Index > node_random
      (transaction->random_index(osm_base_settings().NODES));
  for (map< uint32, uint32 >::iterator it(used_nodes.begin());
      it != used_nodes.end(); ++it)
    it->second = node_random.get(it->first).val();
  for (vector< Way >::iterator wit(ways_to_insert.begin());
      wit != ways_to_insert.end(); ++wit)
  {
    vector< uint32 > nd_idxs;
    for (vector< uint32 >::const_iterator nit(wit->nds.begin());
        nit != wit->nds.end(); ++nit)
      nd_idxs.push_back(used_nodes[*nit]);
    
    wit->index = Way::calc_index(nd_idxs);
  }
  vector< Way >::const_iterator wit(ways_to_insert.begin());
  for (vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::iterator
      mit(ways_meta_to_insert.begin()); mit != ways_meta_to_insert.end(); ++mit)
  {
    while ((wit != ways_to_insert.end()) && (wit->id < mit->first.ref))
      ++wit;
    if (wit == ways_to_insert.end())
      break;
    
    if (wit->id == mit->first.ref)
      mit->second = wit->index;
  }
}

void Way_Updater::update_way_ids(map< uint32, vector< uint32 > >& to_delete)
{
  // process the ways itself
  // keep always the most recent (last) element of all equal elements
  stable_sort(ids_to_modify.begin(), ids_to_modify.end(),
	      pair_comparator_by_id);
	      vector< pair< uint32, bool > >::iterator modi_begin
	      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id)
	      .base());
  ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
  stable_sort(ways_to_insert.begin(), ways_to_insert.end(), way_comparator_by_id);
      vector< Way >::iterator ways_begin
      (unique(ways_to_insert.rbegin(), ways_to_insert.rend(), way_equal_id).base());
  ways_to_insert.erase(ways_to_insert.begin(), ways_begin);
  
  Random_File< Uint31_Index > random
      (transaction->random_index(osm_base_settings().WAYS));
  vector< Way >::const_iterator wit(ways_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
  it != ids_to_modify.end(); ++it)
  {
    Uint31_Index index(random.get(it->first));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
    if ((wit != ways_to_insert.end()) && (it->first == wit->id))
    {
      if (it->second)
      {
	random.put(it->first, Uint31_Index(wit->index));
	if ((index.val() > 0) && (index.val() != wit->index))
	  moved_ways.push_back(make_pair(it->first, index.val()));
      }
      ++wit;
    }
  }
  sort(moved_ways.begin(), moved_ways.end());
}

void Way_Updater::update_members(const map< uint32, vector< uint32 > >& to_delete)
{
  map< Uint31_Index, set< Way_Skeleton > > db_to_delete;
  map< Uint31_Index, set< Way_Skeleton > > db_to_insert;
  
  for (map< uint32, vector< uint32 > >::const_iterator
    it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint31_Index idx(it->first);
    for (vector< uint32 >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      db_to_delete[idx].insert(Way_Skeleton(*it2, vector< uint32 >()));
  }
  vector< Way >::const_iterator wit(ways_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
  it != ids_to_modify.end(); ++it)
  {
    if ((wit != ways_to_insert.end()) && (it->first == wit->id))
    {
      if (it->second)
      {
	Uint31_Index idx(wit->index);
	db_to_insert[idx].insert(Way_Skeleton(*wit));
      }
      ++wit;
    }
  }
  
  Block_Backend< Uint31_Index, Way_Skeleton > way_db
      (transaction->data_index(osm_base_settings().WAYS));
  way_db.update(db_to_delete, db_to_insert);
}

void Way_Updater::prepare_delete_tags
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
  Block_Backend< Tag_Index_Local, Uint32_Index > ways_db
      (transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL));
  Tag_Index_Local current_index;
  Tag_Entry tag_entry;
  current_index.index = 0xffffffff;
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    it(ways_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == ways_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	tags_to_delete.push_back(tag_entry);
      current_index = it.index();
      tag_entry.index = it.index().index;
      tag_entry.key = it.index().key;
      tag_entry.value = it.index().value;
      tag_entry.ids.clear();
    }
    
    set< uint32 >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
      tag_entry.ids.push_back(it.object().val());
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}

void Way_Updater::prepare_tags
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
  Block_Backend< Tag_Index_Local, Uint32_Index > ways_db
      (transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL));
  Tag_Index_Local current_index;
  Tag_Entry tag_entry;
  current_index.index = 0xffffffff;
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    it(ways_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == ways_db.range_end()); ++it)
  {
    if (!(current_index == it.index()))
    {
      if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
	tags_to_delete.push_back(tag_entry);
      current_index = it.index();
      tag_entry.index = it.index().index;
      tag_entry.key = it.index().key;
      tag_entry.value = it.index().value;
      tag_entry.ids.clear();
    }
    
    set< uint32 >& handle(to_delete_coarse[it.index().index]);
    if (handle.find(it.object().val()) != handle.end())
    {
      Way* way(binary_search_for_id(ways_to_insert, it.object().val()));
      if (way != 0)
	way->tags.push_back(make_pair(it.index().key, it.index().value));
      tag_entry.ids.push_back(it.object().val());
    }
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}

void Way_Updater::update_way_tags_local(const vector< Tag_Entry >& tags_to_delete)
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
    
    set< Uint32_Index > way_ids;
    for (vector< uint32 >::const_iterator it2(it->ids.begin());
    it2 != it->ids.end(); ++it2)
    way_ids.insert(*it2);
    
    db_to_delete[index] = way_ids;
  }
  
  vector< Way >::const_iterator wit(ways_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
  it != ids_to_modify.end(); ++it)
  {
    if ((wit != ways_to_insert.end()) && (it->first == wit->id))
    {
      if (it->second)
      {
	Tag_Index_Local index;
	index.index = wit->index & 0xffffff00;
	
	for (vector< pair< string, string > >::const_iterator
	  it2(wit->tags.begin()); it2 != wit->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(wit->id);
	  db_to_delete[index];
	}
      }
      ++wit;
    }
  }
  
  Block_Backend< Tag_Index_Local, Uint32_Index > way_db
      (transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL));
  way_db.update(db_to_delete, db_to_insert);
}

void Way_Updater::update_way_tags_global(const vector< Tag_Entry >& tags_to_delete)
{
  map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
  map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
  
  for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
  it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Global index;
    index.key = it->key;
    index.value = it->value;
    
    set< Uint32_Index > way_ids;
    for (vector< uint32 >::const_iterator it2(it->ids.begin());
    it2 != it->ids.end(); ++it2)
    db_to_delete[index].insert(*it2);
  }
  
  vector< Way >::const_iterator wit(ways_to_insert.begin());
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
  it != ids_to_modify.end(); ++it)
  {
    if ((wit != ways_to_insert.end()) && (it->first == wit->id))
    {
      if (it->second)
      {
	Tag_Index_Global index;
	
	for (vector< pair< string, string > >::const_iterator
	  it2(wit->tags.begin()); it2 != wit->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(wit->id);
	  db_to_delete[index];
	}
      }
      ++wit;
    }
  }
  
  Block_Backend< Tag_Index_Global, Uint32_Index > way_db
      (transaction->data_index(osm_base_settings().WAY_TAGS_GLOBAL));
  way_db.update(db_to_delete, db_to_insert);
}

void Way_Updater::merge_files(string from, string into)
{
  Nonsynced_Transaction from_transaction(false, false, db_dir, from);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  merge_file< Uint31_Index, Way_Skeleton >
      (from_transaction, into_transaction, from, *osm_base_settings().WAYS);
  merge_file< Tag_Index_Local, Uint32_Index >
      (from_transaction, into_transaction, from, *osm_base_settings().WAY_TAGS_LOCAL);
  merge_file< Tag_Index_Global, Uint32_Index >
      (from_transaction, into_transaction, from, *osm_base_settings().WAY_TAGS_GLOBAL);
  if (meta)
  {
    merge_file< Uint31_Index, OSM_Element_Metadata_Skeleton >
        (from_transaction, into_transaction, from, *meta_settings().WAYS_META);
  }
}
