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
uint64_t replicate_current_map_file(
    Osm_Backend_Callback* callback, Transaction& transaction, uint64_t flush_count)
{
  Block_Backend< Index, Skeleton >
      from_db(transaction.data_index(current_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  
  uint64_t id_lower_limit = 0;
  uint64_t id_max_seen = 1;
  while (id_lower_limit < id_max_seen)
  {
    std::vector< Index > idx_buf(flush_count, Index{ 0u });

    for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
    {
      id_max_seen = std::max(id_max_seen, (uint64_t)it.object().id.val());

      if (id_lower_limit <= it.object().id.val() && it.object().id.val() < id_lower_limit + flush_count)
        idx_buf[it.object().id.val() - id_lower_limit] = it.index();
    }
    {
      Random_File< typename Skeleton::Id_Type, Index >
          into_random(into_transaction.random_index(current_skeleton_file_properties< Skeleton >()));

      auto end = std::min((uint64_t)idx_buf.size(), id_max_seen - id_lower_limit + 1);
      for (uint64_t i = 0; i < end; ++i)
      {
        if (idx_buf[i].val())
          into_random.put(id_lower_limit + i, idx_buf[i]);
      }
    }
    
    id_lower_limit += flush_count;
  }
  
  return id_max_seen;
}


template< typename Index, typename Skeleton, typename Delta >
uint64_t replicate_attic_map_file(
    Osm_Backend_Callback* callback, Transaction& transaction, uint64_t flush_count)
{
  Block_Backend< Index, Attic< Delta > >
      from_db(transaction.data_index(attic_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  
  uint64_t id_lower_limit = 0;
  uint64_t id_max_seen = 1;
  while (id_lower_limit < id_max_seen)
  {
    std::map< typename Skeleton::Id_Type, std::set< Index > > idx_lists;
    std::vector< Index > idx_buf(flush_count, Index{ 0u });

    for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
    {
      id_max_seen = std::max(id_max_seen, (uint64_t)it.object().id.val());

      if (id_lower_limit <= it.object().id.val() && it.object().id < id_lower_limit + flush_count)
      {
        Index existing = idx_buf[it.object().id.val() - id_lower_limit];        
        if (existing.val() == 0)
          idx_buf[it.object().id.val() - id_lower_limit] = it.index();
        else if (!(existing == it.index()))
        {
          auto& list = idx_lists[it.object().id];
          
          if (existing.val() != 0xff)
          {
            idx_buf[it.object().id.val() - id_lower_limit] = Index(0xffu);
            list.insert(existing);
          }
          list.insert(it.index());
        }
      }
    }
    {
      Random_File< typename Skeleton::Id_Type, Index >
          into_random(into_transaction.random_index(attic_skeleton_file_properties< Skeleton >()));

      auto end = std::min((uint64_t)idx_buf.size(), id_max_seen - id_lower_limit + 1);
      for (uint64_t i = 0; i < end; ++i)
      {
        if (idx_buf[i].val())
          into_random.put(id_lower_limit + i, idx_buf[i]);
      }
    }
    {
      Block_Backend< typename Skeleton::Id_Type, Index > into_db(
          into_transaction.data_index(attic_idx_list_properties< Skeleton >()));
      into_db.update({}, idx_lists);
    }
    
    id_lower_limit += flush_count;
  }
  
  return id_max_seen;
}


#endif
