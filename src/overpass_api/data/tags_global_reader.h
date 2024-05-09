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

#ifndef DE__OSM3S___OVERPASS_API__DATA__TAGS_GLOBAL_READER_H
#define DE__OSM3S___OVERPASS_API__DATA__TAGS_GLOBAL_READER_H


#include <map>
#include <string>
#include <vector>


#include "../../template_db/block_backend.h"
#include "../core/datatypes.h"
#include "../core/settings.h"
#include "../statements/statement.h"
#include "../dispatch/resource_manager.h"
#include "filenames.h"
#include "filter_by_tags.h"
#include "regular_expression.h"


template < typename T >
struct Optional
{
  Optional(T* obj_) : obj(obj_) {}
  ~Optional() { delete obj; }

  T* obj;
};


template< typename Id_Type >
bool operator<(const std::pair< Id_Type, Uint31_Index >& lhs, const std::pair< Id_Type, Uint31_Index >& rhs)
{
  return lhs.first < rhs.first;
}


template< typename Id_Type >
bool operator==(const std::pair< Id_Type, Uint31_Index >& lhs, const std::pair< Id_Type, Uint31_Index >& rhs)
{
  return lhs.first == rhs.first;
}


struct Limit_Abort_Condition
{
public:
  Limit_Abort_Condition(uint64_t limit_) : limit(limit_) {}

  bool positive_cnt(uint64_t cnt)
  {
    has_triggered |= cnt > limit;
    return cnt > limit;
  }

  uint64_t limit = 0;
  bool has_triggered = false;
};


template< typename Id_Type, typename Abort_Condition >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_kv(
    const std::pair< std::string, std::string >& kvit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db,
    Abort_Condition* abort, std::vector< std::pair< Id_Type, Uint31_Index > >* relevant_ids = 0)
{
  std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id;
  Ranges< Tag_Index_Global > tag_req = get_kv_req(kvit.first, kvit.second);

  for (auto it2 = tags_db.range_begin(tag_req); !(it2 == tags_db.range_end()); ++it2)
  {
    if (!relevant_ids || std::binary_search(
        relevant_ids->begin(), relevant_ids->end(), std::make_pair(it2.object().id, Uint31_Index(0u))))
      timestamp_per_id[it2.object().id] = std::make_pair(NOW, it2.object().idx);

    if (abort && abort->positive_cnt(timestamp_per_id.size()))
      return {};
  }

  for (auto it2 = attic_tags_db.range_begin(tag_req); !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp &&
        (!relevant_ids || std::binary_search(
        relevant_ids->begin(), relevant_ids->end(), std::make_pair(it2.object().id, Uint31_Index(0u)))))
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }

    if (abort && abort->positive_cnt(timestamp_per_id.size()))
      return {};
  }

  Ranges< Tag_Index_Global > ranges = get_k_req(kvit.first);

  for (auto it2 = attic_tags_db.range_begin(ranges); !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp)
    {
      typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
        if (it2.object().timestamp < it->second.first)
          timestamp_per_id.erase(it);
      }
    }
  }

  return timestamp_per_id;
}


template< class Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_k(
    std::vector< std::string >::const_iterator kit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db)
{
  std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id;
  Ranges< Tag_Index_Global > ranges = get_k_req(*kit);

  for (auto it2 = tags_db.range_begin(ranges); !(it2 == tags_db.range_end()); ++it2)
    timestamp_per_id[it2.object().id] = std::make_pair(NOW, it2.object().idx);

  for (auto it2 = attic_tags_db.range_begin(ranges); !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp && it2.index().value != void_tag_value())
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }

  for (auto it2 = attic_tags_db.range_begin(ranges); !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp && it2.index().value == void_tag_value())
    {
      typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
        if (it2.object().timestamp < it->second.first)
          timestamp_per_id.erase(it);
      }
    }
  }

  return timestamp_per_id;
}


