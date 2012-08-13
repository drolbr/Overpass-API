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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__META_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__META_UPDATER_H

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>

#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../../template_db/transaction.h"

using namespace std;

template < class TObject, class TCompFunc, class TEqualFunc >
vector< TObject* > sort_elems_to_insert
    (vector< TObject >& elems_to_insert,
     TCompFunc& elem_comparator_by_id,
     TEqualFunc& elem_equal_id);

template < class TObject >
void collect_new_indexes
    (const vector< TObject* >& elems_ptr, map< uint32, uint32 >& new_index_by_id);
    
void prepare_delete_tags
    (File_Blocks_Index_Base& tags_local, vector< Tag_Entry >& tags_to_delete,
     const map< uint32, vector< uint32 > >& to_delete);

template < class TObject >
void prepare_tags
    (File_Blocks_Index_Base& tags_local, vector< TObject* >& elems_ptr,
     vector< Tag_Entry >& tags_to_delete,
     const map< uint32, vector< uint32 > >& to_delete);

template < class TObject, class Update_Logger >
void update_tags_local
    (File_Blocks_Index_Base& tags_local, vector< TObject* >& elems_ptr,
     const vector< pair< uint32, bool > >& ids_to_modify,
     const vector< Tag_Entry >& tags_to_delete,
     Update_Logger* update_logger);

template < class TObject >
void update_tags_global
    (File_Blocks_Index_Base& tags_global, vector< TObject* >& elems_ptr,
     const vector< pair< uint32, bool > >& ids_to_modify,
     const vector< Tag_Entry >& tags_to_delete);
       
void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
   const vector< pair< uint32, bool > >& ids_to_modify,
   const map< uint32, vector< uint32 > >& to_delete,
   map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > >& db_to_delete,
   map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > >& db_to_insert);

inline void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
   const vector< pair< uint32, bool > >& ids_to_modify,
   const map< uint32, vector< uint32 > >& to_delete)
{
  map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > > db_to_delete;
  map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > > db_to_insert;

  process_meta_data(file_blocks_index, meta_to_insert, ids_to_modify,
		    to_delete, db_to_delete, db_to_insert);
  
  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton > user_db
      (&file_blocks_index);
  user_db.update(db_to_delete, db_to_insert);  
}


template< class Update_Logger >
void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
   const vector< pair< uint32, bool > >& ids_to_modify,
   const map< uint32, vector< uint32 > >& to_delete,
   Update_Logger* update_logger)
{
  map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > > db_to_delete;
  map< Uint31_Index, set< OSM_Element_Metadata_Skeleton > > db_to_insert;

  process_meta_data(file_blocks_index, meta_to_insert, ids_to_modify,
		    to_delete, db_to_delete, db_to_insert);
  
  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton > user_db
      (&file_blocks_index);
  if (update_logger)
    user_db.update(db_to_delete, db_to_insert, *update_logger);
  else
    user_db.update(db_to_delete, db_to_insert);  
}


void create_idxs_by_id
    (const vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert,
     map< uint32, vector< uint32 > >& idxs_by_user_id);
   
void process_user_data(Transaction& transaction, map< uint32, string >& user_by_id,
   map< uint32, vector< uint32 > >& idxs_by_user_id);

void collect_old_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   const map< uint32, vector< uint32 > >& to_delete,
   map< uint32, uint32 >& new_index_by_id,
   vector< pair< OSM_Element_Metadata_Skeleton, uint32 > >& meta_to_insert);

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

void rename_referred_file(const string& db_dir, const string& from, const string& to,
			  const File_Properties& file_prop);
			   
class Transaction_Collection
{
  public:
    Transaction_Collection(bool writeable, bool use_shadow,
			   const string& db_dir, const vector< string >& file_name_extensions);
    ~Transaction_Collection();
    
    void remove_referred_files(const File_Properties& file_prop);
    
    vector< string > file_name_extensions;
    vector< Transaction* > transactions;
};

template < typename TIndex, typename TObject >
class Block_Backend_Collection
{
  public:
    Block_Backend_Collection
        (Transaction_Collection& transactions, const File_Properties& file_prop);
    ~Block_Backend_Collection();
    
