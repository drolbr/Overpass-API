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


#include "../../template_db/block_backend.h"
#include "../../template_db/block_backend_write.h"
#include "../../template_db/transaction.h"
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


namespace
{
  constexpr uint64 THRESHOLD_8 = 8*1024;
  constexpr uint64 THRESHOLD_16 = 512*1024;
  constexpr uint64 THRESHOLD_24 = 32*1024*1024;
}


inline uint calc_tag_split_level(uint64 cnt)
{
  return cnt < THRESHOLD_8 ? 0
      : cnt < THRESHOLD_16 ? 8
      : cnt < THRESHOLD_24 ? 16
      : 24;
}


template< typename Ref_Entry >
void rebuild_db_to_insert(
    std::map< Tag_Index_Global_KVI, std::vector< Ref_Entry > >& db_to_insert,
    const std::string& key, const std::string& value, uint target_level)
{
  std::vector< std::vector< Ref_Entry > > to_do;
  for (auto it = db_to_insert.lower_bound(Tag_Index_Global_KVI(key, value)); it != db_to_insert.end(); ++it)
  {
    if (it->first.key != key || it->first.value != value)
      break;
    to_do.push_back({});
    to_do.back().swap(it->second);
  }

  for (const auto& i : to_do)
  {
    for (auto j : i)
      db_to_insert[Tag_Index_Global_KVI(key, value, j.idx.val() & (0xffffffff<<(32-target_level)))].push_back(j);
  }
}


template< typename Ref_Entry >
void rebuild_db_to_insert(
    std::map< Tag_Index_Global_KVI, std::set< Ref_Entry > >& db_to_insert,
    const std::string& key, const std::string& value, uint target_level)
{
  std::vector< std::set< Ref_Entry > > to_do;
  for (auto it = db_to_insert.lower_bound(Tag_Index_Global_KVI(key, value)); it != db_to_insert.end(); ++it)
  {
    if (it->first.key != key || it->first.value != value)
      break;
    to_do.push_back({});
    to_do.back().swap(it->second);
  }

  for (const auto& i : to_do)
  {
    for (auto j : i)
      db_to_insert[Tag_Index_Global_KVI(key, value, j.idx.val() & (0xffffffff<<(32-target_level)))].insert(j);
  }
}


template< typename Container >
void adapt_data_to_actual_idx(
    std::map< Tag_Index_Global_KVI, Container >& obj_to_adapt,
    const std::map< std::string, std::vector< Frequent_Value_Entry > >& frequent)
{
  auto it = obj_to_adapt.begin();
  for (const auto& i : frequent)
  {
    while (it != obj_to_adapt.end() && it->first.key < i.first)
      ++it;
    if (it != obj_to_adapt.end() && it->first.key == i.first)
    {
      for (const auto& j : i.second)
      {
        while (it != obj_to_adapt.end() && it->first.key == i.first && it->first.value < j.value)
          ++it;
        if (it != obj_to_adapt.end() && it->first.key == i.first && it->first.value == j.value)
          rebuild_db_to_insert(obj_to_adapt, i.first, j.value, j.level);
      }
    }
  }
}


template< typename Ref_Entry >
inline void reorganize_tag_split_level(
    File_Blocks_Index_Base* tags_file, uint target_level, const std::string& key, const std::string& value)
{
  Ranges< Tag_Index_Global > ranges{ Tag_Index_Global{ key, value }, Tag_Index_Global{ key, value + (char)0 } };

  std::map< Tag_Index_Global, std::set< Ref_Entry > > obj_to_delete;
  {
    Block_Backend< Tag_Index_Global, Ref_Entry > db(tags_file);
    for (auto it = db.range_begin(ranges); !(it == db.range_end()); ++it)
      obj_to_delete[it.index()].insert(it.object());
  }

  std::map< Tag_Index_Global, std::vector< Ref_Entry > > obj_to_insert;
  for (auto it = obj_to_delete.begin(); it != obj_to_delete.end(); ++it)
  {
    for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      obj_to_insert[Tag_Index_Global_KVI(key, value, it2->idx.val() & (0xffffffff<<(32-target_level)))]
          .push_back(*it2);
  }

  Block_Backend< Tag_Index_Global, Ref_Entry > db(tags_file);
  db.update(obj_to_delete, obj_to_insert);
}