template< class Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_kregv(
    std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator krit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db,
    std::vector< std::pair< Id_Type, Uint31_Index > >* relevant_ids = 0)
{
  std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id;
  Ranges< Tag_Index_Global > ranges = get_k_req(krit->first);

  for (auto it2 = tags_db.range_begin(ranges); !(it2 == tags_db.range_end()); ++it2)
  {
    if ((!relevant_ids || std::binary_search(
        relevant_ids->begin(), relevant_ids->end(), std::make_pair(it2.object().id, Uint31_Index(0u))))
        && krit->second->matches(it2.index().value))
      timestamp_per_id[it2.object().id] = std::make_pair(NOW, it2.object().idx);
  }

  for (auto it2 = attic_tags_db.range_begin(ranges); !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp && it2.index().value != void_tag_value()
        && (!relevant_ids || std::binary_search(
            relevant_ids->begin(), relevant_ids->end(), std::make_pair(it2.object().id, Uint31_Index(0u))))
        && krit->second->matches(it2.index().value))
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }

  for (auto it2 = attic_tags_db.range_begin(ranges); !(it2 == attic_tags_db.range_end()); ++it2)
  {
    if (it2.object().timestamp > timestamp)
    {
      typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
        if (it2.object().timestamp < it->second.first)
          timestamp_per_id.erase(it);
      }
    }
  }

  return timestamp_per_id;
}


template< typename Skeleton, typename Id_Type >
std::map< Id_Type, std::pair< uint64, Uint31_Index > > collect_attic_regkregv(
    std::vector< std::pair< Regular_Expression*, Regular_Expression* > >::const_iterator krit, uint64 timestamp,
    Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >& tags_db,
    Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >& attic_tags_db,
    Resource_Manager& rman, const Statement& stmt)
{
  std::map< Id_Type, std::map< std::string, std::pair< uint64, Uint31_Index > > > timestamp_per_id;
  Ranges< Tag_Index_Global > ranges = get_regk_req< Skeleton >(krit->first, rman, stmt);

  std::string last_key = void_tag_value();
  bool matches = false;
  for (auto it2 = tags_db.range_begin(ranges); !(it2 == tags_db.range_end()); ++it2)
  {
    if (it2.index().key != last_key)
    {
      last_key = it2.index().key;
      matches = krit->first->matches(it2.index().key);
    }
    if (matches && krit->second->matches(it2.index().value))
      timestamp_per_id[it2.object().id][last_key] = std::make_pair(NOW, it2.object().idx);
  }

  last_key = void_tag_value();
  matches = false;
  for (auto it2 = attic_tags_db.flat_begin(); !(it2 == attic_tags_db.flat_end()); ++it2)
  {
    if (it2.index().key != last_key)
    {
      last_key = it2.index().key;
      matches = krit->first->matches(it2.index().key);
    }
    if (it2.object().timestamp > timestamp && matches && it2.index().value != void_tag_value()
        && krit->second->matches(it2.index().value))
    {
      std::pair< uint64, Uint31_Index >& ref = timestamp_per_id[it2.object().id][last_key];
      if (ref.first == 0 || it2.object().timestamp < ref.first)
        ref = std::make_pair(it2.object().timestamp, it2.object().idx);
    }
  }

  last_key = void_tag_value();
  matches = false;
  for (auto it2 = attic_tags_db.flat_begin(); !(it2 == attic_tags_db.flat_end()); ++it2)
  {
    if (it2.index().key != last_key)
    {
      last_key = it2.index().key;
      matches = krit->first->matches(it2.index().key);
    }
    if (matches && it2.object().timestamp > timestamp)
    {
      typename std::map< Id_Type, std::map< std::string, std::pair< uint64, Uint31_Index > > >::iterator
          it = timestamp_per_id.find(it2.object().id);
      if (it != timestamp_per_id.end())
      {
	typename std::map< std::string, std::pair< uint64, Uint31_Index > >::iterator
	    it3 = it->second.find(last_key);
	if (it3 != it->second.end())
	{
	  if (it2.object().timestamp < it3->second.first)
	    it->second.erase(it3);
	}
      }
    }
  }

  std::map< Id_Type, std::pair< uint64, Uint31_Index > > result;
  for (typename std::map< Id_Type, std::map< std::string, std::pair< uint64, Uint31_Index > > >::const_iterator
      it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
  {
    if (!it->second.empty())
      result[it->first] = it->second.begin()->second;
  }

  return result;
}


struct Trivial_Regex
{
public:
  bool matches(const std::string&) const { return true; }
};


template< typename Id_Type, typename Iterator, typename Key_Regex, typename Val_Regex >
void filter_id_list(
    std::vector< std::pair< Id_Type, Uint31_Index > >& new_ids, bool& filtered,
    Iterator begin, Iterator end, const Key_Regex& key_regex, const Val_Regex& val_regex,
    bool limit_size)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > old_ids;
  old_ids.swap(new_ids);

  for (Iterator it = begin; !(it == end); ++it)
  {
    if (key_regex.matches(it.index().key) && it.index().value != void_tag_value()
        && val_regex.matches(it.index().value) && (!filtered ||
	std::binary_search(old_ids.begin(), old_ids.end(), std::make_pair(it.object().id, Uint31_Index(0u)))))
      new_ids.push_back(std::make_pair(it.object().id, it.object().idx));

    if (!filtered && limit_size && new_ids.size() == 1024*1024)
    {
      new_ids.clear();
      return;
    }
  }

  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());

  filtered = true;
}


