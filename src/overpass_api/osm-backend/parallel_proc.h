/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__PARALLEL_PROC_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__PARALLEL_PROC_H

#include <atomic>
#include <functional>
#include <future>
#include <vector>


inline void process_package(std::vector< std::function< void() > >& f, const int parallel_processes)
{
  // skip thread creation if no parallel processing requested
  if (parallel_processes <= 1)
  {
    for (int i = 0; i < f.size(); i++)
      f[i]();
    f.clear();
    return;
  }

  std::vector< std::future< void > > futures;
  std::atomic< unsigned int > package{0};

  for (int i = 0; i < parallel_processes; i++)
  {
    futures.push_back(
        std::async(std::launch::async, [&]
      {
        while (true) {
          int current_package = package++;
          if (current_package >= f.size())
            return;
          f[current_package]();
        }
      }));
  }

  for (auto &e : futures)
    e.get();

  futures.clear();
  f.clear();
}

#endif
