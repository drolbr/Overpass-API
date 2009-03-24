#ifndef NODE_STRINGS_FILE_IO
#define NODE_STRINGS_FILE_IO

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "file_types.h"
#include "raw_file_db.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

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

static string NODE_TAG_ID_STATS = (string)DATADIR + "node_tag_id_stats.dat";

const unsigned int FLUSH_TAGS_INTERVAL = 32*1024*1024;
const uint32 TAG_SORT_BUFFER_SIZE = 128*1024*1024;

//-----------------------------------------------------------------------------

struct KeyValue
{
  KeyValue(string key_, string value_) : key(key_), value(value_) {}
  
  string key, value;
};

inline bool operator<(const KeyValue& kv_1, const KeyValue& kv_2)
{
  if (kv_1.key < kv_2.key)
    return true;
  else if (kv_1.key > kv_2.key)
    return false;
  return (kv_1.value < kv_2.value);
}

struct NodeCollection
{
  NodeCollection() : position(0), bitmask(0)
  {
    id = ++next_node_tag_id;
  }
  
  void insert(int32 node_id, int32 ll_idx)
  {
    if (nodes.empty())
    {
      position = ll_idx;
      bitmask = 0;
    }
    nodes.push_back(node_id);
    bitmask |= (position ^ ll_idx);
  }
  
  void merge(int32 id_, int32 ll_idx)
  {
    id = id_;
    bitmask |= (position ^ ll_idx);
  }
  
  int32 position;
  uint32 bitmask;
  uint32 id;
  vector< int32 > nodes;
  static uint32 next_node_tag_id;
};

void flush_node_tags(uint& current_run, map< KeyValue, NodeCollection >& node_tags);
		     
void node_tag_statistics(uint& current_run, vector< uint32 >& split_idx);

void node_tag_split_and_index(uint& current_run, vector< uint32 >& split_idx, uint32*& block_of_id);

void node_tag_create_id_node_idx(uint32* block_of_id);

void node_tag_create_node_id_idx(uint32* block_of_id, uint32 max_node_id);

void node_tag_id_statistics();

void node_string_delete_insert(map< pair< string, string >, pair< uint32, uint32 >* >& new_tags_ids,
			       set< pair< uint32, uint32 > >& moved_local_ids);

void select_kv_to_ids
    (string key, string value, set< uint32 >& string_ids_global,
     set< uint32 >& string_ids_local, set< uint32 >& string_idxs_local);

void select_ids_to_kvs
    (const map< uint32, uint32 > ids_local,
     const set< uint32 > ids_global,
     map< uint32, pair< string, string > >& kvs);

#endif
