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

#ifndef DE__OSM3S___TEMPLATE_DB__RANGES_H
#define DE__OSM3S___TEMPLATE_DB__RANGES_H

#include <set>


template< typename Index >
class Ranges
{
public:
  Ranges(const std::set< std::pair< Index, Index > >& data_) : data(data_) {}
  Ranges(Index begin, Index end) : data({{ begin, end }}) {}
  
  class Iterator
  {
  public:
    Iterator(typename std::set< std::pair< Index, Index > >::const_iterator it_) : it(it_) {}
    Iterator() {}
    const Index& lower_bound() const { return it->first; }
    const Index& upper_bound() const { return it->second; }
    const std::pair< Index, Index >& operator*() const { return *it; }
    const Iterator& operator++()
    {
      ++it;
      return *this;
    }
    bool operator==(const Iterator& rhs) const { return it == rhs.it; }
    bool operator!=(const Iterator& rhs) const { return it != rhs.it; }
  private:
    typename std::set< std::pair< Index, Index > >::const_iterator it; 
  };
  
  Iterator begin() const { return Iterator(data.begin()); }
  Iterator end() const { return Iterator(data.end()); }
  bool empty() const { return data.empty(); }
  
  Ranges skip_start(Index lower_bound) const;

private:
  std::set< std::pair< Index, Index > > data;
};


template< typename Index >
Ranges< Index > Ranges< Index >::skip_start(Index lower_bound) const
{
  Ranges< Index > result({});
  for (const auto& i : data)
  {
    if (lower_bound < i.second)
      result.data.insert({ std::max(i.first, lower_bound), i.second });
  }
  return result;
}


#endif
