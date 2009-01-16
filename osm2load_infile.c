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

class Entry
{
  public:
    Entry() : next(0), id(0), value(0), ey_buf(1048576), lg_buf(1048576) {}
  
    Entry* next;
    int id;
    char* value;
};

class Value_Detect
{
  public:
    Value_Detect() : is_new(false), current_max(1)
    {
      entries.resize(16*16*16*16*16*16);
    }
  
    int find(const char* val);
    
    bool is_new;
  private:
    int current_max;
    vector< Entry > entries;
};

inline int strposcmp(const char* a, const char* b)
{
  unsigned int i(0);
  while ((a[i] != 0) && (b[i] != 0) && (a[i] == b[i]))
    ++i;
  if (a[i] == b[i])
    return 0;
  else
    return i;
}

int Value_Detect::find(const char* val)
{
  unsigned int length(strlen(val));
  
  if (!length)
    return 1;
  
  unsigned int i(0);
  unsigned int idx(0);
  while ((i < 6) && (i < length))
    idx = 16*idx + (((unsigned char)val[i++])%16);
  
  Entry* entry = &entries[idx];
  if (entry->value == 0)
  {
    entry->value = (char*) malloc(length+1);
    strcpy(entry->value, val);
    is_new = true;
    entry->id = ++current_max;
    
    return entry->id;
  }
  const char* diff_entry(0);
  unsigned int diff_max(0);
  i = 0;
  while (true)
  {
    i = strposcmp(val, entry->value);
    if (!i)
    {
      is_new = false;
      return entry->id;
    }
    if (entry->value[0] == 0)
    {
      if (!strposcmp(val+(*((unsigned short*)((entry->value)+1))), (char*)((entry->value)+7)))
      {
	is_new = false;
	return entry->id;
      }
    }
    else if (i > diff_max)
    {
      diff_max = i;
      diff_entry = entry->value;
    }
    
    if (!entry->next)
      break;
    entry = entry->next;
  }
  entry->next = (Entry*) malloc(sizeof(Entry));
  entry = entry->next;
  entry->value = (char*) malloc(length+1);
  entry->next = 0;
  strcpy(entry->value, val);
  is_new = true;
  entry->id = ++current_max;
    
  return entry->id;
}

const int NODE = 1;
const int WAY = 2;
const int RELATION = 3;
int tag_type(0);
unsigned int current_id(0);

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
/*  mysql_query(mysql, "create database if not exists osm");
  mysql_query(mysql, "drop database osm");
  mysql_query(mysql, "create database osm");*/
  mysql_query(mysql, "use osm");

/*  mysql_query(mysql, "create table nodes (id int, lat_idx int, lat int, lon int)");
  mysql_query(mysql, "create table node_tags (id int unsigned, key_ int unsigned, value_ int unsigned)");
  mysql_query(mysql, "create table ways (id int)");
  mysql_query(mysql, "create table way_members (id int unsigned, count int unsigned, ref int unsigned)");
  mysql_query(mysql, "create table way_tags (id int unsigned, key_ int unsigned, value_ int unsigned)");
  mysql_query(mysql, "create table relations (id int)");
  mysql_query(mysql, "create table relation_node_members (id int unsigned, ref int unsigned, role int unsigned)");
  mysql_query(mysql, "create table relation_way_members (id int unsigned, ref int unsigned, role int unsigned)");
  mysql_query(mysql, "create table relation_relation_members (id int unsigned, ref int unsigned, role int unsigned)");
  mysql_query(mysql, "create table relation_tags (id int unsigned, key_ int unsigned, value_ int unsigned)");
  mysql_query(mysql, "create table member_roles (id int unsigned, role varchar(21844))");
  mysql_query(mysql, "create table key_s (id int unsigned, key_ varchar(21844))");
  mysql_query(mysql, "create table value_s (id int unsigned, value_ varchar(21844))");

  mysql_query(mysql, "create table conflicts (id int, message varchar(21844))");
  mysql_query(mysql, "create table node_conflicts (id int, conflict int)");
  mysql_query(mysql, "create table way_conflicts (id int, conflict int)");
  mysql_query(mysql, "create table relation_conflicts (id int, conflict int)");
  
  mysql_query(mysql, "create table areas (id int)");
  mysql_query(mysql, "create table area_segments (id int, lat_idx int, min_lat int, min_lon int, max_lat int, max_lon int)");
  mysql_query(mysql, "create table area_tags (id int, key_ int unsigned, value_ int unsigned)");*/
}

