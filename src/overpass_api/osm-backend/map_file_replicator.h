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

  Nonsynced_Transaction into_transaction(true, false, transaction.get_db_dir(), ".next");
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
  Nonsynced_Transaction into_transaction(true, false, transaction.get_db_dir(), ".next");
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
void dump_current_map_file(Osm_Backend_Callback* callback, Transaction& transaction)
{
  Block_Backend< Index, Skeleton >
      from_db(transaction.data_index(current_skeleton_file_properties< Skeleton >()));

  Nonsynced_Transaction into_transaction(true, false, transaction.get_db_dir(), ".next");
  Random_File< typename Skeleton::Id_Type, Index >
      into_random(into_transaction.random_index(current_skeleton_file_properties< Skeleton >()));

  for (auto it = from_db.flat_begin(); !(it == from_db.flat_end()); ++it)
  {
    Index map_idx = into_random.get(it.object().id);
    std::cout<<(map_idx == it.index().val())<<'\t'
        <<std::hex<<it.index().val()<<'\t'<<map_idx.val()<<'\t'
        <<std::dec<<it.object().id.val()<<'\n';
  }
}


#endif
