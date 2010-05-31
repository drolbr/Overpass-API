#ifndef DATATYPES_DEFINED
#define DATATYPES_DEFINED

#include <cstring>
#include <map>
#include <set>
#include <vector>

using namespace std;

typedef unsigned int uint;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

struct Uint32_Index
{
  Uint32_Index(uint32 i) : value(i) {}
  Uint32_Index(void* data) : value(*(uint32*)data) {}
  
  uint32 size_of() const
  {
    return 4;
  }
  
  static uint32 max_size_of()
  {
    return 4;
  }
  
  static uint32 size_of(void* data)
  {
    return 4;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = value;
  }
  
  bool operator<(const Uint32_Index& index) const
  {
    return this->value < index.value;
  }
  
  bool operator==(const Uint32_Index& index) const
  {
    return this->value == index.value;
  }
  
  uint32 val() const
  {
    return value;
  }
  
  protected:
    uint32 value;
};

struct Uint31_Index : Uint32_Index
{
  Uint31_Index(uint32 i) : Uint32_Index(i) {}
  Uint31_Index(void* data) : Uint32_Index(*(uint32*)data) {}
  
  bool operator<(const Uint31_Index& index) const
  {
    if ((this->value & 0x7fffffff) != (index.value & 0x7fffffff))
    {
      return (this->value & 0x7fffffff) < (index.value & 0x7fffffff);
    }
    return (this->value < index.value);
  }
};

struct String_Object
{
  String_Object(string s) : value(s) {}
  String_Object(void* data) : value()
  {
    value = string(((int8*)data + 2), *(uint16*)data);
  }
  
  uint32 size_of() const
  {
    return value.length() + 2;
  }
  
  static uint32 size_of(void* data)
  {
    return *(uint16*)data + 2;
  }
  
  void to_data(void* data) const
  {
    *(uint16*)data = value.length();
    memcpy(((uint8*)data + 2), value.data(), value.length());
  }
  
  bool operator<(const String_Object& index) const
  {
    return this->value < index.value;
  }
  
  bool operator==(const String_Object& index) const
  {
    return this->value == index.value;
  }
  
  string val() const
  {
    return value;
  }
  
  protected:
    string value;
};

struct Node
{
  uint32 id;
  uint32 ll_upper_;
  uint32 ll_lower_;
  vector< pair< string, string > > tags;
  
  Node() {}
  
  Node(uint32 id_, double lat, double lon)
  : id(id_), ll_upper_(ll_upper(lat, lon)), ll_lower_(ll_lower(lat, lon))
  {}
  
  static uint32 ll_upper(double lat, double lon)
  {
    uint32 result(0), ilat((lat + 90.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
    
    for (uint32 i(0); i < 16; ++i)
    {
      result |= ((0x1<<(i+16))&ilat)>>(15-i);
      result |= ((0x1<<(i+16))&(uint32)ilon)>>(16-i);
    }
    
    return result;
  }
  
  static uint32 ll_lower(double lat, double lon)
  {
    uint32 result(0), ilat((lat + 90.0)*10000000+0.5);
    int32 ilon(lon*10000000 + (lon > 0 ? 0.5 : -0.5));
    
    for (uint32 i(0); i < 16; ++i)
    {
      result |= ((0x1<<i)&ilat)<<(i+1);
      result |= ((0x1<<i)&(uint32)ilon)<<i;
    }
    
    return result;
  }
  
  static double lat(uint32 ll_upper, uint32 ll_lower)
  {
    uint32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<(31-2*i))&ll_upper)<<i;
      result |= ((0x1<<(31-2*i))&ll_lower)>>(16-i);
    }
    
    return ((double)result)/10000000 - 90.0;
  }
  
  static double lon(uint32 ll_upper, uint32 ll_lower)
  {
    int32 result(0);
    
    for (uint32 i(0); i < 16; i+=1)
    {
      result |= ((0x1<<(30-2*i))&ll_upper)<<(i+1);
      result |= ((0x1<<(30-2*i))&ll_lower)>>(15-i);
    }
    
    return ((double)result)/10000000;
  }
};

struct Node_Comparator_By_Id {
  bool operator() (const Node& a, const Node& b)
  {
    return (a.id < b.id);
  }
};

struct Node_Skeleton
{
  uint32 id;
  uint32 ll_lower;
  
  Node_Skeleton() {}
  
  Node_Skeleton(void* data)
  {
    id = *(uint32*)data;
    ll_lower = *(uint32*)((uint8*)data+4);
  }
  
