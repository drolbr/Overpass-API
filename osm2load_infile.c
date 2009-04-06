/*****************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the license contained in the
 * COPYING file that comes with the expat distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "script_datatypes.h"
#include "expat_justparse_interface.h"

#include <mysql.h>

using namespace std;

//-----------------------------------------------------------------------------
//patch to save RAM

class Entry
{
  public:
    Entry() : value(0), next(0), id(0) {}
  
    char* value;
    Entry* next;
    int id;
};

class Value_Detect
{
  public:
    Value_Detect() : is_new(false), current_max(1)
    {
      entries.resize(4*4*4 * 4*4*4 * 4*4*4 * 4*4*4, 0);
    }
  
    int find(const char* val);
    
    bool is_new;
  private:
    int current_max;
    vector< Entry* > entries;
};

inline int strposcmp(const char* a, const char* b)
{
  int i(0);
  while ((a[i] != 0) && (b[i] != 0) && (a[i] == b[i]))
    ++i;
  if (a[i] == b[i])
    return -1;
  else
    return i;
}

int Value_Detect::find(const char* val)
{
  unsigned int length(strlen(val));
  
  if (length == 0)
  {
    is_new = false;
    return 1;
  }
  
  unsigned int i(0);
  unsigned int idx(0);
  while ((i < 12) && (i < length))
  {
    idx = 4*idx + (((unsigned char)val[i])%4);
    ++i;
  }
  while (i < 12)
  {
    idx = 4*idx + 3;
    ++i;
  }
  
  Entry** last_entry(&entries[idx]);
  Entry* entry(entries[idx]);
  while ((entry) && (((void**)entry)[0] == 0))
  {
    if (i < length)
    {
      unsigned int idx2(((unsigned char)val[i])%4 + 1);
      last_entry = (Entry**)(&(((void**)(entry))[idx2]));
      entry = *(Entry**)(&(((void**)(entry))[idx2]));
    }
    else
    {
      last_entry = (Entry**)(&(((void**)(entry))[4]));
      entry = *(Entry**)(&(((void**)(entry))[4]));
    }
    ++i;
  }
  unsigned int chain_count(0);
  while (entry)
  {
    if (!strcmp(val, entry->value))
    {
      is_new = false;
      return entry->id;
    }
    
    ++chain_count;
    last_entry = &(entry->next);
    entry = entry->next;
  }
  
  entry = (Entry*) malloc(sizeof(Entry));
  *last_entry = entry;
  entry->value = (char*) malloc(length+1);
  entry->next = 0;
  strcpy(entry->value, val);
  is_new = true;
  entry->id = ++current_max;
  
  if (chain_count < 16)
    return entry->id;
  
  unsigned int entry_id(entry->id);
  i = 12;
  last_entry = &(entries[idx]);
  entry = entries[idx];
  while ((entry) && (((void**)entry)[0] == 0))
  {
    if (i < length)
    {
      unsigned int idx2(((unsigned char)val[i])%4 + 1);
      last_entry = (Entry**)(&(((void**)(entry))[idx2]));
      entry = *(Entry**)(&(((void**)(entry))[idx2]));
    }
    else
    {
      last_entry = (Entry**)(&(((void**)(entry))[4]));
      entry = *(Entry**)(&(((void**)(entry))[4]));
    }
    ++i;
  }
  
  void** new_block = (void**) malloc(5 * sizeof(void*));
  new_block[0] = 0;
  Entry** new_ey[4];
  for (unsigned int j(1); j < 5 ; ++j)
  {
    new_ey[j-1] = (Entry**)(&(new_block[j]));
    new_block[j] = 0;
  }
  
  while (entry)
  {
    if (strlen(entry->value) < i)
      idx = 3;
    else
      idx = (unsigned char)(entry->value[i])%4;
    *(new_ey[idx]) = entry;
    new_ey[idx] = &(entry->next);
    entry = entry->next;
  }
  for (unsigned int j(0); j < 4 ; ++j)
    *(new_ey[j]) = 0;
  *last_entry = (Entry*) new_block;
  
  return entry_id;
}

//-----------------------------------------------------------------------------

const int NODE = 1;
const int WAY = 2;
const int RELATION = 3;
int tag_type(0);
unsigned int current_id(0);
// patch cope with server power limits by ignoring uncommon node tags
bool lowmem(false);
bool no_tag_limit(true);
set< string > allowed_node_tags;
bool split_tables(false);
unsigned char* node_cache;
unsigned char current_block;
ostringstream way_tags_temp;
const unsigned int NR_SUBTABLES = 128;
const unsigned int MULTIPLE_BLOCKS = NR_SUBTABLES + 1;

map< string, unsigned int > member_roles;
map< string, unsigned int > keys;
map< string, unsigned int > values;

const unsigned int FLUSH_INTERVAL = 100000;
unsigned int structure_count(0);

unsigned int way_member_count(0);

/*ofstream nodes_out("/tmp/db_area_nodes.tsv");
ofstream* nodes_sub_out;*/
/*ofstream node_tags_out("/tmp/db_area_node_tags.tsv");
ofstream* node_tags_sub_out;*/
/*ofstream ways_out("/tmp/db_area_ways.tsv");
ofstream* ways_sub_out;*/
/*ofstream way_members_out("/tmp/db_area_way_members.tsv");
ofstream* way_members_sub_out;*/
/*ofstream way_tags_out("/tmp/db_area_way_tags.tsv");
ofstream* way_tags_sub_out;*/
ofstream relations_out("/tmp/db_area_relations.tsv");
ofstream relation_node_members_out("/tmp/db_area_relation_node_members.tsv");
ofstream relation_way_members_out("/tmp/db_area_relation_way_members.tsv");
ofstream relation_relation_members_out("/tmp/db_area_relation_relation_members.tsv");
ofstream relation_tags_out("/tmp/db_area_relation_tags.tsv");
ofstream member_roles_out("/tmp/db_area_member_roles.tsv");
ofstream keys_out("/tmp/db_area_keys.tsv");
ofstream values_out("/tmp/db_area_values.tsv");
MYSQL* mysql(NULL);

