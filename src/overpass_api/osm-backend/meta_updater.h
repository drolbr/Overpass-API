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


//-----------------------------------------------------------------------------


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
    std::map< Index, std::vector< Meta_Per_Changeset_Skeleton > >&& to_merge,
    Data_By_Id< Skeleton >& data_by_id);


#endif
