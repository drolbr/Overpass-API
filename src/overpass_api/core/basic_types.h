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
  
  protected:
    uint32 value;
};

struct Uint31_Index : Uint32_Index
{
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

#endif
