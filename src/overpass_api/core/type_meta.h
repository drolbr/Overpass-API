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

#include <cstdint>
#include <cstring>
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


#endif
