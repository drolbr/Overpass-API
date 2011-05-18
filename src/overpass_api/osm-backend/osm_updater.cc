#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/settings.h"
#include "../dispatch/dispatcher.h"
#include "../frontend/output.h"
#include "node_updater.h"
#include "osm_updater.h"
#include "relation_updater.h"
#include "way_updater.h"

using namespace std;

/**
 * Tests the library node_updater, way_updater and relation_updater
 * with a sample OSM file
 */

namespace
{
  Node_Updater* node_updater;
  Node current_node;
  Way_Updater* way_updater;
  Way current_way;
  Relation_Updater* relation_updater;
  Relation current_relation;
  int state;
  const int IN_NODES = 1;
  const int IN_WAYS = 2;
  const int IN_RELATIONS = 3;
  int modify_mode = 0;
  const int DELETE = 1;
  
  uint32 osm_element_count;
  Osm_Backend_Callback* callback;

  
  inline void tag_start(const char **attr)
  {
    string key(""), value("");
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	key = attr[i+1];
      if (!strcmp(attr[i], "v"))
	value = attr[i+1];
    }
    if (current_node.id > 0)
      current_node.tags.push_back(make_pair(key, value));
    else if (current_way.id > 0)
      current_way.tags.push_back(make_pair(key, value));
    else if (current_relation.id > 0)
      current_relation.tags.push_back(make_pair(key, value));
  }


  inline void nd_start(const char **attr)
  {
    if (current_way.id > 0)
    {
      unsigned int ref(0);
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "ref"))
	  ref = atoi(attr[i+1]);
      }
      current_way.nds.push_back(ref);
    }
  }


  inline void member_start(const char **attr)
  {
    if (current_relation.id > 0)
    {
      unsigned int ref(0);
      string type, role;
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "ref"))
	  ref = atoi(attr[i+1]);
	if (!strcmp(attr[i], "type"))
	  type = attr[i+1];
	if (!strcmp(attr[i], "role"))
	  role = attr[i+1];
      }
      Relation_Entry entry;
      entry.ref = ref;
      if (type == "node")
	entry.type = Relation_Entry::NODE;
      else if (type == "way")
	entry.type = Relation_Entry::WAY;
      else if (type == "relation")
	entry.type = Relation_Entry::RELATION;
      entry.role = relation_updater->get_role_id(role);
      current_relation.members.push_back(entry);
    }
  }


  inline void node_start(const char **attr)
  {
    if (state == 0)
      state = IN_NODES;
    
    unsigned int id(0);
    double lat(100.0), lon(200.0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (!strcmp(attr[i], "lat"))
	lat = atof(attr[i+1]);
      if (!strcmp(attr[i], "lon"))
	lon = atof(attr[i+1]);
    }
    current_node = Node(id, lat, lon);
  }
  
  
  inline void node_end()
  {
    if (modify_mode == DELETE)
      node_updater->set_id_deleted(current_node.id);
    else
      node_updater->set_node(current_node);
    if (osm_element_count >= 4*1024*1024)
    {
      callback->node_elapsed(current_node.id);
      node_updater->update(callback, true);
      callback->parser_started();
      osm_element_count = 0;
    }
    current_node.id = 0;
  }
  
  
  inline void way_start(const char **attr)
  {
    if (state == IN_NODES)
    {
      callback->nodes_finished();
      node_updater->update(callback);
      way_updater->update_moved_idxs(callback, node_updater->get_moved_nodes());
      callback->parser_started();
      osm_element_count = 0;
      state = IN_WAYS;
    }
    
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
    current_way = Way(id);
  }
  

  inline void way_end()
  {
    if (modify_mode == DELETE)
      way_updater->set_id_deleted(current_way.id);
    else
      way_updater->set_way(current_way);
    if (osm_element_count >= 4*1024*1024)
    {
      callback->way_elapsed(current_way.id);
      way_updater->update(callback, true);
      callback->parser_started();
      osm_element_count = 0;
    }
    current_way.id = 0;
  }
  
  
  inline void relation_end()
  {
    if (modify_mode == DELETE)
      relation_updater->set_id_deleted(current_relation.id);
    else
      relation_updater->set_relation(current_relation);
    if (osm_element_count >= 4*1024*1024)
    {
      callback->relation_elapsed(current_relation.id);
      relation_updater->update(callback);
      callback->parser_started();
      osm_element_count = 0;
    }
    current_relation.id = 0;
  }  


  inline void relation_start(const char **attr)
  {
    if (state == IN_NODES)
    {
      callback->nodes_finished();
      node_updater->update(callback);
      relation_updater->update_moved_idxs
          (node_updater->get_moved_nodes(), way_updater->get_moved_ways());
      callback->parser_started();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == IN_WAYS)
    {
      callback->ways_finished();
      way_updater->update(callback);
      relation_updater->update_moved_idxs
      (node_updater->get_moved_nodes(), way_updater->get_moved_ways());
      callback->parser_started();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
    current_relation = Relation(id);
  }
}


