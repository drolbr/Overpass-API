#ifndef FILE_TYPES
#define FILE_TYPES

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "raw_file_db.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using namespace std;

typedef char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

//-----------------------------------------------------------------------------

static const char* NODE_TAG_ID_NODE_LOCAL_FILE = "/opt/osm_why_api/node_tag_id_node_local.dat";
static const char* NODE_TAG_ID_NODE_LOCAL_IDX = "/opt/osm_why_api/node_tag_id_node_local.idx";
static const char* NODE_TAG_ID_NODE_GLOBAL_FILE = "/opt/osm_why_api/node_tag_id_node_global.dat";
static const char* NODE_TAG_ID_NODE_GLOBAL_IDX = "/opt/osm_why_api/node_tag_id_node_global.idx";
static const char* NODE_TAG_NODE_ID_FILE = "/opt/osm_why_api/node_tag_node_id.dat";
static const char* NODE_TAG_NODE_ID_IDX = "/opt/osm_why_api/node_tag_node_id.idx";

const uint32 TAG_ID_NODE_LOCAL_BLOCKSIZE = 4*1024*1024;
const uint32 TAG_ID_NODE_GLOBAL_BLOCKSIZE = 4*1024*1024;
const uint32 TAG_NODE_ID_BLOCKSIZE = 4*1024*1024;

static const char* NODE_STRING_DATA = "/opt/osm_why_api/node_strings.dat";
static const char* NODE_STRING_IDX = "/opt/osm_why_api/node_strings.idx";

const uint NODE_TAG_SPATIAL_PARTS = 32;
const int NODE_STRING_BLOCK_SIZE = 1024*1024;

//-----------------------------------------------------------------------------

const char* NODE_DATA = "/opt/osm_why_api/nodes.dat";
const char* NODE_IDX = "/opt/osm_why_api/nodes.b.idx";
const char* NODE_IDXA = "/opt/osm_why_api/nodes.1.idx";

const char* WAY_DATA = "/opt/osm_why_api/ways.dat";
const char* WAY_IDX = "/opt/osm_why_api/ways.b.idx";
const char* WAY_IDXA = "/opt/osm_why_api/ways.1.idx";
const char* WAY_IDXSPAT = "/opt/osm_why_api/ways.spatial.idx";
const char* WAY_ALLTMP = "/tmp/ways.dat";

//-----------------------------------------------------------------------------

struct Tag_Id_Node_Local
{
  uint32 blocksize() const { return TAG_ID_NODE_LOCAL_BLOCKSIZE; }
  const char* data_file() const { return NODE_TAG_ID_NODE_LOCAL_FILE; }
  const char* index_file() const { return NODE_TAG_ID_NODE_LOCAL_IDX; }
  
  typedef uint32 Index;
  uint32 size_of_Index() const { return sizeof(uint32); }
  
  uint32 size_of_buf(uint8* elem) const
  {
    return (elem[4]*sizeof(uint32) + 2*sizeof(uint8) + sizeof(uint32));
  }
};

struct Tag_Id_Node_Local_Reader : public Tag_Id_Node_Local
{
  Tag_Id_Node_Local_Reader(const set< uint32 >& ids, const set< uint32 >& idxs, set< int >& result)
  : result_(result), ids_(ids), idxs_(idxs) {}
  
  typedef set< uint32 >::const_iterator Index_Iterator;
  Index_Iterator idxs_begin() const { return idxs_.begin(); }
  Index_Iterator idxs_end() const { return idxs_.end(); }
  
  void process(uint8* buf)
  {
    if (ids_.find(*((uint32*)buf)) != ids_.end())
    {
      for (uint32 j(0); j < *((uint8*)&(buf[4])); ++j)
	result_.insert(*((uint32*)&(buf[6 + 4*j])));
    }
  }
  
  protected:
    set< int >& result_;
    const set< uint32 >& ids_;
    const set< uint32 >& idxs_;
};

struct Tag_Id_Node_Local_Multiint_Reader : public Tag_Id_Node_Local_Reader
{
  Tag_Id_Node_Local_Multiint_Reader
      (const set< uint32 >& ids, const set< uint32 >& idxs, const set< int >& source, set< int >& result)
  : Tag_Id_Node_Local_Reader(ids, idxs, result), source_(source) {}
  