template< typename Id_Type, typename Iterator, typename Key_Regex, typename Val_Regex >
void filter_id_list(
    std::vector< Id_Type >& new_ids, bool& filtered,
    Iterator begin, Iterator end, const Key_Regex& key_regex, const Val_Regex& val_regex)
{
  std::vector< Id_Type > old_ids;
  old_ids.swap(new_ids);

  for (Iterator it = begin; !(it == end); ++it)
  {
    if (key_regex.matches(it.index().key) && it.index().value != void_tag_value()
        && val_regex.matches(it.index().value) &&
	(!filtered || std::binary_search(old_ids.begin(), old_ids.end(), it.object())))
      new_ids.push_back(it.object());
  }

  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());

  filtered = true;
}


template< typename Id_Type, typename Container >
void filter_id_list(
    std::vector< std::pair< Id_Type, Uint31_Index > >& new_ids, bool& filtered,
    const Container& container, bool limit_size)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > old_ids;
  old_ids.swap(new_ids);

  if (!filtered && limit_size && container.size() >= 1024*1024)
    return;

  for (typename Container::const_iterator it = container.begin(); it != container.end(); ++it)
  {
    if (!filtered ||
	std::binary_search(old_ids.begin(), old_ids.end(), std::make_pair(it->first, Uint31_Index(0u))))
      new_ids.push_back(std::make_pair(it->first, it->second.second));
  }

  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());

  filtered = true;
}


class Statement;


