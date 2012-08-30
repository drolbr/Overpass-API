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

#ifndef DE__OSM3S___OVERPASS_API__CORE__TYPE_RELATION_H
#define DE__OSM3S___OVERPASS_API__CORE__TYPE_RELATION_H

#include "basic_types.h"
#include "index_computations.h"

#include <cstring>
#include <map>
#include <set>
#include <vector>

using namespace std;

struct Relation_Entry
{
  Relation_Entry() : ref(0), type(0), role(0) {}
  
  uint32 ref;
  uint32 type;
  uint32 role;
  const static uint32 NODE = 1;
  const static uint32 WAY = 2;
  const static uint32 RELATION = 3;
};

struct Relation
{
  uint32 id;
  uint32 index;
  vector< Relation_Entry > members;
  vector< Uint31_Index > node_idxs;
  vector< Uint31_Index > way_idxs;
  vector< pair< string, string > > tags;
  
  Relation() {}
  
  Relation(uint32 id_)
  : id(id_)
  {}
  
  Relation(uint32 id_, uint32 index_, const vector< Relation_Entry >& members_)
  : id(id_), index(index_), members(members_) {}
  
  static uint32 calc_index(const vector< uint32 >& memb_idxs)
  {
    return ::calc_index(memb_idxs);
  }
};

struct Relation_Comparator_By_Id {
  bool operator() (const Relation* a, const Relation* b)
  {
    return (a->id < b->id);
  }
};

struct Relation_Equal_Id {
  bool operator() (const Relation* a, const Relation* b)
  {
    return (a->id == b->id);
  }
};

struct Relation_Skeleton
{
  uint32 id;
  vector< Relation_Entry > members;
  vector< Uint31_Index > node_idxs;
  vector< Uint31_Index > way_idxs;
  
  Relation_Skeleton() {}
  
  Relation_Skeleton(void* data)
  {
    id = *(uint32*)data;
    members.resize(*((uint32*)data + 1));
    node_idxs.resize(*((uint32*)data + 2), 0u);
    way_idxs.resize(*((uint32*)data + 3), 0u);
    for (uint i(0); i < *((uint32*)data + 1); ++i)
    {
      members[i].ref = *((uint32*)data + 4 + 2*i);
      members[i].role = *((uint32*)data + 5 + 2*i) & 0xffffff;
      members[i].type = *((uint8*)data + 23 + 8*i);
    }
    uint32* start_ptr = (uint32*)data + 4 + 2*members.size();
    for (uint i = 0; i < node_idxs.size(); ++i)
      node_idxs[i] = *(start_ptr + i);
    start_ptr = (uint32*)data + 4 + 2*members.size() + node_idxs.size();
    for (uint i = 0; i < way_idxs.size(); ++i)
      way_idxs[i] = *(start_ptr + i);
  }
  
  Relation_Skeleton(const Relation& rel)
      : id(rel.id), members(rel.members), node_idxs(rel.node_idxs), way_idxs(rel.way_idxs) {}
  
  Relation_Skeleton(uint32 id_, const vector< Relation_Entry >& members_,
		    const vector< Uint31_Index >& node_idxs_,
		    const vector< Uint31_Index >& way_idxs_)
      : id(id_), members(members_), node_idxs(node_idxs_), way_idxs(way_idxs_) {}
  
  uint32 size_of() const
  {
    return 16 + 8*members.size() + 4*node_idxs.size() + 4*way_idxs.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (16 + 8 * *((uint32*)data + 1) + 4* *((uint32*)data + 2) + 4* *((uint32*)data + 3));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *((uint32*)data + 1) = members.size();
    *((uint32*)data + 2) = node_idxs.size();
    *((uint32*)data + 3) = way_idxs.size();
    for (uint i = 0; i < members.size(); ++i)
    {
      *((uint32*)data + 4 + 2*i) = members[i].ref;
      *((uint32*)data + 5 + 2*i) = members[i].role & 0xffffff;
      *((uint8*)data + 23 + 8*i) = members[i].type;
    }
    Uint31_Index* start_ptr = (Uint31_Index*)data + 4 + 2*members.size();
    for (uint i = 0; i < node_idxs.size(); ++i)
      *(start_ptr + i) = node_idxs[i];
    start_ptr = (Uint31_Index*)data + 4 + 2*members.size() + node_idxs.size();
    for (uint i = 0; i < way_idxs.size(); ++i)
      *(start_ptr + i) = way_idxs[i];
  }
  
  bool operator<(const Relation_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Relation_Skeleton& a) const
  {
    return this->id == a.id;
  }
};

#endif
