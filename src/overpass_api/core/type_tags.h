/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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


struct Tag_Index_Local
{
  uint32 index;
  std::string key;
  std::string value;
  
  Tag_Index_Local() {}
  
  template< typename Id_Type >
  Tag_Index_Local(const Tag_Entry< Id_Type >& entry)
      : index(entry.index), key(entry.key), value(entry.value) {}
  
  Tag_Index_Local(Uint31_Index index_, std::string key_, std::string value_)
      : index(index_.val() & 0x7fffff00), key(key_), value(value_) {}
  
  Tag_Index_Local(void* data)
  {
    index = (*((uint32*)data + 1))<<8;
    key = std::string(((int8*)data + 7), *(uint16*)data);
    value = std::string(((int8*)data + 7 + key.length()),
		   *((uint16*)data + 1));
  }
  
  uint32 size_of() const
  {
    return 7 + key.length() + value.length();
  }
  
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
};


inline const std::string& void_tag_value()
{
  static std::string void_value = " ";
  if (void_value == " ")
    void_value[0] = 0xff;
  return void_value;
}


template< class TIndex >
void formulate_range_query
    (std::set< std::pair< Tag_Index_Local, Tag_Index_Local > >& range_set,
     const std::set< TIndex >& coarse_indices)
{
  for (typename std::set< TIndex >::const_iterator
    it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = it->val();
    lower.key = "";
    lower.value = "";
    upper.index = it->val() + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(std::make_pair(lower, upper));
  }
}


template< class TIndex, class TObject >
void generate_ids_by_coarse
  (std::set< TIndex >& coarse_indices,
   std::map< uint32, std::vector< typename TObject::Id_Type > >& ids_by_coarse,
   const std::map< TIndex, std::vector< TObject > >& items)
{
  std::set< TIndex > new_coarse_indices;

  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    new_coarse_indices.insert(TIndex(it->first.val() & 0x7fffff00));
    std::vector< typename TObject::Id_Type >& ids_by_coarse_ = ids_by_coarse[it->first.val() & 0x7fffff00];
    
    for (typename std::vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      ids_by_coarse_.push_back(it2->id);
  }

  coarse_indices.insert(new_coarse_indices.begin(),new_coarse_indices.end());

  for (typename std::set< TIndex >::const_iterator
       it2(new_coarse_indices.begin()); it2!= new_coarse_indices.end(); ++it2)
  {
    std::vector< typename TObject::Id_Type >& ids_by_coarse_ = ids_by_coarse[it2->val()];
    std::sort(ids_by_coarse_.begin(), ids_by_coarse_.end());
    ids_by_coarse_.erase(std::unique(ids_by_coarse_.begin(), ids_by_coarse_.end()), ids_by_coarse_.end());
  }
}


template< class TIndex, class TObject >
void generate_ids_by_coarse
  (std::set< TIndex >& coarse_indices,
   std::map< uint32, std::vector< Attic< typename TObject::Id_Type > > >& ids_by_coarse,
   const std::map< TIndex, std::vector< TObject > >& items)
{
  std::set< TIndex > new_coarse_indices;

  for (typename std::map< TIndex, std::vector< TObject > >::const_iterator
    it(items.begin()); it != items.end(); ++it)
  {
    new_coarse_indices.insert(TIndex(it->first.val() & 0x7fffff00));
    std::vector< Attic< typename TObject::Id_Type > >& ids_by_coarse_ = ids_by_coarse[it->first.val() & 0x7fffff00];
    
    for (typename std::vector< TObject >::const_iterator it2(it->second.begin());
        it2 != it->second.end(); ++it2)
      ids_by_coarse_.push_back
          (Attic< typename TObject::Id_Type >(it2->id, it2->timestamp));
    
    std::sort(ids_by_coarse_.begin(), ids_by_coarse_.end());
    ids_by_coarse_.erase(std::unique(ids_by_coarse_.begin(), ids_by_coarse_.end()), ids_by_coarse_.end());
  }

  coarse_indices.insert(new_coarse_indices.begin(),new_coarse_indices.end());

  for (typename std::set< TIndex >::const_iterator
       it2(new_coarse_indices.begin()); it2!= new_coarse_indices.end(); ++it2)
  {
    std::vector< Attic< typename TObject::Id_Type > >& ids_by_coarse_  = ids_by_coarse[it2->val()];
    std::sort(ids_by_coarse_.begin(), ids_by_coarse_.end());
    ids_by_coarse_.erase(std::unique(ids_by_coarse_.begin(), ids_by_coarse_.end()), ids_by_coarse_.end());
  }
}


struct Tag_Index_Global
{
  std::string key;
  std::string value;
  
  Tag_Index_Global() {}
  
  Tag_Index_Global(void* data)
  {
    key = std::string(((int8*)data + 4), *(uint16*)data);
    value = std::string(((int8*)data + 4 + key.length()),
		   *((uint16*)data + 1));
  }
  
  Tag_Index_Global(const Tag_Index_Local& tag_idx) : key(tag_idx.key), value(tag_idx.value) {}
  
  Tag_Index_Global(const std::string& key_, const std::string& value_) : key(key_), value(value_) {}
  
  uint32 size_of() const
  {
    return 4 + key.length() + value.length();
  }
  
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
  
  bool operator<(const Tag_Index_Global& a) const
  {
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Tag_Index_Global& a) const
  {
    if (key != a.key)
      return false;
    return (value == a.value);
  }
  
  static uint32 max_size_of()
  {
    throw Unsupported_Error("static uint32 Tag_Index_Global::max_size_of()");
    return 0;
  }
};


template< typename Id_Type >
struct Tag_Object_Global
{
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
