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

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_WAY_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_WAY_H

#include "basic_types.h"
#include "index_computations.h"
#include "type_node.h"

#include <cstring>
#include <map>
#include <set>
#include <vector>

using namespace std;

struct Way
{
  typedef Uint32_Index Id_Type;
  
  Id_Type id;
  uint32 index;
  vector< Node::Id_Type > nds;
//   vector< Uint31_Index > segment_idxs;
  vector< Quad_Coord > geometry;
  vector< pair< string, string > > tags;
  
  Way() : id(0u), index(0) {}
  
  Way(uint32 id_)
  : id(id_), index(0)
  {}
  
  Way(uint32 id_, uint32 index_, const vector< Node::Id_Type >& nds_)
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
  typedef Way::Id_Type Id_Type;

  Id_Type id;
  vector< Node::Id_Type > nds;
  //vector< Uint31_Index > segment_idxs;
  vector< Quad_Coord > geometry;
  
  Way_Skeleton() : id(0u) {}
  
  Way_Skeleton(void* data) : id(*(uint32*)data)
  {
    nds.reserve(*((uint16*)data + 2));
    for (int i(0); i < *((uint16*)data + 2); ++i)
      nds.push_back(*(uint64*)((uint16*)data + 4 + 4*i));
    uint16* start_ptr = (uint16*)data + 4 + 4*nds.size();
//     segment_idxs.resize(*((uint16*)data + 3), 0u);
//     for (int i(0); i < *((uint16*)data + 3); ++i)
//       segment_idxs[i] = *(Uint31_Index*)(start_ptr + 2*i);
    geometry.reserve(*((uint16*)data + 3));
    for (int i(0); i < *((uint16*)data + 3); ++i)
      geometry.push_back(Quad_Coord(*(uint32*)(start_ptr + 4*i), *(uint32*)(start_ptr + 4*i + 2)));
  }
  
//   Way_Skeleton(const Way& way)
//   : id(way.id), nds(way.nds), segment_idxs(way.segment_idxs) {}
  Way_Skeleton(const Way& way)
      : id(way.id), nds(way.nds), geometry(way.geometry) {}
  
//   Way_Skeleton(uint32 id_, const vector< Node::Id_Type >& nds_, const vector< Uint31_Index >& segment_idxs_)
//   : id(id_), nds(nds_), segment_idxs(segment_idxs_) {}
  Way_Skeleton(uint32 id_, const vector< Node::Id_Type >& nds_, const vector< Quad_Coord >& geometry_)
      : id(id_), nds(nds_), geometry(geometry_) {}
  
  uint32 size_of() const
  {
    return 8 + 8*nds.size() + 8*geometry.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (8 + 8 * *((uint16*)data + 2) + 8 * *((uint16*)data + 3));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id.val();
    *((uint16*)data + 2) = nds.size();
//     *((uint16*)data + 3) = segment_idxs.size();
    *((uint16*)data + 3) = geometry.size();
    for (uint i(0); i < nds.size(); ++i)
      *(uint64*)((uint16*)data + 4 + 4*i) = nds[i].val();
    uint16* start_ptr = (uint16*)data + 4 + 4*nds.size();
//     for (uint i(0); i < segment_idxs.size(); ++i)
//       *(Uint31_Index*)(start_ptr + 2*i) = segment_idxs[i];
    for (uint i(0); i < geometry.size(); ++i)
    {
      *(uint32*)(start_ptr + 4*i) = geometry[i].ll_upper;
      *(uint32*)(start_ptr + 4*i + 2) = geometry[i].ll_lower;
    }
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


inline std::vector< Uint31_Index > calc_segment_idxs(const std::vector< uint32 >& nd_idxs)
{
  std::vector< Uint31_Index > result;
  std::vector< uint32 > segment_nd_idxs(2, 0);
  for (std::vector< uint32 >::size_type i = 1; i < nd_idxs.size(); ++i)
  {
    segment_nd_idxs[0] = nd_idxs[i-1];
    segment_nd_idxs[1] = nd_idxs[i];
    Uint31_Index segment_index = Way::calc_index(segment_nd_idxs);
    if ((segment_index.val() & 0x80000000) != 0)
      result.push_back(segment_index);
  }
  sort(result.begin(), result.end());
  result.erase(unique(result.begin(), result.end()), result.end());
  
  return result;
}


#endif
