#ifndef TYPE_WAY_DEFINED
#define TYPE_WAY_DEFINED

#include <cstring>
#include <map>
#include <set>
#include <vector>

#include "basic_types.h"

using namespace std;

struct Way
{
  uint32 id;
  uint32 index;
  vector< uint32 > nds;
  vector< pair< string, string > > tags;
  
  Way() {}
  
  Way(uint32 id_)
  : id(id_)
  {}
  
  static uint32 calc_index(const vector< uint32 >& nd_idxs)
  {
    if (nd_idxs.empty())
      return 0;
    
    uint32 bitmask(0), value(nd_idxs[0]);
    for (uint i(1); i < nd_idxs.size(); ++i)
      bitmask |= (value ^ nd_idxs[i]);
    if (bitmask & 0xff000000)
      value = 0x80000040;
    else if (bitmask & 0xffff0000)
      value = (value & 0xff000000) | 0x80000030;
    else if (bitmask & 0xffffff00)
      value = (value & 0xffff0000) | 0x80000020;
    else if (bitmask)
      value = (value & 0xffffff00) | 0x80000010;
    
    return value;
  }
};

struct Way_Comparator_By_Id {
  bool operator() (const Way& a, const Way& b)
  {
    return (a.id < b.id);
  }
};

struct Way_Equal_Id {
  bool operator() (const Way& a, const Way& b)
  {
    return (a.id == b.id);
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
