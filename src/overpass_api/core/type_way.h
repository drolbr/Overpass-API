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
  
  Way_Skeleton() {}
  
  Way_Skeleton(void* data)
  {
    id = *(uint32*)data;
    nds.resize(*((uint16*)data + 2));
    for (int i(0); i < *((uint16*)data + 2); ++i)
      nds[i] = *(uint32*)((uint16*)data + 3 + 2*i);
  }
  
  Way_Skeleton(const Way& way)
  : id(way.id), nds(way.nds) {}
  
  Way_Skeleton(uint32 id_, const vector< uint32 >& nds_)
  : id(id_), nds(nds_) {}
  
  uint32 size_of() const
  {
    return 6 + 4*nds.size();
  }
  
  static uint32 size_of(void* data)
  {
    return (6 + 4 * *((uint16*)data + 2));
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *((uint16*)data + 2) = nds.size();
    for (uint i(0); i < nds.size(); ++i)
      *(uint32*)((uint16*)data + 3 + 2*i) = nds[i];
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
