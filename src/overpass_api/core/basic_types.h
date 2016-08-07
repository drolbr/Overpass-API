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

#ifndef DE__OSM3S___OVERPASS_API__CORE__BASIC_TYPES_H
#define DE__OSM3S___OVERPASS_API__CORE__BASIC_TYPES_H


#include <vector>


typedef unsigned int uint;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

struct Uint32_Index
{
  Uint32_Index() : value(0u) {}
  Uint32_Index(uint32 i) : value(i) {}
  Uint32_Index(void* data) : value(*(uint32*)data) {}
  
  uint32 size_of() const
  {
    return 4;
  }
  
  static uint32 max_size_of()
  {
    return 4;
  }
  
  static uint32 size_of(void* data)
  {
    return 4;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = value;
  }
  
  bool operator<(const Uint32_Index& index) const
  {
    return this->value < index.value;
  }
  
  bool operator==(const Uint32_Index& index) const
  {
    return this->value == index.value;
  }
  
  Uint32_Index operator++()
  {
    ++value;
    return this;
  }
  
  Uint32_Index operator+=(Uint32_Index offset)
  {
    value += offset.val();
    return this;
  }
  
  Uint32_Index operator+(Uint32_Index offset) const
  {
    Uint32_Index temp(*this);
    return (temp += offset);
  }
  
  uint32 val() const
  {
    return value;
  }
  
  static uint32 get_val(const void* data)
  {
    return *(uint32*)data;
  }

  protected:
    uint32 value;
};


inline Uint32_Index inc(Uint32_Index idx)
{
  return Uint32_Index(idx.val() + 1);
}


inline Uint32_Index dec(Uint32_Index idx)
{
  return Uint32_Index(idx.val() - 1);
}


inline unsigned long long difference(Uint32_Index lhs, Uint32_Index rhs)
{
  return rhs.val() - lhs.val();
}


struct Uint31_Index : Uint32_Index
{
  Uint31_Index() : Uint32_Index() {}
  Uint31_Index(uint32 i) : Uint32_Index(i) {}
  Uint31_Index(void* data) : Uint32_Index(*(uint32*)data) {}
  
  bool operator<(const Uint31_Index& index) const
  {
    if ((this->value & 0x7fffffff) != (index.value & 0x7fffffff))
    {
      return (this->value & 0x7fffffff) < (index.value & 0x7fffffff);
    }
    return (this->value < index.value);
  }
};


inline Uint31_Index inc(Uint31_Index idx)
{
  if (idx.val() & 0x80000000)
    return Uint31_Index((idx.val() & 0x7fffffff) + 1);
  else
    return Uint31_Index(idx.val() | 0x80000000);
}


inline unsigned long long difference(Uint31_Index lhs, Uint31_Index rhs)
{
  return 2*(rhs.val() - lhs.val()) - ((lhs.val()>>31) & 0x1) + ((rhs.val()>>31) & 0x1);
}


struct Uint64
{
  Uint64() : value(0ull) {}
  Uint64(uint64 i) : value(i) {}
  Uint64(void* data) : value(*(uint64*)data) {}
  
  uint32 size_of() const { return 8; }
  static uint32 max_size_of() { return 8; }
  static uint32 size_of(void* data) { return 8; }
  
  void to_data(void* data) const
  {
    *(uint64*)data = value;
  }
  
  bool operator<(const Uint64& index) const
  {
    return this->value < index.value;
  }
  
  bool operator==(const Uint64& index) const
  {
    return this->value == index.value;
  }
  
  Uint64 operator++()
  {
    ++value;
    return this;
  }
  
  Uint64 operator+=(Uint64 offset)
  {
    value += offset.val();
    return this;
  }
  
  Uint64 operator+(Uint64 offset) const
  {
    Uint64 temp(*this);
    return (temp += offset);
  }
  
  uint64 val() const { return value; }
  
  protected:
    uint64 value;
};


struct Quad_Coord
{
  Quad_Coord() : ll_upper(0), ll_lower(0) {}
  Quad_Coord(uint32 ll_upper_, uint32 ll_lower_) : ll_upper(ll_upper_), ll_lower(ll_lower_) {}
  
  uint32 ll_upper;
  uint32 ll_lower;
  
