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

#ifndef DE__OSM3S___OVERPASS_API__DATA__UTILS_H
#define DE__OSM3S___OVERPASS_API__DATA__UTILS_H


#include "../core/datatypes.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>


template< typename Index, typename Skeleton >
unsigned int count(const std::map< Index, std::vector< Skeleton > >& elems)
{
  uint result = 0;
  for (typename std::map< Index, std::vector< Skeleton > >::const_iterator it = elems.begin();
       it != elems.end(); ++it)
    result += it->second.size();
  return result;
}


template < typename T >
std::string to_string(T t)
{
  std::ostringstream out;
  out<<std::setprecision(14)<<t;
  return out.str();
}


template < typename T >
std::string fixed_to_string(T t, unsigned int precision)
{
  std::ostringstream out;
  out<<std::fixed<<std::setprecision(precision)<<t;
  return out.str();
}


inline bool try_double(const std::string& input, double& result)
{
  if (input == "")
    return false;
  
  const char* input_c = input.c_str();
  char* end_c = 0;
  errno = 0;
  result = strtod(input_c, &end_c);  
  return input_c + input.size() == end_c;
}


inline bool try_starts_with_double(const std::string& input, double& result)
{
  if (input == "")
    return false;
  
  const char* input_c = input.c_str();
  char* end_c = 0;
  errno = 0;
  result = strtod(input_c, &end_c); 
  return !errno && input_c != end_c;
}


inline std::string double_suffix(const std::string& input)
{
  if (input == "")
    return "";
  
  const char* input_c = input.c_str();
  char* end_c = 0;
  errno = 0;
  strtod(input_c, &end_c);  
  
  if (!errno && input_c != end_c)
  {
    while (*end_c && isspace(*end_c))
      ++end_c;
    return end_c;
  }
  
  return "";
}


inline bool try_int64(const std::string& input, int64& result)
{
  if (input == "")
    return false;
  
  const char* input_c = input.c_str();
  char* end_c = 0;
  errno = 0;
  result = strtoll(input_c, &end_c, 0);
  return input_c + input.size() == end_c;
}


inline bool string_represents_boolean_true(const std::string& val)
{
  double val_d = 0;
  if (try_double(val, val_d))
    return val_d != 0;
  return !val.empty();
}


template< typename T >
struct Array
{
  Array(unsigned int size) : ptr(0), size_(size)
  {
    if (size > 0)
      ptr = new T[size];
  }
  ~Array() { delete[] ptr; }
  
  const T& operator[](unsigned int i) const { return ptr[i]; }  
  T& operator[](unsigned int i) { return ptr[i]; }  
  unsigned int size() const { return size_; }
  
private:
  T* ptr;
  unsigned int size_;
};


template< typename Object >
struct Owner
{
  Owner(Object* ptr_) : ptr(ptr_) {}
  ~Owner() { delete ptr; }
  
  operator bool() const { return ptr; }
  Object& operator*() const { return *ptr; }
  
private:
  Owner(const Owner&);
  Owner& operator=(const Owner&);
  
  Object* ptr;
};


template< typename Pointer >
struct Owning_Array
{
  Owning_Array() {}
  ~Owning_Array()
  {
    for (typename std::vector< Pointer >::iterator it = content.begin(); it != content.end(); ++it)
      delete *it;
  }
  
  const Pointer& operator[](uint i) const { return content[i]; }
  void push_back(Pointer ptr) { content.push_back(ptr); }
  uint size() const { return content.size(); }
  
private:
  Owning_Array(const Owning_Array&);
  Owning_Array& operator=(const Owning_Array&);
  
  std::vector< Pointer > content;
};


template< typename Index, typename Object >
void sort_second(std::map< Index, std::vector< Object > >& items)
{
  for (typename std::map< Index, std::vector< Object > >::iterator it = items.begin(); it != items.end(); ++it)
    std::sort(it->second.begin(), it->second.end());
}


inline void sort(Set& set)
{
  sort_second(set.nodes);
  sort_second(set.attic_nodes);
  sort_second(set.ways);
  sort_second(set.attic_ways);
  sort_second(set.relations);
  sort_second(set.attic_relations);
  sort_second(set.areas);
  sort_second(set.deriveds);
}


#endif
