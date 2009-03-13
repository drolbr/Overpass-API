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

#include "script_datatypes.h"

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

static const char* DATADIR = "/opt/osm_why_api/";

static const char* NODE_ID_NODE_FILE = "nodes_2.dat";
static const char* NODE_ID_NODE_IDX = "nodes_2.idx";
static const char* NODE_ID_NODE_IDIDX = "nodes_2.i.idx";
static const char* NODE_TAG_ID_NODE_LOCAL_FILE = "node_tag_id_node_local.dat";
static const char* NODE_TAG_ID_NODE_LOCAL_IDX = "node_tag_id_node_local.idx";
static const char* NODE_TAG_ID_NODE_GLOBAL_FILE = "node_tag_id_node_global.dat";
static const char* NODE_TAG_ID_NODE_GLOBAL_IDX = "node_tag_id_node_global.idx";
static const char* NODE_TAG_NODE_ID_FILE = "node_tag_node_id.dat";
static const char* NODE_TAG_NODE_ID_IDX = "node_tag_node_id.idx";

const uint32 NODE_ID_NODE_BLOCKSIZE = 8*1024*1024;
const uint32 TAG_ID_NODE_LOCAL_BLOCKSIZE = 4*1024*1024;
const uint32 TAG_ID_NODE_GLOBAL_BLOCKSIZE = 4*1024*1024;
const uint32 TAG_NODE_ID_BLOCKSIZE = 4*1024*1024;

const uint NODE_TAG_SPATIAL_PARTS = 32;
const int NODE_STRING_BLOCK_SIZE = 1024*1024;

//-----------------------------------------------------------------------------

static const char* NODE_DATA = "/opt/osm_why_api/nodes.dat";
static const char* NODE_IDX = "/opt/osm_why_api/nodes.b.idx";
static const char* NODE_IDXA = "/opt/osm_why_api/nodes.1.idx";

static const char* WAY_DATA = "/opt/osm_why_api/ways.dat";
static const char* WAY_IDX = "/opt/osm_why_api/ways.b.idx";
static const char* WAY_IDXA = "/opt/osm_why_api/ways.1.idx";
static const char* WAY_IDXSPAT = "/opt/osm_why_api/ways.spatial.idx";
static const char* WAY_ALLTMP = "/tmp/ways.dat";

//-----------------------------------------------------------------------------

struct Node_Id_Node
{
  uint32 blocksize() const { return NODE_ID_NODE_BLOCKSIZE; }
  const string data_file() const { return ((string)DATADIR + NODE_ID_NODE_FILE); }
  const string index_file() const { return ((string)DATADIR + NODE_ID_NODE_IDX); }
  const string id_idx_file() const { return ((string)DATADIR + NODE_ID_NODE_IDIDX); }
  
  typedef int32 Index;
  uint32 size_of_Index() const { return sizeof(uint32); }
  
  uint32 size_of_buf(uint8* elem) const
  {
    return (3*sizeof(uint32));
  }
};

struct Node_Id_Node_Index_Iterator
{
  Node_Id_Node_Index_Iterator(const set< pair< int32, int32 > >::const_iterator& it_inside_,
			      const set< pair< int32, int32 > >::const_iterator& it_border_, int32 pos_)
  : it_inside(it_inside_), it_border(it_border_), pos(pos_) {}
  
  set< pair< int32, int32 > >::const_iterator it_inside, it_border;
  int32 pos;
};

inline Node_Id_Node::Index operator*(const Node_Id_Node_Index_Iterator& a)
{
  return a.pos;
}

inline bool operator==(const Node_Id_Node_Index_Iterator& a, const Node_Id_Node_Index_Iterator& b)
{
  return ((a.it_inside == b.it_inside) && (b.it_border == b.it_border) && (a.pos == b.pos));
}

inline bool operator!=(const Node_Id_Node_Index_Iterator& a, const Node_Id_Node_Index_Iterator& b)
{
  return (!((a.it_inside == b.it_inside) && (b.it_border == b.it_border) && (a.pos == b.pos)));
}

struct Node_Id_Node_Range_Reader : public Node_Id_Node
{
  Node_Id_Node_Range_Reader(const set< pair< int32, int32 > >& idxs_inside,
			    const set< pair< int32, int32 > >& idxs_border,
                            set< Node >& result_inside, set< Node >& result_border)
    : result_inside_(result_inside), result_border_(result_border),
    idxs_inside_(idxs_inside), idxs_border_(idxs_border),
    it_inside(idxs_inside.begin()), it_border(idxs_border.begin()) {}
  
