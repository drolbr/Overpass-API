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


void Update_Way_Logger::flush()
{
  cout<<"Insert:\n";
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator it = insert.begin();
      it != insert.end(); ++it)
  {
    cout<<it->second.first.id;
    for (vector< uint32 >::const_iterator it2 = it->second.first.nds.begin();
	 it2 != it->second.first.nds.end(); ++it2)
      cout<<'\t'<<*it2;
    cout<<"\t\t"<<it->second.first.tags.size();
    if (it->second.second)
      cout<<'\t'<<it->second.second->version<<'\t'<<it->second.second->user_id;
    cout<<'\n';
  }
  cout<<'\n';
  cout<<"Keep:\n";
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator it = keep.begin();
      it != keep.end(); ++it)
  {
    cout<<it->second.first.id;
    for (vector< uint32 >::const_iterator it2 = it->second.first.nds.begin();
	 it2 != it->second.first.nds.end(); ++it2)
      cout<<'\t'<<*it2;
    cout<<"\t\t"<<it->second.first.tags.size();
    if (it->second.second)
      cout<<'\t'<<it->second.second->version<<'\t'<<it->second.second->user_id;
    cout<<'\n';
  }
  cout<<'\n';
  cout<<"Delete:\n";
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator it = erase.begin();
      it != erase.end(); ++it)
  {
    cout<<it->second.first.id;
    for (vector< uint32 >::const_iterator it2 = it->second.first.nds.begin();
	 it2 != it->second.first.nds.end(); ++it2)
      cout<<'\t'<<*it2;
    cout<<"\t\t"<<it->second.first.tags.size();
    if (it->second.second)
      cout<<'\t'<<it->second.second->version<<'\t'<<it->second.second->user_id;
    cout<<'\n';
  }
  cout<<'\n';
}


Update_Way_Logger::~Update_Way_Logger()
{
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator it = insert.begin();
      it != insert.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator it = keep.begin();
      it != keep.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator it = erase.begin();
      it != erase.end(); ++it)
  {
    if (it->second.second)
      delete it->second.second;
  }
}


Way_Updater::Way_Updater(Transaction& transaction_, bool meta_)
  : update_counter(0), transaction(&transaction_),
    external_transaction(true), partial_possible(false), meta(meta_)
{}

Way_Updater::Way_Updater(string db_dir_, bool meta_)
  : update_counter(0), transaction(0),
    external_transaction(false), partial_possible(true), db_dir(db_dir_), meta(meta_)
{
  partial_possible = !file_exists
      (db_dir + 
       osm_base_settings().WAYS->get_file_name_trunk() +
       osm_base_settings().WAYS->get_data_suffix() +
       osm_base_settings().WAYS->get_index_suffix());
}

namespace
{
  Way_Comparator_By_Id way_comparator_by_id;
  Way_Equal_Id way_equal_id;
}

