#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "expat_justparse_interface.h"
#include "file_types.h"
#include "node_strings_file_io.h"
#include "raw_file_db.h"
#include "script_datatypes.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <mysql.h>

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

void postprocess_nodes(Node_Id_Node_Writer& node_writer)
{
  make_block_index< Node_Id_Node_Writer >(node_writer);
  make_id_index< Node_Id_Node_Writer >(node_writer);
}

//-----------------------------------------------------------------------------

void localise_and_flush_ways
    (vector< Way_ >& ways,
     Indexed_Ordered_Id_To_Many_Writer< Way_Storage, vector< Way_ > >& writer)
{
  //query used nodes
  set< Node > used_nodes;
  set< int32 > used_nodes_ids;
  for (vector< Way_ >::const_iterator it(ways.begin()); it != ways.end(); ++it)
  {
    for (vector< Way_::Data >::const_iterator it2((*it).data.begin());
	 it2 != (*it).data.end(); ++it2)
      used_nodes_ids.insert(*it2);
  }
  Node_Id_Node_By_Id_Reader nodes_reader(used_nodes_ids, used_nodes);
  select_by_id< Node_Id_Node_By_Id_Reader >(nodes_reader);
  used_nodes_ids.clear();
  
  //calculate for each ways its index
  for (vector< Way_ >::iterator it(ways.begin()); it != ways.end(); ++it)
  {
    Way_::Index bitmask(0), position(0);
    for (vector< Way_::Data >::const_iterator it2((*it).data.begin());
	 it2 != (*it).data.end(); ++it2)
    {
      const set< Node >::const_iterator node_it(used_nodes.find(Node(*it2, 0, 0)));
      if (node_it == used_nodes.end())
      {
	//this node is referenced but does not exist
	//cerr<<"E "<<*it2<<'\n';
	continue;
      }
      if (position == 0)
	position = ll_idx(node_it->lat, node_it->lon);
      else
	bitmask |= (position ^ ll_idx(node_it->lat, node_it->lon));
    }
    
    while (bitmask)
    {
      bitmask = bitmask>>8;
      position = (position>>8) | 0x88000000;
    }
    (*it).head.first = position;
  }
  
  //write ways to file
  sort(ways.begin(), ways.end());
  flush_data(writer, ways.begin(), ways.end());
}

//-----------------------------------------------------------------------------

uint32 max_node_id(0);
uint32 max_way_id(0);

multimap< int, Node > nodes;
Node_Id_Node_Writer node_writer;
map< NodeKeyValue, NodeCollection > node_tags;
int current_type(0);
int32 current_id;
int32 current_ll_idx;
set< string > allowed_node_tags;
uint tag_count(0);
unsigned int structure_count(0);
int state(0);
int ways_fd;
uint32* way_buf;
uint32 way_buf_pos(0);
const int NODE = 1;
const int WAY = 2;
const int RELATION = 3;
const unsigned int FLUSH_INTERVAL = 32*1024*1024;
const unsigned int WAYND_FLUSH_INTERVAL = 32*1024*1024;
const unsigned int DOT_INTERVAL = 512*1024;
uint current_run(0);
vector< uint32 > split_idx;
uint32* block_of_id;