template< typename Skeleton, typename Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > collect_ids(
    const std::vector< std::string >& keys,
    const std::vector< std::pair< std::string, std::string > >& key_values,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_regexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_regexes,
    const File_Properties& file_prop, const File_Properties& attic_file_prop,
    Resource_Manager& rman, const Statement& stmt,
    uint64 timestamp, Query_Filter_Strategy check_keys_late, bool& result_valid)
{
  if (key_values.empty() && keys.empty() && key_regexes.empty() && regkey_regexes.empty())
    return std::vector< std::pair< Id_Type, Uint31_Index > >();

  Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  Optional< Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > > > attic_tags_db
      (timestamp == NOW ? 0 :
        new Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >
        (rman.get_transaction()->data_index(&attic_file_prop)));

  // Handle simple Key-Value pairs
  std::vector< std::pair< Id_Type, Uint31_Index > > new_ids;
  bool filtered = false;
  result_valid = false;

  for (std::vector< std::pair< std::string, std::string > >::const_iterator kvit = key_values.begin();
       kvit != key_values.end(); ++kvit)
  {
    if (timestamp == NOW)
    {
      Ranges< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
      filter_id_list(
          new_ids, filtered, tags_db.range_begin(tag_req), tags_db.range_end(),
          Trivial_Regex(), Trivial_Regex(), check_keys_late == prefer_ranges || check_keys_late == ids_useful);
      if (!filtered)
        return new_ids;
    }
    else
    {
      Limit_Abort_Condition abort_limit{ filtered ? std::numeric_limits< uint64_t >::max() : 1024*1024 };
      filter_id_list(new_ids, filtered,
          collect_attic_kv(*kvit, timestamp, tags_db, *attic_tags_db.obj, &abort_limit),
          check_keys_late == prefer_ranges || check_keys_late == ids_useful);
      if (abort_limit.has_triggered)
        return new_ids;
    }

    rman.health_check(stmt);
  }

  if (check_keys_late != prefer_ranges)
  {
    // Handle simple Keys Only
    for (std::vector< std::string >::const_iterator kit = keys.begin(); kit != keys.end(); ++kit)
    {
      if (timestamp == NOW)
      {
        Ranges< Tag_Index_Global > ranges = get_k_req(*kit);
	filter_id_list(
            new_ids, filtered, tags_db.range_begin(ranges), tags_db.range_end(),
            Trivial_Regex(), Trivial_Regex(), check_keys_late == ids_useful);
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_k(kit, timestamp, tags_db, *attic_tags_db.obj),
            check_keys_late == ids_useful);
      if (!filtered)
        return new_ids;

      rman.health_check(stmt);
    }

    // Handle Key-Regular-Expression-Value pairs
    for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator krit = key_regexes.begin();
	 krit != key_regexes.end(); ++krit)
    {
      if (timestamp == NOW)
      {
        Ranges< Tag_Index_Global > ranges = get_k_req(krit->first);
        filter_id_list(
            new_ids, filtered, tags_db.range_begin(ranges), tags_db.range_end(),
            Trivial_Regex(), *krit->second, check_keys_late == ids_useful);
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_kregv(krit, timestamp, tags_db, *attic_tags_db.obj),
            check_keys_late == ids_useful);
      if (!filtered)
        return new_ids;

      rman.health_check(stmt);
    }

    // Handle Regular-Key-Regular-Expression-Value pairs
    for (std::vector< std::pair< Regular_Expression*, Regular_Expression* > >::const_iterator it = regkey_regexes.begin();
	 it != regkey_regexes.end(); ++it)
    {
      if (timestamp == NOW)
      {
        Ranges< Tag_Index_Global > ranges = get_regk_req< Skeleton >(it->first, rman, stmt);
        filter_id_list(
            new_ids, filtered, tags_db.range_begin(ranges), tags_db.range_end(),
            *it->first, *it->second, check_keys_late == ids_useful);
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_regkregv< Skeleton, Id_Type >(
	    it, timestamp, tags_db, *attic_tags_db.obj, rman, stmt),
            check_keys_late == ids_useful);
      if (!filtered)
        return new_ids;

      rman.health_check(stmt);
    }
  }

  result_valid = true;
  return new_ids;
}


template< class Id_Type >
std::vector< Id_Type > collect_ids(
    const std::vector< std::string >& keys,
    const std::vector< std::pair< std::string, std::string > >& key_values,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_regexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_regexes,
    const File_Properties& file_prop, Resource_Manager& rman, const Statement& stmt,
    Query_Filter_Strategy check_keys_late)
{
  if (key_values.empty() && keys.empty() && key_regexes.empty() && regkey_regexes.empty())
    return std::vector< Id_Type >();

  Block_Backend< Tag_Index_Global, Id_Type > tags_db
      (rman.get_transaction()->data_index(&file_prop));

  // Handle simple Key-Value pairs
  std::vector< Id_Type > new_ids;
  bool filtered = false;

  for (std::vector< std::pair< std::string, std::string > >::const_iterator kvit = key_values.begin();
       kvit != key_values.end(); ++kvit)
  {
    Ranges< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
    filter_id_list(new_ids, filtered,
	tags_db.range_begin(tag_req), tags_db.range_end(), Trivial_Regex(), Trivial_Regex());

    rman.health_check(stmt);
  }

  if (check_keys_late != prefer_ranges)
  {
    // Handle simple Keys Only
    for (std::vector< std::string >::const_iterator kit = keys.begin(); kit != keys.end(); ++kit)
    {
      Ranges< Tag_Index_Global > ranges = get_k_req(*kit);
      filter_id_list(new_ids, filtered,
	  tags_db.range_begin(ranges), tags_db.range_end(),
	      Trivial_Regex(), Trivial_Regex());

      rman.health_check(stmt);
    }

    // Handle Key-Regular-Expression-Value pairs
    for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator krit = key_regexes.begin();
	 krit != key_regexes.end(); ++krit)
    {
      Ranges< Tag_Index_Global > ranges = get_k_req(krit->first);
      filter_id_list(new_ids, filtered, tags_db.range_begin(ranges), tags_db.range_end(),
          Trivial_Regex(), *krit->second);

      rman.health_check(stmt);
    }

    // Handle Key-Regular-Expression-Value pairs
    for (std::vector< std::pair< Regular_Expression*, Regular_Expression* > >::const_iterator it = regkey_regexes.begin();
	 it != regkey_regexes.end(); ++it)
    {
      filter_id_list(new_ids, filtered,
	  tags_db.flat_begin(), tags_db.flat_end(), *it->first, *it->second);

      rman.health_check(stmt);
    }
  }

  return new_ids;
}