  void process(uint8* buf)
  {
    if (ids_.find(*((uint32*)buf)) != ids_.end())
    {
      for (uint32 j(0); j < *((uint8*)&(buf[4])); ++j)
      {
	if (source_.find(*((uint32*)&(buf[6 + 4*j]))) != source_.end())
	  result_.insert(*((uint32*)&(buf[6 + 4*j])));
      }
    }
  }
  
  private:
    const set< int >& source_;
};

struct Tag_MultiNode_Id_Local_Reader : public Tag_Id_Node_Local
{
  Tag_MultiNode_Id_Local_Reader
      (map< uint32, set< uint32 > >& node_to_id, const set< uint32 >& idxs)
  : node_to_id_(node_to_id), idxs_(idxs) {}
  
  typedef set< uint32 >::const_iterator Index_Iterator;
  Index_Iterator idxs_begin() const { return idxs_.begin(); }
  Index_Iterator idxs_end() const { return idxs_.end(); }
  
  void process(uint8* buf)
  {
    for (uint32 j(0); j < *((uint8*)&(buf[4])); ++j)
    {
      map< uint32, set< uint32 > >::iterator it
	  (node_to_id_.find(*((uint32*)&(buf[6 + 4*j]))));
      if (it != node_to_id_.end())
	it->second.insert(*((uint32*)buf));
    }
  }
  
  private:
    map< uint32, set< uint32 > >& node_to_id_;
    const set< uint32 >& idxs_;
};

struct Tag_Id_Node_Local_Writer : public Tag_Id_Node_Local
{
  Tag_Id_Node_Local_Writer(uint32* block_of_id) : block_index_(), block_of_id_(block_of_id) {}
  
  const multimap< uint32, uint16 >& block_index() const { return block_index_; }
  multimap< uint32, uint16 >& block_index() { return block_index_; }
  
  typedef vector< uint32* >::const_iterator Iterator;
  
  uint32 size_of(const vector< uint32* >::const_iterator& it) const
  {
    return ((*it)[1]*sizeof(uint32) + 2*sizeof(uint8) + sizeof(uint32));
  }
  
  uint32 index_of(const vector< uint32* >::const_iterator& it) const
  {
    return ((block_of_id_[**it]) & 0xffffff00);
  }
  
  uint32 index_of_buf(uint8* elem) const
  {
    return ((block_of_id_[*((uint32*)elem)]) & 0xffffff00);
  }
  
  int32 compare(const vector< uint32* >::const_iterator& it, uint8* buf) const
  {
    if (((block_of_id_[**it]) & 0xffffff00) < ((block_of_id_[*((uint32*)buf)]) & 0xffffff00))
      return RAW_DB_LESS;
    else if (((block_of_id_[**it]) & 0xffffff00) > ((block_of_id_[*((uint32*)buf)]) & 0xffffff00))
      return RAW_DB_GREATER;
    if (**it < *((uint32*)buf))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  void to_buf(uint8* dest, const vector< uint32* >::const_iterator& it) const
  {
    ((uint32*)dest)[0] = (*it)[0];
    dest[4] = (*it)[1];
    dest[5] = block_of_id_[**it];
    memcpy(&(dest[6]), &((*it)[2]), (*it)[1]*sizeof(uint32));
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    ((uint32*)dest)[0] = i;
  }

  private:
    multimap< uint32, uint16 > block_index_;
    uint32* block_of_id_;
};

//-----------------------------------------------------------------------------

struct Tag_Id_Node_Global
{
  uint32 blocksize() const { return TAG_ID_NODE_GLOBAL_BLOCKSIZE; }
  const char* data_file() const { return NODE_TAG_ID_NODE_GLOBAL_FILE; }
  const char* index_file() const { return NODE_TAG_ID_NODE_GLOBAL_IDX; }
  
  typedef uint32 Index;
  uint32 size_of_Index() const { return sizeof(uint32); }
  