void node_start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
    tag_start(attr);
  else if (!strcmp(el, "node"))
    node_start(attr);
  else if (!strcmp(el, "delete"))
    modify_mode = DELETE;
}

void node_end(const char *el)
{
  if (!strcmp(el, "node"))
    node_end();
  else if (!strcmp(el, "delete"))
    modify_mode = 0;
  ++osm_element_count;
}

void way_start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
    tag_start(attr);
  else if (!strcmp(el, "nd"))
    nd_start(attr);
  else if (!strcmp(el, "way"))
    way_start(attr);
  else if (!strcmp(el, "delete"))
    modify_mode = DELETE;
}

void way_end(const char *el)
{
  if (!strcmp(el, "way"))
    way_end();
  else if (!strcmp(el, "delete"))
    modify_mode = 0;
  ++osm_element_count;
}

void relation_start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
    tag_start(attr);
  else if (!strcmp(el, "member"))
    member_start(attr);
  else if (!strcmp(el, "relation"))
    relation_start(attr);
  else if (!strcmp(el, "delete"))
    modify_mode = DELETE;
}

void relation_end(const char *el)
{
  if (!strcmp(el, "relation"))
    relation_end();
  else if (!strcmp(el, "delete"))
    modify_mode = 0;
  ++osm_element_count;
}

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
    tag_start(attr);
  else if (!strcmp(el, "nd"))
    nd_start(attr);
  else if (!strcmp(el, "member"))
    member_start(attr);
  else if (!strcmp(el, "node"))
    node_start(attr);
  else if (!strcmp(el, "way"))
    way_start(attr);
  else if (!strcmp(el, "relation"))
    relation_start(attr);
  else if (!strcmp(el, "delete"))
    modify_mode = DELETE;
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
    node_end();
  else if (!strcmp(el, "way"))
    way_end();
  else if (!strcmp(el, "relation"))
    relation_end();
  else if (!strcmp(el, "delete"))
    modify_mode = 0;
  ++osm_element_count;
}

void finish_updater()
{
  if (state == IN_NODES)
    callback->nodes_finished();
  else if (state == IN_WAYS)
    callback->ways_finished();
  else if (state == IN_RELATIONS)
    callback->relations_finished();
  
  if (state == IN_NODES)
  {
    node_updater->update(callback);
    way_updater->update_moved_idxs(callback, node_updater->get_moved_nodes());
    state = IN_WAYS;
  }
  if (state == IN_WAYS)
  {  
    way_updater->update(callback);
    relation_updater->update_moved_idxs
        (node_updater->get_moved_nodes(), way_updater->get_moved_ways());
    state = IN_RELATIONS;
  }
  if (state == IN_RELATIONS)
    relation_updater->update(callback);
}

void parse_file_completely(FILE* in)
{
  callback->parser_started();
  
  parse(stdin, start, end);
  
  finish_updater();
}

void parse_nodes_only(FILE* in)
{
  parse(in, node_start, node_end);
}

void parse_ways_only(FILE* in)
{
  parse(in, way_start, way_end);
}

void parse_relations_only(FILE* in)
{
  parse(in, relation_start, relation_end);
}

Osm_Updater::Osm_Updater(Osm_Backend_Callback* callback_, bool transactional)
  : transaction(true, true, ""), dispatcher_client(0),
    node_updater_(transactional ? &transaction : 0),
    way_updater_(transactional ? &transaction : 0),
    relation_updater_(transactional ? &transaction : 0)
{
  if (transactional)
  {
    dispatcher_client = new Dispatcher_Client(shared_name);
    dispatcher_client->write_start();
    set_basedir(dispatcher_client->get_db_dir());
    transaction.data_index(de_osm3s_file_ids::NODES);
    transaction.random_index(de_osm3s_file_ids::NODES);
    transaction.data_index(de_osm3s_file_ids::NODE_TAGS_LOCAL);
    transaction.data_index(de_osm3s_file_ids::NODE_TAGS_GLOBAL);
    transaction.data_index(de_osm3s_file_ids::WAYS);
    transaction.random_index(de_osm3s_file_ids::WAYS);
    transaction.data_index(de_osm3s_file_ids::WAY_TAGS_LOCAL);
    transaction.data_index(de_osm3s_file_ids::WAY_TAGS_GLOBAL);
    transaction.data_index(de_osm3s_file_ids::RELATIONS);
    transaction.random_index(de_osm3s_file_ids::RELATIONS);
    transaction.data_index(de_osm3s_file_ids::RELATION_ROLES);
    transaction.data_index(de_osm3s_file_ids::RELATION_TAGS_LOCAL);
    transaction.data_index(de_osm3s_file_ids::RELATION_TAGS_GLOBAL);
  }
  state = 0;
  osm_element_count = 0;
  node_updater = &node_updater_;
  way_updater = &way_updater_;
  relation_updater = &relation_updater_;
  callback = callback_;
}

Osm_Updater::~Osm_Updater()
{
  if (dispatcher_client)
  {
    transaction.flush();
    dispatcher_client->write_commit();
    delete dispatcher_client;
  }
}
