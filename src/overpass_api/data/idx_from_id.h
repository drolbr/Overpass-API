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

#ifndef DE__OSM3S___OVERPASS_API__DATA__IDX_FROM_ID_H
#define DE__OSM3S___OVERPASS_API__DATA__IDX_FROM_ID_H

#include "filenames.h"
#include "request_context.h"

#include <vector>


/* Returns for the given set of ids the set of corresponding indexes.
 * The function requires that the ids are sorted ascending by id.
 */
template< typename Index, typename Skeleton >
std::vector< Index > get_indexes_
    (const std::vector< typename Skeleton::Id_Type >& ids, Request_Context& context, bool get_attic_idxs = false)
{
  std::vector< Index > result;

  Random_File< typename Skeleton::Id_Type, Index > current(context.random_index
      (current_skeleton_file_properties< Skeleton >()));
  for (typename std::vector< typename Skeleton::Id_Type >::const_iterator
      it = ids.begin(); it != ids.end(); ++it)
    result.push_back(current.get(it->val()));

  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  if (context.get_desired_timestamp() != NOW || get_attic_idxs)
  {
    Random_File< typename Skeleton::Id_Type, Index > attic_random(context.random_index
        (attic_skeleton_file_properties< Skeleton >()));
    std::vector< typename Skeleton::Id_Type > idx_list_ids;
    for (typename std::vector< typename Skeleton::Id_Type >::const_iterator
        it = ids.begin(); it != ids.end(); ++it)
    {
      if (attic_random.get(it->val()).val() == 0)
        ;
      else if (attic_random.get(it->val()) == 0xff)
        idx_list_ids.push_back(it->val());
      else
        result.push_back(attic_random.get(it->val()));
    }

    Block_Backend< typename Skeleton::Id_Type, Index > idx_list_db
        (context.data_index(attic_idx_list_properties< Skeleton >()));
    for (typename Block_Backend< typename Skeleton::Id_Type, Index >::Discrete_Iterator
        it(idx_list_db.discrete_begin(idx_list_ids.begin(), idx_list_ids.end()));
        !(it == idx_list_db.discrete_end()); ++it)
      result.push_back(it.object());

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
  }

  return result;
}


#endif
