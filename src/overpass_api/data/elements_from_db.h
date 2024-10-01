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

#ifndef DE__OSM3S___OVERPASS_API__DATA__ELEMENTS_FROM_DB_H
#define DE__OSM3S___OVERPASS_API__DATA__ELEMENTS_FROM_DB_H


#include "abstract_processing.h"
#include "collect_items.h"
#include "request_context.h"
#include "timeless.h"

#include <map>
#include <vector>


class Resource_Manager;
class Statement;


template < typename Index, typename Skeleton >
Timeless< Index, Skeleton > get_elements_from_db(
    const Ranges< Index >& ranges, const Statement& query, Resource_Manager& rman)
{
  Request_Context context(&query, rman);
  return collect_items_range< Index, Skeleton >(context, ranges, Trivial_Predicate< Skeleton >());
}


template < typename Index, typename Object >
class Collect_Items
{
public:
  Collect_Items(
      const std::vector< typename Object::Id_Type >& ids_, bool invert_ids_,
      const Ranges< Index >& ranges_,
      const Statement& query_, Resource_Manager& rman_)
      : ids(&ids_), invert_ids(invert_ids_), ranges(ranges_), query(&query_), rman(&rman_),
      min_idx(ranges_.empty() ? Index() : ranges_.begin().lower_bound()) {}

  bool get_chunk(
      std::map< Index, std::vector< Object > >& elements,
      std::map< Index, std::vector< Attic< Object > > >& attic_elements);

private:
  const std::vector< typename Object::Id_Type >* ids;
  bool invert_ids;
  Ranges< Index > ranges;
  const Statement* query;
  Resource_Manager* rman;
  Index min_idx;
};


template < typename Index, typename Object, typename Predicate >
bool get_elements_by_id_from_db_generic(
    std::map< Index, std::vector< Object > >& elements,
    std::map< Index, std::vector< Attic< Object > > >& attic_elements,
    const Predicate& pred,
    const Ranges< Index >& ranges, Index& cur_idx,
    Request_Context& context)
{
  if (ranges.empty())
    return false;
  return collect_items_range(context, ranges, pred, cur_idx, elements, attic_elements);
}


template < typename Index, typename Object >
bool Collect_Items< Index, Object >::get_chunk(
    std::map< Index, std::vector< Object > >& elements,
    std::map< Index, std::vector< Attic< Object > > >& attic_elements)
{
  Request_Context context(query, *rman);
  elements.clear();
  attic_elements.clear();
  if (invert_ids)
  {
    if (ids->empty())
      return get_elements_by_id_from_db_generic(
          elements, attic_elements, Trivial_Predicate< Object >(), ranges, min_idx, context);
    else
      return get_elements_by_id_from_db_generic(
          elements, attic_elements, Not_Predicate< Object, Id_Predicate< Object > >(Id_Predicate< Object >(*ids)),
          ranges, min_idx, context);
  }
  return get_elements_by_id_from_db_generic(
      elements, attic_elements, Id_Predicate< Object >(*ids), ranges, min_idx, context);
}


#endif
