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

static const char* NODE_ID_NODE_FILE = "nodes.dat";
static const char* NODE_ID_NODE_IDX = "nodes.idx";
static const char* NODE_ID_NODE_IDIDX = "nodes.i.idx";
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

static const char* WAY_FILE_BASE = "ways_2";
// const uint32 WAY_BLOCKSIZE = 4*1024*1024;

//-----------------------------------------------------------------------------

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
  uint32 size_of_part(const Iterator& it) const { return size_of(it); }
  
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
  
  bool to_buf(uint8* dest, const Iterator& it) const
  {
    *(uint32*)&(dest[0]) = it->second.id;
    *(int32*)&(dest[4]) = it->second.lat;
    *(int32*)&(dest[8]) = it->second.lon;
    return true;
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

struct Node_Id_Node_Updater : public Node_Id_Node
{
  Node_Id_Node_Updater(const set< Node >& delete_nodes, const set< Node >& new_nodes)
    : data(), block_index_()
  {
    for (set< Node >::const_iterator it(delete_nodes.begin()); it != delete_nodes.end(); ++it)
      data.insert(make_pair< pair< int32, int32 >, pair< bool, Node > >
	  (make_pair< int32, int32 >(ll_idx(it->lat, it->lon), it->id),
	   make_pair< bool, Node >(false, *it)));
    for (set< Node >::const_iterator it(new_nodes.begin()); it != new_nodes.end(); ++it)
      data.insert(make_pair< pair< int32, int32 >, pair< bool, Node > >
	  (make_pair< int32, int32 >(ll_idx(it->lat, it->lon), it->id),
	   make_pair< bool, Node >(true, *it)));
  }
  
  const multimap< Index, uint16 >& block_index() const { return block_index_; }
  multimap< Index, uint16 >& block_index() { return block_index_; }
  
  typedef multimap< pair< int32, int32 >, pair< bool, Node > >::const_iterator Iterator;
  Iterator elem_begin() { return data.begin(); }
  Iterator elem_end() { return data.end(); }
  
  typedef vector< uint16 >::const_iterator Id_Block_Iterator;
  Id_Block_Iterator block_of_elem_begin() { return block_ids.begin(); }
  
  uint32 size_of(const Iterator& it) const
  {
    if (it->second.first)
      return (3*sizeof(uint32));
    else
      return 0;
  }
  
  uint32 size_of_part(const Iterator& it) const { return size_of(it); }
  
  Index index_of(const Iterator& it) const
  {
    return (it->first.first);
  }
  
  Index index_of_buf(uint8* elem) const
  {
    return (ll_idx(*(int32*)&(elem[4]), *(int32*)&(elem[8])));
  }
  
  int32 compare(const Iterator& it, uint8* buf) const
  {
    if (it->first.first < ll_idx(*(int32*)&(buf[4]), *(int32*)&(buf[8])))
      return RAW_DB_LESS;
    else if (it->first.first > ll_idx(*(int32*)&(buf[4]), *(int32*)&(buf[8])))
      return RAW_DB_GREATER;
    if (it->first.second < *(int32*)&(buf[0]))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  bool to_buf(uint8* dest, const Iterator& it, uint16 block_id)
  {
    if (it->second.first)
    {
      *(uint32*)&(dest[0]) = it->second.second.id;
      *(int32*)&(dest[4]) = it->second.second.lat;
      *(int32*)&(dest[8]) = it->second.second.lon;
      block_ids.push_back(block_id);
    }
    return true;
  }
  
  uint8 keep_this_elem(uint8* elem) const
  {
    int32 ll_idx_(ll_idx(*(int32*)&(elem[4]), *(int32*)&(elem[8])));
    Iterator it(data.lower_bound(make_pair< int32, int32 >(ll_idx_, *(int32*)&(elem[0]))));
    while ((it != data.end()) && (it->first.second == *(int32*)&(elem[0])))
    {
      if ((!(it->second.first)) && (it->second.second.id == *(int32*)&(elem[0])))
        return 0;
      ++it;
    }
    return 1;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    *(uint32*)&(dest[0]) = i;
  }

  int32 id_of(const Iterator& it) const
  {
    return it->second.second.id;
  }
  
  uint32 id_of_buf(uint8* elem) const
  {
    return (*(int32*)&(elem[0]));
  }
  
  void set_first_new_block(uint16 block_id) { first_new_block_ = block_id; }
  uint16 first_new_block() const { return first_new_block_; }
  
private:
  multimap< pair< int32, int32 >, pair< bool, Node > > data;
  multimap< Index, uint16 > block_index_;
  uint16 first_new_block_;
  vector< uint16 > block_ids;
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

struct Tag_Id_MultiNode_Local_Reader : public Tag_Id_Node_Local
{
  Tag_Id_MultiNode_Local_Reader
      (map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > >& id_to_node,
       const set< Node >& nodes, const set< uint32 >& idxs)
  : id_to_node_(id_to_node), nodes_(nodes), idxs_(idxs) {}
  
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
      set< Node >::const_iterator it
	  (nodes_.find(Node(*(uint32*)&(buf[6 + 4*j]), 0, 0)));
      if (it != nodes_.end())
	id_to_node_[make_pair< uint32, uint32 >
	    (ll_idx(it->lat, it->lon) & 0xffffff00, *(uint32*)&(buf[0]))].first.insert(it->id);
    }
  }
  
  private:
    map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > >& id_to_node_;
    const set< Node >& nodes_;
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
  uint32 size_of_part(const vector< uint32* >::const_iterator& it) const { return size_of(it); }
  
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
  
  bool to_buf(uint8* dest, const vector< uint32* >::const_iterator& it) const
  {
    ((uint32*)dest)[0] = (*it)[0];
    dest[4] = (*it)[1];
    dest[5] = block_of_id_[**it];
    memcpy(&(dest[6]), &((*it)[2]), (*it)[1]*sizeof(uint32));
    return true;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    ((uint32*)dest)[0] = i;
  }

  private:
    multimap< uint32, uint16 > block_index_;
    uint32* block_of_id_;
};

struct Tag_Id_Node_Local_Updater : public Tag_Id_Node_Local
{
  Tag_Id_Node_Local_Updater
      (const map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > >& local_ids,
       map< uint32, set< uint32 > >& moved_ids, const vector< uint32 >& local_id_idxs,
       const vector< uint32 >& spatial_boundaries)
  : local_ids_(local_ids), patched_local_ids_(), moved_ids_(moved_ids), block_index_(),
    local_id_idxs_(local_id_idxs), spatial_boundaries_(spatial_boundaries), remaining_size(0) {}
  
  const multimap< Index, uint16 >& block_index() const { return block_index_; }
  multimap< Index, uint16 >& block_index() { return block_index_; }
  
  typedef map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > >::const_iterator Iterator;
  Iterator elem_begin() { return local_ids_.begin(); }
  Iterator elem_end() { return local_ids_.end(); }
  
  uint32 size_of(const Iterator& it) const
  {
    if ((!(it->second.second.empty())) && (*(it->second.second.begin()) == 0))
      return 0;
    map< pair< uint32, uint32 >, set< uint32 > >::const_iterator pit(patched_local_ids_.find(it->first));
    int32 size_of_(it->second.second.size());
    if (pit != patched_local_ids_.end())
      size_of_ = pit->second.size();
    if (size_of_ >= 1)
      return (((size_of_ - 1) / 255 + 1)*6 + size_of_*4);
    else
      return 6;
  }
  
  uint32 size_of_part(const Iterator& it) const
  {
    int32 size_of_(remaining_size);
    if (remaining_size == 0)
    {
      if ((!(it->second.second.empty())) && (*(it->second.second.begin()) == 0))
        return 0;
      
      map< pair< uint32, uint32 >, set< uint32 > >::const_iterator pit(patched_local_ids_.find(it->first));
      if (pit == patched_local_ids_.end())
        size_of_ = it->second.second.size();
      else
        size_of_ = pit->second.size();
    }
    if (size_of_ > 255)
      size_of_ = 255;
    return (6 + size_of_*4);
  }
  
  Index index_of(const Iterator& it) const
  {
    return (it->first.first);
  }
  
  Index index_of_buf(uint8* elem) const
  {
    return ((local_id_idxs_[*(uint32*)&(elem[0])] & 0xffffff00));
  }
  
  int32 compare(const Iterator& it, uint8* buf) const
  {
    if (it->first.first < (local_id_idxs_[*(uint32*)&(buf[0])] & 0xffffff00))
      return RAW_DB_LESS;
    else if (it->first.first > (local_id_idxs_[*(uint32*)&(buf[0])] & 0xffffff00))
      return RAW_DB_GREATER;
    if (it->first.second < *((uint32*)buf))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  bool to_buf(uint8* dest, const Iterator& it, uint16 block_id)
  {
    if ((!(it->second.second.empty())) && (*(it->second.second.begin()) == 0))
      return true;
    
    static set< uint32 >::const_iterator nit;
    uint spatial_part(0);
    while (spatial_boundaries_[spatial_part] < it->first.first)
      ++spatial_part;
    ++spatial_part;
    ((uint32*)dest)[0] = it->first.second;
    dest[5] = spatial_part;
    map< pair< uint32, uint32 >, set< uint32 > >::const_iterator
	pit(patched_local_ids_.find(it->first));
    if (pit == patched_local_ids_.end())
    {
      uint pos(6);
      if (!remaining_size)
      {
        remaining_size = it->second.second.size();
        nit = it->second.second.begin();
      }
      uint upper_limit(remaining_size);
      if (upper_limit > 255)
        upper_limit = 255;
      uint i(0);
      while (i < upper_limit)
      {
        *(uint32*)&(dest[pos]) = *nit;
        ++nit;
        pos += 4;
        ++i;
      }
      remaining_size -= upper_limit;
      dest[4] = upper_limit;
      
      return (remaining_size == 0);
    }
    uint pos(6);
    if (!remaining_size)
    {
      remaining_size = pit->second.size();
      nit = pit->second.begin();
    }
    uint upper_limit(remaining_size);
    if (upper_limit > 255)
      upper_limit = 255;
    uint i(0);
    while (i < upper_limit)
    {
      *(uint32*)&(dest[pos]) = *nit;
      ++nit;
      pos += 4;
      ++i;
    }
    remaining_size -= upper_limit;
    dest[4] = upper_limit;
    
    return (remaining_size == 0);
  }
  
  uint8 keep_this_elem(uint8* elem)
  {
    map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > >::const_iterator
	it(local_ids_.find(make_pair< uint32, uint32 >
	(local_id_idxs_[*(uint32*)&(elem[0])] & 0xffffff00, *(uint32*)&(elem[0]))));
    if (it == local_ids_.end())
      return 1;
    if ((!(it->second.second.empty())) && (*(it->second.second.begin()) == 0))
    {
      map< uint32, set< uint32 > >::iterator mit(moved_ids_.insert(make_pair< uint32, set< uint32 > >
	  (*(uint32*)&(elem[0]), set< uint32 >())).first);
      for (uint i(0); i < elem[4]; ++i)
      {
	if (it->second.first.find(*(uint32*)&(elem[6+4*i])) == it->second.first.end())
	  mit->second.insert(*(uint32*)&(elem[6+4*i]));
      }
      return 0;
    }
    map< pair< uint32, uint32 >, set< uint32 > >::iterator
	pit(patched_local_ids_.insert(make_pair< pair< uint32, uint32 >, set< uint32 > >
	(make_pair< uint32, uint32 >
	    (local_id_idxs_[*(uint32*)&(elem[0])] & 0xffffff00, *(uint32*)&(elem[0])), set< uint32 >()))
	.first);
    for (uint i(0); i < elem[4]; ++i)
    {
      if (it->second.first.find(*(uint32*)&(elem[6+4*i])) == it->second.first.end())
	pit->second.insert(*(uint32*)&(elem[6+4*i]));
    }
    return 1;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    *(uint32*)&(dest[0]) = i;
  }

  void set_first_new_block(uint16 block_id) {}
  
  private:
    const map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > >& local_ids_;
    map< pair< uint32, uint32 >, set< uint32 > > patched_local_ids_;
    map< uint32, set< uint32 > >& moved_ids_;
    multimap< Index, uint16 > block_index_;
    const vector< uint32 >& local_id_idxs_;
    const vector< uint32 >& spatial_boundaries_;
    uint remaining_size;
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
    return (elem[4]*4 + 5);
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
    return ((*it)[1]*4 + 5);
  }
  uint32 size_of_part(const vector< uint32* >::const_iterator& it) const { return size_of(it); }
  
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
  
  bool to_buf(uint8* dest, const vector< uint32* >::const_iterator& it) const
  {
    ((uint32*)dest)[0] = (*it)[0];
    dest[4] = (*it)[1];
    memcpy(&(dest[5]), &((*it)[2]), (*it)[1]*sizeof(uint32));
    return true;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    ((uint32*)dest)[0] = i;
  }
  
  private:
    multimap< uint32, uint16 > block_index_;
};

struct Tag_Id_Node_Global_Updater : public Tag_Id_Node_Global
{
  Tag_Id_Node_Global_Updater
      (const map< uint32, pair< set< uint32 >, set< uint32 > > >& ids_to_be_edited)
  : ids_to_be_edited_(ids_to_be_edited), patched_ids_nodes_(), block_index_(),
    remaining_size(0) {}
  
  const multimap< Index, uint16 >& block_index() const { return block_index_; }
  multimap< Index, uint16 >& block_index() { return block_index_; }
  
  typedef map< uint32, pair< set< uint32 >, set< uint32 > > >::const_iterator Iterator;
  Iterator elem_begin() { return ids_to_be_edited_.begin(); }
  Iterator elem_end() { return ids_to_be_edited_.end(); }
  
  uint32 size_of(const Iterator& it) const
  {
    map< uint32, set< uint32 > >::const_iterator pit(patched_ids_nodes_.find(it->first));
    int32 size_of_(it->second.first.size());
    if (pit != patched_ids_nodes_.end())
      size_of_ = pit->second.size();
    return (((size_of_ - 1) / 255 + 1)*5 + size_of_*4);
  }
  
  uint32 size_of_part(const Iterator& it) const
  {
    int32 size_of_(remaining_size);
    if (remaining_size == 0)
    {
      map< uint32, set< uint32 > >::const_iterator pit(patched_ids_nodes_.find(it->first));
      if (pit == patched_ids_nodes_.end())
        size_of_ = it->second.first.size();
      else
        size_of_ = pit->second.size();
    }
    if (size_of_ > 255)
      size_of_ = 255;
    return (5 + size_of_*4);
  }
  
  Index index_of(const Iterator& it) const
  {
    return (it->first);
  }
  
  Index index_of_buf(uint8* elem) const
  {
    return (*(uint32*)&(elem[0]));
  }
  
  int32 compare(const Iterator& it, uint8* buf) const
  {
    if (it->first < *(uint32*)&(buf[0]))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  bool to_buf(uint8* dest, const Iterator& it, uint16 block_id)
  {
    static set< uint32 >::const_iterator nit;
    *(uint32*)&(dest[0]) = it->first;
    map< uint32, set< uint32 > >::const_iterator pit(patched_ids_nodes_.find(it->first));
    if (pit == patched_ids_nodes_.end())
    {
      uint pos(5);
      if (!remaining_size)
      {
        remaining_size = it->second.first.size();
        nit = it->second.first.begin();
      }
      uint upper_limit(remaining_size);
      if (upper_limit > 255)
        upper_limit = 255;
      uint i(0);
      while (i < upper_limit)
      {
        *(uint32*)&(dest[pos]) = *nit;
        ++nit;
        pos += 4;
        ++i;
      }
      remaining_size -= upper_limit;
      *(uint8*)&(dest[4]) = upper_limit;
      
      return (remaining_size == 0);
    }
    uint pos(5);
    if (!remaining_size)
    {
      remaining_size = pit->second.size();
      nit = pit->second.begin();
    }
    uint upper_limit(remaining_size);
    if (upper_limit > 255)
      upper_limit = 255;
    uint i(0);
    while (i < upper_limit)
    {
      *(uint32*)&(dest[pos]) = *nit;
      ++nit;
      pos += 4;
      ++i;
    }
    remaining_size -= upper_limit;
    *(uint8*)&(dest[4]) = upper_limit;
    
    return (remaining_size == 0);
  }
  
  uint8 keep_this_elem(uint8* elem)
  {
    map< uint32, pair< set< uint32 >, set< uint32 > > >::const_iterator
      it(ids_to_be_edited_.find(*((uint32*)&(elem[0]))));
    if (it == ids_to_be_edited_.end())
      return 1;
    map< uint32, set< uint32 > >::iterator it_set(patched_ids_nodes_.find(it->first));
    if (it_set == patched_ids_nodes_.end())
      it_set = patched_ids_nodes_.insert(make_pair(it->first, it->second.second)).first;
    for (uint16 i(0); i < *((uint8*)&(elem[4])); ++i)
    {
      if (it->second.first.find(*(uint32*)&(elem[4*i + 5])) == it->second.first.end())
        it_set->second.insert(*(uint32*)&(elem[4*i + 5]));
    }
    return 0;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    *(uint32*)&(dest[0]) = i;
  }

  void set_first_new_block(uint16 block_id) {}
  
private:
  const map< uint32, pair< set< uint32 >, set< uint32 > > >& ids_to_be_edited_;
  map< uint32, set< uint32 > > patched_ids_nodes_;
  multimap< Index, uint16 > block_index_;
  uint remaining_size;
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
    return ((*((uint16*)&(elem[8])))*4 + 10);
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
    map< uint32, set< uint32 > >::iterator it(node_to_id_.find(*(uint32*)buf));
    if (it != node_to_id_.end())
    {
      for (uint32 j(0); j < *(uint16*)&(buf[8]); ++j)
	it->second.insert(*(uint32*)&(buf[10 + 4*j]));
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
    return (ids_of_node[read_order[it.i]].size()*4 + 10);
  }
  uint32 size_of_part(const Tag_Node_Id_Iterator& it) const { return size_of(it); }
  
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
  
  bool to_buf(uint8* dest, const Tag_Node_Id_Iterator& it) const
  {
    *((uint32*)&(dest[0])) = read_order[it.i] + offset;
    *((uint32*)&(dest[4])) = ll_idx_[read_order[it.i]];
    *((uint16*)&(dest[8])) = ids_of_node[read_order[it.i]].size();
    for (uint32 i(0); i < ids_of_node[read_order[it.i]].size(); ++i)
      *((uint32*)&(dest[4*i + 10])) = ids_of_node[read_order[it.i]][i];
    return true;
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

struct Tag_Node_Id_Updater : public Tag_Node_Id
{
  static const uint DELETE = 1;
  static const uint INSERT = 2;
  static const uint UPDATE = 3;
  
  Tag_Node_Id_Updater
      (const map< pair< uint32, uint32 >, pair< set< uint32 >, uint > >& nodes_to_be_edited,
       map< uint32, set< uint32 > >& deleted_nodes_ids)
  : nodes_to_be_edited_(nodes_to_be_edited), deleted_nodes_ids_(deleted_nodes_ids),
    patched_nodes_ids_(), block_index_() {}
  
  const multimap< Index, uint16 >& block_index() const { return block_index_; }
  multimap< Index, uint16 >& block_index() { return block_index_; }
  
  typedef map< pair< uint32, uint32 >, pair< set< uint32 >, uint > >::const_iterator Iterator;
  Iterator elem_begin() { return nodes_to_be_edited_.begin(); }
  Iterator elem_end() { return nodes_to_be_edited_.end(); }
  
  uint32 size_of(const Iterator& it) const
  {
    if (it->second.second == DELETE)
      return 0;
    map< uint32, set< uint32 > >::const_iterator pit(patched_nodes_ids_.find(it->first.second));
    if (pit == patched_nodes_ids_.end())
      return (10 + 4*(it->second.first.size()));
    return (10 + 4*(pit->second.size()));
  }
  
  uint32 size_of_part(const Iterator& it) const { return size_of(it); }
  
  Index index_of(const Iterator& it) const
  {
    return (it->first.first);
  }
  
  Index index_of_buf(uint8* elem) const
  {
    return (*(uint32*)&(elem[4]));
  }
  
  int32 compare(const Iterator& it, uint8* buf) const
  {
    if (it->first.first < *((uint32*)&(buf[4])))
      return RAW_DB_LESS;
    else if (it->first.first > *((uint32*)&(buf[4])))
      return RAW_DB_GREATER;
    if (it->first.second < *((uint32*)&(buf[0])))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  
  bool to_buf(uint8* dest, const Iterator& it, uint16 block_id)
  {
    if (it->second.second == DELETE)
      return true;
    *((uint32*)&(dest[0])) = it->first.second;
    *((uint32*)&(dest[4])) = it->first.first;
    map< uint32, set< uint32 > >::const_iterator pit(patched_nodes_ids_.find(it->first.second));
    if (pit == patched_nodes_ids_.end())
    {
      *((uint16*)&(dest[8])) = it->second.first.size();
      uint i(0);
      for (set< uint32 >::const_iterator it2(it->second.first.begin());
           it2 != it->second.first.end(); ++it2)
      {
        *((uint32*)&(dest[4*i + 10])) = *it2;
        ++i;
      }
      return true;
    }
    *((uint16*)&(dest[8])) = pit->second.size();
    uint i(0);
    for (set< uint32 >::const_iterator it2(pit->second.begin());
         it2 != pit->second.end(); ++it2)
    {
      *((uint32*)&(dest[4*i + 10])) = *it2;
      *((uint8*)&(dest[4*i + 14])) = 0;
      ++i;
    }
    return true;
  }
  
  uint8 keep_this_elem(uint8* elem)
  {
    map< pair< uint32, uint32 >, pair< set< uint32 >, uint > >::const_iterator
      it(nodes_to_be_edited_.find(make_pair< uint32, uint32 >
	(*((uint32*)&(elem[4])), *((uint32*)&(elem[0])))));
    if (it == nodes_to_be_edited_.end())
      return 1;
    if ((it->second.second == DELETE) || (it->second.second == INSERT))
    {
      set< uint32 >& id_set(deleted_nodes_ids_[it->first.second]);
      for (uint16 i(0); i < *((uint16*)&(elem[8])); ++i)
	id_set.insert(*(uint32*)&(elem[4*i + 10]));
      return 0;
    }
    set< uint32 >& id_set(patched_nodes_ids_[it->first.second]);
    id_set = it->second.first;
    for (uint16 i(0); i < *((uint16*)&(elem[8])); ++i)
      id_set.insert(*(uint32*)&(elem[4*i + 10]));
    return 0;
  }
  
  void index_to_buf(uint8* dest, const uint32& i) const
  {
    *(uint32*)&(dest[0]) = i;
  }

  void set_first_new_block(uint16 block_id) {}
  
private:
  const map< pair< uint32, uint32 >, pair< set< uint32 >, uint > >& nodes_to_be_edited_;
  map< uint32, set< uint32 > >& deleted_nodes_ids_;
  map< uint32, set< uint32 > > patched_nodes_ids_;
  multimap< Index, uint16 > block_index_;
};

//-----------------------------------------------------------------------------

struct Way_Storage
{
  typedef Way_ Basetype;
  typedef uint32 Id;
  static string base_file_name() { return ((string)DATADIR + WAY_FILE_BASE); }
  static uint32 blocksize() { return (WAY_BLOCKSIZE); }
  
  typedef uint32 Index;
  static uint size_of_Index() { return 4; }
  static void index_to_buf(uint8* buf, const Index& i)
  {
    *(uint32*)&(buf[0]) = i;
  }
  
  typedef pair< Index, Id > Head;
  static uint size_of_Head() { return 8; }
  static void head_from_buf(uint8 const* buf, Head& h)
  {
    h.first = *(uint32*)&(buf[0]);
    h.second = *(uint32*)&(buf[4]);
  }
  static void head_to_buf(uint8* buf, const Head& h)
  {
    *(uint32*)&(buf[0]) = h.first;
    *(uint32*)&(buf[4]) = h.second;
  }
  static Index index_of(const Head& h) { return h.first; }
  static Index index_of_buf(uint8 const* buf) { return *(uint32*)&(buf[0]); }
  static int32 compare(const Head& h, uint8 const* buf)
  {
    if (h.first < *(uint32*)&(buf[0]))
      return RAW_DB_LESS;
    if (h.first > *(uint32*)&(buf[0]))
      return RAW_DB_GREATER;
    if (h.second < *(uint32*)&(buf[4]))
      return RAW_DB_LESS;
    else
      return RAW_DB_GREATER;
  }
  static Id id_of(const Head& h) { return h.second; }
  static Id id_of_buf(uint8* buf) { return *(uint32*)&(buf[4]); }
  
  typedef uint32 Data;
  static uint size_of_Data() { return 4; }
  static void data_from_buf(uint8 const* buf, Data& d) { d = *(uint32*)&(buf[0]); }
  static void data_to_buf(uint8* buf, const Data& d) { *(uint32*)&(buf[0]) = d; }
};

//-----------------------------------------------------------------------------

template < typename Storage >
struct Indexed_Ordered_Id_To_Many_Base
{
  uint32 blocksize() const { return Storage::blocksize(); }
  const string data_file() const { return Storage::base_file_name() + ".dat"; }
  const string index_file() const { return Storage::base_file_name() + ".idx"; }
  const string id_idx_file() const { return Storage::base_file_name() + ".i.idx"; }
  
  typedef typename Storage::Index Index;
  
  uint32 size_of_Index() const { return Storage::size_of_Index(); }
  static uint size_of_buf(uint8 const* buf)
  {
    return Storage::size_of_Head() + 1
      + buf[Storage::size_of_Head()]*Storage::size_of_Data();
  }
};

// assertions:
//   Container c supports
// Container::const_iterator c.begin(), Container::const_iterator c.end(),
// uint32 c.size() and Storage::Basetype operator*(Container::const_iterator).
//   Storage::Basetype has fields: Head head and vector< Data > data
template < typename Storage, typename Container >
struct Indexed_Ordered_Id_To_Many_Writer : Indexed_Ordered_Id_To_Many_Base< Storage >
{
  Indexed_Ordered_Id_To_Many_Writer(const Container& container)
    : container_(container), block_index_(), remaining_size(0), dit()
  {}
  
  const multimap< typename Storage::Index, uint16 >& block_index() const { return block_index_; }
  multimap< typename Storage::Index, uint16 >& block_index() { return block_index_; }
  
  typedef typename Container::const_iterator Iterator;
  Iterator begin() { return container_.begin(); }
  Iterator end() { return container_.end(); }
  
  typename Storage::Index index_of(const Iterator& it) const { return Storage::index_of(it->head); }
  typename Storage::Index index_of_buf(uint8* buf) const { return Storage::index_of_buf(buf); }
  
  uint32 size_of(const Iterator& it) const
  {
    int32 size_of_(it->data.size());
    return (((size_of_ - 1) / 255 + 1)*(Storage::size_of_Head() + 1)
            + size_of_*Storage::size_of_Data());
  }
  
  uint32 size_of_part(const Iterator& it) const
  {
    int32 size_of_(remaining_size);
    if (remaining_size == 0)
      size_of_ = it->data.size();
    if (size_of_ > 255)
      size_of_ = 255;
    return (Storage::size_of_Head() + 1 + size_of_*Storage::size_of_Data());
  }
  
  int32 compare(const Iterator& it, uint8 const* buf) const { return Storage::compare(it->head, buf); }
  
  bool to_buf(uint8* buf, const Iterator& it)
  {
    Storage::head_to_buf(&(buf[0]), it->head);
    uint pos(Storage::size_of_Head() + 1);
    if (remaining_size == 0)
    {
      remaining_size = it->data.size();
      dit = it->data.begin();
    }
    uint upper_limit(remaining_size);
    if (upper_limit > 255)
      upper_limit = 255;
    uint i(0);
    while (i < upper_limit)
    {
      Storage::data_to_buf(&(buf[pos]), *dit);
      ++dit;
      pos += Storage::size_of_Data();
      ++i;
    }
    remaining_size -= upper_limit;
    *(uint8*)&(buf[Storage::size_of_Head()]) = upper_limit;
    
    return (remaining_size == 0);
  }
  
  typename Storage::Id id_of_buf(uint8* buf) const { return Storage::id_of_buf(buf); }
  void index_to_buf(uint8* buf, const typename Storage::Index& i) const { Storage::index_to_buf(buf, i); }
  
private:
  const Container& container_;
  multimap< typename Storage::Index, uint16 > block_index_;
  uint32 remaining_size;
  typename vector< typename Storage::Data >::const_iterator dit;
};

template < typename Basetype_, typename Container_ >
struct Parallel_Container_Iterator
{
  Parallel_Container_Iterator
    (const Container_& cont1_, const Container_& cont2_,
     const typename Container_::const_iterator& it1_, const typename Container_::const_iterator& it2_)
    : cont1(cont1_), cont2(cont2_), it1(it1_), it2(it2_), current_it(0)
  {
    if ((it2 == cont2_.end()) || ((it1 != cont1_.end()) && (*it1 < *it2)))
      current_it = 1;
    else
      current_it = 2;
  }
  
  typedef Basetype_ Basetype;
  typedef Container_ Container;
  
  const Container& cont1;
  const Container& cont2;
  typename Container::const_iterator it1;
  typename Container::const_iterator it2;
  uint current_it;
};

template < typename Parallel_Container_Iterator >
inline typename Parallel_Container_Iterator::Basetype operator*(const Parallel_Container_Iterator& it)
{
  if (it.current_it == 1)
    return *(it.it1);
  else
    return *(it.it2);
}

template < typename Parallel_Container_Iterator >
inline bool operator==(const Parallel_Container_Iterator& a, const Parallel_Container_Iterator& b)
{
  return ((a.it1 == b.it1) && (a.it2 == b.it2));
}

template < typename Parallel_Container_Iterator >
inline bool operator!=(const Parallel_Container_Iterator& a, const Parallel_Container_Iterator& b)
{
  return (!((a.it1 == b.it1) && (a.it2 == b.it2)));
}

template < typename Parallel_Container_Iterator >
inline Parallel_Container_Iterator& operator++(Parallel_Container_Iterator& it)
{
  if (it.current_it == 1)
    ++it.it1;
  else
    ++it.it2;
  if ((it.it2 == it.cont2.end()) || ((it.it1 != it.cont1.end()) && (*(it.it1) < *(it.it2))))
    it.current_it = 1;
  else
    it.current_it = 2;
  return it;
}

// assertions:
//   Container c supports
// Container::const_iterator c.begin(), Container::const_iterator c.end(),
// Container::const_iterator c.find(Storage::Basetype), uint32 c.size() and
// Storage::Basetype operator*(Container::const_iterator).
//   Storage::Basetype has fields: Head head and vector< Data > data
template < typename Storage, typename Container >
struct Indexed_Ordered_Id_To_Many_Updater : Indexed_Ordered_Id_To_Many_Base< Storage >
{
  Indexed_Ordered_Id_To_Many_Updater(const Container& to_delete, const Container& to_insert)
    : to_delete_(to_delete), to_insert_(to_insert), block_index_(), remaining_size(0), dit(), first_new_block_(0), block_ids()
  {}
  
  const multimap< typename Storage::Index, uint16 >& block_index() const { return block_index_; }
  multimap< typename Storage::Index, uint16 >& block_index() { return block_index_; }
  
  typedef Parallel_Container_Iterator< typename Storage::Basetype, Container > Iterator;
  Iterator elem_begin() { return Iterator(to_delete_, to_insert_, to_delete_.begin(), to_insert_.begin()); }
  Iterator elem_end() { return Iterator(to_delete_, to_insert_, to_delete_.end(), to_insert_.end()); }
  
  typename Storage::Index index_of(const Iterator& it) const { return Storage::index_of((*it).head); }
  typename Storage::Index index_of_buf(uint8* buf) const { return Storage::index_of_buf(buf); }
  
  typedef vector< uint16 >::const_iterator Id_Block_Iterator;
  Id_Block_Iterator block_of_elem_begin() { return block_ids.begin(); }
  
  uint32 size_of(const Iterator& it) const
  {
    if (it.current_it == 1)
      return 0;
    
    int32 size_of_((*it).data.size());
    return (((size_of_ - 1) / 255 + 1)*(Storage::size_of_Head() + 1)
            + size_of_*Storage::size_of_Data());
  }
  
  uint32 size_of_part(const Iterator& it) const
  {
    if (it.current_it == 1)
      return 0;
    
    int32 size_of_(remaining_size);
    if (remaining_size == 0)
      size_of_ = (*it).data.size();
    if (size_of_ > 255)
      size_of_ = 255;
    return (Storage::size_of_Head() + 1 + size_of_*Storage::size_of_Data());
  }
  
  int32 compare(const Iterator& it, uint8 const* buf) const { return Storage::compare((*it).head, buf); }

  bool to_buf(uint8* buf, const Iterator& it, uint16 block_id)
  {
    if (it.current_it == 1)
      return true;
    
    Storage::head_to_buf(&(buf[0]), (*it).head);
    uint pos(Storage::size_of_Head() + 1);
    if (remaining_size == 0)
    {
      remaining_size = (*it).data.size();
      dit = (*it).data.begin();
    }
    uint upper_limit(remaining_size);
    if (upper_limit > 255)
      upper_limit = 255;
    uint i(0);
    while (i < upper_limit)
    {
      Storage::data_to_buf(&(buf[pos]), *dit);
      ++dit;
      pos += Storage::size_of_Data();
      ++i;
    }
    remaining_size -= upper_limit;
    *(uint8*)&(buf[Storage::size_of_Head()]) = upper_limit;
    
    block_ids.push_back(block_id);
    return (remaining_size == 0);
  }
  
  uint8 keep_this_elem(uint8* buf)
  {
    typename Storage::Head h;
    Storage::head_from_buf(&(buf[0]), h);
    typename Container::const_iterator it(to_delete_.find(typename Storage::Basetype(h)));
    if (it == to_delete_.end())
      return 1;
    else
      return 0;
  }
  
  void set_first_new_block(uint16 block_id) { first_new_block_ = block_id; }
  uint16 first_new_block() const { return first_new_block_; }
  
  typename Storage::Id id_of(const Iterator& it) { return Storage::id_of((*it).head); }
  typename Storage::Id id_of_buf(uint8* buf) const { return Storage::id_of_buf(buf); }
  void index_to_buf(uint8* buf, const typename Storage::Index& i) const { Storage::index_to_buf(buf, i); }
  
private:
  const Container& to_delete_;
  const Container& to_insert_;
  multimap< typename Storage::Index, uint16 > block_index_;
  uint32 remaining_size;
  typename vector< typename Storage::Data >::const_iterator dit;
  uint16 first_new_block_;
  vector< uint16 > block_ids;
};

#endif
