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

#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../core/type_meta.h"
#include "basic_updater.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <cstdio>
#include <sys/stat.h>


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


void rename_referred_file(const std::string& db_dir, const std::string& from, const std::string& to,
			  const File_Properties& file_prop);


class Transaction_Collection
{
  public:
    Transaction_Collection(Access_Mode access_mode, bool use_shadow,
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


template< typename Index >
struct Meta_By_Changeset_Delta
{
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > to_remove;
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > to_add;
};


template< typename Index, typename Foo >
std::vector< Index > merge_idxs(const std::map< Index, Foo >& to_add, const std::vector< Index >& extra_idxs)
{
  std::vector< Index > result = extra_idxs;

  for (const auto& i : to_add)
    result.push_back(i.first);

  std::sort(result.begin(), result.end());
  return result;
}


template< typename Skeleton >
bool has_entry(const Data_By_Id< Skeleton >& data_by_id, uint64_t ref)
{
  auto it = std::lower_bound(data_by_id.data.begin(), data_by_id.data.end(),
      Data_By_Id< Skeleton >::Entry(Uint31_Index(0ull), Skeleton(ref),
          OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >()));
  return it != data_by_id.data.end() && it->elem.id.val() == ref;
}


// Consumes to_merge which is anyway no longer needed afterwards
// May alter data_by_id to remove objects where younger versions are already present
template< typename Index, typename Skeleton >
Meta_By_Changeset_Delta< Index > load_and_process_current(
    const std::vector< Index >& extra_idxs,
    Transaction& transaction, const File_Properties& cur_meta_file_properties,
    Meta_By_Changeset_Delta< Index >&& to_merge,
    Data_By_Id< Skeleton >& data_by_id)
{
  Meta_By_Changeset_Delta< Index > result;
  
  std::vector< Index > req = merge_idxs(to_merge.to_add, extra_idxs);
  Block_Backend< Index, Meta_Per_Changeset_Skeleton, typename std::vector< Index >::const_iterator > meta_db(
      transaction.data_index(&cur_meta_file_properties));
  auto db_it = meta_db.discrete_begin(req.begin(), req.end());
  
  auto extra_it = to_merge.to_add.begin();
  
  for (auto idx : req)
  {
    auto& loc_to_add = result.to_add[idx];
    auto& loc_to_del = result.to_remove[idx];
    
    while (extra_it != to_merge.to_add.end() && extra_it->first < idx)
      ++extra_it;  // Should never happen, but prevent infinite loop
    while (!(db_it == meta_db.discrete_end()) && db_it.index() < idx)
      ++db_it;  // Should never happen, but prevent infinite loop
      
    std::sort(extra_it->second.begin(), extra_it->second.end());

    while (!(db_it == meta_db.discrete_end()) && db_it.index() == idx)
    {
      if (!db_it.object().get_is_redacted())
      {
        Meta_Per_Changeset_Skeleton* new_entries = nullptr;
        if (extra_it != to_merge.to_add.end() && extra_it->first == idx)
        {
          auto merge_it = std::lower_bound(extra_it->second.begin(), extra_it->second.end(), db_it.object());
          if (merge_it != extra_it->second.end() && merge_it->get_changeset() == db_it.object().get_changeset())
            new_entries = &*merge_it;
        }
        
        const auto& refs = db_it.object().get_refs();
        auto ref_it = refs.begin();
        while (ref_it != refs.end() && !has_entry(data_by_id, ref_it->ref)) 
          ++ref_it;
        
        if (ref_it != refs.end())
        {
          Meta_Per_Changeset_Skeleton combined(
              db_it.object().get_changeset(), db_it.object().get_is_redacted(), db_it.object().get_user_id());
          Meta_Per_Changeset_Skeleton attic(
              db_it.object().get_changeset(), db_it.object().get_is_redacted(), db_it.object().get_user_id());

          for (auto it = refs.begin(); it != ref_it; ++it)
            combined.add_ref(*it);
          
          while (ref_it != refs.end())
          {
            if (has_entry(data_by_id, ref_it->ref))
              attic.add_ref(*ref_it);
            else
              combined.add_ref(*ref_it);

            ++ref_it;
          }
          loc_to_del.push_back(attic);
          
          if (new_entries)
            new_entries->move_refs_to(combined);
          
          loc_to_add.push_back(combined);
        }
        else if (new_entries)
        {
          Meta_Per_Changeset_Skeleton combined = db_it.object();
          new_entries->move_refs_to(combined);
          
          loc_to_del.push_back(*new_entries);
          loc_to_add.push_back(combined);
        }
      }
      
      ++db_it;
    }
    
    if (extra_it != to_merge.to_add.end() && extra_it->first == idx)
    {
      for (auto i : extra_it->second)
      {
        if (!i.get_refs().empty())
          loc_to_add.push_back(i);
      }
    }
  }
  
  return result;
}


#endif
