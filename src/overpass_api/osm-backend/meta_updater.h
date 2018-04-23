/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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



template< typename Id_Type >
struct Meta_Comparator_By_Id {
  bool operator()
  (const std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 >& a,
   const std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 >& b)
   {
     return ((a.first.ref) < b.first.ref);
   }
};


template< typename Id_Type >
struct Meta_Equal_Id {
  bool operator()
  (const std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 >& a,
   const std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 >& b)
   {
     return (a.first.ref == b.first.ref);
   }
};


template < class TObject, class TCompFunc, class TEqualFunc >
std::vector< TObject* > sort_elems_to_insert
    (std::vector< TObject >& elems_to_insert,
     TCompFunc& elem_comparator_by_id,
     TEqualFunc& elem_equal_id);


template < class TObject >
void collect_new_indexes
    (const std::vector< TObject* >& elems_ptr, std::map< uint32, uint32 >& new_index_by_id);


template< class Id_Type >
void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   std::vector< std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 > >& meta_to_insert,
   const std::vector< std::pair< Id_Type, bool > >& ids_to_modify,
   const std::map< uint32, std::vector< Id_Type > >& to_delete,
   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& db_to_delete,
   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& db_to_insert)
{
  static Meta_Comparator_By_Id< Id_Type > meta_comparator_by_id;
  static Meta_Equal_Id< Id_Type > meta_equal_id;

  // fill db_to_delete
  for (typename std::map< uint32, std::vector< Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
  {
    Uint31_Index idx(it->first);
    for (typename std::vector< Id_Type >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      db_to_delete[idx].insert(OSM_Element_Metadata_Skeleton< Id_Type >(*it2));
  }

  // keep always the most recent (last) element of all equal elements
  stable_sort
      (meta_to_insert.begin(), meta_to_insert.end(), meta_comparator_by_id);
  typename std::vector< std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 > >::iterator nodes_begin
      (unique(meta_to_insert.rbegin(), meta_to_insert.rend(), meta_equal_id)
       .base());
  meta_to_insert.erase(meta_to_insert.begin(), nodes_begin);

  // fill insert
  typename std::vector< std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 > >::const_iterator
      nit = meta_to_insert.begin();
  for (typename std::vector< std::pair< Id_Type, bool > >::const_iterator it(ids_to_modify.begin());
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
}


template< class Id_Type >
void process_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   std::vector< std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 > >& meta_to_insert,
   const std::vector< std::pair< Id_Type, bool > >& ids_to_modify,
   const std::map< uint32, std::vector< Id_Type > >& to_delete)
{
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > > db_to_delete;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > > db_to_insert;

  process_meta_data(file_blocks_index, meta_to_insert, ids_to_modify,
		    to_delete, db_to_delete, db_to_insert);

  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Id_Type > > user_db
      (&file_blocks_index);
  user_db.update(db_to_delete, db_to_insert);
}


template< typename Index, typename Object >
void copy_idxs_by_id
    (const std::map< Index, std::set< Object > >& new_data, std::map< uint32, std::vector< uint32 > >& idxs_by_user_id)
{
  for (typename std::map< Index, std::set< Object > >::const_iterator it = new_data.begin();
       it != new_data.end(); ++it)
  {
    uint32 compressed_idx = (it->first.val() & 0xffffff00);
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
      compressed_idx = it->first.val();
    for (typename std::set< Object >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      idxs_by_user_id[it2->user_id].push_back(compressed_idx);
  }
}


void process_user_data(Transaction& transaction, std::map< uint32, std::string >& user_by_id,
   std::map< uint32, std::vector< uint32 > >& idxs_by_user_id);


template< typename Id_Type >
void collect_old_meta_data
  (File_Blocks_Index_Base& file_blocks_index,
   const std::map< uint32, std::vector< Id_Type > >& to_delete,
   std::map< Id_Type, uint32 >& new_index_by_id,
   std::vector< std::pair< OSM_Element_Metadata_Skeleton< Id_Type >, uint32 > >& meta_to_insert)
{
  std::map< Uint31_Index, std::vector< Id_Type > > to_delete_meta;
  for (typename std::map< uint32, std::vector< Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
    to_delete_meta[Uint31_Index(it->first)] = it->second;

  std::set< Uint31_Index > user_idxs;
  for (typename std::map< uint32, std::vector< Id_Type > >::const_iterator
      it(to_delete.begin()); it != to_delete.end(); ++it)
    user_idxs.insert(Uint31_Index(it->first));

  // collect meta_data on its old position
  typename std::map< Uint31_Index, std::vector< Id_Type > >::const_iterator del_it = to_delete_meta.begin();

  Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Id_Type > >
      meta_db(&file_blocks_index);
  typename Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Id_Type > >::Discrete_Iterator
      meta_it(meta_db.discrete_begin(user_idxs.begin(), user_idxs.end()));
  while (!(meta_it == meta_db.discrete_end()))
  {
    while ((del_it != to_delete_meta.end()) && (del_it->first < meta_it.index().val()))
      ++del_it;
    if (del_it == to_delete_meta.end())
      break;

    bool found = false;
    for (typename std::vector< Id_Type >::const_iterator it = del_it->second.begin();
        it != del_it->second.end(); ++it)
      found |= (meta_it.object().ref == *it);

    if (found)
      meta_to_insert.push_back(std::make_pair(meta_it.object(), new_index_by_id[meta_it.object().ref]));
    ++meta_it;
  }
}


void rename_referred_file(const std::string& db_dir, const std::string& from, const std::string& to,
			  const File_Properties& file_prop);

class Transaction_Collection
{
  public:
    Transaction_Collection(bool writeable, bool use_shadow,
			   const std::string& db_dir, const std::vector< std::string >& file_name_extensions);
    ~Transaction_Collection();

    void remove_referred_files(const File_Properties& file_prop);

    std::vector< std::string > file_name_extensions;
    std::vector< Transaction* > transactions;
};

template < typename TIndex, typename TObject >
class Block_Backend_Collection
{
  public:
    Block_Backend_Collection
        (Transaction_Collection& transactions, const File_Properties& file_prop);
    ~Block_Backend_Collection();

    std::vector< Block_Backend< TIndex, TObject >* > dbs;
};

template < typename TIndex, typename TObject >
Block_Backend_Collection< TIndex, TObject >::Block_Backend_Collection
    (Transaction_Collection& transactions, const File_Properties& file_prop)
{
  for (std::vector< Transaction* >::const_iterator it = transactions.transactions.begin();
      it != transactions.transactions.end(); ++it)
    dbs.push_back(new Block_Backend< TIndex, TObject >((*it)->data_index(&file_prop)));
}

template < typename TIndex, typename TObject >
Block_Backend_Collection< TIndex, TObject >::~Block_Backend_Collection()
{
  for (typename std::vector< Block_Backend< TIndex, TObject >* >::const_iterator
      it = dbs.begin(); it != dbs.end(); ++it)
    delete(*it);
}

template < typename TIndex, typename TObject >
void merge_files
    (Transaction_Collection& from_transaction, Transaction& into_transaction,
     const File_Properties& file_prop)
{
  {
    std::map< TIndex, std::set< TObject > > db_to_delete;
    std::map< TIndex, std::set< TObject > > db_to_insert;

    uint32 item_count = 0;
    Block_Backend_Collection< TIndex, TObject > from_dbs(from_transaction, file_prop);
    std::vector< std::pair< typename Block_Backend< TIndex, TObject >::Flat_Iterator,
        typename Block_Backend< TIndex, TObject >::Flat_Iterator > > from_its;
    std::set< TIndex > current_idxs;
    for (typename std::vector< Block_Backend< TIndex, TObject >* >::const_iterator
        it = from_dbs.dbs.begin(); it != from_dbs.dbs.end(); ++it)
    {
      from_its.push_back(std::make_pair((*it)->flat_begin(), (*it)->flat_end()));
      if (!(from_its.back().first == from_its.back().second))
        current_idxs.insert(from_its.back().first.index());
    }
    while (!current_idxs.empty())
    {
      TIndex current_idx = *current_idxs.begin();
      current_idxs.erase(current_idxs.begin());
      for (typename std::vector< std::pair< typename Block_Backend< TIndex, TObject >::Flat_Iterator,
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
std::vector< TObject* > sort_elems_to_insert
    (std::vector< TObject >& elems_to_insert,
     TCompFunc& elem_comparator_by_id,
     TEqualFunc& elem_equal_id)
{
  std::vector< TObject* > elems_ptr;
  for (typename std::vector< TObject >::iterator it = elems_to_insert.begin();
      it != elems_to_insert.end(); ++it)
    elems_ptr.push_back(&*it);

  // keep always the most recent (last) element of all equal elements
  stable_sort(elems_ptr.begin(), elems_ptr.end(), elem_comparator_by_id);
  typename std::vector< TObject* >::iterator elems_begin
      (unique(elems_ptr.rbegin(), elems_ptr.rend(), elem_equal_id).base());
  elems_ptr.erase(elems_ptr.begin(), elems_begin);

  return elems_ptr;
}

template< class TObject >
void collect_new_indexes
    (const std::vector< TObject* >& elems_ptr, std::map< typename TObject::Id_Type, uint32 >& new_index_by_id)
{
  for (typename std::vector< TObject* >::const_iterator it = elems_ptr.begin();
      it != elems_ptr.end(); ++it)
    new_index_by_id[(*it)->id] = (*it)->index;
}

#endif