    vector< Block_Backend< TIndex, TObject >* > dbs;
};

template < typename TIndex, typename TObject >
Block_Backend_Collection< TIndex, TObject >::Block_Backend_Collection
    (Transaction_Collection& transactions, const File_Properties& file_prop)
{
  for (vector< Transaction* >::const_iterator it = transactions.transactions.begin();
      it != transactions.transactions.end(); ++it)
    dbs.push_back(new Block_Backend< TIndex, TObject >((*it)->data_index(&file_prop)));  
}

template < typename TIndex, typename TObject >
Block_Backend_Collection< TIndex, TObject >::~Block_Backend_Collection()
{
  for (typename vector< Block_Backend< TIndex, TObject >* >::const_iterator
      it = dbs.begin(); it != dbs.end(); ++it)
    delete(*it);
}

template < typename TIndex, typename TObject >
void merge_files
    (Transaction_Collection& from_transaction, Transaction& into_transaction,
     const File_Properties& file_prop)
{
  {
    map< TIndex, set< TObject > > db_to_delete;
    map< TIndex, set< TObject > > db_to_insert;
    
    uint32 item_count = 0;
    Block_Backend_Collection< TIndex, TObject > from_dbs(from_transaction, file_prop);
    vector< pair< typename Block_Backend< TIndex, TObject >::Flat_Iterator,
        typename Block_Backend< TIndex, TObject >::Flat_Iterator > > from_its;
    set< TIndex > current_idxs;
    for (typename vector< Block_Backend< TIndex, TObject >* >::const_iterator
        it = from_dbs.dbs.begin(); it != from_dbs.dbs.end(); ++it)
    {
      from_its.push_back(make_pair((*it)->flat_begin(), (*it)->flat_end()));
      if (!(from_its.back().first == from_its.back().second))
        current_idxs.insert(from_its.back().first.index());
    }
    while (!current_idxs.empty())
    {
      TIndex current_idx = *current_idxs.begin();
      current_idxs.erase(current_idxs.begin());
      for (typename vector< pair< typename Block_Backend< TIndex, TObject >::Flat_Iterator,
	      typename Block_Backend< TIndex, TObject >::Flat_Iterator > >::iterator
	  it = from_its.begin(); it != from_its.end(); ++it)
      {
	while (!(it->first == it->second) && (it->first.index() == current_idx))
	{
	  db_to_insert[it->first.index()].insert(it->first.object());
	  ++(it->first);

	  if (++item_count > 4*1024*1024)
	  {
	    Block_Backend< TIndex, TObject > into_db
	        (into_transaction.data_index(&file_prop));
	    into_db.update(db_to_delete, db_to_insert);
	    db_to_insert.clear();
	    item_count = 0;
	  }
	}
	if (!(it->first == it->second))
	  current_idxs.insert(it->first.index());
      }
    }
    
    Block_Backend< TIndex, TObject > into_db
        (into_transaction.data_index(&file_prop));
    into_db.update(db_to_delete, db_to_insert);
  }
  from_transaction.remove_referred_files(file_prop);
}

//-----------------------------------------------------------------------------

template < class TObject, class TCompFunc, class TEqualFunc >
vector< TObject* > sort_elems_to_insert
    (vector< TObject >& elems_to_insert,
     TCompFunc& elem_comparator_by_id,
     TEqualFunc& elem_equal_id)
{
  vector< TObject* > elems_ptr;
  for (typename vector< TObject >::iterator it = elems_to_insert.begin();
      it != elems_to_insert.end(); ++it)
    elems_ptr.push_back(&*it);

  // keep always the most recent (last) element of all equal elements
  stable_sort(elems_ptr.begin(), elems_ptr.end(), elem_comparator_by_id);
  typename vector< TObject* >::iterator elems_begin
      (unique(elems_ptr.rbegin(), elems_ptr.rend(), elem_equal_id).base());
  elems_ptr.erase(elems_ptr.begin(), elems_begin);
  
  return elems_ptr;
}

template < class TObject >
void collect_new_indexes
    (const vector< TObject* >& elems_ptr, map< uint32, uint32 >& new_index_by_id)
{
  for (typename vector< TObject* >::const_iterator it = elems_ptr.begin();
      it != elems_ptr.end(); ++it)
    new_index_by_id[(*it)->id] = (*it)->index;
}