template< typename Ref_Entry >
struct Freq_Tags_Updater
{
  Freq_Tags_Updater(File_Blocks_Index_Base* tags_file_, File_Blocks_Index_Base* freq_file_)
    : tags_file(tags_file_), freq_file(freq_file_) {}

  void reorganize_tags_if_necessary(const std::pair< Tag_Index_Global, Delta_Count >& tag_cnt)
  {
    uint level = calc_tag_split_level(tag_cnt.second.after);
    if (level > 0)
    {
      reorganize_tag_split_level< Ref_Entry>(tags_file, level, tag_cnt.first.key, tag_cnt.first.value);
      freq_to_insert[{ tag_cnt.first.key }].push_back({ tag_cnt.first.value, (uint64)tag_cnt.second.after, level });
    }
  }

  void reorganize_tags_if_necessary(
      const std::string& key, const Frequent_Value_Entry& entry, uint64 kv_cnt)
  {
    uint level = calc_tag_split_level(kv_cnt);
    if (entry.level < level)
    {
      reorganize_tag_split_level< Ref_Entry>(tags_file, level, key, entry.value);
      freq_to_delete[{ key }].insert({ entry.value, (uint64)entry.count, level });
      freq_to_insert[{ key }].push_back({ entry.value, (uint64)kv_cnt, level });
    }
  }

  void write_to_file()
  {
    Block_Backend< String_Index, Frequent_Value_Entry > db(freq_file);
    db.update(freq_to_delete, freq_to_insert);
  }

  ~Freq_Tags_Updater()
  {
    if (freq_file)
      write_to_file();
  }

  File_Blocks_Index_Base* tags_file;
  File_Blocks_Index_Base* freq_file;
  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_delete;
  std::map< String_Index, std::vector< Frequent_Value_Entry > > freq_to_insert;
};


template< typename Ref_Entry >
void update_global_tags(
    File_Blocks_Index_Base* tags_file, File_Blocks_Index_Base* freq_file,
    std::map< Tag_Index_Global, std::set< Ref_Entry > >& obj_to_delete,
    std::map< Tag_Index_Global, std::vector< Ref_Entry > >& obj_to_insert)
{
  std::map< std::string, std::vector< Frequent_Value_Entry > > frequent;
  {
    Block_Backend< String_Index, Frequent_Value_Entry > db(freq_file);
    for (auto it = db.flat_begin(); !(it == db.flat_end()); ++it)
      frequent[it.index().key].push_back(it.object());
  }
  for (auto& i : frequent)
    std::sort(i.second.begin(), i.second.end());

  adapt_data_to_actual_idx(obj_to_delete, frequent);
  adapt_data_to_actual_idx(obj_to_insert, frequent);

  std::map< Tag_Index_Global, Delta_Count > cnt;
  {
    Block_Backend< Tag_Index_Global, Ref_Entry > db(tags_file);
    db.update(obj_to_delete, obj_to_insert, &cnt);
  }

  Freq_Tags_Updater< Ref_Entry > freq_upd{ tags_file, freq_file };
  auto it_cnt = cnt.begin();
  for (auto& i : frequent)
  {
    while (it_cnt != cnt.end() && it_cnt->first.key < i.first)
      freq_upd.reorganize_tags_if_necessary(*it_cnt++);

    if (it_cnt != cnt.end() && it_cnt->first.key == i.first)
    {
      for (auto& j : i.second)
      {
        while (it_cnt != cnt.end() && it_cnt->first.key == i.first && it_cnt->first.value < j.value)
          freq_upd.reorganize_tags_if_necessary(*it_cnt++);

        uint64 kv_cnt = j.count;
        while (it_cnt != cnt.end() && it_cnt->first.key == i.first && it_cnt->first.value == j.value)
        {
          kv_cnt += it_cnt->second.after;
          kv_cnt -= it_cnt->second.before;
          ++it_cnt;
        }
        if (kv_cnt != j.count)
          freq_upd.reorganize_tags_if_necessary(i.first, j, kv_cnt);
      }
    }
  }
  while (it_cnt != cnt.end())
    freq_upd.reorganize_tags_if_necessary(*it_cnt++);

  freq_upd.write_to_file();
}


