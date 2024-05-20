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

#ifndef DE__OSM3S___OVERPASS_API__DATA__USER_BASED_FILTERING_H
#define DE__OSM3S___OVERPASS_API__DATA__USER_BASED_FILTERING_H

#include <set>
#include <vector>


template< typename Index, typename Object >
std::vector< typename Object::Id_Type > touched_ids_by_users(
    Resource_Manager& rman, const Ranges< Index >& ranges, const std::set< Uint32_Index >& user_ids)
{
  std::vector< typename Object::Id_Type > result;

  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > cur_meta_db(
      rman.get_transaction()->data_index(current_meta_file_properties< Object >()));
    for (auto it = cur_meta_db.range_begin(ranges); !(it == cur_meta_db.range_end()); ++it)
    {
      if (user_ids.find(it.object().user_id) != user_ids.end()
          && (result.empty() || !(result.back() == it.object().ref)))
        result.push_back(it.object().ref);
    }
  }
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > attic_meta_db(
      rman.get_transaction()->data_index(attic_meta_file_properties< Object >()));
    for (auto it = attic_meta_db.range_begin(ranges); !(it == attic_meta_db.range_end()); ++it)
    {
      if (user_ids.find(it.object().user_id) != user_ids.end()
          && (result.empty() || !(result.back() == it.object().ref)))
        result.push_back(it.object().ref);
    }
  }

  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());

  return result;
}


template< typename Id_Type >
struct Touch_State
{
  Id_Type ref;
  uint64_t first_touched;
  uint64_t last_touched;
  uint32_t version;
  
  bool operator<(const Touch_State& rhs) const
  { 
    if (ref < rhs.ref)
      return true;
    if (rhs.ref < ref)
      return false;
    return first_touched < rhs.first_touched;
  }
};


template< typename Index, typename Object >
std::vector< Touch_State< typename Object::Id_Type > > detect_impacted_versions(
    Resource_Manager& rman, const Ranges< Index >& ranges, const std::set< Uint32_Index >& user_ids)
{
  std::vector< Touch_State< typename Object::Id_Type > > found;

  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > attic_meta_db(
      rman.get_transaction()->data_index(attic_meta_file_properties< Object >()));
    for (auto it = attic_meta_db.range_begin(ranges); !(it == attic_meta_db.range_end()); ++it)
    {
      if (user_ids.find(it.object().user_id) != user_ids.end())
      {
        if (!found.empty() && found.back().ref == it.object().ref)
        {
          found.back().first_touched = std::min(found.back().first_touched, (uint64_t)it.object().timestamp);
          found.back().last_touched = std::max(found.back().last_touched, (uint64_t)it.object().timestamp);
          found.back().version = std::max(found.back().version, it.object().version);
        }
        else
          found.push_back(
              { it.object().ref, it.object().timestamp, it.object().timestamp, it.object().version });
      }
    }
  }

  std::sort(found.begin(), found.end());
  auto to_it = found.begin();
  for (auto from_it = found.begin(); from_it != found.end(); ++from_it)
  {
    if (to_it->ref < from_it->ref)
    {
      ++to_it;
      *to_it = *from_it;
    }
    else
    {
      to_it->first_touched = std::min(to_it->first_touched, from_it->first_touched);
      to_it->last_touched = std::max(to_it->last_touched, from_it->last_touched);
      to_it->version = std::max(to_it->version, from_it->version);
    }
  }
  if (to_it != found.end())
    found.erase(++to_it, found.end());

  decltype(found) extra_now;
  {
    Block_Backend< Index, OSM_Element_Metadata_Skeleton< typename Object::Id_Type > > cur_meta_db(
      rman.get_transaction()->data_index(current_meta_file_properties< Object >()));
    for (auto it = cur_meta_db.range_begin(ranges); !(it == cur_meta_db.range_end()); ++it)
    {
      auto found_it = std::lower_bound(
          found.begin(), found.end(), Touch_State< typename Object::Id_Type >{ it.object().ref, 0 });
      if (found_it != found.end() && found_it->ref == it.object().ref)
      {
        if (user_ids.find(it.object().user_id) != user_ids.end())
          found_it->last_touched = NOW;
        found_it->version = it.object().version;
      }
      else if (user_ids.find(it.object().user_id) != user_ids.end())
        extra_now.push_back({ it.object().ref, it.object().timestamp, NOW, it.object().version });
    }
  }
  
  std::sort(extra_now.begin(), extra_now.end());
  decltype(found) result;
  result.reserve(found.size() + extra_now.size());
  std::merge(found.begin(), found.end(), extra_now.begin(), extra_now.end(), std::back_inserter(result));

  return result;
}


#endif