template< typename Id_Type >
class Id_Remover
{
public:
  Id_Remover(std::vector< std::pair< Id_Type, Uint31_Index > >& ids_) : ids(ids_), found(ids.size(), false) {}
  ~Id_Remover()
  {
    auto t_it = ids.begin();
    decltype(found.size()) i = 0;
    for (auto s_it = ids.begin(); s_it != ids.end(); ++s_it)
    {
      if (!found[i++])
      {
        *t_it = *s_it;
        ++t_it;
      }
    }
    ids.erase(t_it, ids.end());
  }
  void remove(Id_Type id)
  {
    auto it = std::lower_bound(ids.begin(), ids.end(), std::pair< Id_Type, Uint31_Index >{ id, Uint31_Index(0u) });
    if (it != ids.end() && it->first == id)
      found[std::distance(ids.begin(), it)] = true;
  }

private:
  std::vector< std::pair< Id_Type, Uint31_Index > >& ids;
  std::vector< bool > found;
};


template< class Id_Type >
void filter_non_ids(
    const std::vector< std::pair< std::string, std::string > >& key_nvalues,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_nregexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_nregexes,
    std::vector< std::pair< Id_Type, Uint31_Index > >& ids,
    const File_Properties& file_prop, const File_Properties& attic_file_prop,
    Resource_Manager& rman, const Statement& stmt, uint64 timestamp)
{
  if (key_nvalues.empty() && key_nregexes.empty())
    return;

  Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  Optional< Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > > > attic_tags_db
      (timestamp == NOW ? 0 :
        new Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >
        (rman.get_transaction()->data_index(&attic_file_prop)));
  Id_Remover< Id_Type > idr(ids);

  // Handle Key-Non-Value pairs
  for (std::vector< std::pair< std::string, std::string > >::const_iterator knvit = key_nvalues.begin();
      knvit != key_nvalues.end(); ++knvit)
  {
    if (timestamp == NOW)
    {
      Ranges< Tag_Index_Global > tag_req = get_kv_req(knvit->first, knvit->second);
      for (auto it2 = tags_db.range_begin(tag_req); !(it2 == tags_db.range_end()); ++it2)
        idr.remove(it2.object().id);
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kv(*knvit, timestamp, tags_db, *attic_tags_db.obj, (Limit_Abort_Condition*)nullptr, &ids);

      for (typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::const_iterator
          it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
        idr.remove(it->first);
    }
    rman.health_check(stmt);
  }

  // Handle Key-Regular-Expression-Non-Value pairs
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator knrit = key_nregexes.begin();
      knrit != key_nregexes.end(); ++knrit)
  {
    if (timestamp == NOW)
    {
      Ranges< Tag_Index_Global > ranges = get_k_req(knrit->first);
      for (auto it2 = tags_db.range_begin(ranges); !it2.is_end(); ++it2)
      {
        if (knrit->second->matches(it2.index().value))
          idr.remove(it2.object().id);
      }
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kregv(knrit, timestamp, tags_db, *attic_tags_db.obj, &ids);

      for (typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::const_iterator
          it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
        idr.remove(it->first);
    }
    rman.health_check(stmt);
  }
}


template< class Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > collect_non_ids(
    const std::vector< std::pair< std::string, std::string > >& key_nvalues,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_nregexes,
    const File_Properties& file_prop, const File_Properties& attic_file_prop,
    Resource_Manager& rman, const Statement& stmt, uint64 timestamp)
{
  if (key_nvalues.empty() && key_nregexes.empty())
    return std::vector< std::pair< Id_Type, Uint31_Index > >();

  Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > > tags_db
      (rman.get_transaction()->data_index(&file_prop));
  Optional< Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > > > attic_tags_db
      (timestamp == NOW ? 0 :
        new Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< Id_Type > > >
        (rman.get_transaction()->data_index(&attic_file_prop)));

  std::vector< std::pair< Id_Type, Uint31_Index > > new_ids;

  // Handle Key-Non-Value pairs
  for (std::vector< std::pair< std::string, std::string > >::const_iterator knvit = key_nvalues.begin();
      knvit != key_nvalues.end(); ++knvit)
  {
    if (timestamp == NOW)
    {
      Ranges< Tag_Index_Global > tag_req = get_kv_req(knvit->first, knvit->second);
      for (auto it2 = tags_db.range_begin(tag_req); !(it2 == tags_db.range_end()); ++it2)
        new_ids.push_back(std::make_pair(it2.object().id, it2.object().idx));
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kv(*knvit, timestamp, tags_db, *attic_tags_db.obj, (Limit_Abort_Condition*)nullptr);

      for (typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::const_iterator
          it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
        new_ids.push_back(std::make_pair(it->first, it->second.second));
    }
    rman.health_check(stmt);
  }

  // Handle Key-Regular-Expression-Non-Value pairs
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator knrit = key_nregexes.begin();
      knrit != key_nregexes.end(); ++knrit)
  {
    if (timestamp == NOW)
    {
      Ranges< Tag_Index_Global > ranges = get_k_req(knrit->first);
      for (auto it2 = tags_db.range_begin(ranges); !it2.is_end(); ++it2)
      {
        if (knrit->second->matches(it2.index().value))
          new_ids.push_back(std::make_pair(it2.object().id, it2.object().idx));
      }
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kregv(knrit, timestamp, tags_db, *attic_tags_db.obj);

      for (typename std::map< Id_Type, std::pair< uint64, Uint31_Index > >::const_iterator
          it = timestamp_per_id.begin(); it != timestamp_per_id.end(); ++it)
        new_ids.push_back(std::make_pair(it->first, it->second.second));
    }
    rman.health_check(stmt);
  }

  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());

  return new_ids;
}


