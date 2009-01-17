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
    return 1;
  
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
bool lowmem(false);

map< string, unsigned int > member_roles;
map< string, unsigned int > keys;
map< string, unsigned int > values;

const unsigned int FLUSH_INTERVAL = 100000;
unsigned int structure_count(0);

unsigned int way_member_count(0);

ofstream nodes_out("/tmp/db_area_nodes.tsv");
ofstream node_tags_out("/tmp/db_area_node_tags.tsv");
ofstream ways_out("/tmp/db_area_ways.tsv");
ofstream way_members_out("/tmp/db_area_way_members.tsv");
ofstream way_tags_out("/tmp/db_area_way_tags.tsv");
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
}

void postprocess_db()
{
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
}

void flush_to_db()
{
  nodes_out.close();
  node_tags_out.close();
  ways_out.close();
  way_members_out.close();
  way_tags_out.close();
  relations_out.close();
  relation_node_members_out.close();
  relation_way_members_out.close();
  relation_relation_members_out.close();
  relation_tags_out.close();
  member_roles_out.close();
  keys_out.close();
  values_out.close();
  mysql_query(mysql, "load data local infile '/tmp/db_area_nodes.tsv' into table nodes");
  mysql_query(mysql, "load data local infile '/tmp/db_area_node_tags.tsv' into table node_tags");
  mysql_query(mysql, "load data local infile '/tmp/db_area_ways.tsv' into table ways");
  mysql_query(mysql, "load data local infile '/tmp/db_area_way_members.tsv' into table way_members");
  mysql_query(mysql, "load data local infile '/tmp/db_area_way_tags.tsv' into table way_tags");
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
      if (lowmem)
      {
        // patch: map< string > uses about 50 byte overhead per string
        // which is too much for 100 million strings in 2 GB memory
	unsigned int value_id(val_detect.find(value.c_str()));
	if (val_detect.is_new)
	{
	  values_out<<value_id<<'\t';
	  escape_infile_xml(values_out, value);
	  values_out<<'\n';
	}
      }
      else
      {
	unsigned int value_id(0);
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
	node_tags_out<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';
      else if (tag_type == WAY)
	way_tags_out<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';
      else if (tag_type == RELATION)
	relation_tags_out<<current_id<<'\t'<<key_id<<'\t'<<value_id<<'\n';
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
      way_members_out<<current_id<<'\t'<<++way_member_count<<'\t'<<ref<<'\n';
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
    int lat_idx(100), lat(100*10000000), lon(200*10000000);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (!strcmp(attr[i], "lat"))
      {
	lat = (int)in_lat_lon(attr[i+1]);
	lat_idx = calc_idx(lat);
      }
      if (!strcmp(attr[i], "lon"))
	lon = (int)in_lat_lon(attr[i+1]);
    }
    nodes_out<<id<<'\t'<<lon/10000000<<'\t'<<lat<<'\t'<<lon<<'\n';
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
    ways_out<<id<<'\n';
    tag_type = WAY;
    current_id = id;
    way_member_count = 0;
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
    nodes_out.open("/tmp/db_area_nodes.tsv");
    node_tags_out.open("/tmp/db_area_node_tags.tsv");
    ways_out.open("/tmp/db_area_ways.tsv");
    way_members_out.open("/tmp/db_area_way_members.tsv");
    way_tags_out.open("/tmp/db_area_way_tags.tsv");
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
  if ((argc == 2) && (!strcmp(argv[1], "--savemem")))
    lowmem = true;
  else if (argc > 1)
  {
    cout<<"Usage: "<<argv[0]<<" [--savemem]\n";
    return 0;
  }
  
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", NULL, 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    cerr<<"Connection to database failed.\n";
    return 1;
  }
  
  prepare_db();
  
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