void prepare_db()
{
  mysql_query(mysql, "use osm");
  if (split_tables)
  {
/*    nodes_sub_out = new ofstream[NR_SUBTABLES];*/
/*    node_tags_sub_out = new ofstream[NR_SUBTABLES];*/
/*    ways_sub_out = new ofstream[NR_SUBTABLES];*/
/*    way_members_sub_out = new ofstream[NR_SUBTABLES];*/
/*    way_tags_sub_out = new ofstream[NR_SUBTABLES];*/
    for (unsigned int i(0); i < NR_SUBTABLES; ++i)
    {
      ostringstream temp;
/*      temp<<"/tmp/db_area_nodes_"<<i<<".tsv";
      nodes_sub_out[i].open(temp.str().c_str());
      temp.str("");*/
/*      temp<<"/tmp/db_area_node_tags_"<<i<<".tsv";
      node_tags_sub_out[i].open(temp.str().c_str());
      temp.str("");*/
/*      temp<<"/tmp/db_area_ways_"<<i<<".tsv";
      ways_sub_out[i].open(temp.str().c_str());
      temp.str("");*/
/*      temp<<"/tmp/db_area_way_members_"<<i<<".tsv";
      way_members_sub_out[i].open(temp.str().c_str());
      temp.str("");*/
/*      temp<<"/tmp/db_area_way_tags_"<<i<<".tsv";
      way_tags_sub_out[i].open(temp.str().c_str());*/
    }
  }
}

void postprocess_db()
{
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
}

