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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__MAP_FILE_REPLICATOR_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__MAP_FILE_REPLICATOR_H


#include "../../template_db/block_backend.h"
#include "../../template_db/block_backend_write.h"
#include "../../template_db/transaction.h"
#include "../data/filenames.h"


template< typename Index, typename Skeleton >
void replicate_current_map_file(Osm_Backend_Callback* callback, Transaction& transaction)
{
  Block_Backend< Index, Skeleton >
      from_db(transaction.data_index(current_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  Random_File< typename Skeleton::Id_Type, Index >
      into_random(into_transaction.random_index(current_skeleton_file_properties< Skeleton >()));

  for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
    into_random.put(it.object().id, it.index().val());
}


template< typename Index, typename Skeleton >
void replicate_attic_map_file(Osm_Backend_Callback* callback, Transaction& transaction)
{
  Block_Backend< Index, Attic< Skeleton > >
      from_db(transaction.data_index(attic_skeleton_file_properties< Skeleton >()));

  std::map< typename Skeleton::Id_Type, std::set< Index > > idx_lists;
  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  {
    Random_File< typename Skeleton::Id_Type, Index >
        into_random(into_transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

    for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
    {
      Index existing = into_random.get(it.object().id);
      if (existing.val() == 0)
        into_random.put(it.object().id, it.index().val());
      else if (!(existing == it.index()))
      {
        auto& list = idx_lists[it.object().id];
        
        if (existing.val() != 0xff)
        {
          into_random.put(it.object().id, Index(0xffu));
          list.insert(existing);
        }
        list.insert(it.index());
      }
    }
  }
  {
    Block_Backend< typename Skeleton::Id_Type, Index > into_db(
        into_transaction.data_index(attic_idx_list_properties< Skeleton >()));
    into_db.update({}, idx_lists);
  }
}


template< typename Index, typename Skeleton >
void compare_current_map_files(Transaction& transaction, typename Skeleton::Id_Type maxid)
{
  Random_File< typename Skeleton::Id_Type, Index >
      from_random(transaction.random_index(current_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::readonly, false, transaction.get_db_dir(), ".next");
  Random_File< typename Skeleton::Id_Type, Index >
      into_random(into_transaction.random_index(current_skeleton_file_properties< Skeleton >()));

  uint64_t total = 0;
  uint64_t num_zero = 0;
  uint64_t num_equal = 0;
  for (decltype(maxid) i = 0ull; !(maxid < i); ++i)
  {
    Index from_idx = from_random.get(i);
    Index into_idx = into_random.get(i);
    
    ++total;
    if (from_idx == into_idx)
    {
      ++num_equal;
      num_zero += (from_idx.val() == 0);
    }
    else
      std::cout<<std::dec<<"Id "<<i.val()<<": "<<std::hex<<from_idx.val()<<' '<<into_idx.val()<<'\n';
  }
  if (total == num_equal)
    std::cout<<"Compared "<<std::dec<<total<<" entries of which all are equal and "<<num_zero<<" are zero.\n";
  else
    std::cout<<"FAILED: Compared "<<std::dec<<total<<" entries of which only "<<num_equal<<" are equal and "<<num_zero<<" are zero.\n";
}


template< typename Index, typename Skeleton >
void compare_attic_map_files(Transaction& transaction, typename Skeleton::Id_Type maxid)
{
  Random_File< typename Skeleton::Id_Type, Index >
      from_random(transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::readonly, false, transaction.get_db_dir(), ".next");
  Random_File< typename Skeleton::Id_Type, Index >
      into_random(into_transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

  uint64_t total = 0;
  uint64_t num_zero = 0;
  uint64_t num_equal = 0;
  for (decltype(maxid) i = 0ull; !(maxid < i); ++i)
  {
    Index from_idx = from_random.get(i);
    Index into_idx = into_random.get(i);
    
    ++total;
    if (from_idx == into_idx)
    {
      ++num_equal;
      num_zero += (from_idx.val() == 0);
    }
    else
      std::cout<<std::dec<<"Id "<<i.val()<<": "<<std::hex<<from_idx.val()<<' '<<into_idx.val()<<'\n';
  }
  if (total == num_equal)
    std::cout<<"Compared "<<std::dec<<total<<" entries of which all are equal and "<<num_zero<<" are zero.\n";
  else
    std::cout<<"FAILED: Compared "<<std::dec<<total<<" entries of which only "<<num_equal<<" are equal and "<<num_zero<<" are zero.\n";
}


template< typename Index, typename Skeleton >
void compare_idx_lists(Transaction& transaction)
{
  Block_Backend< typename Skeleton::Id_Type, Index > from_db(
      transaction.data_index(attic_idx_list_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::readonly, false, transaction.get_db_dir(), ".next");
  Block_Backend< typename Skeleton::Id_Type, Index > into_db(
      into_transaction.data_index(attic_idx_list_properties< Skeleton >()));

  uint64_t missing_from = 0;
  uint64_t missing_into = 0;
  uint64_t matching = 0;
  auto into_it = into_db.flat_begin(); 
  for (auto from_it = from_db.flat_begin(); !(from_it == from_db.flat_end()); ++from_it)
  {
    //std::cout<<"DEBUG "<<std::dec<<from_it.index().val()<<' '<<std::hex<<from_it.object().val()<<'\n';
    
    while (!(into_it == into_db.flat_end()) && into_it.index() < from_it.index())
    {
      std::cout<<"Element "<<std::dec<<into_it.index().val()<<" with value "
          <<std::hex<<into_it.object().val()<<" is missing in existing idx_list.\n";
      ++missing_from;
      ++into_it;
    }

    if (into_it == into_db.flat_end() || from_it.index() < into_it.index())
    {
      std::cout<<"Element "<<std::dec<<from_it.index().val()<<" with value "
          <<std::hex<<from_it.object().val()<<" is missing from next.\n";
      ++missing_into;
    }
    else if (from_it.index() == into_it.index())
    {
      while (!(into_it == into_db.flat_end()) && from_it.index() == into_it.index()
          && into_it.object() < from_it.object())
      {
        std::cout<<"Element "<<std::dec<<from_it.index().val()<<" has extra value "
            <<std::hex<<into_it.object().val()<<" in next idx_list.\n";
        ++missing_from;
        ++into_it;
      }

      if (!(into_it == into_db.flat_end()) && from_it.index() == into_it.index())
      {
        if (from_it.object() < into_it.object())
        {
          std::cout<<"Element "<<std::dec<<from_it.index().val()<<" has extra value "
              <<std::hex<<from_it.object().val()<<" in existing idx_list.\n";
          ++missing_into;
        }
        else if (from_it.object() == into_it.object())
        {
          ++matching;
          ++into_it;
        }
      }
    }
  }
  while (!(into_it == into_db.flat_end()))
  {
    std::cout<<"Element "<<std::dec<<into_it.index().val()<<" with value "
        <<std::hex<<into_it.object().val()<<" is missing in existing idx_list.\n";
    ++missing_from;
    ++into_it;
  }
  
  if (missing_from > 0 || missing_into > 0)
    std::cout<<"FAILED: Idx lists have "<<std::dec<<missing_into<<" extra objects in existing idx_list and "
        <<missing_from<<" extra objects in next idx_list.\n";
  else
    std::cout<<"Found "<<std::dec<<matching<<" objects in idx_lists.\n";
}


#endif