void postprocess_db()
{
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  
/*  cerr<<"Creating index on ... ";
  mysql_query(mysql, "set session myisam_sort_buffer_size = 1073741824");
  
  cerr<<"nodes";
  mysql_query(mysql, "alter table nodes add unique key(id)");
  cerr<<", node_tags";
  mysql_query(mysql, "alter table node_tags add key(id)");
  cerr<<", ways";
  mysql_query(mysql, "alter table ways add unique key(id)");
  cerr<<", way_members";
  mysql_query(mysql, "alter table way_members add key(id)");
  cerr<<", way_tags";
  mysql_query(mysql, "alter table way_tags add key(id)");
  cerr<<", relations";
  mysql_query(mysql, "alter table relations add unique key(id)");
  cerr<<", relation_node_members";
  mysql_query(mysql, "alter table relation_node_members add key(id)");
  cerr<<", relation_way_members";
  mysql_query(mysql, "alter table relation_way_members add key(id)");
  cerr<<", relation_relation_members";
  mysql_query(mysql, "alter table relation_relation_members add key(id)");
  cerr<<", relation_tags";
  mysql_query(mysql, "alter table relation_tags add key(id)");
  cerr<<", member_roles";
  mysql_query(mysql, "alter table member_roles add unique key(id)");
  cerr<<", key_s";
  mysql_query(mysql, "alter table key_s add unique key(id)");
  cerr<<", value_s";
  mysql_query(mysql, "alter table value_s add unique key(id)");
  cerr<<", conflicts";
  mysql_query(mysql, "alter table conflicts add unique key(id)");
  mysql_query(mysql, "insert conflicts values (0, '')");
  cerr<<", node_conflicts";
  mysql_query(mysql, "alter table node_conflicts add key(id)");
  cerr<<", way_conflicts";
  mysql_query(mysql, "alter table way_conflicts add key(id)");
  cerr<<", relation_conflicts";
  mysql_query(mysql, "alter table relation_conflicts add key(id)");
  cerr<<", areas";
  mysql_query(mysql, "alter table areas add unique key(id)");
  mysql_query(mysql, "insert areas values (0)");
  cerr<<", area_segments";
  mysql_query(mysql, "alter table area_segments add key(id)");
  cerr<<", area_tags";
  mysql_query(mysql, "alter table area_tags add key(id)");
  
  cerr<<", nodes";
  mysql_query(mysql, "alter table nodes add index(lat_idx, lon)");
  cerr<<", node_tags";
  mysql_query(mysql, "alter table node_tags add index(key_, value_)");
  cerr<<", way_members";
  mysql_query(mysql, "alter table way_members add index(ref)");
  cerr<<", way_tags";
  mysql_query(mysql, "alter table way_tags add index(key_, value_)");
  cerr<<", relation_tags";
  mysql_query(mysql, "alter table relation_tags add index(key_, value_)");
  cerr<<", key_s";
  mysql_query(mysql, "alter table key_s add index(key_)");
  cerr<<", value_s";
  mysql_query(mysql, "alter table value_s add index(value_)");
  cerr<<", area_segments";
  mysql_query(mysql, "alter table area_segments add index(lat_idx, min_lon)");*/
  cerr<<'\n';
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
      // patch: map< string > uses about 50 byte overhead per string
      // which is too much for 100 million strings in 2 GB memory
      unsigned int value_id(val_detect.find(value.c_str()));
      if (val_detect.is_new)
      {
	values_out<<value_id<<'\t';
	escape_infile_xml(values_out, value);
	values_out<<'\n';
      }
/*      unsigned int value_id(0);
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
      }*/
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
  
  //test whether the database is successfully populated
/*  mysql_query(mysql, "select * from nodes where id = 317077361");
  
  MYSQL_RES* result(mysql_store_result(mysql));
  if (result)
  {
    unsigned int num_fields(mysql_num_fields(result));
    MYSQL_ROW row(mysql_fetch_row(result));
    while (row)
    {
      for (unsigned int i(0); i < num_fields; ++i)
	if (row[i])
	  cout<<row[i]<<'\t';
      cout<<'\n';
      row = mysql_fetch_row(result);
    }
  }*/
  //end of test sequence
  
  postprocess_db();
  
  mysql_close(mysql);
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