void flush_to_db()
{
/*  nodes_out.close();*/
/*  node_tags_out.close();*/
/*  ways_out.close();*/
/*  way_members_out.close();*/
/*  way_tags_out.close();*/
  relations_out.close();
  relation_node_members_out.close();
  relation_way_members_out.close();
  relation_relation_members_out.close();
  relation_tags_out.close();
  member_roles_out.close();
  keys_out.close();
  values_out.close();
  
/*  if (split_tables)
  {
    for (unsigned int i(0); i < NR_SUBTABLES; ++i)
    {
      nodes_sub_out[i].close();
      node_tags_sub_out[i].close();
      ways_sub_out[i].close();
      way_members_sub_out[i].close();
      way_tags_sub_out[i].close();
      
      ostringstream temp;
      temp<<"load data local infile '/tmp/db_area_nodes_"<<i<<".tsv' into table nodes_"<<i;
      mysql_query(mysql, temp.str().c_str());
      temp.str("");
      temp<<"load data local infile '/tmp/db_area_node_tags_"<<i<<".tsv' into table node_tags_"<<i;
      mysql_query(mysql, temp.str().c_str());
      temp.str("");
      temp<<"load data local infile '/tmp/db_area_ways_"<<i<<".tsv' into table ways_"<<i;
      mysql_query(mysql, temp.str().c_str());
      temp.str("");
      temp<<"load data local infile '/tmp/db_area_way_members_"<<i<<".tsv' into table way_members_"<<i;
      mysql_query(mysql, temp.str().c_str());
      temp.str("");
      temp<<"load data local infile '/tmp/db_area_way_tags_"<<i<<".tsv' into table way_tags_"<<i;
      mysql_query(mysql, temp.str().c_str());
    }
    mysql_query(mysql, "load data local infile '/tmp/db_area_ways.tsv' into table ways_world");
    mysql_query(mysql, "load data local infile '/tmp/db_area_way_tags.tsv' into table way_tags_world");
  }
  else
  {
    mysql_query(mysql, "load data local infile '/tmp/db_area_nodes.tsv' into table nodes");
    mysql_query(mysql, "load data local infile '/tmp/db_area_node_tags.tsv' into table node_tags");
    mysql_query(mysql, "load data local infile '/tmp/db_area_ways.tsv' into table ways");
    mysql_query(mysql, "load data local infile '/tmp/db_area_way_members.tsv' into table way_members");
    mysql_query(mysql, "load data local infile '/tmp/db_area_way_tags.tsv' into table way_tags");
  }*/
  
  mysql_query(mysql, "load data local infile '/tmp/db_area_relations.tsv' into table relations");
  mysql_query(mysql, "load data local infile '/tmp/db_area_relation_node_members.tsv' into table relation_node_members");
  mysql_query(mysql, "load data local infile '/tmp/db_area_relation_way_members.tsv' into table relation_way_members");
  mysql_query(mysql, "load data local infile '/tmp/db_area_relation_relation_members.tsv' into table relation_relation_members");
  mysql_query(mysql, "load data local infile '/tmp/db_area_relation_tags.tsv' into table relation_tags");
  mysql_query(mysql, "load data local infile '/tmp/db_area_member_roles.tsv' into table member_roles");
  mysql_query(mysql, "load data local infile '/tmp/db_area_keys.tsv' into table key_s");
  mysql_query(mysql, "load data local infile '/tmp/db_area_values.tsv' into table value_s");
}

void start(const char *el, const char **attr)
{
  // patch: map< string > uses about 50 byte overhead per string
  // which is too much for 100 million strings in 2 GB memory
  static Value_Detect val_detect = Value_Detect();
  
  if (!strcmp(el, "tag"))
  {
    if (tag_type != 0)
    {
      string key(""), value("");
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "k"))
	  key = attr[i+1];
	if (!strcmp(attr[i], "v"))
	  value = attr[i+1];
      }
      // patch cope with server power limits by ignoring uncommon node tags
/*      if ((tag_type != NODE) || (no_tag_limit) || (allowed_node_tags.find(key) != allowed_node_tags.end()))*/
      if (tag_type == RELATION)
      {
	unsigned int key_id(0);
	map< string, unsigned int >::const_iterator it(keys.find(key));
	if (it != keys.end())
	  key_id = it->second;
	else
	{
	  key_id = keys.size()+1;
	  keys.insert(make_pair< string, unsigned int >(key, key_id));
	  keys_out<<key_id<<'\t';
	  escape_infile_xml(keys_out, key);
	  keys_out<<'\n';
	}
	unsigned int value_id(0);
	if (lowmem)
	{
        // patch: map< string > uses about 50 byte overhead per string
        // which is too much for 100 million strings in 2 GB memory
	  value_id = val_detect.find(value.c_str());
	  if (val_detect.is_new)
	  {
	    values_out<<value_id<<'\t';
	    escape_infile_xml(values_out, value);
	    values_out<<'\n';
	  }
	}
	else
	{
	  it = values.find(value);
	  if (it != values.end())
	    value_id = it->second;
	  else
	  {
	    value_id = values.size()+1;
	    values.insert(make_pair< string, unsigned int >(value, value_id));
	    values_out<<value_id<<'\t';
	    escape_infile_xml(values_out, value);
	    values_out<<'\n';
	  }
	}
	if (tag_type == NODE)
	{
/*	  if (split_tables)
	  {
	    unsigned char idx(node_cache[current_id]);
	    if (idx < NR_SUBTABLES)
	      (node_tags_sub_out[idx])<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';
	  }
	  else
	    node_tags_out<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';*/
	}
	else if (tag_type == WAY)
	{
/*	  if (split_tables)
	    way_tags_temp<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';
	  else
	    way_tags_out<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';*/
	}
	else if (tag_type == RELATION)
	  relation_tags_out<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';
      }
    }
  }
  else if (!strcmp(el, "nd"))
  {
    if (tag_type == WAY)
    {
      unsigned int ref(0);
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "ref"))
	  ref = atoi(attr[i+1]);
      }
      if (split_tables)
      {
	unsigned char idx(node_cache[ref]);
	if (idx < NR_SUBTABLES)
	  /*(way_members_sub_out[idx])<<current_id<<'\t'<<++way_member_count<<'\t'<<ref<<'\n'*/;
	if (current_block != idx)
	{
	  if (current_block == NR_SUBTABLES)
	    current_block = idx;
	  else
	    current_block = MULTIPLE_BLOCKS;
	}
      }