template< class Id_Type >
std::vector< Id_Type > collect_non_ids(
    const std::vector< std::pair< std::string, std::string > >& key_nvalues,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_nregexes,
    const File_Properties& file_prop, Resource_Manager& rman, const Statement& stmt)
{
  if (key_nvalues.empty() && key_nregexes.empty())
    return std::vector< Id_Type >();

  Block_Backend< Tag_Index_Global, Id_Type > tags_db
      (rman.get_transaction()->data_index(&file_prop));

  std::vector< Id_Type > new_ids;

  // Handle Key-Non-Value pairs
  for (std::vector< std::pair< std::string, std::string > >::const_iterator knvit = key_nvalues.begin();
      knvit != key_nvalues.end(); ++knvit)
  {
    Ranges< Tag_Index_Global > ranges = get_k_req(knvit->first);
    for (auto it2 = tags_db.range_begin(ranges); !it2.is_end(); ++it2)
    {
      if (it2.index().value == knvit->second)
        new_ids.push_back(it2.object());
    }

    rman.health_check(stmt);
  }

  // Handle Key-Regular-Expression-Non-Value pairs
  for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator knrit = key_nregexes.begin();
      knrit != key_nregexes.end(); ++knrit)
  {
    Ranges< Tag_Index_Global > ranges = get_k_req(knrit->first);
    for (auto it2 = tags_db.range_begin(ranges); !it2.is_end(); ++it2)
    {
      if (it2.index().value != void_tag_value() && knrit->second->matches(it2.index().value))
        new_ids.push_back(it2.object());
    }

    rman.health_check(stmt);
  }

  sort(new_ids.begin(), new_ids.end());
  new_ids.erase(unique(new_ids.begin(), new_ids.end()), new_ids.end());

  return new_ids;
}


