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


#include "../core/basic_types.h"


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


inline bool try_double(const std::string& input, double& result)
{
  if (input == "")
    return false;
  
  const char* input_c = input.c_str();
  char* end_c = 0;
  result = strtod(input_c, &end_c);
  return input_c + input.size() == end_c;
}


inline bool try_int64(const std::string& input, int64& result)
{
  if (input == "")
    return false;
  
  const char* input_c = input.c_str();
  char* end_c = 0;
  result = strtoll(input_c, &end_c, 0);
  return input_c + input.size() == end_c;
}


template< typename T >
struct Array
{
  Array(unsigned int size) : ptr(0)
  {
    if (size > 0)
      ptr = new T[size];
  }
  ~Array() { delete[] ptr; }
  
  T* ptr;
};


#endif