  uint32 size_of_buf(uint8* elem) const
  {
    return (elem[4]*sizeof(uint32) + sizeof(uint8) + sizeof(uint32));
  }
};

struct Tag_Id_Node_Global_Reader : public Tag_Id_Node_Global
{
  Tag_Id_Node_Global_Reader(const set< uint32 >& ids, set< int >& result)
  : result_(result), ids_(ids) {}
  
  typedef set< uint32 >::const_iterator Index_Iterator;
  Index_Iterator idxs_begin() const { return ids_.begin(); }
  Index_Iterator idxs_end() const { return ids_.end(); }
  
  void process(uint8* buf)
  {
    if (ids_.find(*((uint32*)buf)) != ids_.end())
    {
      for (uint32 j(0); j < *((uint8*)&(buf[4])); ++j)
	result_.insert(*((uint32*)&(buf[5 + 4*j])));
    }
  }
  
  protected:
    set< int >& result_;
    const set< uint32 >& ids_;
};

struct Tag_Id_Node_Global_Multiint_Reader : public Tag_Id_Node_Global_Reader
{
  Tag_Id_Node_Global_Multiint_Reader
      (const set< uint32 >& ids, const set< int >& source, set< int >& result)
  : Tag_Id_Node_Global_Reader(ids, result), source_(source) {}
  
  void process(uint8* buf)
  {
    if (ids_.find(*((uint32*)buf)) != ids_.end())
    {
      for (uint32 j(0); j < *((uint8*)&(buf[4])); ++j)
      {
	if (source_.find(*((uint32*)&(buf[5 + 4*j]))) != source_.end())
	  result_.insert(*((uint32*)&(buf[5 + 4*j])));
      }
    }
  }
  
  private:
    const set< int >& source_;
};

struct Tag_Id_Node_Global_Writer : public Tag_Id_Node_Global
{
  Tag_Id_Node_Global_Writer() : block_index_() {}
  
  const multimap< uint32, uint16 >& block_index() const { return block_index_; }
  multimap< uint32, uint16 >& block_index() { return block_index_; }
  
  typedef vector< uint32* >::const_iterator Iterator;
  
  uint32 size_of(const vector< uint32* >::const_iterator& it) const
  {
    return ((*it)[1]*sizeof(uint32) + sizeof(uint8) + sizeof(uint32));
  }
  
  uint32 index_of(const vector< uint32* >::const_iterator& it) const
  {
    return **it;
  }
  
  uint32 index_of_buf(uint8* elem) const
  {
    return *((uint32*)elem);
  }
  
  int32 compare(const vector< uint32* >::const_iterator& it, uint8* buf) const
  {
    if (**it < *((uint32*)buf))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  void to_buf(uint8* dest, const vector< uint32* >::const_iterator& it) const
  {
    ((uint32*)dest)[0] = (*it)[0];
    dest[4] = (*it)[1];
    memcpy(&(dest[5]), &((*it)[2]), (*it)[1]*sizeof(uint32));
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    ((uint32*)dest)[0] = i;
  }
  
  private:
    multimap< uint32, uint16 > block_index_;
};

//-----------------------------------------------------------------------------

struct Tag_Node_Id
{
  uint32 blocksize() const { return TAG_NODE_ID_BLOCKSIZE; }
  const char* data_file() const { return NODE_TAG_NODE_ID_FILE; }
  const char* index_file() const { return NODE_TAG_NODE_ID_IDX; }
  
  typedef uint32 Index;
  uint32 size_of_Index() const { return sizeof(uint32); }
  
  uint32 size_of_buf(uint8* elem) const
  {
    return ((*((uint16*)&(elem[8])))*(sizeof(uint32) + sizeof(uint8)) + sizeof(uint16) + 2*sizeof(uint32));
  }
};

struct Tag_Node_Id_Reader : public Tag_Node_Id
{
  Tag_Node_Id_Reader
      (map< uint32, set< uint32 > >& node_to_id, const set< uint32 >& idxs)
  : node_to_id_(node_to_id), idxs_(idxs) {}
  
  typedef set< uint32 >::const_iterator Index_Iterator;
  Index_Iterator idxs_begin() const { return idxs_.begin(); }
  Index_Iterator idxs_end() const { return idxs_.end(); }
  