/*      else
	way_members_out<<current_id<<'\t'<<++way_member_count<<'\t'<<ref<<'\n';*/
    }
  }
  else if (!strcmp(el, "member"))
  {
    if (tag_type == RELATION)
    {
      unsigned int ref(0);
      string role(""), type("");
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "ref"))
	  ref = atoi(attr[i+1]);
	if (!strcmp(attr[i], "role"))
	  role = attr[i+1];
	if (!strcmp(attr[i], "type"))
	  type = attr[i+1];
      }
      unsigned int role_id(0);
      map< string, unsigned int >::const_iterator it(member_roles.find(role));
      if (it != member_roles.end())
	role_id = it->second;
      else
      {
	role_id = member_roles.size()+1;
	member_roles.insert(make_pair< string, unsigned int >(role, role_id));
	member_roles_out<<role_id<<'\t';
	escape_infile_xml(member_roles_out, role);
	member_roles_out<<'\n';
      }
      if (type == "node")
	relation_node_members_out<<current_id<<'\t'<<ref<<'\t'<<role_id<<'\n';
      else if (type == "way")
	relation_way_members_out<<current_id<<'\t'<<ref<<'\t'<<role_id<<'\n';
      else if (type == "relation")
	relation_relation_members_out<<current_id<<'\t'<<ref<<'\t'<<role_id<<'\n';
    }
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
    if (split_tables)
    {
      unsigned char idx(ll_idx(lat, lon)>>24);
      node_cache[id] = idx;
/*      (nodes_sub_out[idx])<<id<<'\t'<<lat<<'\t'<<lon<<'\n';*/
    }
/*    else
      nodes_out<<id<<'\t'<<lat<<'\t'<<lon<<'\n';*/
    tag_type = NODE;
    current_id = id;
  }
  else if (!strcmp(el, "way"))
  {
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
/*    if (!split_tables)
      ways_out<<id<<'\n';*/
    tag_type = WAY;
    current_id = id;
    way_member_count = 0;
    current_block = NR_SUBTABLES;
    way_tags_temp.str("");
  }
  else if (!strcmp(el, "relation"))
  {
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
    relations_out<<id<<'\n';
    tag_type = RELATION;
    current_id = id;
  }
}

void end(const char *el)
{
  ++structure_count;
  if (!strcmp(el, "node"))
  {
    tag_type = 0;
    current_id = 0;
  }
  else if (!strcmp(el, "way"))
  {
    if (split_tables)
    {
      if (current_block < NR_SUBTABLES)
      {
/*	(ways_sub_out[current_block])<<current_id<<'\n';*/
/*	(way_tags_sub_out[current_block])<<way_tags_temp.str();*/
      }
      else
      {
/*	ways_out<<current_id<<'\n';*/
/*	way_tags_out<<way_tags_temp.str();*/
      }
      way_tags_temp.str("");
    }
    tag_type = 0;
    current_id = 0;
  }
  else if (!strcmp(el, "relation"))
  {
    tag_type = 0;
    current_id = 0;
  }
  if (structure_count == FLUSH_INTERVAL)
  {
    flush_to_db();
    cerr<<'.';
    if (split_tables)
    {
/*      for (unsigned int i(0); i < NR_SUBTABLES; ++i)
      {
	ostringstream temp;
	temp<<"/tmp/db_area_nodes_"<<i<<".tsv";
	nodes_sub_out[i].open(temp.str().c_str());
	temp.str("");
	temp<<"/tmp/db_area_node_tags_"<<i<<".tsv";
	node_tags_sub_out[i].open(temp.str().c_str());
	temp.str("");
	temp<<"/tmp/db_area_ways_"<<i<<".tsv";
	ways_sub_out[i].open(temp.str().c_str());
	temp.str("");
	temp<<"/tmp/db_area_way_members_"<<i<<".tsv";
	way_members_sub_out[i].open(temp.str().c_str());
	temp.str("");
	temp<<"/tmp/db_area_way_tags_"<<i<<".tsv";
	way_tags_sub_out[i].open(temp.str().c_str());
      }*/
    }
/*    nodes_out.open("/tmp/db_area_nodes.tsv");*/
/*    node_tags_out.open("/tmp/db_area_node_tags.tsv");*/
/*    ways_out.open("/tmp/db_area_ways.tsv");*/
/*    way_members_out.open("/tmp/db_area_way_members.tsv");*/
/*    way_tags_out.open("/tmp/db_area_way_tags.tsv");*/
    relations_out.open("/tmp/db_area_relations.tsv");
    relation_node_members_out.open("/tmp/db_area_relation_node_members.tsv");
    relation_way_members_out.open("/tmp/db_area_relation_way_members.tsv");
    relation_relation_members_out.open("/tmp/db_area_relation_relation_members.tsv");
    relation_tags_out.open("/tmp/db_area_relation_tags.tsv");
    member_roles_out.open("/tmp/db_area_member_roles.tsv");
    keys_out.open("/tmp/db_area_keys.tsv");
    values_out.open("/tmp/db_area_values.tsv");
    structure_count = 0;
  }
}