void Way_Updater::update(Osm_Backend_Callback* callback, bool partial,
	      Update_Way_Logger* update_logger)
{
  for (vector< Way >::const_iterator it = ways_to_insert.begin(); it != ways_to_insert.end(); ++it)
    update_logger->insertion(*it);
  if (meta)
  {
    for (vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::const_iterator
        it = ways_meta_to_insert.begin(); it != ways_meta_to_insert.end(); ++it)
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
  
  if (!external_transaction)
    transaction = new Nonsynced_Transaction(true, false, db_dir, "");
  
  map< uint32, vector< uint32 > > to_delete;
  callback->update_started();
  vector< Way* > ways_ptr = sort_elems_to_insert
      (ways_to_insert, way_comparator_by_id, way_equal_id);
  compute_indexes(ways_ptr);
  callback->compute_indexes_finished();
  update_way_ids(ways_ptr, to_delete);
  callback->update_ids_finished();
  update_members(ways_ptr, to_delete, update_logger);
  callback->update_coords_finished();
  
  vector< Tag_Entry > tags_to_delete;
  prepare_delete_tags(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
		      tags_to_delete, to_delete);
  callback->prepare_delete_tags_finished();
  update_tags_local(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
		    ways_ptr, ids_to_modify, tags_to_delete, update_logger);
  callback->tags_local_finished();
  update_tags_global(*transaction->data_index(osm_base_settings().WAY_TAGS_GLOBAL),
		     ways_ptr, ids_to_modify, tags_to_delete);
  callback->tags_global_finished();
  if (meta)
  {
    map< uint32, vector< uint32 > > idxs_by_id;
    create_idxs_by_id(ways_meta_to_insert, idxs_by_id);
    process_meta_data(*transaction->data_index(meta_settings().WAYS_META),
		      ways_meta_to_insert, ids_to_modify, to_delete, update_logger);
    process_user_data(*transaction, user_by_id, idxs_by_id);
  }
  callback->update_finished();
  
  ids_to_modify.clear();
  ways_to_insert.clear();
  
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
  else if (partial_possible && partial/* && !map_file_existed_before*/)
  {
    string to(".0a");
    to[2] += update_counter % 16;
    rename_referred_file(db_dir, "", to, *osm_base_settings().WAYS);
    rename_referred_file(db_dir, "", to, *osm_base_settings().WAY_TAGS_LOCAL);
    rename_referred_file(db_dir, "", to, *osm_base_settings().WAY_TAGS_GLOBAL);
    if (meta)
      rename_referred_file(db_dir, "", to, *meta_settings().WAYS_META);
    
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
  vector< Way* > ways_ptr = sort_elems_to_insert
      (ways_to_insert, way_comparator_by_id, way_equal_id);
  update_way_ids(ways_ptr, to_delete);
  if (meta)
  {
    map< uint32, uint32 > new_index_by_id;
    collect_new_indexes(ways_ptr, new_index_by_id);
    collect_old_meta_data(*transaction->data_index(meta_settings().WAYS_META), to_delete,
		          new_index_by_id, ways_meta_to_insert);
  }
  update_members(ways_ptr, to_delete, 0);
  
  vector< Tag_Entry > tags_to_delete;
  prepare_tags(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
	       ways_ptr, tags_to_delete, to_delete);
  update_tags_local(*transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL),
		    ways_ptr, ids_to_modify, tags_to_delete, (Update_Way_Logger*)0);
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

vector< Uint31_Index > calc_segment_idxs(const vector< uint32 >& nd_idxs)
{
  vector< Uint31_Index > result;
  vector< uint32 > segment_nd_idxs(2, 0);
  for (vector< uint32 >::size_type i = 1; i < nd_idxs.size(); ++i)
  {
    segment_nd_idxs[0] = nd_idxs[i-1];
    segment_nd_idxs[1] = nd_idxs[i];
    Uint31_Index segment_index = Way::calc_index(segment_nd_idxs);
    if ((segment_index.val() & 0x80000000) != 0)
      result.push_back(segment_index);
  }
  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());
  
  return result;
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

    vector< Uint31_Index > segment_idxs;
    if ((index & 0x80000000) != 0 && (index & 0xfc) != 0)
      segment_idxs = calc_segment_idxs(nd_idxs);
    
    if (wit->index != index || wit->segment_idxs != segment_idxs)
    {
      ids_to_modify.push_back(make_pair(wit->id, true));
      ways_to_insert.push_back(*wit);
      ways_to_insert.back().index = index;
      ways_to_insert.back().segment_idxs = segment_idxs;
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

void Way_Updater::compute_indexes(vector< Way* >& ways_ptr)
{
  static Meta_Comparator_By_Id meta_comparator_by_id;
  static Meta_Equal_Id meta_equal_id;
  
  // keep always the most recent (last) element of all equal elements
  stable_sort
      (ways_meta_to_insert.begin(), ways_meta_to_insert.end(), meta_comparator_by_id);
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::iterator meta_begin
      (unique(ways_meta_to_insert.rbegin(), ways_meta_to_insert.rend(), meta_equal_id).base());
  ways_meta_to_insert.erase(ways_meta_to_insert.begin(), meta_begin);
  
  // retrieve the indices of the referred nodes
  map< uint32, uint32 > used_nodes;
  for (vector< Way* >::const_iterator wit = ways_ptr.begin();
      wit != ways_ptr.end(); ++wit)
  {
    for (vector< uint32 >::const_iterator nit = (*wit)->nds.begin();
        nit != (*wit)->nds.end(); ++nit)
      used_nodes[*nit] = 0;
  }
  Random_File< Uint32_Index > node_random
      (transaction->random_index(osm_base_settings().NODES));
  for (map< uint32, uint32 >::iterator it(used_nodes.begin());
      it != used_nodes.end(); ++it)
    it->second = node_random.get(it->first).val();

  for (vector< Way* >::iterator wit = ways_ptr.begin(); wit != ways_ptr.end(); ++wit)
  {
    vector< uint32 > nd_idxs;
    for (vector< uint32 >::const_iterator nit = (*wit)->nds.begin();
        nit != (*wit)->nds.end(); ++nit)
      nd_idxs.push_back(used_nodes[*nit]);
    
    (*wit)->index = Way::calc_index(nd_idxs);
    if (((*wit)->index & 0x80000000) != 0 && ((*wit)->index & 0xfc) != 0)
      (*wit)->segment_idxs = calc_segment_idxs(nd_idxs);
  }
  vector< Way* >::const_iterator wit = ways_ptr.begin();
  for (vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::iterator
      mit(ways_meta_to_insert.begin()); mit != ways_meta_to_insert.end(); ++mit)
  {
    while ((wit != ways_ptr.end()) && ((*wit)->id < mit->first.ref))
      ++wit;
    if (wit == ways_ptr.end())
      break;
    
    if ((*wit)->id == mit->first.ref)
      mit->second = (*wit)->index;
  }
}

void Way_Updater::update_way_ids
    (const vector< Way* >& ways_ptr, map< uint32, vector< uint32 > >& to_delete)
{
  // process the ways itself
  // keep always the most recent (last) element of all equal elements
  stable_sort(ids_to_modify.begin(), ids_to_modify.end(),
	      pair_comparator_by_id);
	      vector< pair< uint32, bool > >::iterator modi_begin
	      (unique(ids_to_modify.rbegin(), ids_to_modify.rend(), pair_equal_id)
	      .base());
  ids_to_modify.erase(ids_to_modify.begin(), modi_begin);
  
  Random_File< Uint31_Index > random
      (transaction->random_index(osm_base_settings().WAYS));
  vector< Way* >::const_iterator wit = ways_ptr.begin();
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
  it != ids_to_modify.end(); ++it)
  {
    Uint31_Index index(random.get(it->first));
    if (index.val() > 0)
      to_delete[index.val()].push_back(it->first);
    if ((wit != ways_ptr.end()) && (it->first == (*wit)->id))
    {
      if (it->second)
      {
	random.put(it->first, Uint31_Index((*wit)->index));
	if ((index.val() > 0) && (index.val() != (*wit)->index))
	  moved_ways.push_back(make_pair(it->first, index.val()));
      }
      ++wit;
    }
  }
  sort(moved_ways.begin(), moved_ways.end());
}

void Way_Updater::update_members
    (const vector< Way* >& ways_ptr, const map< uint32, vector< uint32 > >& to_delete,
       Update_Way_Logger* update_logger)
{
  map< Uint31_Index, set< Way_Skeleton > > db_to_delete;
  map< Uint31_Index, set< Way_Skeleton > > db_to_insert;
  
  for (map< uint32, vector< uint32 > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint31_Index idx(it->first);
    for (vector< uint32 >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      db_to_delete[idx].insert(Way_Skeleton(*it2, vector< uint32 >(), vector< Uint31_Index >()));
  }
  vector< Way* >::const_iterator wit = ways_ptr.begin();
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((wit != ways_ptr.end()) && (it->first == (*wit)->id))
    {
      if (it->second)
      {
	Uint31_Index idx = (*wit)->index;
	db_to_insert[idx].insert(Way_Skeleton(**wit));
      }
      ++wit;
    }
  }
  
  Block_Backend< Uint31_Index, Way_Skeleton > way_db
      (transaction->data_index(osm_base_settings().WAYS));
  if (update_logger)
    way_db.update(db_to_delete, db_to_insert, *update_logger);
  else
    way_db.update(db_to_delete, db_to_insert);
}

void Way_Updater::merge_files(const vector< string >& froms, string into)
{
  Transaction_Collection from_transactions(false, false, db_dir, froms);
  Nonsynced_Transaction into_transaction(true, false, db_dir, into);
  ::merge_files< Uint31_Index, Way_Skeleton >
      (from_transactions, into_transaction, *osm_base_settings().WAYS);
  ::merge_files< Tag_Index_Local, Uint32_Index >
      (from_transactions, into_transaction, *osm_base_settings().WAY_TAGS_LOCAL);
  ::merge_files< Tag_Index_Global, Uint32_Index >
      (from_transactions, into_transaction, *osm_base_settings().WAY_TAGS_GLOBAL);
  if (meta)
  {
    ::merge_files< Uint31_Index, OSM_Element_Metadata_Skeleton >
        (from_transactions, into_transaction, *meta_settings().WAYS_META);
  }
}
