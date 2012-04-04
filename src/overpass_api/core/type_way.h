/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_WAY_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_WAY_H

#include "basic_types.h"
#include "index_computations.h"

#include <cstring>
#include <map>
#include <set>
#include <vector>

using namespace std;

struct Way
{
  uint32 id;
  uint32 index;
  vector< uint32 > nds;
  vector< Uint31_Index > segment_idxs;
  vector< pair< string, string > > tags;
  
  Way() : id(0), index(0) {}
  
  Way(uint32 id_)
  : id(id_), index(0)
  {}
  
  Way(uint32 id_, uint32 index_, const vector< uint32 >& nds_)
  : id(id_), index(index_), nds(nds_) {}
  
  static uint32 calc_index(const vector< uint32 >& nd_idxs)
  {
    return ::calc_index(nd_idxs);
  }
};

struct Way_Comparator_By_Id {
  bool operator() (const Way* a, const Way* b)
  {
    return (a->id < b->id);
  }
};

struct Way_Equal_Id {
  bool operator() (const Way* a, const Way* b)
  {
    return (a->id == b->id);
  }
};

struct Way_Skeleton
{
  uint32 id;
  vector< uint32 > nds;
  vector< Uint31_Index > segment_idxs;
  
  Way_Skeleton() {}
  
  Way_Skeleton(void* data)
  {
    id = *(uint32*)data;
    nds.resize(*((uint16*)data + 2));
    for (int i(0); i < *((uint16*)data + 2); ++i)
      nds[i] = *(uint32*)((uint16*)data + 4 + 2*i);
    uint16* start_ptr = (uint16*)data + 4 + 2*nds.size();
    segment_idxs.resize(*((uint16*)data + 3), 0u);
    for (int i(0); i < *((uint16*)data + 3); ++i)
      segment_idxs[i] = *(Uint31_Index*)(start_ptr + 2*i);
  }
  
  Way_Skeleton(const Way& way)
  : id(way.id), nds(way.nds), segment_idxs(way.segment_idxs) {}
  
  Way_Skeleton(uint32 id_, const vector< uint32 >& nds_, const vector< Uint31_Index >& segment_idxs_)
  : id(id_), nds(nds_), segment_idxs(segment_idxs_) {}
  
  uint32 size_of() const
  {
    return 8 + 4*nds.size() + 4*segment_idxs.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (8 + 4 * *((uint16*)data + 2) + 4 * *((uint16*)data + 3));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *((uint16*)data + 2) = nds.size();
    *((uint16*)data + 3) = segment_idxs.size();
    for (uint i(0); i < nds.size(); ++i)
      *(uint32*)((uint16*)data + 4 + 2*i) = nds[i];
    uint16* start_ptr = (uint16*)data + 4 + 2*nds.size();
    for (uint i(0); i < segment_idxs.size(); ++i)
      *(Uint31_Index*)(start_ptr + 2*i) = segment_idxs[i];
  }
  
  bool operator<(const Way_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Way_Skeleton& a) const
  {
    return this->id == a.id;
  }
};

#endif
