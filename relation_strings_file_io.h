#ifndef RELATION_STRINGS_FILE_IO
#define RELATION_STRINGS_FILE_IO

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

const static string RELATION_TAG_ID_STATS("relation_tag_id_stats.dat");

const unsigned int FLUSH_RELATION_TAGS_INTERVAL = 32*1024*1024;
const uint32 RELATION_TAG_SORT_BUFFER_SIZE = 128*1024*1024;

//-----------------------------------------------------------------------------

struct RelationKeyValue
{
  RelationKeyValue(string key_, string value_) : key(key_), value(value_) {}
  
  string key, value;
};

inline bool operator<(const RelationKeyValue& kv_1, const RelationKeyValue& kv_2)
{
  if (kv_1.key < kv_2.key)
    return true;
  else if (kv_1.key > kv_2.key)
    return false;
  return (kv_1.value < kv_2.value);
}

struct RelationCollection
{
  RelationCollection() : position(0), bitmask(0)
  {
    id = ++next_relation_tag_id;
  }
  
  void insert(uint32 relation_id)
  {
    relations.push_back(relation_id);
  }
  
  void merge(uint32 id_, uint32 ll_idx)
  {
    id = id_;
    bitmask |= (position ^ ll_idx);
  }
  
  uint32 position;
  uint32 bitmask;
  uint32 id;
  vector< uint32 > relations;
  static uint32 next_relation_tag_id;
};

void flush_relation_tags(uint& current_run, map< RelationKeyValue, RelationCollection >& relation_tags, vector< Relation_ >& relations);

void relation_tag_statistics(uint& current_run, vector< uint32 >& split_idx);

void relation_tag_split_and_index(uint& current_run, vector< uint32 >& split_idx, uint32*& block_of_id);

void relation_tag_create_id_relation_idx(uint32* block_of_id);

void relation_tag_create_relation_id_idx(uint32* block_of_id, uint32 max_relation_id);

void relation_tag_id_statistics();

void relation_string_delete_insert(map< pair< string, string >, pair< uint32, uint32 >* >& new_tags_ids,
			       set< pair< uint32, uint32 > >& moved_local_ids,
	  vector< uint32 >& local_id_idx, vector< uint32 >& spatial_boundaries_);

void relation_tag_id_statistics_remake();

void select_relation_kv_to_ids
    (string key, string value, set< uint32 >& string_ids_global,
     set< uint32 >& string_ids_local, set< uint32 >& string_idxs_local);

void select_relation_ids_to_kvs
    (const map< uint32, uint32 > ids_local,
     const set< uint32 > ids_global,
     map< uint32, pair< string, string > >& kvs);

struct Relation_Less_By_Id : public binary_function< Relation_, Relation_, bool >
{
  bool operator()(const Relation_& x, const Relation_& y) const { return (x.head < y.head); }
};

#endif
