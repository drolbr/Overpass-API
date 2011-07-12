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
  bool operator() (const Relation& a, const Relation& b)
  {
    return (a.id < b.id);
  }
};

struct Relation_Equal_Id {
  bool operator() (const Relation& a, const Relation& b)
  {
    return (a.id == b.id);
  }
};

struct Relation_Skeleton
{
  uint32 id;
  vector< Relation_Entry > members;
  
  Relation_Skeleton() {}
  
  Relation_Skeleton(void* data)
  {
    id = *(uint32*)data;
    members.resize(*((uint32*)data + 1));
    for (uint i(0); i < *((uint32*)data + 1); ++i)
    {
      members[i].ref = *((uint32*)data + 2 + 2*i);
      members[i].role = *((uint32*)data + 3 + 2*i) & 0xffffff;
      members[i].type = *((uint8*)data + 15 + 8*i);
    }
  }
  
  Relation_Skeleton(const Relation& rel)
  : id(rel.id), members(rel.members) {}
  
  Relation_Skeleton(uint32 id_, const vector< Relation_Entry >& members_)
  : id(id_), members(members_) {}
  
  uint32 size_of() const
  {
    return 8 + 8*members.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (8 + 8 * *((uint32*)data + 1));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *((uint32*)data + 1) = members.size();
    for (uint i(0); i < members.size(); ++i)
    {
      *((uint32*)data + 2 + 2*i) = members[i].ref;
      *((uint32*)data + 3 + 2*i) = members[i].role & 0xffffff;
      *((uint8*)data + 15 + 8*i) = members[i].type;
    }
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
