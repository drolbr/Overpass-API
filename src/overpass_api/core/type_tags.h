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

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_TAGS_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_TAGS_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../../template_db/ranges.h"
#include "basic_types.h"


struct Unsupported_Error
{
  Unsupported_Error(const std::string& method_name_) : method_name(method_name_) {}
  std::string method_name;
};


template< class Id_Type >
struct Tag_Entry
{
  uint32 index;
  std::string key;
  std::string value;
  std::vector< Id_Type > ids;
};


inline int strnncmp(const char* lhs, const char* rhs, int lhs_len, int rhs_len)
{
  if (rhs_len < lhs_len)
  {
    int cmp = strncmp(lhs, rhs, rhs_len);
    return cmp ? cmp : 1;
  }
  int cmp = strncmp(lhs, rhs, lhs_len);
  return cmp ? cmp : -(lhs_len < rhs_len);
}


struct String_Index
{
  std::string key;

  String_Index() {}
  String_Index(std::string key_) : key(key_) {}

  String_Index(void* data)
  {
    key = std::string(((int8*)data + 2), *(uint16*)data);
  }

  static bool equal(void* lhs, void* rhs)
  { return *(uint16*)lhs == *(uint16*)rhs && !memcmp(lhs, rhs, *(uint16*)lhs); }
  bool less(void* rhs) const
  {
    return strnncmp(&key[0], ((const char*)rhs)+2, key.size(), *((uint16*)rhs)) < 0;
  }
  bool leq(void* rhs) const
  {
    return strnncmp(&key[0], ((const char*)rhs)+2, key.size(), *((uint16*)rhs)) <= 0;
  }
  bool equal(void* rhs) const
  {
    return key.size() == *(uint16*)rhs && !memcmp(&key[0], ((const char*)rhs) + 2, *((uint16*)rhs));
  }

  uint32 size_of() const
  {
    return 2 + key.length();
  }

  static constexpr uint32 const_size() { return 0; }

  static uint32 size_of(void* data)
  {
    return *((uint16*)data) + 2;
  }

  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    memcpy((uint8*)data + 2, key.data(), key.length());
  }

  bool operator<(const String_Index& a) const
  {
    return (key < a.key);
  }

  bool operator==(const String_Index& a) const
  {
    return key == a.key;
  }

  static uint32 max_size_of()
  {
    throw Unsupported_Error("static uint32 String_Index::max_size_of()");
    return 0;
  }
  
  static String_Index min() { return String_Index{ "" }; }
  static String_Index max() { return String_Index{ "\xff" }; }
};


struct Tag_Index_Local
{
  uint32 index;
  std::string key;
  std::string value;

  Tag_Index_Local() {}

  template< typename Id_Type >
  Tag_Index_Local(const Tag_Entry< Id_Type >& entry)
      : index(entry.index), key(entry.key), value(entry.value) {}

  Tag_Index_Local(uint32 index_, std::string key_, std::string value_)
      : index(index_), key(key_), value(value_) {}
  Tag_Index_Local(Uint31_Index index_, std::string key_, std::string value_)
      : index(index_.val() & 0x7fffff00), key(key_), value(value_) {}

  Tag_Index_Local(void* data)
  {
    index = (*((uint32*)data + 1))<<8;
    key = std::string(((int8*)data + 7), *(uint16*)data);
    value = std::string(((int8*)data + 7 + key.length()),
		   *((uint16*)data + 1));
  }

  static bool equal(void* lhs, void* rhs)
  { return *(uint32*)lhs == *(uint32*)rhs && !memcmp(lhs, rhs, *(uint16*)lhs + *(((uint16*)lhs)+2) + 7); }
  bool less(void* rhs) const
  {
    uint32 lhs_idx = index>>8;
    uint32 rhs_idx = (*(((uint32*)rhs) + 1) & 0xffffff);
    if ((lhs_idx & 0x7fffff) != (rhs_idx & 0x7fffff))
      return (lhs_idx & 0x7fffff) < (rhs_idx & 0x7fffff);
    if (lhs_idx != rhs_idx)
      return lhs_idx < rhs_idx;

    int keycmp = strnncmp(&key[0], ((const char*)rhs)+7, key.size(), *((uint16*)rhs));
    if (keycmp)
      return keycmp < 0;
    return strnncmp(&value[0], ((const char*)rhs) + 7 + *(uint16*)rhs, value.size(), *(((uint16*)rhs)+1)) < 0;
  }
  bool leq(void* rhs) const
  {
    uint32 lhs_idx = index>>8;
    uint32 rhs_idx = (*(((uint32*)rhs) + 1) & 0xffffff);
    if ((lhs_idx & 0x7fffff) != (rhs_idx & 0x7fffff))
      return (lhs_idx & 0x7fffff) < (rhs_idx & 0x7fffff);
    if (lhs_idx != rhs_idx)
      return lhs_idx < rhs_idx;

    int keycmp = strnncmp(&key[0], ((const char*)rhs)+7, key.size(), *((uint16*)rhs));
    if (keycmp)
      return keycmp < 0;
    return strnncmp(&value[0], ((const char*)rhs) + 7 + *(uint16*)rhs, value.size(), *(((uint16*)rhs)+1)) <= 0;
  }
  bool equal(void* rhs) const
  {
    return key.size() == *(uint16*)rhs && value.size() == *((uint16*)rhs + 1)
      && (index>>8) == (*(((uint32*)rhs) + 1) & 0xffffff)
      && !memcmp(&key[0], ((const char*)rhs) + 7, *((uint16*)rhs))
      && !memcmp(&value[0], ((const char*)rhs) + 7 + *(uint16*)rhs, *((uint16*)rhs + 1));
  }

