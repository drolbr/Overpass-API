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

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_API_KEY_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_API_KEY_H

#include <cstring>
#include <map>
#include <set>
#include <vector>

#include <iomanip>
#include <iostream>
#include "type_node.h"

#include "basic_types.h"


inline uint16 assemble_uint16(const void* data)
{
  return (uint16)(*((uint8*)data + 1))<<8 | (uint16)*(uint8*)data;
}


inline uint32 assemble_uint32(const void* data)
{
  return ((uint32)(*((uint8*)data + 3))<<24)
      | ((uint32)(*((uint8*)data + 2))<<16)
      | ((uint32)(*((uint8*)data + 1))<<8)
      | (uint32)*(uint8*)data;
}


inline uint64 assemble_uint64(const void* data)
{
  return ((uint64)(*((uint8*)data + 7))<<56)
      | ((uint64)(*((uint8*)data + 6))<<48)
      | ((uint64)(*((uint8*)data + 5))<<40)
      | ((uint64)(*((uint8*)data + 4))<<32)
      | ((uint64)(*((uint8*)data + 3))<<24)
      | ((uint64)(*((uint8*)data + 2))<<16)
      | ((uint64)(*((uint8*)data + 1))<<8)
      | (uint64)*(uint8*)data;
}


struct Api_Key_Entry
{
  static const int KEY_SIZE = 21;

  typedef uint64 Id_Type;

  std::string key;
  bool users_allowed;
  uint16 rate_limit;

  Api_Key_Entry() : users_allowed(false), rate_limit(0) {}

  Api_Key_Entry(void* data) : key(KEY_SIZE, ' '), users_allowed(false), rate_limit(0)
  {
    memcpy(&key[0], data, KEY_SIZE);
    users_allowed = *((uint8*)data + KEY_SIZE);
    rate_limit = assemble_uint16((uint8*)data + (KEY_SIZE + 1));
  }

  uint32 size_of() const
  {
    return KEY_SIZE + 3;
  }

  static uint32 size_of(void* data)
  {
    return KEY_SIZE + 3;
  }

  uint32 get_key() const
  {
    return assemble_uint32(&key[0]);
  }

  Id_Type get_id() const
  {
    return assemble_uint64(&key[0]);
  }

  static Id_Type get_id(void* data)
  {
    return assemble_uint64(data);
  }

  void to_data(void* data) const
  {
    memcpy(data, &key[0], KEY_SIZE);
    *((uint8*)data + KEY_SIZE) = users_allowed;
    memcpy((uint8*)data + (KEY_SIZE + 1), &rate_limit, 2);
  }

  bool operator<(const Api_Key_Entry& a) const
  {
    return (this->key < a.key);
  }

  bool operator==(const Api_Key_Entry& a) const
  {
    return (this->key == a.key);
  }
};


#endif
