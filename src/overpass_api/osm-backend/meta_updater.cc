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

#include <cstdio>
#include <sys/stat.h>

#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "meta_updater.h"

using namespace std;

void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
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
      (meta_to_insert.begin(), meta_to_insert.end(), meta_comparator_by_id);
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::iterator nodes_begin
      (unique(meta_to_insert.rbegin(), meta_to_insert.rend(), meta_equal_id)
       .base());
  meta_to_insert.erase(meta_to_insert.begin(), nodes_begin);
  
  // fill insert
  vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::const_iterator
      nit = meta_to_insert.begin();
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((nit != meta_to_insert.end()) && (it->first == nit->first.ref))
    {
      if (it->second)
	db_to_insert[Uint31_Index(nit->second)].insert(nit->first);
      ++nit;
    }
  }
  
  meta_to_insert.clear();
  
  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton > user_db
      (&file_blocks_index);
  user_db.update(db_to_delete, db_to_insert);
}

void create_idxs_by_id
    (const vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
     map< uint32, vector< uint32 > >& idxs_by_user_id)
{
  for (vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >::const_iterator
      it = meta_to_insert.begin(); it != meta_to_insert.end(); ++it)
  {
    uint32 compressed_idx = (it->second & 0xffffff00);
    if ((compressed_idx & 0x80000000) && ((it->second & 0x3) == 0))
      compressed_idx = it->second;
    idxs_by_user_id[it->first.user_id].push_back(compressed_idx);
  }
}
  
void process_user_data(Transaction& transaction, map< uint32, string >& user_by_id,
		       map< uint32, vector< uint32 > >& idxs_by_user_id)
{
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
  {
    map< Uint32_Index, set< Uint31_Index > > db_to_delete;
    map< Uint32_Index, set< Uint31_Index > > db_to_insert;
  
    for (map< uint32, vector< uint32 > >::const_iterator it = idxs_by_user_id.begin();
        it != idxs_by_user_id.end(); ++it)
    {
      set< Uint31_Index >& ins = db_to_delete[it->first];
      for (vector< uint32 >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
	ins.insert(Uint31_Index(*it2));
    }
    for (map< uint32, vector< uint32 > >::const_iterator it = idxs_by_user_id.begin();
        it != idxs_by_user_id.end(); ++it)
    {
      set< Uint31_Index >& ins = db_to_insert[it->first];
      for (vector< uint32 >::const_iterator it2 = it->second.begin();
          it2 != it->second.end(); ++it2)
	ins.insert(Uint31_Index(*it2));
    }
    user_by_id.clear();
  
    Block_Backend< Uint32_Index, Uint31_Index > user_db
        (transaction.data_index(meta_settings().USER_INDICES));
    user_db.update(db_to_delete, db_to_insert);
  }
}

void collect_old_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   const map< uint32, vector< uint32 > >& to_delete,
   map< uint32, uint32 >& new_index_by_id,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert)
{
  map< Uint31_Index, vector< uint32 > > to_delete_meta;
  for (map< uint32, vector< uint32 > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
    to_delete_meta[Uint31_Index(it->first)] = it->second;
  
  set< Uint31_Index > user_idxs;
  for (map< uint32, vector< uint32 > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
    user_idxs.insert(Uint31_Index(it->first));
  
  // collect meta_data on its old position
  map< Uint31_Index, vector< uint32 > >::const_iterator del_it = to_delete_meta.begin();
  
  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton >
      meta_db(&file_blocks_index);
  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton >::Discrete_Iterator
      meta_it(meta_db.discrete_begin(user_idxs.begin(), user_idxs.end()));
  while (!(meta_it == meta_db.discrete_end()))
  {
    while ((del_it != to_delete_meta.end()) && (del_it->first < meta_it.index().val()))
      ++del_it;
    if (del_it == to_delete_meta.end())
      break;
    
    bool found = false;
    for (vector< uint32 >::const_iterator it = del_it->second.begin();
        it != del_it->second.end(); ++it)
      found |= (meta_it.object().ref == *it);
    
    if (found)
      meta_to_insert.push_back(make_pair(meta_it.object(), new_index_by_id[meta_it.object().ref]));
    ++meta_it;
  }
}

Transaction_Collection::Transaction_Collection
    (bool writeable, bool use_shadow,
     const string& db_dir, const vector< string >& file_name_extensions_)
     : file_name_extensions(file_name_extensions_)
{
  for (vector< string >::const_iterator it = file_name_extensions.begin();
      it != file_name_extensions.end(); ++it)
    transactions.push_back(new Nonsynced_Transaction(writeable, use_shadow, db_dir, *it));
}

Transaction_Collection::~Transaction_Collection()
{
  for (vector< Transaction* >::const_iterator it = transactions.begin();
      it != transactions.end(); ++it)
    delete(*it);
}

void Transaction_Collection::remove_referred_files(const File_Properties& file_prop)
{
  for (vector< string >::const_iterator it = file_name_extensions.begin();
      it != file_name_extensions.end(); ++it)
  {
    remove((transactions.front()->get_db_dir()
           + file_prop.get_file_name_trunk() + *it
           + file_prop.get_data_suffix()
           + file_prop.get_index_suffix()).c_str());
    remove((transactions.front()->get_db_dir()
           + file_prop.get_file_name_trunk() + *it
           + file_prop.get_data_suffix()).c_str());
  }
}

void rename_referred_file(const string& db_dir, const string& from, const string& to,
			  const File_Properties& file_prop)
{
  rename((db_dir
      + file_prop.get_file_name_trunk() + from
      + file_prop.get_data_suffix()
      + file_prop.get_index_suffix()).c_str(),
	 (db_dir
      + file_prop.get_file_name_trunk() + to
      + file_prop.get_data_suffix()
      + file_prop.get_index_suffix()).c_str());
  rename((db_dir
      + file_prop.get_file_name_trunk() + from
      + file_prop.get_data_suffix()).c_str(),
	 (db_dir
      + file_prop.get_file_name_trunk() + to
      + file_prop.get_data_suffix()).c_str());
}

void prepare_delete_tags
    (File_Blocks_Index_Base& tags_local, vector< Tag_Entry >& tags_to_delete,
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
  Block_Backend< Tag_Index_Local, Uint32_Index > rels_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry tag_entry;
  current_index.index = 0xffffffff;
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    it(rels_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == rels_db.range_end()); ++it)
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