  uint32 size_of() const
  {
    return 7 + key.length() + value.length();
  }

  static constexpr uint32 const_size() { return 0; }

  static uint32 size_of(void* data)
  {
    return (*((uint16*)data) + *((uint16*)data + 1) + 7);
  }

  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    *((uint32*)data + 1) = index>>8;
    memcpy(((uint8*)data + 7), key.data(), key.length());
    memcpy(((uint8*)data + 7 + key.length()), value.data(),
	   value.length());
  }

  bool operator<(const Tag_Index_Local& a) const
  {
    if ((index & 0x7fffffff) != (a.index & 0x7fffffff))
      return ((index & 0x7fffffff) < (a.index & 0x7fffffff));
    if (index != a.index)
      return (index < a.index);
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }

  bool operator==(const Tag_Index_Local& a) const
  {
    if (index != a.index)
      return false;
    if (key != a.key)
      return false;
    return (value == a.value);
  }

  static uint32 max_size_of()
  {
    throw Unsupported_Error("static uint32 Tag_Index_Local::max_size_of()");
    return 0;
  }
  
  static Tag_Index_Local min() { return Tag_Index_Local{ 0u, "", "" }; }
  static Tag_Index_Local max() { return Tag_Index_Local{ 0x7fffffff, "\xff", "\xff" }; }
};


inline const std::string& void_tag_value()
{
  static std::string void_value = " ";
  if (void_value == " ")
    void_value[0] = '\xff';
  return void_value;
}