  void process(uint8* buf)
  {
    map< uint32, set< uint32 > >::iterator it(node_to_id_.find(*((uint32*)buf)));
    if (it != node_to_id_.end())
    {
      for (uint32 j(0); j < *((uint16*)&(buf[8])); ++j)
	it->second.insert(*((uint32*)&(buf[10 + 5*j])));
    }
  }
  
  private:
    map< uint32, set< uint32 > >& node_to_id_;
    const set< uint32 >& idxs_;
};

struct Tag_Node_Id_Iterator
{
  Tag_Node_Id_Iterator
      (vector< vector< uint32 > >& vect, const vector< uint32 >& read_order, uint32 pos)
    : i(pos), read_order_(read_order), ids_of_node(vect) {}
  
  uint32 i;
  const vector< uint32 >& read_order_;
  vector< vector< uint32 > >& ids_of_node;
};

inline Tag_Node_Id_Iterator& operator++(Tag_Node_Id_Iterator& t)
{
  ++t.i;
  return t;
}

inline bool operator==(const Tag_Node_Id_Iterator& a, const Tag_Node_Id_Iterator& b)
{
  return (a.i == b.i);
}

inline bool operator!=(const Tag_Node_Id_Iterator& a, const Tag_Node_Id_Iterator& b)
{
  return (a.i != b.i);
}

struct Tag_Node_Id_Writer : public Tag_Node_Id
{
  Tag_Node_Id_Writer(uint32* ll_idx, uint8* blocklet_of_id)
    : ids_of_node(), offset(1),
    block_index_(), ll_idx_(ll_idx), blocklet_of_id_(blocklet_of_id)
     {}
  
  const multimap< uint32, uint16 >& block_index() const { return block_index_; }
  multimap< uint32, uint16 >& block_index() { return block_index_; }
  
  typedef Tag_Node_Id_Iterator Iterator;
  
  Iterator begin() { return Tag_Node_Id_Iterator(ids_of_node, read_order, 0); }
  Iterator end() { return Tag_Node_Id_Iterator(ids_of_node, read_order, read_order.size()); }

  uint32 size_of(const Tag_Node_Id_Iterator& it) const
  {
    return (ids_of_node[read_order[it.i]].size()*(sizeof(uint32)
	+ sizeof(uint8)) + sizeof(uint16) + 2*sizeof(uint32));
  }
  
  uint32 index_of(const Tag_Node_Id_Iterator& it) const
  {
    return ll_idx_[read_order[it.i]];
  }
  
  uint32 index_of_buf(uint8* elem) const
  {
    return *((uint32*)&(elem[4]));
  }
  
  int32 compare(const Tag_Node_Id_Iterator& it, uint8* buf) const
  {
    if (ll_idx_[read_order[it.i]] < *((uint32*)&(buf[4])))
      return RAW_DB_LESS;
    else if (ll_idx_[read_order[it.i]] > *((uint32*)&(buf[4])))
      return RAW_DB_GREATER;
    if (read_order[it.i] + offset < *((uint32*)&(buf[0])))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  void to_buf(uint8* dest, const Tag_Node_Id_Iterator& it) const
  {
    *((uint32*)&(dest[0])) = read_order[it.i] + offset;
    *((uint32*)&(dest[4])) = ll_idx_[read_order[it.i]];
    *((uint16*)&(dest[8])) = ids_of_node[read_order[it.i]].size();
    for (uint32 i(0); i < ids_of_node[read_order[it.i]].size(); ++i)
    {
      *((uint32*)&(dest[5*i + 10])) = ids_of_node[read_order[it.i]][i];
      *((uint8*)&(dest[5*i + 14])) = blocklet_of_id_[ids_of_node[read_order[it.i]][i]];
    }
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    ((uint32*)dest)[0] = i;
  }
  
  vector< vector< uint32 > > ids_of_node;
  vector< uint32 > read_order;
  uint32 offset;
  
private:
  multimap< uint32, uint16 > block_index_;
  uint32* ll_idx_;
  uint8* blocklet_of_id_;
};

#endif
