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

#ifndef DE__OSM3S___TEMPLATE_DB__RANGES_DEF_H
#define DE__OSM3S___TEMPLATE_DB__RANGES_DEF_H

#include "ranges.h"

#include <algorithm>


template< typename Index >
Ranges< Index > Ranges< Index >::intersect(const Ranges< Index >& rhs) const
{
  if (rhs.is_global())
    return *this;
  else if (this->is_global())
    return rhs;

  Ranges< Index > result;
  auto it_a = data.begin();
  auto it_b = rhs.data.begin();

  while (it_a != data.end() && it_b != rhs.data.end())
  {
    if (!(it_a->first < it_b->second))
      ++it_b;
    else if (!(it_b->first < it_a->second))
      ++it_a;
    else if (it_b->second < it_a->second)
    {
      result.push_back(std::max(it_a->first, it_b->first), it_b->second);
      ++it_b;
    }
    else
    {
      result.push_back(std::max(it_a->first, it_b->first), it_a->second);
      ++it_a;
    }
  }

  return result;
}


template< typename Index >
Ranges< Index > Ranges< Index >::union_(const Ranges< Index >& rhs) const
{
  Ranges< Index > result;

  auto it_a = data.begin();
  auto it_b = rhs.data.begin();

  while (it_a != data.end() && it_b != rhs.data.end())
  {
    if (it_a->first < it_b->first)
    {
      result.data.push_back(*it_a);
      ++it_a;
    }
    else
    {
      result.data.push_back(*it_b);
      ++it_b;
    }
  }
  while (it_a != data.end())
  {
    result.data.push_back(*it_a);
    ++it_a;
  }
  while (it_b != rhs.data.end())
  {
    result.data.push_back(*it_b);
    ++it_b;
  }

  //result.sort();
  return result;
}


template< typename Index >
Ranges< Index > Ranges< Index >::skip_start(Index lower_bound) const
{
  Ranges< Index > result;
  for (const auto& i : data)
  {
    if (lower_bound < i.second)
      result.data.push_back({ std::max(i.first, lower_bound), i.second });
  }
  return result;
}


template< typename Index >
void Ranges< Index >::sort()
{
//   int order = 0;
//   uint64_t size = data.size();
//   while (size)
//   {
//     size = size>>1;
//     ++order;
//   }
//   static int count = 0;
//   static int total = 0;
//   static std::vector< int > stat;
//   if (stat.size() <= order)
//     stat.resize(order+1);
//   if (data.size() > 2*1024*1024)
//     ++total;
//   ++stat[order];
//   total += data.size();
//   if (++count % 200 == 0)
//   {
//     std::cerr<<"#Ranges::sort "<<count<<' '<<total<<'\n';
//     for (int i = 0; i < stat.size(); ++i)
//       std::cerr<<'\t'<<stat[i];
//     std::cerr<<'\n';
//   }

  std::sort(data.begin(), data.end());

  if (!data.empty())
  {
    decltype(data) result;
    auto it = data.begin();
    std::pair< Index, Index > buf = *it;
    for (++it; it != data.end(); ++it)
    {
      if (buf.second < it->first)
      {
        result.push_back(buf);
        buf = *it;
      }
      else
        buf.second = std::max(buf.second, it->second);
    }
    result.push_back(buf);
    result.swap(data);
  }
}


#endif
