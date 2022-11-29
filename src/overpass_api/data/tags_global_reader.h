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


class Statement;


template < typename T >
struct Optional
{
  Optional(T* obj_) : obj(obj_) {}
  ~Optional() { delete obj; }

  T* obj;
};


struct Trivial_Regex
{
public:
  bool matches(const std::string&) const { return true; }
};


template< typename Id_Type, typename Iterator, typename Key_Regex, typename Val_Regex >
void filter_id_list(
    std::vector< std::pair< Id_Type, Uint31_Index > >& new_ids, bool& filtered,
    Iterator begin, Iterator end, const Key_Regex& key_regex, const Val_Regex& val_regex,
    Query_Filter_Strategy& check_keys_late)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > old_ids;
  old_ids.swap(new_ids);

  for (Iterator it = begin; !(it == end); ++it)
  {
    if (key_regex.matches(it.index().key) && it.index().value != void_tag_value()
        && val_regex.matches(it.index().value) && (!filtered ||
	std::binary_search(old_ids.begin(), old_ids.end(), std::make_pair(it.object().id, Uint31_Index(0u)))))
      new_ids.push_back(std::make_pair(it.object().id, it.object().idx));

    if (!filtered && new_ids.size() == 1024*1024)
    {
      if (check_keys_late == prefer_ranges)
      {
        new_ids.clear();
        return;
      }
      else if (check_keys_late == ids_useful)
      {
        check_keys_late = prefer_ranges;
        new_ids.clear();
        return;
      }
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
    const Container& container)
{
  std::vector< std::pair< Id_Type, Uint31_Index > > old_ids;
  old_ids.swap(new_ids);

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


template< typename Skeleton, typename Id_Type >
std::vector< std::pair< Id_Type, Uint31_Index > > collect_ids(
    const std::vector< std::string >& keys,
    const std::vector< std::pair< std::string, std::string > >& key_values,
    const std::vector< std::pair< std::string, Regular_Expression* > >& key_regexes,
    const std::vector< std::pair< Regular_Expression*, Regular_Expression* > >& regkey_regexes,
    const File_Properties& file_prop, const File_Properties& attic_file_prop,
    Resource_Manager& rman, const Statement& stmt,
    uint64 timestamp, Query_Filter_Strategy& check_keys_late, bool& result_valid)
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

  for (std::vector< std::pair< std::string, std::string > >::const_iterator kvit = key_values.begin();
       kvit != key_values.end(); ++kvit)
  {
    if (timestamp == NOW)
    {
      std::set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
      filter_id_list(new_ids, filtered,
		     tags_db.discrete_begin(tag_req.begin(), tag_req.end()), tags_db.discrete_end(),
		     Trivial_Regex(), Trivial_Regex(), check_keys_late);
      if (!filtered)
      {
	result_valid = false;
	break;
      }
    }
    else
      filter_id_list(new_ids, filtered, collect_attic_kv(kvit, timestamp, tags_db, *attic_tags_db.obj));

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
            Trivial_Regex(), Trivial_Regex(), check_keys_late);
        if (!filtered)
        {
          result_valid = false;
          break;
        }
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_k(kit, timestamp, tags_db, *attic_tags_db.obj));

      rman.health_check(stmt);
    }
  }

  if (check_keys_late != prefer_ranges)
  {
    // Handle Key-Regular-Expression-Value pairs
    for (std::vector< std::pair< std::string, Regular_Expression* > >::const_iterator krit = key_regexes.begin();
	 krit != key_regexes.end(); ++krit)
    {
      if (timestamp == NOW)
      {
        Ranges< Tag_Index_Global > ranges = get_k_req(krit->first);
	filter_id_list(new_ids, filtered,
	    tags_db.range_begin(ranges), tags_db.range_end(), Trivial_Regex(), *krit->second, check_keys_late);
        if (!filtered)
        {
          result_valid = false;
          break;
        }
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_kregv(krit, timestamp, tags_db, *attic_tags_db.obj));

      rman.health_check(stmt);
    }
  }

  if (check_keys_late != prefer_ranges)
  {
    // Handle Regular-Key-Regular-Expression-Value pairs
    for (std::vector< std::pair< Regular_Expression*, Regular_Expression* > >::const_iterator it = regkey_regexes.begin();
	 it != regkey_regexes.end(); ++it)
    {
      if (timestamp == NOW)
      {
        Ranges< Tag_Index_Global > ranges = get_regk_req< Skeleton >(it->first, rman, stmt);
	filter_id_list(new_ids, filtered,
	    tags_db.range_begin(ranges), tags_db.range_end(), *it->first, *it->second, check_keys_late);
        if (!filtered)
        {
          result_valid = false;
          break;
        }
      }
      else
	filter_id_list(new_ids, filtered, collect_attic_regkregv< Skeleton, Id_Type >(
	    it, timestamp, tags_db, *attic_tags_db.obj, rman, stmt));

      rman.health_check(stmt);
    }
  }

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
    std::set< Tag_Index_Global > tag_req = get_kv_req(kvit->first, kvit->second);
    filter_id_list(new_ids, filtered,
	tags_db.discrete_begin(tag_req.begin(), tag_req.end()), tags_db.discrete_end(),
	    Trivial_Regex(), Trivial_Regex());

    rman.health_check(stmt);
  }

  // Handle simple Keys Only
  if (check_keys_late != prefer_ranges)
  {
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
      std::set< Tag_Index_Global > tag_req = get_kv_req(knvit->first, knvit->second);
      for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Discrete_Iterator
          it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
          !(it2 == tags_db.discrete_end()); ++it2)
        idr.remove(it2.object().id);
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kv(knvit, timestamp, tags_db, *attic_tags_db.obj, &ids);

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
      std::set< Tag_Index_Global > tag_req = get_kv_req(knvit->first, knvit->second);
      for (typename Block_Backend< Tag_Index_Global, Tag_Object_Global< Id_Type > >::Discrete_Iterator
          it2(tags_db.discrete_begin(tag_req.begin(), tag_req.end()));
          !(it2 == tags_db.discrete_end()); ++it2)
        new_ids.push_back(std::make_pair(it2.object().id, it2.object().idx));
    }
    else
    {
      std::map< Id_Type, std::pair< uint64, Uint31_Index > > timestamp_per_id
          = collect_attic_kv(knvit, timestamp, tags_db, *attic_tags_db.obj);

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
    uint64 timestamp, Answer_State& answer_state, Query_Filter_Strategy& check_keys_late,
    const File_Properties& file_prop, const File_Properties& attic_file_prop,
    Resource_Manager& rman, const Statement& stmt)
{
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
            file_prop, attic_file_prop, rman, stmt, timestamp, check_keys_late, result_valid);
    ids.invert = !result_valid;

    if (!key_nvalues.empty() || (check_keys_late != prefer_ranges && !key_nregexes.empty()))
      filter_non_ids< Id_Type >(
          key_nvalues, key_nregexes, regkey_nregexes, id_idxs, file_prop, attic_file_prop, rman, stmt, timestamp);

    for (auto it = id_idxs.begin(); it != id_idxs.end(); ++it)
    {
      ids.ids.push_back(it->first);
      range_vec.push_back(it->second);
    }

    if (ids.ids.empty() && result_valid)
      answer_state = data_collected;
  }
  else if ((!key_nvalues.empty() || !key_nregexes.empty()) && check_keys_late != prefer_ranges)
  {
    std::vector< std::pair< Id_Type, Uint31_Index > > id_idxs =
        collect_non_ids< Id_Type >(
            key_nvalues, key_nregexes, file_prop, attic_file_prop, rman, stmt, timestamp);
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
    Id_Constraint< Id_Type >& ids, Answer_State& answer_state, Query_Filter_Strategy check_keys_late,
    const File_Properties& file_prop, Resource_Manager& rman, const Statement& stmt)
{
  if (!key_values.empty()
      || (check_keys_late != prefer_ranges
          && (!keys.empty() || !key_regexes.empty() || !regkey_regexes.empty())))
  {
    ids.invert = false;
    collect_ids< Id_Type >(
        keys, key_values, key_regexes, regkey_regexes,
        file_prop, rman, stmt, check_keys_late).swap(ids.ids);
    if (!key_nvalues.empty() || !key_nregexes.empty() || !regkey_nregexes.empty())
    {
      std::vector< Id_Type > non_ids = collect_non_ids< Id_Type >(
          key_nvalues, key_nregexes, file_prop, rman, stmt);
      std::vector< Id_Type > diff_ids(ids.ids.size(), Id_Type());
      diff_ids.erase(set_difference(ids.ids.begin(), ids.ids.end(), non_ids.begin(), non_ids.end(),
                     diff_ids.begin()), diff_ids.end());
      ids.ids.swap(diff_ids);
    }
    if (ids.ids.empty())
      answer_state = data_collected;
  }
  else if ((!key_nvalues.empty() || !key_nregexes.empty() || !regkey_nregexes.empty())
      && check_keys_late != prefer_ranges)
    collect_non_ids< Id_Type >(key_nvalues, key_nregexes, file_prop, rman, stmt).swap(ids.ids);
}


#endif
