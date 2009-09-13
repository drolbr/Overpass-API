#ifndef WAY_STRINGS_FILE_IO
#define WAY_STRINGS_FILE_IO

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

const static string WAY_TAG_ID_STATS("way_tag_id_stats.dat");

const unsigned int FLUSH_WAY_TAGS_INTERVAL = 32*1024*1024;
const uint32 WAY_TAG_SORT_BUFFER_SIZE = 128*1024*1024;

//-----------------------------------------------------------------------------

struct WayKeyValue
{
  WayKeyValue(string key_, string value_) : key(key_), value(value_) {}
  
  string key, value;
};

inline bool operator<(const WayKeyValue& kv_1, const WayKeyValue& kv_2)
{
  if (kv_1.key < kv_2.key)
    return true;
  else if (kv_1.key > kv_2.key)
    return false;
  return (kv_1.value < kv_2.value);
}

struct WayCollection
{
  WayCollection() : position(0), bitmask(0)
  {
    id = ++next_way_tag_id;
  }
  
  void insert(uint32 way_id, uint32 ll_idx)
  {
    if (ways.empty())
    {
      position = ll_idx;
      bitmask = 0;
    }
    ways.push_back(way_id);
    bitmask |= (position ^ ll_idx);
  }
  
  void merge(uint32 id_, uint32 ll_idx)
  {
    id = id_;
    bitmask |= (position ^ ll_idx);
  }
  
  uint32 position;
  uint32 bitmask;
  uint32 id;
  vector< uint32 > ways;
  static uint32 next_way_tag_id;
};

void flush_way_tags(uint& current_run, map< WayKeyValue, WayCollection >& way_tags, vector< Way_ >& ways);

void way_tag_statistics(uint& current_run, vector< uint32 >& split_idx);

void way_tag_split_and_index(uint& current_run, vector< uint32 >& split_idx, uint32*& block_of_id);

void way_tag_create_id_way_idx(uint32* block_of_id);

void way_tag_create_way_id_idx(uint32* block_of_id, uint32 max_way_id);

void way_tag_id_statistics();

void way_string_cache_reset();

void way_string_delete_insert(map< pair< string, string >, pair< uint32, uint32 >* >& new_tags_ids,
			       set< pair< uint32, uint32 > >& moved_local_ids,
	  vector< uint32 >& local_id_idx, vector< uint32 >& spatial_boundaries_);

void way_tag_id_statistics_remake();

void select_way_kv_to_ids
    (string key, string value, set< uint32 >& string_ids_global,
     set< uint32 >& string_ids_local, set< uint32 >& string_idxs_local);

void select_way_ids_to_kvs
    (const map< uint32, uint32 > ids_local,
     const set< uint32 > ids_global,
     map< uint32, pair< string, string > >& kvs);

struct Way_Less_By_Id : public binary_function< Way_, Way_, bool >
{
  bool operator()(const Way_& x, const Way_& y) const { return (x.head.second < y.head.second); }
};

#endif