template< typename Skeleton >
void update_current_global_tags(
    std::map< Tag_Index_Global, std::set< Tag_Object_Global< typename Skeleton::Id_Type > > >& obj_to_delete,
    std::map< Tag_Index_Global, std::vector< Tag_Object_Global< typename Skeleton::Id_Type > > >& obj_to_insert,
    Transaction& transaction)
{
  update_global_tags(
      transaction.data_index(current_global_tags_file_properties< Skeleton >()),
      transaction.data_index(current_global_tag_frequency_file_properties< Skeleton >()),
      obj_to_delete, obj_to_insert);
}


template< typename Skeleton >
void update_attic_global_tags(
    std::map< Tag_Index_Global, std::set< Attic< Tag_Object_Global< typename Skeleton::Id_Type > > > >&& obj_to_delete,
    std::map< Tag_Index_Global, std::vector< Attic< Tag_Object_Global< typename Skeleton::Id_Type > > > >&& obj_to_insert,
    Transaction& transaction)
{
  update_global_tags(
      transaction.data_index(attic_global_tags_file_properties< Skeleton >()),
      transaction.data_index(attic_global_tag_frequency_file_properties< Skeleton >()),
      obj_to_delete, obj_to_insert);
}


template< typename Ref_Entry, typename Target_Idx >
struct Migrate_Loop_Flush
{
  void flush_if_large(uint64 per_kv, std::map< Tag_Index_Global_KVI, std::vector< Ref_Entry > >& db_to_insert)
  {
    total += per_kv;
    if (total >= THRESHOLD_24)
    {
      callback->migration_flush();
      for (auto& i : db_to_insert)
        std::sort(i.second.begin(), i.second.end());
      into_db.update({}, db_to_insert);
      db_to_insert.clear();
      total = 0;
    }
  }

  void flush_single_kv(std::map< Tag_Index_Global_KVI, std::vector< Ref_Entry > >& db_to_insert)
  {
    callback->migration_flush_single_kv();
    for (auto& i : db_to_insert)
      std::sort(i.second.begin(), i.second.end());
    into_db.update({}, db_to_insert);
    db_to_insert.clear();
  }

  void flush_the_rest(std::map< Tag_Index_Global_KVI, std::vector< Ref_Entry > >& db_to_insert)
  {
    callback->migration_flush();
    for (auto& i : db_to_insert)
      std::sort(i.second.begin(), i.second.end());
    into_db.update({}, db_to_insert);
  }

  Block_Backend< Target_Idx, Ref_Entry >& into_db;
  uint64 total;
  Osm_Backend_Callback* callback;
};


