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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__TAGS_GLOBAL_WRITER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__TAGS_GLOBAL_WRITER_H


#include "../data/filenames.h"


struct Frequent_Value_Entry
{
  typedef void Id_Type;

  std::string value;
  uint64 count;
  uint level;

  Frequent_Value_Entry() : count(0), level(0) {}
  
  Frequent_Value_Entry(const std::string& value_, uint64 count_, uint level_)
      : value(value_), count(count_), level(level_) {}

  Frequent_Value_Entry(void* data)
  {
    count = *(uint64*)data;
    level = *((uint8*)data + 8);
    value = std::string((int8*)data + 11, *(uint16*)((uint8*)data + 9));
  }

  uint32 size_of() const
  {
    return 11 + value.size();
  }

  static uint32 size_of(void* data)
  {
    return 11 + *(uint16*)((uint8*)data + 9);
  }

  void to_data(void* data) const
  {
    *(uint64*)data = count;
    *((uint8*)data + 8) = level;
    *(uint16*)((uint8*)data + 9) = value.size();
    if (!value.empty())
      memcpy((int8*)data + 11, &value[0], value.size());
  }

  bool operator<(const Frequent_Value_Entry& a) const
  {
    return value < a.value;
  }

  bool operator==(const Frequent_Value_Entry& a) const
  {
    return value == a.value;
  }
};


inline uint calc_tag_split_level(uint64 cnt)
{
  return cnt < 8192 ? 0 
      : cnt < 512*1024 ? 8
      : cnt < 32*1024*1024 ? 16
      : 24;
}


inline void reorganize_tag_split_level(
    uint old_level, uint new_level, const std::string& key, const std::string& value)
{
  // ...
}


template< typename Skeleton >
void update_current_global_tags(
    const std::map< Tag_Index_Global,
        std::set< Tag_Object_Global< typename Skeleton::Id_Type > > >& attic_objects,
    const std::map< Tag_Index_Global,
        std::set< Tag_Object_Global< typename Skeleton::Id_Type > > >& new_objects,
    Transaction& transaction)
{
  std::map< std::string, std::vector< Frequent_Value_Entry > > frequent;
  {
    Block_Backend< String_Index, Frequent_Value_Entry >
        db(transaction.data_index(current_global_tag_frequency_file_properties< Skeleton >()));
    for (auto it = db.flat_begin(); !(it == db.flat_end()); ++it)
      frequent[it.index().key].push_back(it.object());
  }
  for (auto& i : frequent)
    std::sort(i.second.begin(), i.second.end());

  auto it_attic = attic_objects.begin();
  auto it_new = new_objects.begin();
  for (const auto& i : frequent)
  {
    while (it_new != new_objects.end() && it_new->first.key < i.first)
      ++it_new;
    if (it_new != new_objects.end() && it_new->first.key == i.first)
    {
      for (const auto& j : i.second)
      {
        while (it_new != new_objects.end() && it_new->first.key == i.first && it_new->first.value < j.value)
          ++it_new;
        if (it_new != new_objects.end() && it_new->first.key == i.first && it_new->first.value == j.value)
        {
          // adjust new_objects
        }
      }
    }

    while (it_attic != attic_objects.end() && it_attic->first.key < i.first)
      ++it_attic;
    if (it_attic != attic_objects.end() && it_attic->first.key == i.first)
    {
      for (const auto& j : i.second)
      {
        while (it_attic != attic_objects.end() && it_attic->first.key == i.first && it_attic->first.value < j.value)
          ++it_attic;
        if (it_attic != attic_objects.end() && it_attic->first.key == i.first && it_attic->first.value == j.value)
        {
          // adjust attic_objects
        }
      }
    }
  }
  
  std::map< Tag_Index_Global, Delta_Count > cnt;
  {
    Block_Backend< Tag_Index_Global, Tag_Object_Global< typename Skeleton::Id_Type > >
        db(transaction.data_index(current_global_tags_file_properties< Skeleton >()));
    db.update(attic_objects, new_objects, &cnt);
  }

  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_delete;
  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_insert;
  
  auto it_cnt = cnt.begin();
  for (auto& i : frequent)
  {
    while (it_cnt != cnt.end() && it_cnt->first.key < i.first)
    {
      uint level = calc_tag_split_level(it_cnt->second.after);
      if (level > 0)
      {
        reorganize_tag_split_level(0, level, it_cnt->first.key, it_cnt->first.value);
        freq_to_insert[{ it_cnt->first.key }].insert({ it_cnt->first.value, (uint64)it_cnt->second.after, level });
      }
      ++it_cnt;
    }
    if (it_cnt != cnt.end() && it_cnt->first.key == i.first)
    {
      for (auto& j : i.second)
      {
        while (it_cnt != cnt.end() && it_cnt->first.value < j.value)
        {
          uint level = calc_tag_split_level(it_cnt->second.after);
          if (level > 0)
          {
            reorganize_tag_split_level(0, level, it_cnt->first.key, it_cnt->first.value);
            freq_to_insert[{ it_cnt->first.key }].insert({ it_cnt->first.value, (uint64)it_cnt->second.after, level });
          }
          ++it_cnt;
        }
        uint64 kv_cnt = j.count;
        while (it_cnt != cnt.end() && it_cnt->first.key == i.first && it_cnt->first.value == j.value)
        {
          kv_cnt += it_cnt->second.after;
          kv_cnt -= it_cnt->second.before;
          ++it_cnt;
        }
        if (kv_cnt != j.count)
        {
          uint level = calc_tag_split_level(kv_cnt);
          if (level != j.level)
            reorganize_tag_split_level(j.level, level, i.first, j.value);
          freq_to_delete[{ i.first }].insert({ j.value, (uint64)j.count, level });
          freq_to_insert[{ i.first }].insert({ j.value, (uint64)kv_cnt, level });        
        }
      }
    }
  }
  while (it_cnt != cnt.end())
  {
    uint level = calc_tag_split_level(it_cnt->second.after);
    if (level > 0)
    {
      reorganize_tag_split_level(0, level, it_cnt->first.key, it_cnt->first.value);
      freq_to_insert[{ it_cnt->first.key }].insert({ it_cnt->first.value, (uint64)it_cnt->second.after, level });
    }
    ++it_cnt;
  }

  {
    Block_Backend< String_Index, Frequent_Value_Entry >
        db(transaction.data_index(current_global_tag_frequency_file_properties< Skeleton >()));
    db.update(freq_to_delete, freq_to_insert);
  }
}


template< typename Skeleton >
void update_attic_global_tags(
    const std::map< Tag_Index_Global,
        std::set< Attic< Tag_Object_Global< typename Skeleton::Id_Type > > > >& attic_objects,
    const std::map< Tag_Index_Global,
        std::set< Attic< Tag_Object_Global< typename Skeleton::Id_Type > > > >& new_objects,
    Transaction& transaction)
{
  std::map< Tag_Index_Global, Delta_Count > cnt;
  Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< typename Skeleton::Id_Type > > >
      db(transaction.data_index(attic_global_tags_file_properties< Skeleton >()));
  db.update(attic_objects, new_objects, &cnt);
  std::cerr<<"DEBUG_22b05c\n"
      <<attic_global_tags_file_properties< Skeleton >()->get_file_name_trunk()<<'\n';
  for (auto& i : cnt)
  {
    if (i.second.after > 1000)
      std::cerr<<i.second.after<<'\t'<<i.first.key<<'\t'<<i.first.value<<'\n';
  }
}


#endif