template< class Index >
Ranges< Tag_Index_Local > formulate_range_query(const std::set< Index >& coarse_indices)
{
  Ranges< Tag_Index_Local > ranges;
  for (auto it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    ranges.push_back({ it->val(), "", "" }, { it->val() + 0x100, "", "" });
  return ranges;
}


template< class Value >
Ranges< Tag_Index_Local > formulate_range_query(const std::map< uint32, Value >& coarse_indices)
{
  Ranges< Tag_Index_Local > ranges;
  for (typename std::map< uint32, Value >::const_iterator it = coarse_indices.begin(); it != coarse_indices.end(); ++it)
    ranges.push_back({ it->first, "", "" }, { it->first + 0x100, "", "" });
  return ranges;
}


template< class TIndex, class TObject >
void generate_ids_by_coarse
  (std::map< uint32, std::vector< typename TObject::Id_Type > >& ids_by_coarse,
   const std::map< TIndex, std::vector< TObject > >& items)
{
  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    std::vector< typename TObject::Id_Type >& ids_by_coarse_ = ids_by_coarse[it->first.val() & 0x7fffff00];

    for (typename std::vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      ids_by_coarse_.push_back(it2->id);
  }

  for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator
      it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
  {
    std::vector< typename TObject::Id_Type >& ids_by_coarse_ = it->second;
    std::sort(ids_by_coarse_.begin(), ids_by_coarse_.end());
    ids_by_coarse_.erase(std::unique(ids_by_coarse_.begin(), ids_by_coarse_.end()), ids_by_coarse_.end());
  }
  for (typename std::map< uint32, std::vector< typename TObject::Id_Type > >::iterator
      it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    std::sort(it->second.begin(), it->second.end());
}


template< class TIndex, class TObject >
void generate_ids_by_coarse
  (std::map< uint32, std::vector< Attic< typename TObject::Id_Type > > >& ids_by_coarse,
   const std::map< TIndex, std::vector< TObject > >& items)
{
  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    std::vector< Attic< typename TObject::Id_Type > >& ids_by_coarse_ = ids_by_coarse[it->first.val() & 0x7fffff00];

    for (typename std::vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      ids_by_coarse_.push_back
          (Attic< typename TObject::Id_Type >(it2->id, it2->timestamp));
  }

  for (typename std::map< uint32, std::vector< Attic< typename TObject::Id_Type > > >::iterator
      it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
  {
    std::vector< Attic< typename TObject::Id_Type > >& ids_by_coarse_ = it->second;
    std::sort(ids_by_coarse_.begin(), ids_by_coarse_.end());
    ids_by_coarse_.erase(std::unique(ids_by_coarse_.begin(), ids_by_coarse_.end()), ids_by_coarse_.end());
  }
  for (typename std::map< uint32, std::vector< Attic< typename TObject::Id_Type > > >::iterator
      it = ids_by_coarse.begin(); it != ids_by_coarse.end(); ++it)
    std::sort(it->second.begin(), it->second.end());
}


struct Tag_Index_Global_Until756
{
  std::string key;
  std::string value;

  Tag_Index_Global_Until756() {}

  Tag_Index_Global_Until756(void* data)
  {
    key = std::string(((int8*)data + 4), *(uint16*)data);
    value = std::string(((int8*)data + 4 + key.length()),
		   *((uint16*)data + 1));
  }

  Tag_Index_Global_Until756(const Tag_Index_Local& tag_idx) : key(tag_idx.key), value(tag_idx.value) {}

  Tag_Index_Global_Until756(const std::string& key_, const std::string& value_) : key(key_), value(value_) {}

  static bool equal(void* lhs, void* rhs)
  { return *(uint32*)lhs == *(uint32*)rhs && !memcmp(lhs, rhs, *(uint16*)lhs + *((uint16*)lhs + 1) + 4); }
  bool less(void* rhs) const
  {
    int keycmp = strnncmp(&key[0], ((const char*)rhs)+4, key.size(), *((uint16*)rhs));
    if (keycmp)
      return keycmp < 0;
    return strnncmp(&value[0], ((const char*)rhs) + 4 + *(uint16*)rhs, value.size(), *(((uint16*)rhs)+1)) < 0;
  }
  bool leq(void* rhs) const
  {
    int keycmp = strnncmp(&key[0], ((const char*)rhs)+4, key.size(), *((uint16*)rhs));
    if (keycmp)
      return keycmp < 0;
    return strnncmp(&value[0], ((const char*)rhs) + 4 + *(uint16*)rhs, value.size(), *(((uint16*)rhs)+1)) <= 0;
  }
  bool equal(void* rhs) const
  {
    return key.size() == *(uint16*)rhs && value.size() == *((uint16*)rhs + 1)
      && !memcmp(&key[0], ((const char*)rhs) + 4, *((uint16*)rhs))
      && !memcmp(&value[0], ((const char*)rhs) + 4 + *(uint16*)rhs, *((uint16*)rhs + 1));
  }

  uint32 size_of() const
  {
    return 4 + key.length() + value.length();
  }

  static constexpr uint32 const_size() { return 0; }

  static uint32 size_of(void* data)
  {
    return (*((uint16*)data) + *((uint16*)data + 1) + 4);
  }

  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    memcpy(((uint8*)data + 4), key.data(), key.length());
    memcpy(((uint8*)data + 4 + key.length()), value.data(),
	   value.length());
  }

  bool operator<(const Tag_Index_Global_Until756& a) const
  {
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }

  bool operator==(const Tag_Index_Global_Until756& a) const
  {
    if (key != a.key)
      return false;
    return (value == a.value);
  }

  static uint32 max_size_of()
  {
    throw Unsupported_Error("static uint32 Tag_Index_Global_Until756::max_size_of()");
    return 0;
  }
  
  static Tag_Index_Global_Until756 min() { return Tag_Index_Global_Until756{ "", "" }; }
  static Tag_Index_Global_Until756 max() { return Tag_Index_Global_Until756{ "\xff", "\xff" }; }
};


struct Tag_Index_Global_KVI
{
  std::string key;
  std::string value;
  uint32 idx;

  Tag_Index_Global_KVI() : idx(0) {}

  Tag_Index_Global_KVI(void* data)
  {
    key = std::string(((int8*)data + 8), *(uint16*)data);
    value = std::string(((int8*)data + 8 + key.length()), *((uint16*)data + 1));
    idx = *(uint32*)((int8*)data + 4);
  }

  Tag_Index_Global_KVI(const Tag_Index_Local& tag_idx) : key(tag_idx.key), value(tag_idx.value), idx(0) {}

  Tag_Index_Global_KVI(const std::string& key_, const std::string& value_) : key(key_), value(value_), idx(0) {}
  Tag_Index_Global_KVI(const std::string& key_, const std::string& value_, uint32 idx_)
    : key(key_), value(value_), idx(idx_) {}

  static bool equal(void* lhs, void* rhs)
  { return *(uint32*)lhs == *(uint32*)rhs
      && !memcmp(lhs, rhs, *(uint16*)lhs + *((uint16*)lhs + 1) + 8); }
  bool less(void* rhs) const
  {
    int keycmp = strnncmp(&key[0], ((const char*)rhs)+8, key.size(), *((uint16*)rhs));
    if (keycmp)
      return keycmp < 0;
    int valcmp = strnncmp(&value[0], ((const char*)rhs) + 8 + *(uint16*)rhs, value.size(), *(((uint16*)rhs)+1));
    if (valcmp)
      return valcmp < 0;
    return idx < *(uint32*)((int8*)rhs + 4);
  }
  bool leq(void* rhs) const
  {
    int keycmp = strnncmp(&key[0], ((const char*)rhs)+8, key.size(), *((uint16*)rhs));
    if (keycmp)
      return keycmp < 0;
    int valcmp = strnncmp(&value[0], ((const char*)rhs) + 8 + *(uint16*)rhs, value.size(), *(((uint16*)rhs)+1));
    if (valcmp)
      return valcmp < 0;
    return idx <= *(uint32*)((int8*)rhs + 4);
  }
  bool equal(void* rhs) const
  {
    return key.size() == *(uint16*)rhs && value.size() == *((uint16*)rhs + 1) && idx == *(uint32*)((int8*)rhs + 4)
      && !memcmp(&key[0], ((const char*)rhs) + 8, *((uint16*)rhs))
      && !memcmp(&value[0], ((const char*)rhs) + 8 + *(uint16*)rhs, *((uint16*)rhs + 1));
  }

  uint32 size_of() const
  {
    return 8 + key.length() + value.length();
  }

  static constexpr uint32 const_size() { return 0; }

  static uint32 size_of(void* data)
  {
    return *((uint16*)data) + *((uint16*)data + 1) + 8;
  }

  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    *(uint32*)((int8*)data + 4) = idx;
    memcpy((uint8*)data + 8, key.data(), key.length());
    memcpy((uint8*)data + 8 + key.length(), value.data(), value.length());
  }

  bool operator<(const Tag_Index_Global_KVI& a) const
  {
    if (key != a.key)
      return (key < a.key);
    if (value != a.value)
      return (value < a.value);
    return idx < a.idx;
  }

  bool operator==(const Tag_Index_Global_KVI& a) const
  {
    return key == a.key && value == a.value && idx == a.idx;
  }

  static uint32 max_size_of()
  {
    throw Unsupported_Error("static uint32 Tag_Index_Global_KVI::max_size_of()");
    return 0;
  }
  
  static Tag_Index_Global_KVI min() { return Tag_Index_Global_KVI{ "", "" }; }
  static Tag_Index_Global_KVI max() { return Tag_Index_Global_KVI{ "\xff", "\xff" }; }
};


typedef Tag_Index_Global_KVI Tag_Index_Global;


template< typename Id_Type_ >
struct Tag_Object_Global
{
  typedef Id_Type_ Id_Type;

  Uint31_Index idx;
  Id_Type id;

  Tag_Object_Global() {}

  Tag_Object_Global(Id_Type id_, Uint31_Index idx_) : idx(idx_), id(id_) {}

  Tag_Object_Global(void* data)
  {
    idx = Uint31_Index(((*((uint32*)data))<<8) & 0xffffff00);
    id = Id_Type((void*)((uint8*)data + 3));
  }

  uint32 size_of() const
  {
    return 3 + id.size_of();
  }

  static uint32 size_of(void* data)
  {
    return 3 + Id_Type::size_of((void*)((uint8*)data + 3));
  }

  void to_data(void* data) const
  {
    *(uint32*)data = ((idx.val()>>8) & 0x7fffff);
    id.to_data((void*)((uint8*)data + 3));
  }

  bool operator<(const Tag_Object_Global& a) const
  {
    if (id < a.id)
      return true;
    if (a.id < id)
      return false;

    return (idx < a.idx);
  }

  bool operator==(const Tag_Object_Global& a) const
  {
    return (id == a.id && idx == a.idx);
  }

  static uint32 max_size_of()
  {
    return 3 + Id_Type::max_size_of();
  }
};


#endif