template < class TObject >
void prepare_tags
    (File_Blocks_Index_Base& tags_local, vector< TObject* >& elems_ptr,
     vector< Tag_Entry >& tags_to_delete,
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
  Block_Backend< Tag_Index_Local, Uint32_Index > elems_db(&tags_local);
  Tag_Index_Local current_index;
  Tag_Entry tag_entry;
  current_index.index = 0xffffffff;
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    it(elems_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
     !(it == elems_db.range_end()); ++it)
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
      TObject* elem(binary_ptr_search_for_id(elems_ptr, it.object().val()));
      if (elem != 0)
	elem->tags.push_back(make_pair(it.index().key, it.index().value));
      tag_entry.ids.push_back(it.object().val());
    }
  }
  if ((current_index.index != 0xffffffff) && (!tag_entry.ids.empty()))
    tags_to_delete.push_back(tag_entry);
}

template < class TObject, class Update_Logger >
void update_tags_local
    (File_Blocks_Index_Base& tags_local, vector< TObject* >& elems_ptr,
     const vector< pair< uint32, bool > >& ids_to_modify,
     const vector< Tag_Entry >& tags_to_delete,
     Update_Logger* update_logger)
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
    
    set< Uint32_Index > elem_ids;
    for (vector< uint32 >::const_iterator it2(it->ids.begin());
        it2 != it->ids.end(); ++it2)
      elem_ids.insert(*it2);
    
    db_to_delete[index] = elem_ids;
  }
  
  typename vector< TObject* >::const_iterator rit = elems_ptr.begin();
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((rit != elems_ptr.end()) && (it->first == (*rit)->id))
    {
      if (it->second)
      {
	Tag_Index_Local index;
	index.index = (*rit)->index & 0xffffff00;
	
	for (vector< pair< string, string > >::const_iterator
	  it2((*rit)->tags.begin()); it2 != (*rit)->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(it->first);
	  db_to_delete[index];
	}
      }
      ++rit;
    }
  }
  
  Block_Backend< Tag_Index_Local, Uint32_Index > elem_db(&tags_local);
  if (update_logger)
    elem_db.update(db_to_delete, db_to_insert, *update_logger);
  else
    elem_db.update(db_to_delete, db_to_insert);
}

template < class TObject >
void update_tags_global
    (File_Blocks_Index_Base& tags_global, vector< TObject* >& elems_ptr,
     const vector< pair< uint32, bool > >& ids_to_modify,
     const vector< Tag_Entry >& tags_to_delete)
{
  map< Tag_Index_Global, set< Uint32_Index > > db_to_delete;
  map< Tag_Index_Global, set< Uint32_Index > > db_to_insert;
  
  for (vector< Tag_Entry >::const_iterator it(tags_to_delete.begin());
      it != tags_to_delete.end(); ++it)
  {
    Tag_Index_Global index;
    index.key = it->key;
    index.value = it->value;
    
    set< Uint32_Index > elem_ids;
    for (vector< uint32 >::const_iterator it2(it->ids.begin());
    it2 != it->ids.end(); ++it2)
    db_to_delete[index].insert(*it2);
  }
  
  typename vector< TObject* >::const_iterator rit = elems_ptr.begin();
  for (vector< pair< uint32, bool > >::const_iterator it(ids_to_modify.begin());
      it != ids_to_modify.end(); ++it)
  {
    if ((rit != elems_ptr.end()) && (it->first == (*rit)->id))
    {
      if (it->second)
      {
	Tag_Index_Global index;
	
	for (vector< pair< string, string > >::const_iterator
	  it2((*rit)->tags.begin()); it2 != (*rit)->tags.end(); ++it2)
	{
	  index.key = it2->first;
	  index.value = it2->second;
	  db_to_insert[index].insert(it->first);
	  db_to_delete[index];
	}
      }
      ++rit;
    }
  }
  
  Block_Backend< Tag_Index_Global, Uint32_Index > elem_db(&tags_global);
  elem_db.update(db_to_delete, db_to_insert);
}

#endif
