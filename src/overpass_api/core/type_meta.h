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

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_META_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_META_H

#include "nibble_varint_rw.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>


struct User_Data
{
  typedef uint32_t Id_Type;

  Id_Type id;
  std::string name;

  User_Data() : id(0) {}

  User_Data(void* data)
  {
    id = *(uint32_t*)data;
    name = std::string((char*)((int8_t*)data + 6), (std::string::size_type)*(uint16_t*)((int8_t*)data + 4));
  }

  uint32_t size_of() const
  {
    return 6 + name.length();
  }

  static uint32_t size_of(void* data)
  {
    return 6 + *(uint16_t*)((int8_t*)data + 4);
  }

  void to_data(void* data) const
  {
    *(uint32_t*)data = id;
    *(uint16_t*)((int8_t*)data + 4) = name.length();
    memcpy(((int8_t*)data + 6), name.data(), name.length());
  }

  bool operator<(const User_Data& a) const
  {
    return (id < a.id);
  }

  bool operator==(const User_Data& a) const
  {
    return (id == a.id);
  }
};


struct OSM_Element_Metadata
{
  OSM_Element_Metadata() : version(0), timestamp(0), changeset(0), user_id(0) {}

  uint32_t version;
  uint64_t timestamp;
  uint32_t changeset;
  uint32_t user_id;
  std::string user_name;

  bool operator<(const OSM_Element_Metadata&) const { return false; }
};


template< typename Id_Type_ >
struct OSM_Element_Metadata_Skeleton
{
  typedef Id_Type_ Id_Type;

  Id_Type ref;
  uint32_t version;
  uint64_t timestamp;
  uint32_t changeset;
  uint32_t user_id;

  OSM_Element_Metadata_Skeleton() : version(0), timestamp(0), changeset(0), user_id(0) {}

  OSM_Element_Metadata_Skeleton(Id_Type ref_)
    : ref(ref_), version(0), timestamp(0), changeset(0), user_id(0) {}

  OSM_Element_Metadata_Skeleton(Id_Type ref_, const OSM_Element_Metadata& meta)
    : ref(ref_),
      version(meta.version), timestamp(meta.timestamp),
      changeset(meta.changeset), user_id(meta.user_id) {}

  OSM_Element_Metadata_Skeleton(Id_Type ref_, uint64_t timestamp_)
    : ref(ref_), version(0), timestamp(timestamp_),
      changeset(0), user_id(0) {}

  OSM_Element_Metadata_Skeleton(void* data)
    : ref(*(Id_Type*)data)
  {
    version = *(uint32_t*)((int8_t*)data + sizeof(Id_Type));
    timestamp = (*(uint64_t*)((int8_t*)data + sizeof(Id_Type) + 4) & 0xffffffffffull);
    changeset = *(uint32_t*)((int8_t*)data + sizeof(Id_Type) + 9);
    user_id = *(uint32_t*)((int8_t*)data + sizeof(Id_Type) + 13);
  }

  uint32_t size_of() const
  {
    return 17 + sizeof(Id_Type);
  }

  static uint32_t size_of(void* data)
  {
    return 17 + sizeof(Id_Type);
  }

  void to_data(void* data) const
  {
    *(Id_Type*)data = ref;
    *(uint32_t*)((int8_t*)data + sizeof(Id_Type)) = version;
    *(uint64_t*)((int8_t*)data + sizeof(Id_Type) + 4) = timestamp;
    *(uint32_t*)((int8_t*)data + sizeof(Id_Type) + 9) = changeset;
    *(uint32_t*)((int8_t*)data + sizeof(Id_Type) + 13) = user_id;
  }

  bool operator<(const OSM_Element_Metadata_Skeleton& a) const
  {
    if (ref < a.ref)
      return true;
    else if (a.ref < ref)
      return false;
    return (timestamp < a.timestamp);
  }

  bool operator==(const OSM_Element_Metadata_Skeleton& a) const
  {
    return (ref == a.ref);
  }
};


class Meta_Per_Changeset_Skeleton
{
public:
  struct Entry
  {
    uint64_t ref;
    uint_fast32_t version;
    uint64_t timestamp;
    
    bool operator<(const Entry& rhs)
    { return ref < rhs.ref; }
  };
  
  Meta_Per_Changeset_Skeleton(uint64_t changeset_, bool is_redacted_, uint64_t user_id_)
      : changeset(changeset_), is_redacted(is_redacted_), user_id(user_id_) {}

/*  Meta_Per_Changeset_Skeleton(void* data)
  {
    Nibble_Varint_Reader src(data, 8);
    src.resize(src.read_flex< uint32_t >(SIZE_RULES));
    uint64_t base_timestamp = src.read_flex< uint64_t >(BASE_TIMESTAMP_RULES);
    Id_Type changeset = src.read_flex< ? >(CHANGESET_ID_RULES);
    Id_Type user_id = src.read_flex< ? >(USER_ID_RULES);
    uint8_t flags = src.read_fixed(4);
    is_redacted = (flags & 0x01);
  
    if (src.has_data())
      refs.push_back({
        src.read_flex< uint64_t >(OBJ_ID_RULES),
        src.read_flex< uint64_t >(VERSION_RULES),
        src.read_flex< uint64_t >(DELTA_TIMESTAMP_RULES) + base_timestamp - 1 });
    while (src.has_data())
      refs.push_back({
        src.read_flex< uint64_t >(OBJ_ID_RULES) + refs.back().ref,
        src.read_flex< uint64_t >(VERSION_RULES),
        src.read_flex< uint64_t >(DELTA_TIMESTAMP_RULES) + base_timestamp - 1 });
  }*/

