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

#ifndef DE__OSM3S___OVERPASS_API__DATA__TIMELESS_H
#define DE__OSM3S___OVERPASS_API__DATA__TIMELESS_H

#include "utils.h"


template < typename Index, typename Object, typename Predicate >
void filter_items(const Predicate& predicate, std::map< Index, std::vector< Object > >& data)
{
  for (typename std::map< Index, std::vector< Object > >::iterator it = data.begin();
  it != data.end(); ++it)
  {
    std::vector< Object > local_into;
    for (typename std::vector< Object >::const_iterator iit = it->second.begin();
    iit != it->second.end(); ++iit)
    {
      if (predicate.match(*iit))
	local_into.push_back(*iit);
    }
    it->second.swap(local_into);
  }
}


template< typename TObject >
struct Compare_By_Id
{
  bool operator()(const TObject& lhs, const TObject& rhs) { return lhs.id < rhs.id; }
};


template< class TIndex, class TObject >
bool indexed_set_union(std::map< TIndex, std::vector< TObject > >& result,
		       const std::map< TIndex, std::vector< TObject > >& summand)
{
  bool result_has_grown = false;

  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
      it = summand.begin(); it != summand.end(); ++it)
  {
    if (it->second.empty())
      continue;

    std::vector< TObject >& target = result[it->first];
    if (target.empty())
    {
      target = it->second;
      result_has_grown = true;
      continue;
    }

    if (it->second.size() == 1 && target.size() > 64)
    {
      typename std::vector< TObject >::iterator it_target
          = std::lower_bound(target.begin(), target.end(), it->second.front());
      if (it_target == target.end())
      {
        target.push_back(it->second.front());
        result_has_grown = true;
      }
      else if (!(*it_target == it->second.front()))
      {
        target.insert(it_target, it->second.front());
        result_has_grown = true;
      }
    }
    else
    {
      std::vector< TObject > other;
      other.swap(target);
      std::set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
                back_inserter(target), Compare_By_Id< TObject >());

      result_has_grown |= (target.size() > other.size());
    }
  }

  return result_has_grown;
}


template< typename Index, typename Object >
struct Timeless
{
  Timeless() {}
  Timeless(
      const std::map< Index, std::vector< Object > >& current_,
      const std::map< Index, std::vector< Attic< Object > > >& attic_)
      : current(current_), attic(attic_) {}
  
  Timeless& swap(
      std::map< Index, std::vector< Object > >& rhs_current,
      std::map< Index, std::vector< Attic< Object > > >& rhs_attic)
  {
    current.swap(rhs_current);
    attic.swap(rhs_attic);
    return *this;
  }

  Timeless& sort()
  {
    sort_second(current);
    sort_second(attic);
    return *this;
  }

  Timeless& set_union(const Timeless< Index, Object >& rhs)
  {
    indexed_set_union(current, rhs.current);
    indexed_set_union(attic, rhs.attic);
    return *this;
  }

  template< typename Predicate >
  Timeless& filter_items(const Predicate& predicate)
  {
    ::filter_items(predicate, current);
    ::filter_items(predicate, attic);
    return *this;
  }

  Timeless& filter_by_id(const std::vector< typename Object::Id_Type >& ids);

  Timeless& keep_matching_skeletons(uint64 timestamp);
  
  const std::map< Index, std::vector< Object > >& get_current() const { return current; }
  const std::map< Index, std::vector< Attic< Object > > >& get_attic() const { return attic; }

private:
  std::map< Index, std::vector< Object > > current;
  std::map< Index, std::vector< Attic< Object > > > attic;
};


template< typename Index, typename Object >
Timeless< Index, Object >& Timeless< Index, Object >::keep_matching_skeletons(uint64 timestamp)
{
  if (timestamp == NOW)
  {
    attic.clear();
    return *this;
  }

  std::map< typename Object::Id_Type, uint64 > timestamp_by_id;

  for (const auto& i : current)
  {
    for (const auto& j : i.second)
      timestamp_by_id[j.id] = NOW;
  }

  for (const auto& i : attic)
  {
    for (const auto& j : i.second)
    {
      uint64& stored_timestamp = timestamp_by_id[j.id];
      if (j.timestamp > timestamp && (stored_timestamp == 0 || stored_timestamp > j.timestamp))
        stored_timestamp = j.timestamp;
    }
  }

  for (auto& i : current)
  {
    std::vector< Object > local_into;
    for (const auto& j : i.second)
    {
      if (timestamp_by_id[j.id] == NOW)
        local_into.push_back(j);
    }
    local_into.swap(i.second);
  }

  for (auto& i : attic)
  {
    std::vector< Attic< Object > > local_into;
    for (const auto& j : i.second)
    {
      if (timestamp_by_id[j.id] == j.timestamp)
        local_into.push_back(j);
    }
    local_into.swap(i.second);
  }

  return *this;
}


template< typename Index, typename Object >
Timeless< Index, Object >& Timeless< Index, Object >::filter_by_id(
    const std::vector< typename Object::Id_Type >& ids)
{
  for (auto& i : current)
  {
    std::vector< Object > into;
    for (const auto& j : i.second)
    {
      if (std::binary_search(ids.begin(), ids.end(), j.id))
        into.push_back(j);
    }
    into.swap(i.second);
  }
  for (auto& i : attic)
  {
    std::vector< Attic< Object > > into;
    for (const auto& j : i.second)
    {
      if (std::binary_search(ids.begin(), ids.end(), j.id))
        into.push_back(j);
    }
    into.swap(i.second);
  }

  return *this;
}


struct Timeless_Set
{
  Timeless< Uint32_Index, Node_Skeleton > nodes;
  Timeless< Uint31_Index, Way_Skeleton > ways;
  Timeless< Uint31_Index, Relation_Skeleton > relations;

  std::map< Uint31_Index, std::vector< Area_Skeleton > > areas;
  std::map< Uint31_Index, std::vector< Derived_Structure > > deriveds;
  
  void swap(Set& rhs)
  {
    nodes.swap(rhs.nodes, rhs.attic_nodes);
    ways.swap(rhs.ways, rhs.attic_ways);
    relations.swap(rhs.relations, rhs.attic_relations);
    
    areas.swap(rhs.areas);
    deriveds.swap(rhs.deriveds);
  }
};


#endif