  bool operator==(const Quad_Coord& rhs) const
  {
    return ll_upper == rhs.ll_upper && ll_lower == rhs.ll_lower;
  }
};


template< typename Element_Skeleton >
struct Attic : public Element_Skeleton
{
  Attic(const Element_Skeleton& elem, uint64 timestamp_) : Element_Skeleton(elem), timestamp(timestamp_) {}
  
  uint64 timestamp;
  
  Attic(void* data)
    : Element_Skeleton(data),
      timestamp(*(uint64*)((uint8*)data + Element_Skeleton::size_of(data)) & 0xffffffffffull) {}
  
  uint32 size_of() const
  {
    return Element_Skeleton::size_of() + 5;
  }
  
  static uint32 size_of(void* data)
  {
    return Element_Skeleton::size_of(data) + 5;
  }
  
  void to_data(void* data) const
  {
    Element_Skeleton::to_data(data);
    void* pos = (uint8*)data + Element_Skeleton::size_of();
    *(uint32*)(pos) = (timestamp & 0xffffffffull);
    *(uint8*)((uint8*)pos+4) = ((timestamp & 0xff00000000ull)>>32);
  }
  
  bool operator<(const Attic& rhs) const
  {
    if (*static_cast< const Element_Skeleton* >(this) < *static_cast< const Element_Skeleton* >(&rhs))
      return true;
    else if (*static_cast< const Element_Skeleton* >(&rhs) < *static_cast< const Element_Skeleton* >(this))
      return false;
    return (timestamp < rhs.timestamp);
  }
  
  bool operator==(const Attic& rhs) const
  {
    return (*static_cast< const Element_Skeleton* >(this) == rhs && timestamp == rhs.timestamp);
  }
};


template< typename Attic >
struct Delta_Comparator
{
public:
  bool operator()(const Attic& lhs, const Attic& rhs) const
  {
    if (lhs.id == rhs.id)
      return rhs.timestamp < lhs.timestamp;
    else
      return lhs.id < rhs.id;
  }
};


template< typename Object >
void make_delta(const std::vector< Object >& source, const std::vector< Object >& reference,
                std::vector< uint >& to_remove, std::vector< std::pair< uint, Object > >& to_add)
{
  //Detect a common prefix
  uint prefix_length = 0;
  while (prefix_length < source.size() && prefix_length < reference.size()
      && source[prefix_length] == reference[prefix_length])
    ++prefix_length;
  
  //Detect a common suffix
  uint suffix_length = 1;
  while (suffix_length < source.size() - prefix_length && suffix_length < reference.size() - prefix_length
      && source[source.size() - suffix_length] == reference[reference.size() - suffix_length])
    ++suffix_length;
  --suffix_length;
  
  for (uint i = prefix_length; i < reference.size() - suffix_length; ++i)
    to_remove.push_back(i);
  
  for (uint i = prefix_length; i < source.size() - suffix_length; ++i)
    to_add.push_back(std::make_pair(i, source[i]));
}


template< typename Object >
void copy_elems(const std::vector< Object >& source, std::vector< std::pair< uint, Object > >& target)
{
  uint i = 0;
  for (typename std::vector< Object >::const_iterator it = source.begin(); it != source.end(); ++it)
    target.push_back(std::make_pair(i++, *it));
}


template< typename Object >
void expand_diff(const std::vector< Object >& reference,
    const std::vector< uint >& removed, const std::vector< std::pair< uint, Object > >& added,
    std::vector< Object >& target)
{
  target.reserve(reference.size() - removed.size() + added.size());
  std::vector< uint >::const_iterator it_removed = removed.begin();
  typename std::vector< std::pair< uint, Object > >::const_iterator it_added = added.begin();
  for (uint i = 0; i < reference.size(); ++i)
  {
    while (it_added != added.end() && target.size() == it_added->first)
    {
      target.push_back(it_added->second);
      ++it_added;
    }
      
    if (it_removed == removed.end() || i < *it_removed)
      target.push_back(reference[i]);
    else
      ++it_removed;
  }
  while (it_added != added.end() && target.size() == it_added->first)
  {
    target.push_back(it_added->second);
    ++it_added;
  }
}


#endif
