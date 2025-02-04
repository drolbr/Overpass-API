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


/* General remarks about data flow for meta updates:
 * - Latest (i.e. current) versions of then exisiting objects are extracted in meta_from_fresh_data(..)
 *   into current based on their index from new_data. ...
 * - Other versions of then existing objects are extracted in meta_from_fresh_data(..)
 *   into attic based on their index from new_data. ...
 * - Each earliest version of a then deleted object is discovered not before load_and_process_current(..)
 *   because it is the earliest time at which the proper index is known.
 * - Other version of a then deleted object are extracted in meta_from_fresh_data(..)
 *   into attic based on their index from new_data.
 * By design of the database, versions of deletions are never current even if they are the latest version.
*/

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
void copy_idxs_by_id(
    const std::map< Index, std::set< Object > >& new_data,
    std::map< uint32, std::vector< uint32 > >& idxs_by_user_id)
{
  for (auto it = new_data.begin(); it != new_data.end(); ++it)
  {
    uint32 compressed_idx = (it->first.val() & 0xffffff00);
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
      compressed_idx = it->first.val();
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      idxs_by_user_id[it2->user_id].push_back(compressed_idx);
  }
}


template< typename Index >
void copy_idxs_by_user_id(
    const std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >& new_data,
    std::map< uint32_t, std::vector< uint32_t > >& idxs_by_user_id)
{
  for (auto it = new_data.begin(); it != new_data.end(); ++it)
  {
    uint32 compressed_idx = (it->first.val() & 0xffffff00);
    if ((it->first.val() & 0x80000000) && ((it->first.val() & 0x3) == 0))
      compressed_idx = it->first.val();
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      idxs_by_user_id[it2->get_user_id()].push_back(compressed_idx);
  }
}


void process_user_data(Transaction& transaction, std::map< uint32, std::string >& user_by_id,
   std::map< uint32, std::vector< uint32 > >& idxs_by_user_id);


void rename_referred_file(
    const std::string& db_dir, const std::string& from, const std::string& to, const File_Properties& file_prop);


struct Meta_By_Changeset_Triple
{
  std::map< Uint31_Index, std::vector< Meta_Per_Changeset_Skeleton > > stripped_moved;
  std::map< Uint31_Index, std::vector< Meta_Per_Changeset_Skeleton > > new_attic;
  std::map< Uint31_Index, std::vector< Meta_Per_Changeset_Skeleton > > new_current;
};


// Processes the metas of implicitly moved skeletons
// Returns
// - the changesets stripped from the entries that no longer are current because moved
// - the newly created attic entries resulting from moving the metas to attic
// - the newly created current entries resulting from moving the metas to their new current index
template< typename Skeleton >
Meta_By_Changeset_Triple load_and_process_moved_current(
    Transaction& transaction, const File_Properties& cur_meta_file_properties,
    const std::map< Uint31_Index, std::set< Skeleton > >& implicitly_moved_skeletons,
    std::vector< std::pair< typename Skeleton::Id_Type, Uint31_Index > > new_positions);


template< typename Index >
struct Meta_By_Changeset_Timeless
{
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > current;
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > attic;
};


// Assert: data_by_id is already sorted
template< typename Index, typename Skeleton >
Meta_By_Changeset_Timeless< Index > meta_from_fresh_data(const Data_By_Id< Skeleton >& data_by_id);


template< typename Index >
struct Meta_By_Changeset_Delta
{
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > to_remove;
  std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > to_add;
};


// Consumes to_merge which is anyway no longer needed afterwards.
// May alter data_by_id to remove objects where younger versions are already present - not yet implemented.
// The remove part of the return value is populated such that it
// can serve both for the update of current and record of attics to migrate.
template< typename Index, typename Skeleton >
Meta_By_Changeset_Delta< Index > load_and_process_current(
    const std::vector< std::pair< typename Skeleton::Id_Type, Index > >& extra_idxs,
    Transaction& transaction, const File_Properties& cur_meta_file_properties,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& stripped_moved,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge,
    const Data_By_Id< Skeleton >& data_by_id);


template< typename Index >
std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > merge_meta(
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& lhs,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& rhs);


template< typename Index, typename Id_Type >
std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > > realign_idx_on_meta(
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& arg,
    const std::map< Id_Type, std::vector< Attic< Index > > >& new_attic_idx_by_id_and_time);


// Consumes to_merge which is anyway no longer needed afterwards.
template< typename Index >
Meta_By_Changeset_Delta< Index > load_and_process_attic(
    Transaction& transaction, const File_Properties& attic_meta_file_properties,
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge);


/* Adds to attic_meta and new_meta the meta elements to delete resp. add from only
   implicitly moved ways. */
template< typename Id_Type >
void new_implicit_meta
    (const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >&
         existing_meta,
     const std::vector< std::pair< Id_Type, Uint31_Index > >& new_positions,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& attic_meta,
     std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Id_Type > > >& new_meta)
{
  for (auto it_idx = existing_meta.begin(); it_idx != existing_meta.end(); ++it_idx)
  {
    std::set< OSM_Element_Metadata_Skeleton< Id_Type > >& handle(attic_meta[it_idx->first]);
    for (auto it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
      handle.insert(*it);
  }

  for (auto it_idx = existing_meta.begin(); it_idx != existing_meta.end(); ++it_idx)
  {
    for (auto it = it_idx->second.begin(); it != it_idx->second.end(); ++it)
    {
      const Uint31_Index* idx = binary_pair_search(new_positions, it->ref);
      if (idx)
        new_meta[*idx].insert(*it);
    }
  }
}


// Assert: refs are sorted by ref
template< typename Index, typename Skeleton >
std::vector< Index > lookup_relevant_idxs(
    const std::vector< typename Data_By_Id< Skeleton >::Simple_Ref >& refs,
    Transaction& transaction,
    const File_Properties& random_file_props, const File_Properties& list_file_props);


// Assert: refs are sorted by ref
template< typename Index, typename Simple_Ref >
Meta_By_Changeset_Delta< Index > load_and_process_redactions(
    const std::vector< Simple_Ref >& refs, const std::vector< Index >& req,
    Transaction& transaction, const File_Properties& attic_meta_file_properties);


#endif
