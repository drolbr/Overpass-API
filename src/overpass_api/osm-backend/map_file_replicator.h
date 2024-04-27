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
  
  typename Skeleton::Id_Type id_lower_limit = 0ull;
  typename Skeleton::Id_Type flush_count = 4*1024*1024*1024ull;
  typename Skeleton::Id_Type id_max_seen = 1ull;
  while (id_lower_limit < id_max_seen)
  {
    uint64_t progress = 0;
    std::vector< Index > idx_buf(flush_count.val(), Index{ 0u });

    for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
    {
      id_max_seen = std::max(id_max_seen, it.object().id);

      if (!(it.object().id < id_lower_limit) && it.object().id < id_lower_limit + flush_count)
        idx_buf[it.object().id.val() - id_lower_limit.val()] = it.index();
      
      if (++progress % (1024*1024) == 0)
        std::cerr<<'-';
    }
    {
      Random_File< typename Skeleton::Id_Type, Index >
          into_random(into_transaction.random_index(current_skeleton_file_properties< Skeleton >()));

      auto end = std::min(
          (decltype(id_lower_limit.val()))idx_buf.size(), id_max_seen.val() - id_lower_limit.val() + 1);
      for (uint64_t i = 0; i < end; ++i)
      {
        if (idx_buf[i].val())
          into_random.put(id_lower_limit + i, idx_buf[i]);
      
        if (++progress % (1024*1024) == 0)
          std::cerr<<'=';
      }
    }
    
    std::cerr<<"\nDEBUG_current "<<id_lower_limit.val()<<' '<<flush_count.val()<<' '<<id_max_seen.val()<<'\n';
    id_lower_limit += flush_count;
  }
}


template< typename Index, typename Skeleton >
void replicate_attic_map_file(Osm_Backend_Callback* callback, Transaction& transaction)
{
  Block_Backend< Index, Attic< Skeleton > >
      from_db(transaction.data_index(attic_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  
  typename Skeleton::Id_Type id_lower_limit = 0ull;
  typename Skeleton::Id_Type flush_count = 4*1024*1024*1024ull;
  typename Skeleton::Id_Type id_max_seen = 1ull;
  while (id_lower_limit < id_max_seen)
  {
    uint64_t progress = 0;
    std::map< typename Skeleton::Id_Type, std::set< Index > > idx_lists;
    std::vector< Index > idx_buf(flush_count.val(), Index{ 0u });

    for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
    {
      id_max_seen = std::max(id_max_seen, it.object().id);

      if (!(it.object().id < id_lower_limit) && it.object().id < id_lower_limit + flush_count)
      {
        Index existing = idx_buf[it.object().id.val() - id_lower_limit.val()];        
        if (existing.val() == 0)
          idx_buf[it.object().id.val() - id_lower_limit.val()] = it.index();
        else if (!(existing == it.index()))
        {
          auto& list = idx_lists[it.object().id];
          
          if (existing.val() != 0xff)
          {
            idx_buf[it.object().id.val() - id_lower_limit.val()] = Index(0xffu);
            list.insert(existing);
          }
          list.insert(it.index());
        }
      }
      
      if (++progress % (1024*1024) == 0)
        std::cerr<<'-';
    }
    {
      Random_File< typename Skeleton::Id_Type, Index >
          into_random(into_transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

      auto end = std::min(
          (decltype(id_lower_limit.val()))idx_buf.size(), id_max_seen.val() - id_lower_limit.val() + 1);
      for (uint64_t i = 0; i < end; ++i)
      {
        if (idx_buf[i].val())
          into_random.put(id_lower_limit + i, idx_buf[i]);
      
        if (++progress % (1024*1024) == 0)
          std::cerr<<'=';
      }
    }
    {
      Block_Backend< typename Skeleton::Id_Type, Index > into_db(
          into_transaction.data_index(attic_idx_list_properties< Skeleton >()));
      into_db.update({}, idx_lists);
    }
    
    std::cerr<<"\nDEBUG_attic "<<id_lower_limit.val()<<' '<<flush_count.val()<<' '<<id_max_seen.val()<<'\n';
    id_lower_limit += flush_count;
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
      std::cout<<std::dec<<"Id "<<i.val()<<": "<<std::hex<<from_idx.val()<<' '<<into_idx.val()
        <<' '<<std::dec<<lat(from_idx.val(), 0u)<<','<<lon(from_idx.val(), 0u)
        <<' '<<std::dec<<lat(into_idx.val(), 0u)<<','<<lon(into_idx.val(), 0u)
        <<'\n';
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
      std::cout<<std::dec<<"Id "<<i.val()<<": "<<std::hex<<from_idx.val()<<' '<<into_idx.val()
        <<' '<<std::dec<<lat(from_idx.val(), 0u)<<','<<lon(from_idx.val(), 0u)
        <<' '<<std::dec<<lat(into_idx.val(), 0u)<<','<<lon(into_idx.val(), 0u)
        <<'\n';
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
          <<std::hex<<into_it.object().val()
          <<' '<<std::dec<<lat(into_it.object().val(), 0u)<<','<<lon(into_it.object().val(), 0u)
          <<" is missing in existing idx_list.\n";
      ++missing_from;
      ++into_it;
    }

    if (into_it == into_db.flat_end() || from_it.index() < into_it.index())
    {
      std::cout<<"Element "<<std::dec<<from_it.index().val()<<" with value "
          <<std::hex<<from_it.object().val()
          <<' '<<std::dec<<lat(from_it.object().val(), 0u)<<','<<lon(from_it.object().val(), 0u)
          <<" is missing from next.\n";
      ++missing_into;
    }
    else if (from_it.index() == into_it.index())
    {
      while (!(into_it == into_db.flat_end()) && from_it.index() == into_it.index()
          && into_it.object() < from_it.object())
      {
        std::cout<<"Element "<<std::dec<<from_it.index().val()<<" has extra value "
            <<std::hex<<into_it.object().val()
            <<' '<<std::dec<<lat(into_it.object().val(), 0u)<<','<<lon(into_it.object().val(), 0u)
            <<" in next idx_list.\n";
        ++missing_from;
        ++into_it;
      }

      if (!(into_it == into_db.flat_end()) && from_it.index() == into_it.index())
      {
        if (from_it.object() < into_it.object())
        {
          std::cout<<"Element "<<std::dec<<from_it.index().val()<<" has extra value "
              <<std::hex<<from_it.object().val()
              <<' '<<std::dec<<lat(from_it.object().val(), 0u)<<','<<lon(from_it.object().val(), 0u)
              <<" in existing idx_list.\n";
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
        <<std::hex<<into_it.object().val()
        <<' '<<std::dec<<lat(into_it.object().val(), 0u)<<','<<lon(into_it.object().val(), 0u)
        <<" is missing in existing idx_list.\n";
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