template< typename Skeleton, typename Id_Type, typename Index >
void progress_1(
    const std::vector< std::string >& keys,
    const std::vector< std::pair< std::string, std::string > >& key_values,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_regexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_regexes,
    const std::vector< std::pair< std::string, std::string > >& key_nvalues,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_nregexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_nregexes,
    Id_Constraint< Id_Type >& ids, std::vector< Index >& range_vec,
    uint64 timestamp, Query_Filter_Strategy& check_keys_late,
    Resource_Manager& rman, const Statement& stmt)
{
  File_Properties* file_prop = current_global_tags_file_properties< Skeleton >();
  File_Properties* attic_file_prop = attic_global_tags_file_properties< Skeleton >();

  ids.ids.clear();
  range_vec.clear();
  if (!key_values.empty()
      || (check_keys_late != prefer_ranges
          && (!keys.empty() || !key_regexes.empty() || !regkey_regexes.empty())))
  {
    bool result_valid = true;
    std::vector< std::pair< Id_Type, Uint31_Index > > id_idxs =
        collect_ids< Skeleton, Id_Type >(
            keys, key_values, key_regexes, regkey_regexes,
            *file_prop, *attic_file_prop, rman, stmt, timestamp, check_keys_late, result_valid);
    if (check_keys_late == ids_useful && !result_valid)
      check_keys_late = prefer_ranges;
    ids.invert = !result_valid;

    if (!key_nvalues.empty() || (check_keys_late != prefer_ranges && !key_nregexes.empty()))
      filter_non_ids< Id_Type >(
          key_nvalues, key_nregexes, regkey_nregexes, id_idxs, *file_prop, *attic_file_prop, rman, stmt, timestamp);

    for (auto it = id_idxs.begin(); it != id_idxs.end(); ++it)
    {
      ids.ids.push_back(it->first);
      range_vec.push_back(it->second);
    }
  }
  else if ((!key_nvalues.empty() || !key_nregexes.empty()) && check_keys_late != prefer_ranges)
  {
    std::vector< std::pair< Id_Type, Uint31_Index > > id_idxs =
        collect_non_ids< Id_Type >(
            key_nvalues, key_nregexes, *file_prop, *attic_file_prop, rman, stmt, timestamp);
    for (typename std::vector< std::pair< Id_Type, Uint31_Index > >::const_iterator it = id_idxs.begin();
        it != id_idxs.end(); ++it)
      ids.ids.push_back(it->first);
  }
}


template< class Id_Type >
void progress_1(
    const std::vector< std::string >& keys,
    const std::vector< std::pair< std::string, std::string > >& key_values,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_regexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_regexes,
    const std::vector< std::pair< std::string, std::string > >& key_nvalues,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_nregexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_nregexes,
    Id_Constraint< Id_Type >& ids, Query_Filter_Strategy check_keys_late,
    Resource_Manager& rman, const Statement& stmt)
{
  if (!key_values.empty()
      || (check_keys_late != prefer_ranges
          && (!keys.empty() || !key_regexes.empty() || !regkey_regexes.empty())))
  {
    ids.invert = false;
    collect_ids< Id_Type >(
        keys, key_values, key_regexes, regkey_regexes,
        *area_settings().AREA_TAGS_GLOBAL, rman, stmt, check_keys_late).swap(ids.ids);
    if (!key_nvalues.empty() || !key_nregexes.empty() || !regkey_nregexes.empty())
    {
      std::vector< Id_Type > non_ids = collect_non_ids< Id_Type >(
          key_nvalues, key_nregexes, *area_settings().AREA_TAGS_GLOBAL, rman, stmt);
      std::vector< Id_Type > diff_ids(ids.ids.size(), Id_Type());
      diff_ids.erase(set_difference(ids.ids.begin(), ids.ids.end(), non_ids.begin(), non_ids.end(),
                     diff_ids.begin()), diff_ids.end());
      ids.ids.swap(diff_ids);
    }
  }
  else if ((!key_nvalues.empty() || !key_nregexes.empty() || !regkey_nregexes.empty())
      && check_keys_late != prefer_ranges)
    collect_non_ids< Id_Type >(key_nvalues, key_nregexes, *area_settings().AREA_TAGS_GLOBAL, rman, stmt).swap(ids.ids);
}


#endif
