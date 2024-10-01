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

#include "request_context.h"

#include <map>


uint64 eval_map(const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes)
{
  uint64 size(0);
  for (std::map< Uint32_Index, std::vector< Node_Skeleton > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    size += it->second.size()*8 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Way_Skeleton > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
    size += it->second.size()*128 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Relation_Skeleton > >& relations)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Relation_Skeleton > >::const_iterator
      it(relations.begin()); it != relations.end(); ++it)
    size += it->second.size()*192 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >& nodes)
{
  uint64 size(0);
  for (std::map< Uint32_Index, std::vector< Attic< Node_Skeleton > > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    size += it->second.size()*16 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >& ways)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Attic< Way_Skeleton > > >::const_iterator
      it(ways.begin()); it != ways.end(); ++it)
    size += it->second.size()*136 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >& relations)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Attic< Relation_Skeleton > > >::const_iterator
      it(relations.begin()); it != relations.end(); ++it)
    size += it->second.size()*200 + 64;
  return size;
}


uint64 eval_map(const std::map< Uint31_Index, std::vector< Area_Skeleton > >& areas)
{
  uint64 size(0);
  for (std::map< Uint31_Index, std::vector< Area_Skeleton > >::const_iterator
      it(areas.begin()); it != areas.end(); ++it)
    size += it->second.size()*128 + 64;
  return size;
}