template< typename Ref_Entry, typename Src_Idx, typename Target_Idx >
void migrate_loop(
    Block_Backend< Src_Idx, Ref_Entry >& from_db, Block_Backend< Target_Idx, Ref_Entry >& into_db,
    Osm_Backend_Callback* callback, std::map< String_Index, std::set< Frequent_Value_Entry > >& freq_to_insert)
{
  auto from_it = from_db.flat_begin();

  std::map< Tag_Index_Global_KVI, std::vector< Ref_Entry > > db_to_insert;
  auto to_it = db_to_insert.begin();

  // We have two different flush criteria here:
  // - total number of objects after a couple of relatively small kvs
  // - number of objects alone for a sufficently large kv
  // On top of this, we need to rebuild the index for the affected kv if a kv passes its size threshold.
  Migrate_Loop_Flush< Ref_Entry, Target_Idx > flush{ into_db, 0, callback };
  uint64 per_kv = 0;
  uint64 level_threshold = THRESHOLD_8;
  uint level = 0;
  while (!(from_it == from_db.flat_end()))
  {
    if (to_it == db_to_insert.end() || to_it->first.key != from_it.index().key
        || to_it->first.value != from_it.index().value)
    {
      if (level > 0)
        freq_to_insert[{ to_it->first.key }].insert({ to_it->first.value, per_kv, level });

      flush.flush_if_large(per_kv, db_to_insert);

      to_it = db_to_insert.insert({ Tag_Index_Global_KVI(from_it.index().key, from_it.index().value), {} }).first;
      level_threshold = THRESHOLD_8;
      level = 0;
      per_kv = 0;
    }
    else if (per_kv >= level_threshold)
    {
      if (level < 24)
      {
        level = calc_tag_split_level(per_kv);
        rebuild_db_to_insert(db_to_insert, from_it.index().key, from_it.index().value, level);
        level_threshold = (level < 16 ? THRESHOLD_16 : THRESHOLD_24);
      }
      if (level == 24)
      {
        flush.flush_single_kv(db_to_insert);

        to_it = db_to_insert.insert({ Tag_Index_Global_KVI(from_it.index().key, from_it.index().value), {} }).first;
        level_threshold += THRESHOLD_24;
      }
    }

    if (level == 0)
      to_it->second.push_back(from_it.object());
    else
      db_to_insert[Tag_Index_Global_KVI(
          from_it.index().key, from_it.index().value, (from_it.object().idx.val() & (0xffffffff<<(32-level))))]
          .push_back(from_it.object());
    ++per_kv;
    ++from_it;
  }
  if (to_it != db_to_insert.end() && level > 0)
    freq_to_insert[{ to_it->first.key }].insert({ to_it->first.value, per_kv, level });

  flush.flush_the_rest(db_to_insert);
}


template< typename Skeleton >
void migrate_current_global_tags(Osm_Backend_Callback* callback, Transaction& transaction)
{
  callback->migration_started(current_global_tags_file_properties< Skeleton >()->get_file_name_trunk());

  Block_Backend< Tag_Index_Global_Until756, Tag_Object_Global< typename Skeleton::Id_Type > >
      from_db(transaction.data_index(current_global_tags_file_properties_756< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  Block_Backend< Tag_Index_Global_KVI, Tag_Object_Global< typename Skeleton::Id_Type > >
      into_db(into_transaction.data_index(current_global_tags_file_properties< Skeleton >()));

  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_insert;
  migrate_loop(from_db, into_db, callback, freq_to_insert);

  callback->migration_write_frequent();
  {
    Block_Backend< String_Index, Frequent_Value_Entry >
        db(transaction.data_index(current_global_tag_frequency_file_properties< Skeleton >()));
    db.update({}, freq_to_insert);
  }
  callback->migration_completed();
}


template< typename Skeleton >
void migrate_attic_global_tags(Osm_Backend_Callback* callback, Transaction& transaction)
{
  callback->migration_started(attic_global_tags_file_properties< Skeleton >()->get_file_name_trunk());

  Block_Backend< Tag_Index_Global_Until756, Attic< Tag_Object_Global< typename Skeleton::Id_Type > > >
      from_db(transaction.data_index(attic_global_tags_file_properties_756< Skeleton >()));

  Nonsynced_Transaction into_transaction(Access_Mode::truncate, false, transaction.get_db_dir(), ".next");
  Block_Backend< Tag_Index_Global_KVI, Attic< Tag_Object_Global< typename Skeleton::Id_Type > > >
      into_db(into_transaction.data_index(attic_global_tags_file_properties< Skeleton >()));

  std::map< String_Index, std::set< Frequent_Value_Entry > > freq_to_insert;
  migrate_loop(from_db, into_db, callback, freq_to_insert);

  callback->migration_write_frequent();
  {
    Block_Backend< String_Index, Frequent_Value_Entry >
        db(transaction.data_index(attic_global_tag_frequency_file_properties< Skeleton >()));
    db.update({}, freq_to_insert);
  }
  callback->migration_completed();
}


#endif