  typedef Node_Id_Node_Index_Iterator Index_Iterator;
  Index_Iterator idxs_begin() const
  {
    int32 pos(0);
    if ((!(idxs_inside_.empty())) && ((idxs_border_.empty()) ||
	   (idxs_inside_.begin()->first < idxs_border_.begin()->first)))
      pos = idxs_inside_.begin()->first;
    else if (!(idxs_border_.empty()))
      pos = idxs_border_.begin()->first;
    return Node_Id_Node_Index_Iterator(idxs_inside_.begin(), idxs_border_.begin(), pos);
  }
  Index_Iterator idxs_end() const
  {
    return Node_Id_Node_Index_Iterator(idxs_inside_.end(), idxs_border_.end(), 0);
  }
  void inc_idx(Index_Iterator& it, const Index& idx)
  {
    while ((it.it_inside != idxs_inside_.end()) && (it.it_inside->second <= idx))
      ++it.it_inside;
    while ((it.it_border != idxs_border_.end()) && (it.it_border->second <= idx))
      ++it.it_border;
    if (it.it_inside != idxs_inside_.end())
    {
      it.pos = it.it_inside->first;
      if ((it.it_border != idxs_border_.end()) &&
	   (idxs_border_.begin()->first < idxs_inside_.begin()->first))
	it.pos = it.it_border->first;
      if (idx >= it.pos)
	it.pos = idx+1;
    }
    else if (it.it_border != idxs_border_.end())
    {
      it.pos = it.it_border->first;
      if (idx >= it.pos)
	it.pos = idx+1;
    }
    else
      it.pos = 0;
  }

  void block_notify(uint8* buf)
  {
    int32 buf_idx(ll_idx(*(int32*)&(buf[4]), *(int32*)&(buf[8])));
    it_inside = idxs_inside_.lower_bound(make_pair< int32, int32 >
	(buf_idx, buf_idx));
    if (it_inside != idxs_inside_.begin())
      --it_inside;
    it_border = idxs_border_.lower_bound(make_pair< int32, int32 >
	(buf_idx, buf_idx));
    if (it_border != idxs_border_.begin())
      --it_border;
  }
  
  void process(uint8* buf)
  {
    int32 buf_idx(ll_idx(*(int32*)&(buf[4]), *(int32*)&(buf[8])));
    while ((it_inside != idxs_inside_.end()) && (it_inside->second < buf_idx))
      ++it_inside;
    while ((it_border != idxs_border_.end()) && (it_border->second < buf_idx))
      ++it_border;
    if ((it_inside != idxs_inside_.end()) && (buf_idx >= it_inside->first))
      result_inside_.insert(Node(*(int32*)&(buf[0]), *(int32*)&(buf[4]), *(int32*)&(buf[8])));
    if ((it_border != idxs_border_.end()) && (buf_idx >= it_border->first))
      result_border_.insert(Node(*(int32*)&(buf[0]), *(int32*)&(buf[4]), *(int32*)&(buf[8])));
  }
  
private:
  set< Node >& result_inside_;
  set< Node >& result_border_;
  const set< pair< int32, int32 > >& idxs_inside_, idxs_border_;
  set< pair< int32, int32 > >::const_iterator it_inside, it_border;
};

struct Node_Id_Node_Range_Count : public Node_Id_Node
{
  Node_Id_Node_Range_Count(const set< pair< int32, int32 > >& idxs_inside,
			    const set< pair< int32, int32 > >& idxs_border)
  : idxs_inside_(idxs_inside), idxs_border_(idxs_border), result(0) {}
  
  void count_idx(const vector< Index >::const_iterator& idx_begin,
		 const vector< Index >::const_iterator& idx_end)
  {
    set< pair< int32, int32 > >::const_iterator it_inside(idxs_inside_.begin());
    set< pair< int32, int32 > >::const_iterator it_border(idxs_border_.begin());
    vector< Index >::const_iterator it(idx_begin);
    if (it == idx_end)
      return;
    Index last_idx(*it);
    while (++it != idx_end)
    {
      while ((it_inside != idxs_inside_.end()) && (it_inside->second < *it))
      {
	result += (long long)blocksize()*
	    (it_inside->second - it_inside->first + 1)/(*it - last_idx + 1)/12;
	++it_inside;
      }
      while ((it_border != idxs_border_.end()) && (it_border->second < *it))
      {
	result += (long long)blocksize()*
	    (it_border->second - it_border->first + 1)/(*it - last_idx + 1)/12;
	++it_border;
      }
      last_idx = *it;
    }
  }
  
  uint32 get_result() { return result; }
  
private:
  const set< pair< int32, int32 > >& idxs_inside_, idxs_border_;
  uint32 result;
};

struct Node_Id_Node_By_Id_Reader : public Node_Id_Node
{
  Node_Id_Node_By_Id_Reader(const set< int32 >& ids, set< Node >& result)
    : result_(result), ids_(ids) {}
  
  typedef set< int32 >::const_iterator Iterator;
  Iterator ids_begin() const { return ids_.begin(); }
  Iterator ids_end() const { return ids_.end(); }
  
  void process(uint8* buf)
  {
    if (ids_.find(*((int32*)buf)) != ids_.end())
      result_.insert(Node(*(int32*)&(buf[0]), *(int32*)&(buf[4]), *(int32*)&(buf[8])));
  }
  
protected:
  set< Node >& result_;
  const set< int32 >& ids_;
};