vector< Way_ > ways_;
Way_ current_way(0, 0);
Indexed_Ordered_Id_To_Many_Writer< Way_Storage, vector< Way_ > > ways_writer(ways_);

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
  {
    if (current_type == NODE)
    {
      string key(""), value("");
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "k"))
	  key = attr[i+1];
	if (!strcmp(attr[i], "v"))
	  value = attr[i+1];
      }
      NodeCollection& nc(node_tags[NodeKeyValue(key, value)]);
      nc.insert(current_id, current_ll_idx);
      if (++tag_count >= FLUSH_NODE_TAGS_INTERVAL)
      {
	flush_node_tags(current_run, node_tags);
	node_tags.clear();
	tag_count = 0;
      }
    }
  }
  else if (!strcmp(el, "nd"))
  {
    unsigned int ref(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "ref"))
	ref = atoi(attr[i+1]);
    }
    current_way.data.push_back(ref);
  }
  else if (!strcmp(el, "node"))
  {
    unsigned int id(0);
    int lat(100*10000000), lon(200*10000000);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (!strcmp(attr[i], "lat"))
	lat = (int)in_lat_lon(attr[i+1]);
      if (!strcmp(attr[i], "lon"))
	lon = (int)in_lat_lon(attr[i+1]);
    }
    nodes.insert(make_pair< int, Node >
	(ll_idx(lat, lon), Node(id, lat, lon)));
    
    current_type = NODE;
    current_id = id;
    current_ll_idx = ll_idx(lat, lon);
    
    if (id > max_node_id)
      max_node_id = id;
  }
  else if (!strcmp(el, "way"))
  {
    if (state == NODE)
    {
      flush_data< Node_Id_Node_Writer >
	  (node_writer, nodes.begin(), nodes.end());
      nodes.clear();
      
      flush_node_tags(current_run, node_tags);
      node_tags.clear();
      
      node_tag_statistics(current_run, split_idx);
      node_tag_split_and_index(current_run, split_idx, block_of_id);
      node_tag_create_id_node_idx(block_of_id);
      node_tag_create_node_id_idx(block_of_id, max_node_id);
      node_tag_id_statistics();
      
      postprocess_nodes(node_writer);
      
      state = WAY;
      structure_count = 0;
    }
    
    Way_::Id id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
    current_way.head.second = id;
    
    if (id > max_way_id)
      max_way_id = id;
  }
  else if (!strcmp(el, "relation"))
  {
    if (state == NODE)
    {
      flush_data< Node_Id_Node_Writer >
	  (node_writer, nodes.begin(), nodes.end());
      nodes.clear();
      
      node_tag_statistics(current_run, split_idx);
      node_tag_split_and_index(current_run, split_idx, block_of_id);
      node_tag_create_id_node_idx(block_of_id);
      node_tag_create_node_id_idx(block_of_id, max_node_id);
      node_tag_id_statistics();
      
      postprocess_nodes(node_writer);
    }
    else if (state == WAY)
    {
      localise_and_flush_ways(ways_, ways_writer);
      ways_.clear();
      make_block_index(ways_writer);
      make_id_index(ways_writer);
    }
    
    state = RELATION;
  }
}

void end(const char *el)
{
  if (!strcmp(el, "nd"))
  {
    ++structure_count;
    if (structure_count % DOT_INTERVAL == 0)
      cerr<<'.';
  }
  else if (!strcmp(el, "node"))
  {
    current_type = 0;
    ++structure_count;
    if (structure_count % DOT_INTERVAL == 0)
      cerr<<'.';
  
    if (structure_count >= FLUSH_INTERVAL)
    {
      if (state == NODE)
      {
	flush_data< Node_Id_Node_Writer >
	    (node_writer, nodes.begin(), nodes.end());
	nodes.clear();
      }
    
      structure_count = 0;
    }
  }
  else if (!strcmp(el, "way"))
  {
    ways_.push_back(current_way);
    current_way.data.clear();
    
    if (structure_count > WAYND_FLUSH_INTERVAL)
    {
      localise_and_flush_ways(ways_, ways_writer);
      ways_.clear();
      structure_count = 0;
    }
  }
  else if (!strcmp(el, "relation"))
  {
  }
}

int main(int argc, char *argv[])
{
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  state = NODE;
  
  try
  {
    //reading the main document
    parse(stdin, start, end);
    
    if (state == NODE)
    {
      flush_data< Node_Id_Node_Writer >
	  (node_writer, nodes.begin(), nodes.end());
      nodes.clear();
      
      node_tag_statistics(current_run, split_idx);
      node_tag_split_and_index(current_run, split_idx, block_of_id);
      node_tag_create_id_node_idx(block_of_id);
      node_tag_create_node_id_idx(block_of_id, max_node_id);
      node_tag_id_statistics();
      
      postprocess_nodes(node_writer);
    }
    else if (state == WAY)
    {
      localise_and_flush_ways(ways_, ways_writer);
      ways_.clear();
      make_block_index(ways_writer);
      make_id_index(ways_writer);
    }
  }
  catch(File_Error e)
  {
    cerr<<"\nopen64: "<<e.error_number<<'\n';
  }
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
