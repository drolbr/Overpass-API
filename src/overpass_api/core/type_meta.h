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
#include <vector>


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


class Meta_Per_Changeset_Skeleton
{
public:
  struct Entry
  {
    uint64_t ref;
    uint_fast32_t version;
    uint64_t timestamp;
    
    bool operator<(const Entry& rhs)
    {
      if (ref != rhs.ref)
        return ref < rhs.ref;
      return version < rhs.version;
    }
    bool operator==(const Entry& rhs)
    { return ref == rhs.ref && version == rhs.version; }
  };
  
  typedef uint64_t Id_Type;
  
  Meta_Per_Changeset_Skeleton(uint64_t changeset_, bool is_redacted_, uint64_t user_id_)
      : changeset(changeset_), is_redacted(is_redacted_), user_id(user_id_) {}
      
  Meta_Per_Changeset_Skeleton(const Meta_Per_Changeset_Skeleton& base, std::vector< Entry >&& refs_)
      : changeset(base.changeset), is_redacted(base.is_redacted), user_id(base.user_id), refs(refs_) {}

  Meta_Per_Changeset_Skeleton(void* data)
  {
    Nibble_Varint_Reader src(SIZE_RULES, (uint8_t*)data);
    uint64_t base_timestamp = src.read_flex< uint64_t >(BASE_TIMESTAMP_RULES);
    changeset = src.read_flex< uint64_t >(CHANGESET_ID_RULES);
    uint8_t flags = src.read_fixed< uint64_t >(4);
    user_id = src.read_flex< uint64_t >(USER_ID_RULES);
    is_redacted = (flags & 0x01);
  
    uint64_t last_ref = 0;
    while (src.good_flex())
    {
      uint64_t version = src.read_flex< uint64_t >(VERSION_RULES);
      uint64_t ref = src.read_flex< uint64_t >(OBJ_DELTA_RULES) + last_ref;
      uint64_t timestamp = src.read_flex< uint64_t >(DELTA_TIMESTAMP_RULES) + base_timestamp - 1;
      refs.push_back({ ref, version, timestamp });
      last_ref = ref;
    }
  }

  void to_data(void* data) const
  {
    prepare_write();

    Nibble_Varint_Writer dest((uint8_t*)data);
    dest.write_flex(SIZE_RULES, Nibble_Varint_Writer::brutto_size_in_bytes(cached_size));
    dest.write_flex(BASE_TIMESTAMP_RULES, cached_base_timestamp);
    dest.write_flex(CHANGESET_ID_RULES, changeset);
    dest.write_fixed(4, is_redacted);
    dest.write_flex(USER_ID_RULES, user_id);
    
    uint64_t last_ref = 0;
    for (const auto& i : refs)
    {
      dest.write_flex(VERSION_RULES, i.version);
      dest.write_flex(OBJ_DELTA_RULES, i.ref - last_ref);
      dest.write_flex(DELTA_TIMESTAMP_RULES, i.timestamp - cached_base_timestamp + 1);
      last_ref = i.ref;
    }
    
    dest.pad_to_byte();
  }

  const std::vector< Entry >& get_refs() const { return refs; }
  const uint64_t get_changeset() const { return changeset; }
  const bool get_is_redacted() const { return is_redacted; }
  const uint64_t get_user_id() const { return user_id; }
  
  void add_ref(const Entry& entry)
  {
    raw_mode = false;
    refs.push_back(entry);
  }
  
  void move_refs_to(Meta_Per_Changeset_Skeleton& arg)
  {
    for (auto i : refs)
      arg.refs.push_back(i);
    refs.clear();
  }
  
  template< typename Func >
  std::vector< Entry > move_refs_if(Func f)
  {
    std::vector< Entry > result;
    
    auto to_it = refs.begin();
    for (auto from_it = refs.begin(); from_it != refs.end(); ++from_it)
    {
      if (f(*from_it))
        result.push_back(*from_it);
      else
      {
        if (from_it != to_it)
          *to_it = *from_it;
        ++to_it;
      }
    }
    refs.erase(to_it, refs.end());
    
    return result;
  }

  uint32_t size_of() const
  {
    prepare_write();
    return Nibble_Varint_Writer::brutto_size_in_bytes(cached_size);
  }

  static uint32_t size_of(void* data)
  {
    return Nibble_Varint_Reader((const uint8_t*)data, 8).read_flex< uint64_t >(SIZE_RULES);
  }

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
  
  static constexpr uint32_t SIZE_RULES = 0x18100c08;
  static constexpr uint32_t BASE_TIMESTAMP_RULES = 0x302c2824;
  static constexpr uint32_t CHANGESET_ID_RULES = 0x2824201c;
  static constexpr uint32_t USER_ID_RULES = 0x24201c14;
  static constexpr uint32_t OBJ_DELTA_RULES = 0x2c280c04;
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
    refs.erase(std::unique(refs.begin(), refs.end()), refs.end());
  
    cached_base_timestamp = std::numeric_limits< uint64_t >::max();
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
          Nibble_Varint_Writer::size_in_bits(OBJ_DELTA_RULES, i.ref - last_ref)
          + Nibble_Varint_Writer::size_in_bits(VERSION_RULES, i.version)
          + Nibble_Varint_Writer::size_in_bits(DELTA_TIMESTAMP_RULES, i.timestamp - cached_base_timestamp + 1);
      last_ref = i.ref;
    }    
    cached_size = payload_size + Nibble_Varint_Writer::size_of_size_in_bytes(SIZE_RULES, payload_size);

    raw_mode = true;
  }
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
      
  OSM_Element_Metadata_Skeleton(
      const Meta_Per_Changeset_Skeleton& cset, const Meta_Per_Changeset_Skeleton::Entry& entry)
    : ref(entry.ref), version(entry.version), timestamp(entry.timestamp),
      changeset(cset.get_changeset()), user_id(cset.get_user_id()) {}

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


#endif
