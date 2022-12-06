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


template< typename Skeleton >
void update_current_global_tags(
    const std::map< Tag_Index_Global,
        std::set< Tag_Object_Global< typename Skeleton::Id_Type > > >& attic_objects,
    const std::map< Tag_Index_Global,
        std::set< Tag_Object_Global< typename Skeleton::Id_Type > > >& new_objects,
    Transaction& transaction)
{
  std::map< std::string, std::vector< Frequent_Value_Entry > > frequent;
  std::map< Tag_Index_Global, uint64 > cnt;
  
  {
    Block_Backend< Tag_Index_Global, Tag_Object_Global< typename Skeleton::Id_Type > >
        db(transaction.data_index(current_global_tags_file_properties< Skeleton >()));
    db.update(attic_objects, new_objects, &cnt);
  }

  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_delete;
  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_insert;

  for (auto& i : cnt)
  {
    if (i.second > 8192)
    {
      uint level = (i.second < 512*1024 ? 8 : i.second < 32*1024*1024 ? 16 : 24);
      
      auto& val_vec = frequent[i.first.key];
      auto it = val_vec.begin();
      while (it != val_vec.end() && it->value != i.first.value)
        ++it;
      if (it == val_vec.end())
      {
        freq_to_insert[i.first.key].insert({ i.first.value, i.second, level });
        // reorganize to level
      }
      else
      {
        if (it->level < level)
          ;//reorganize to level
        freq_to_delete[{ i.first.key }].insert(Frequent_Value_Entry{ i.first.value, (uint64)i.second, level });
        freq_to_insert[{ i.first.key }].insert({ i.first.value, (uint64)i.second, level });        
      }
    }
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
  std::map< Tag_Index_Global, uint64 > cnt;
  Block_Backend< Tag_Index_Global, Attic< Tag_Object_Global< typename Skeleton::Id_Type > > >
      db(transaction.data_index(attic_global_tags_file_properties< Skeleton >()));
  db.update(attic_objects, new_objects, &cnt);
  std::cerr<<"DEBUG_22b05c\n"
      <<attic_global_tags_file_properties< Skeleton >()->get_file_name_trunk()<<'\n';
  for (auto& i : cnt)
  {
    if (i.second > 1000)
      std::cerr<<i.second<<'\t'<<i.first.key<<'\t'<<i.first.value<<'\n';
  }
}


#endif