  Node_Skeleton(const Node& node)
  : id(node.id), ll_lower(node.ll_lower_) {}
  
  Node_Skeleton(uint32 id_, uint32 ll_lower_)
  : id(id_), ll_lower(ll_lower_) {}
  
  uint32 size_of() const
  {
    return 8;
  }
  
  static uint32 size_of(void* data)
  {
    return 8;
  }
  
  void to_data(void* data) const
  {
    *(uint32*)data = id;
    *(uint32*)((uint8*)data+4) = ll_lower;
  }
  
  bool operator<(const Node_Skeleton& a) const
  {
    return this->id < a.id;
  }
  
  bool operator==(const Node_Skeleton& a) const
  {
    return this->id == a.id;
  }
};

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
  
  static uint32 calc_index(const vector< uint32 >& memb_idxs)
  {
    if (memb_idxs.empty())
      return 0;
    
    uint32 bitmask(0), value(memb_idxs[0]);
    for (uint i(1); i < memb_idxs.size(); ++i)
    {
      if (memb_idxs[i] & 0x80000000)
      {
	if (memb_idxs[i] & 0xff == 0x10)
	  bitmask |= 0xff;
	else if (memb_idxs[i] & 0xff == 0x20)
	  bitmask |= 0xffff;
	else if (memb_idxs[i] & 0xff == 0x30)
	  bitmask |= 0xffffff;
	else
	  bitmask |= 0xffffffff;
      }
      else
        bitmask |= (value ^ memb_idxs[i]);
    }
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

struct Relation_Comparator_By_Id {
  bool operator() (const Relation& a, const Relation& b)
  {
    return (a.id < b.id);
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

struct Tag_Entry
{
  uint32 index;
  string key;
  string value;
  vector< uint32 > ids;
};

struct Tag_Index_Local
{
  uint32 index;
  string key;
  string value;
  
  Tag_Index_Local() {}
  
  Tag_Index_Local(void* data)
  {
    index = (*((uint32*)data + 1))<<8;
    key = string(((int8*)data + 7), *(uint16*)data);
    value = string(((int8*)data + 7 + key.length()),
		   *((uint16*)data + 1));
  }
  
  uint32 size_of() const
  {
    return 7 + key.length() + value.length();
  }
  
  static uint32 size_of(void* data)
  {
    return (*((uint16*)data) + *((uint16*)data + 1) + 7);
  }
  
  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    *((uint32*)data + 1) = index>>8;
    memcpy(((uint8*)data + 7), key.data(), key.length());
    memcpy(((uint8*)data + 7 + key.length()), value.data(),
	   value.length());
  }
  
  bool operator<(const Tag_Index_Local& a) const
  {
    if ((index & 0x7fffffff) != (a.index & 0x7fffffff))
      return ((index & 0x7fffffff) < (a.index & 0x7fffffff));
    if (index != a.index)
      return (index < a.index);
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Tag_Index_Local& a) const
  {
    if (index != a.index)
      return false;
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

struct Tag_Index_Global
{
  string key;
  string value;
  
  Tag_Index_Global() {}
  
  Tag_Index_Global(void* data)
  {
    key = string(((int8*)data + 4), *(uint16*)data);
    value = string(((int8*)data + 4 + key.length()),
		   *((uint16*)data + 1));
  }
  
  uint32 size_of() const
  {
    return 4 + key.length() + value.length();
  }
  
  static uint32 size_of(void* data)
  {
    return (*((uint16*)data) + *((uint16*)data + 1) + 4);
  }
  
  void to_data(void* data) const
  {
    *(uint16*)data = key.length();
    *((uint16*)data + 1) = value.length();
    memcpy(((uint8*)data + 4), key.data(), key.length());
    memcpy(((uint8*)data + 4 + key.length()), value.data(),
	   value.length());
  }
  
  bool operator<(const Tag_Index_Global& a) const
  {
    if (key != a.key)
      return (key < a.key);
    return (value < a.value);
  }
  
  bool operator==(const Tag_Index_Global& a) const
  {
    if (key != a.key)
      return false;
    return (value == a.value);
  }
};

/**
  * A dataset that is referred in the scripts by a variable.
  */
struct Set
{
  map< Uint32_Index, vector< Node_Skeleton > > nodes;
  map< Uint31_Index, vector< Way_Skeleton > > ways;
  map< Uint31_Index, vector< Relation_Skeleton > > relations;
};

#endif
