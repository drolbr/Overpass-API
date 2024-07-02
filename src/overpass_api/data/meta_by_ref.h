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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__META_BY_REF_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__META_BY_REF_H

#include "collect_items.h"
#include "filenames.h"
#include "idx_from_id.h"
#include "../core/type_meta.h"
#include "../../template_db/block_backend.h"

#include <vector>


template< typename Skeleton >
struct Ref_Ver_Equal
{
  bool operator()(const OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >& lhs,
                  const OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type >& rhs)
  {
    return (lhs.ref == rhs.ref && lhs.version == rhs.version);
  }
};


template< typename Index, typename Skeleton, typename Id_Type = typename Skeleton::Id_Type >
std::vector< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > meta_by_ref(
    uint64_t ref, Request_Context& context)
{
  std::vector< Index > req = get_indexes_< Index, Skeleton >({ Id_Type(ref) }, context, true);

  std::vector< OSM_Element_Metadata_Skeleton< Id_Type > > result;
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >,
        typename std::vector< Index >::const_iterator > current_meta_db(
        context.data_index(current_meta_file_properties< Skeleton >()));
    for (auto it = current_meta_db.discrete_begin(req.begin(), req.end());
        !(it == current_meta_db.discrete_end()); ++it)
    {
      if (it.object().ref == ref)
        result.push_back(it.object());
    }
  }
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< Id_Type >,
        typename std::vector< Index >::const_iterator > attic_meta_db(
        context.data_index(attic_meta_file_properties< Skeleton >()));
    for (auto it = attic_meta_db.discrete_begin(req.begin(), req.end());
        !(it == attic_meta_db.discrete_end()); ++it)
    {
      if (it.object().ref == ref)
        result.push_back(it.object());
    }
  }

  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end(), Ref_Ver_Equal< Skeleton >()), result.end());
  
  return result;
}


#endif