  const std::vector< Entry >& get_refs() const { return refs; }
  const uint64_t get_changeset() const { return changeset; }
  const bool get_is_redacted() const { return is_redacted; }
  const uint64_t get_user_id() const { return user_id; }
  
  void add_ref(const Entry& entry)
  {
    raw_mode = false;
    refs.push_back(entry);
  }

  uint32_t size_of() const
  {
    prepare_write();
    return cached_size;
  }

  static uint32_t size_of(void* data)
  {
    return Nibble_Varint_Reader((const uint8_t*)data, 8).read_flex< uint64_t >(SIZE_RULES);
  }

/*  void to_data(void* data) const
  {
    prepare_write();

    Nibble_Varint_Writer dest(data, 8);
    dest.write_flex< uint64_t >(BASE_TIMESTAMP_RULES, base_timestamp);
    dest.write_flex< ? >(CHANGESET_ID_RULES, changeset);
    dest.write_flex< ? >(USER_ID_RULES, user_id);
    dest.write_fixed(4, is_redacted);
    
    auto it = refs.begin();
    if (it != refs.end())
    {
      dest.write_flex(OBJ_ID_RULES, it->ref);
      dest.write_flex(VERSION_RULES, it->version);
      dest.write_flex(DELTA_TIMESTAMP_RULES - cached_base_timestamp + 1);
      ++it;
    }
    while (it != refs.end())
    {
      dest.write_flex(OBJ_ID_RULES, it->ref);
      dest.write_flex(VERSION_RULES, it->version);
      dest.write_flex(DELTA_TIMESTAMP_RULES - cached_base_timestamp + 1);
      ++it;
    }
  }*/

  bool operator<(const Meta_Per_Changeset_Skeleton& a) const
  {
    if (changeset == a.changeset)
      return is_redacted < a.is_redacted;

    return changeset < a.changeset;
  }

  bool operator==(const Meta_Per_Changeset_Skeleton& a) const
  {
    return changeset == a.changeset && is_redacted == a.is_redacted;
  }

private:
  uint64_t changeset;
  bool is_redacted = false;
  uint64_t user_id;
  mutable std::vector< Entry > refs;
  
  static constexpr uint32_t SIZE_RULES = 0x30100c08;
  static constexpr uint32_t BASE_TIMESTAMP_RULES = 0x302c2824;
  static constexpr uint32_t CHANGESET_ID_RULES = 0x2824201c;
  static constexpr uint32_t USER_ID_RULES = 0x24201c14;
  static constexpr uint32_t OBJ_ID_RULES = 0x2c282420;
  static constexpr uint32_t VERSION_RULES = 0x18100804;
  static constexpr uint32_t DELTA_TIMESTAMP_RULES = 0x18100804;

  mutable bool raw_mode = false;
  mutable uint_fast32_t cached_size = 0;
  mutable uint64_t cached_base_timestamp = 0;
  
  void prepare_write() const
  {
    if (raw_mode)
      return;
      
    std::sort(refs.begin(), refs.end());
  
    cached_base_timestamp = std::numeric_limits< uint32_t >::max();
    for (const auto& i : refs)
      cached_base_timestamp = std::min(cached_base_timestamp, i.timestamp);
    
    uint_fast32_t payload_size
        = Nibble_Varint_Writer::size_in_bits(BASE_TIMESTAMP_RULES, cached_base_timestamp)
        + Nibble_Varint_Writer::size_in_bits(CHANGESET_ID_RULES, changeset)
        + Nibble_Varint_Writer::size_in_bits(USER_ID_RULES, user_id)
        + 4;
    uint64_t last_ref = 0;
    for (const auto& i : refs)
    {
      payload_size +=
          Nibble_Varint_Writer::size_in_bits(OBJ_ID_RULES, i.ref - last_ref)
          + Nibble_Varint_Writer::size_in_bits(VERSION_RULES, i.version)
          + Nibble_Varint_Writer::size_in_bits(DELTA_TIMESTAMP_RULES, i.timestamp - cached_base_timestamp + 1);
      last_ref = i.ref;
    }
/*
    uint_fast32_t size_length = Nibble_Varint_Writer::size_in_bits(SIZE_RULES, payload_size);
    uint_fast32_t gross_size_length = Nibble_Varint_Writer::size_in_bits(SIZE_RULES, payload_size + size_length);
    cached_size = payload_size + gross_size_length;
*/
    raw_mode = true;
  }
};


#endif