int main(int argc, char *argv[])
{
  int i(0);
  while (++i < argc)
  {
    if (!strcmp(argv[i], "--savemem"))
      lowmem = true;
    else if (!strcmp(argv[i], "--limit-node-tags"))
      no_tag_limit = false;
    else if (!strcmp(argv[i], "--split-tables"))
      split_tables = true;
    else
    {
      cout<<"Usage: "<<argv[0]<<" [--savemem] [--limit-node-tags]\n";
      return 0;
    }
  }
  
  // only necessary for split_tables
  if (split_tables)
  {
    node_cache = (unsigned char*) malloc(512*1024*1024);
    for (unsigned int i(0); i < 512*1024*1024/sizeof(int); ++i)
      ((int*)node_cache)[i] = -1;
  }
  
  // patch cope with server power limits by ignoring uncommon node tags
  allowed_node_tags.insert("highway");
  allowed_node_tags.insert("traffic-calming");
  allowed_node_tags.insert("barrier");
  allowed_node_tags.insert("waterway");
  allowed_node_tags.insert("lock");
  allowed_node_tags.insert("railway");
  allowed_node_tags.insert("aeroway");
  allowed_node_tags.insert("aerialway");
  allowed_node_tags.insert("power");
  allowed_node_tags.insert("man_made");
  allowed_node_tags.insert("leisure");
  allowed_node_tags.insert("amenity");
  allowed_node_tags.insert("shop");
  allowed_node_tags.insert("tourism");
  allowed_node_tags.insert("historic");
  allowed_node_tags.insert("landuse");
  allowed_node_tags.insert("military");
  allowed_node_tags.insert("natural");
  allowed_node_tags.insert("route");
  allowed_node_tags.insert("sport");
  allowed_node_tags.insert("bridge");
  allowed_node_tags.insert("crossing");
  allowed_node_tags.insert("mountain_pass");
  allowed_node_tags.insert("ele");
  allowed_node_tags.insert("operator");
  allowed_node_tags.insert("opening-hours");
  allowed_node_tags.insert("disused");
  allowed_node_tags.insert("wheelchair");
  allowed_node_tags.insert("noexit");
  allowed_node_tags.insert("traffic_sign");
  allowed_node_tags.insert("name");
  allowed_node_tags.insert("alt_name");
  allowed_node_tags.insert("int_name");
  allowed_node_tags.insert("nat_name");
  allowed_node_tags.insert("reg_name");
  allowed_node_tags.insert("loc_name");
  allowed_node_tags.insert("ref");
  allowed_node_tags.insert("int_ref");
  allowed_node_tags.insert("nat_ref");
  allowed_node_tags.insert("reg_ref");
  allowed_node_tags.insert("loc_ref");
  allowed_node_tags.insert("old_ref");
  allowed_node_tags.insert("source_ref");
  allowed_node_tags.insert("icao");
  allowed_node_tags.insert("iata");
  allowed_node_tags.insert("place");
  allowed_node_tags.insert("place_numbers");
  allowed_node_tags.insert("postal_code");
  allowed_node_tags.insert("is_in");
  allowed_node_tags.insert("population");
  
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", NULL, 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    cerr<<"Connection to database failed.\n";
    return 1;
  }
  
  prepare_db();
  
  if (lowmem)
    // patch: map< string > uses about 50 byte overhead per string
    // which is too much for 100 million strings in 2 GB memory
    values_out<<1<<'\t'<<'\n';
  
  //reading the main document
  parse(stdin, start, end);
  
  flush_to_db();
  
  member_roles.clear();
  keys.clear();
  values.clear();
  
  postprocess_db();
  
  mysql_close(mysql);
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