struct Node_Id_Node_Dump : public Node_Id_Node
{
  Node_Id_Node_Dump(uint32 offset, uint32 count, uint32* ll_idx_buf)
  : offset_(offset), count_(count), ll_idx_buf_(ll_idx_buf) {}
  
  void process(uint8* buf)
  {
    if ((*(int32*)&(buf[0]) >= offset_) && ((*(int32*)&(buf[0])) - offset_ < count_))
      ll_idx_buf_[(*(int32*)&(buf[0])) - offset_] =
	  ll_idx((*(int32*)&(buf[4])), (*(int32*)&(buf[8])));
  }
  
  private:
    int32 offset_;
    int32 count_;
    uint32* ll_idx_buf_;
};

struct Node_Id_Node_Writer : public Node_Id_Node
{
  Node_Id_Node_Writer() : block_index_() {}
  
  const multimap< Index, uint16 >& block_index() const { return block_index_; }
  multimap< Index, uint16 >& block_index() { return block_index_; }
  
  typedef multimap< int, Node >::const_iterator Iterator;
  
  uint32 size_of(const Iterator& it) const
  {
    return (3*sizeof(uint32));
  }
  
  Index index_of(const Iterator& it) const
  {
    return (it->first);
  }
  
  Index index_of_buf(uint8* elem) const
  {
    return (ll_idx(*(int32*)&(elem[4]), *(int32*)&(elem[8])));
  }
  
  int32 compare(const Iterator& it, uint8* buf) const
  {
    if (it->first < ll_idx(*(int32*)&(buf[4]), *(int32*)&(buf[8])))
      return RAW_DB_LESS;
    else if (it->first > ll_idx(*(int32*)&(buf[4]), *(int32*)&(buf[8])))
      return RAW_DB_GREATER;
    if (it->second.id < *(int32*)&(buf[0]))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  void to_buf(uint8* dest, const Iterator& it) const
  {
    *(uint32*)&(dest[0]) = it->second.id;
    *(int32*)&(dest[4]) = it->second.lat;
    *(int32*)&(dest[8]) = it->second.lon;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    *(uint32*)&(dest[0]) = i;
  }

  uint32 id_of_buf(uint8* elem) const
  {
    return (*(int32*)&(elem[0]));
  }
  
  uint32 first_new_block() const { return 0; }
  
private:
    multimap< Index, uint16 > block_index_;
};

//-----------------------------------------------------------------------------

struct Tag_Id_Node_Local
{
  uint32 blocksize() const { return TAG_ID_NODE_LOCAL_BLOCKSIZE; }
  const string data_file() const { return ((string)DATADIR + NODE_TAG_ID_NODE_LOCAL_FILE); }
  const string index_file() const { return ((string)DATADIR + NODE_TAG_ID_NODE_LOCAL_IDX); }
  
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
  void inc_idx(Index_Iterator& it, Index& idx)
  {
    ++it;
    while ((it != idxs_end()) && (*it <= idx))
      ++it;
  }
  
  void block_notify(uint8* buf) const {}
  
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
  void inc_idx(Index_Iterator& it, Index& idx)
  {
    ++it;
    while ((it != idxs_end()) && (*it <= idx))
      ++it;
  }

  void block_notify(uint8* buf) const {}
  
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

struct Tag_Id_Count_Local_Reader : public Tag_Id_Node_Local
{
  Tag_Id_Count_Local_Reader(vector< uint32 >& id_count)
  : id_count_(id_count) {}
  
  void process(uint8* buf)
  {
    if (id_count_.size() < *((uint32*)buf))
      id_count_.resize(*((uint32*)buf));
    id_count_[*((uint32*)buf)-1] += *((uint8*)&(buf[4]));
  }
  
  private:
    vector< uint32 >& id_count_;
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
  const string data_file() const { return ((string)DATADIR + NODE_TAG_ID_NODE_GLOBAL_FILE); }
  const string index_file() const { return ((string)DATADIR + NODE_TAG_ID_NODE_GLOBAL_IDX); }
  
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
  void inc_idx(Index_Iterator& it, Index& idx)
  {
    ++it;
    while ((it != idxs_end()) && (*it <= idx))
      ++it;
  }

  void block_notify(uint8* buf) const {}
  
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

struct Tag_Id_Count_Global_Reader : public Tag_Id_Node_Global
{
  Tag_Id_Count_Global_Reader(vector< uint32 >& id_count)
  : id_count_(id_count) {}
  
  void process(uint8* buf)
  {
    if (id_count_.size() < *((uint32*)buf))
      id_count_.resize(*((uint32*)buf));
    id_count_[*((uint32*)buf)-1] += *((uint8*)&(buf[4]));
  }
  
  private:
    vector< uint32 >& id_count_;
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
  const string data_file() const { return ((string)DATADIR + NODE_TAG_NODE_ID_FILE); }
  const string index_file() const { return ((string)DATADIR + NODE_TAG_NODE_ID_IDX); }
  
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
  void inc_idx(Index_Iterator& it, Index& idx)
  {
    ++it;
    while ((it != idxs_end()) && (*it <= idx))
      ++it;
  }

  void block_notify(uint8* buf) const {}
  
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
