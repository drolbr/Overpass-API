/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "node_updater.h"
#include "osm_updater.h"
#include "relation_updater.h"
#include "way_updater.h"
#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/settings.h"
#include "../data/collect_members.h"
#include "../dispatch/resource_manager.h"
#include "../frontend/output.h"

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

using namespace std;

/**
 * Tests the library node_updater, way_updater and relation_updater
 * with a sample OSM file
 */

namespace
{
  Node_Updater* node_updater;
  Update_Node_Logger* update_node_logger;
  Node current_node;
  Way_Updater* way_updater;
  Update_Way_Logger* update_way_logger;
  Way current_way;
  Relation_Updater* relation_updater;
  Update_Relation_Logger* update_relation_logger;
  Relation current_relation;
  int state;
  const int IN_NODES = 1;
  const int IN_WAYS = 2;
  const int IN_RELATIONS = 3;
  int modify_mode = 0;
  const int DELETE = 1;
  OSM_Element_Metadata* meta;
  
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
    if (meta)
      *meta = OSM_Element_Metadata();
    
    unsigned int id(0);
    double lat(100.0), lon(200.0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoll(attr[i+1]);
      if (!strcmp(attr[i], "lat"))
	lat = atof(attr[i+1]);
      if (!strcmp(attr[i], "lon"))
	lon = atof(attr[i+1]);
      if (meta && (!strcmp(attr[i], "version")))
	meta->version = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "timestamp")))
      {
	meta->timestamp = 0;
	meta->timestamp |= (atoll(attr[i+1])<<26); //year
	meta->timestamp |= (atoi(attr[i+1]+5)<<22); //month
	meta->timestamp |= (atoi(attr[i+1]+8)<<17); //day
	meta->timestamp |= (atoi(attr[i+1]+11)<<12); //hour
	meta->timestamp |= (atoi(attr[i+1]+14)<<6); //minute
	meta->timestamp |= atoi(attr[i+1]+17); //second
      }
      if (meta && (!strcmp(attr[i], "changeset")))
	meta->changeset = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "user")))
	meta->user_name = attr[i+1];
      if (meta && (!strcmp(attr[i], "uid")))
	meta->user_id = atoi(attr[i+1]);
    }
    current_node = Node(id, lat, lon);
  }
  
  
  inline void node_end()
  {
    if (modify_mode == DELETE)
      node_updater->set_id_deleted(current_node.id);
    else
      node_updater->set_node(current_node, meta);
    if (osm_element_count >= 4*1024*1024)
    {
      callback->node_elapsed(current_node.id);
      node_updater->update(callback, true, update_node_logger);
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
      node_updater->update(callback, false, update_node_logger);
      way_updater->update_moved_idxs(callback, node_updater->get_moved_nodes(), update_way_logger);
      callback->parser_started();
      osm_element_count = 0;
      state = IN_WAYS;
    }
    else if (state == 0)
      state = IN_WAYS;
    if (meta)
      *meta = OSM_Element_Metadata();
    
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "version")))
	meta->version = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "timestamp")))
      {
	meta->timestamp = 0;
	meta->timestamp |= (atoll(attr[i+1])<<26); //year
	meta->timestamp |= (atoi(attr[i+1]+5)<<22); //month
	meta->timestamp |= (atoi(attr[i+1]+8)<<17); //day
	meta->timestamp |= (atoi(attr[i+1]+11)<<12); //hour
	meta->timestamp |= (atoi(attr[i+1]+14)<<6); //minute
	meta->timestamp |= atoi(attr[i+1]+17); //second
      }
      if (meta && (!strcmp(attr[i], "changeset")))
	meta->changeset = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "user")))
	meta->user_name = attr[i+1];
      if (meta && (!strcmp(attr[i], "uid")))
	meta->user_id = atoi(attr[i+1]);
    }
    current_way = Way(id);
  }
  

  inline void way_end()
  {
    if (modify_mode == DELETE)
      way_updater->set_id_deleted(current_way.id);
    else
      way_updater->set_way(current_way, meta);
    if (osm_element_count >= 4*1024*1024)
    {
      callback->way_elapsed(current_way.id);
      way_updater->update(callback, true, update_way_logger);
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
      relation_updater->set_relation(current_relation, meta);
    if (osm_element_count >= 4*1024*1024)
    {
      callback->relation_elapsed(current_relation.id);
      relation_updater->update(callback, update_relation_logger);
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
      node_updater->update(callback, false, update_node_logger);
      relation_updater->update_moved_idxs
          (node_updater->get_moved_nodes(), way_updater->get_moved_ways(), update_relation_logger);
      callback->parser_started();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == IN_WAYS)
    {
      callback->ways_finished();
      way_updater->update(callback, false, update_way_logger);
      relation_updater->update_moved_idxs
      (node_updater->get_moved_nodes(), way_updater->get_moved_ways(), update_relation_logger);
      callback->parser_started();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == 0)
      state = IN_WAYS;
    if (meta)
      *meta = OSM_Element_Metadata();
    
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "version")))
	meta->version = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "timestamp")))
      {
	meta->timestamp = 0;
	meta->timestamp |= (atoll(attr[i+1])<<26); //year
	meta->timestamp |= (atoi(attr[i+1]+5)<<22); //month
	meta->timestamp |= (atoi(attr[i+1]+8)<<17); //day
	meta->timestamp |= (atoi(attr[i+1]+11)<<12); //hour
	meta->timestamp |= (atoi(attr[i+1]+14)<<6); //minute
	meta->timestamp |= atoi(attr[i+1]+17); //second
      }
      if (meta && (!strcmp(attr[i], "changeset")))
	meta->changeset = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "user")))
	meta->user_name = attr[i+1];
      if (meta && (!strcmp(attr[i], "uid")))
	meta->user_id = atoi(attr[i+1]);
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


void collect_kept_members(Transaction& transaction,
			  Update_Node_Logger& update_node_logger,
			  Update_Way_Logger& update_way_logger)
{
  Resource_Manager rman(transaction);
  map< Uint31_Index, vector< Way_Skeleton > > ways;
  vector< uint32 > node_ids;
  
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.insert_begin(); it != update_way_logger.insert_end(); ++it)
    ways[it->second.first.index].push_back(it->second.first);
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.keep_begin(); it != update_way_logger.keep_end(); ++it)
    ways[it->second.first.index].push_back(it->second.first);
  for (map< uint32, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.erase_begin(); it != update_way_logger.erase_end(); ++it)
    ways[it->second.first.index].push_back(it->second.first);
    
  for (map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.insert_begin(); it != update_node_logger.insert_end(); ++it)
    node_ids.push_back(it->second.first.id);
  for (map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.keep_begin(); it != update_node_logger.keep_end(); ++it)
    node_ids.push_back(it->second.first.id);
  for (map< uint32, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.erase_begin(); it != update_node_logger.erase_end(); ++it)
    node_ids.push_back(it->second.first.id);
  
  map< Uint32_Index, vector< Node_Skeleton > > kept_nodes =
      way_members(0, rman, ways, 0, &node_ids, true);

  //cout<<"Kept nodes:\n";
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = kept_nodes.begin();
       it != kept_nodes.end(); ++it)
  {
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      ;//cout<<it2->id<<'\n';
  } 
  //cout<<'\n';
}


void Osm_Updater::finish_updater()
{
  if (state == IN_NODES)
    callback->nodes_finished();
  else if (state == IN_WAYS)
    callback->ways_finished();
  else if (state == IN_RELATIONS)
    callback->relations_finished();
  
  if (state == IN_NODES)
  {
    node_updater->update(callback, false, update_node_logger);
    way_updater->update_moved_idxs(callback, node_updater->get_moved_nodes(), update_way_logger);
    state = IN_WAYS;
  }
  if (state == IN_WAYS)
  {  
    way_updater->update(callback, false, update_way_logger);
    relation_updater->update_moved_idxs
        (node_updater->get_moved_nodes(), way_updater->get_moved_ways(), update_relation_logger);
    state = IN_RELATIONS;
  }
  if (state == IN_RELATIONS)
    relation_updater->update(callback, update_relation_logger);
  
  flush();
  callback->parser_succeeded();
}

void Osm_Updater::parse_file_completely(FILE* in)
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

Osm_Updater::Osm_Updater(Osm_Backend_Callback* callback_, const string& data_version,
			 bool meta_)
  : dispatcher_client(0), meta(meta_)
{
  dispatcher_client = new Dispatcher_Client(osm_base_settings().shared_name);
  Logger logger(dispatcher_client->get_db_dir());
  logger.annotated_log("write_start() start version='" + data_version + '\'');
  dispatcher_client->write_start();
  logger.annotated_log("write_start() end");
  transaction = new Nonsynced_Transaction
      (true, true, dispatcher_client->get_db_dir(), "");
  {
    ofstream version((dispatcher_client->get_db_dir()
        + "osm_base_version.shadow").c_str());
    version<<data_version<<'\n';
  }

  node_updater_ = new Node_Updater(*transaction, meta);
  update_node_logger_ = new Update_Node_Logger();
  way_updater_ = new Way_Updater(*transaction, meta);
  update_way_logger_ = new Update_Way_Logger();
  relation_updater_ = new Relation_Updater(*transaction, meta);
  update_relation_logger_ = new Update_Relation_Logger();

  state = 0;
  osm_element_count = 0;
  node_updater = node_updater_;
  update_node_logger = update_node_logger_;
  way_updater = way_updater_;
  update_way_logger = update_way_logger_;
  relation_updater = relation_updater_;
  update_relation_logger = update_relation_logger_;
  callback = callback_;
  if (meta)
    ::meta = new OSM_Element_Metadata();
}

Osm_Updater::Osm_Updater
    (Osm_Backend_Callback* callback_, string db_dir, const string& data_version, bool meta_)
  : transaction(0), dispatcher_client(0), db_dir_(db_dir), meta(meta_)
{
  {
    ofstream version((db_dir + "osm_base_version").c_str());
    version<<data_version<<'\n';
  }
  
  node_updater_ = new Node_Updater(db_dir, meta);
  update_node_logger_ = new Update_Node_Logger();
  way_updater_ = new Way_Updater(db_dir, meta);
  update_way_logger_ = new Update_Way_Logger();
  relation_updater_ = new Relation_Updater(db_dir, meta);
  update_relation_logger_ = new Update_Relation_Logger();
  
  state = 0;
  osm_element_count = 0;
  node_updater = node_updater_;
  update_node_logger = update_node_logger_;
  way_updater = way_updater_;
  update_way_logger = update_way_logger_;
  relation_updater = relation_updater_;
  update_relation_logger = update_relation_logger_;
  callback = callback_;
  if (meta)
    ::meta = new OSM_Element_Metadata();
}

void Osm_Updater::flush()
{
  if (transaction)
    collect_kept_members(*transaction, *update_node_logger, *update_way_logger);
  
  //update_node_logger_->flush();  
  //update_way_logger_->flush();
  //update_relation_logger->flush();
  
  delete node_updater_;
  node_updater_ = new Node_Updater(db_dir_, meta);
  delete way_updater_;
  way_updater_ = new Way_Updater(db_dir_, meta);
  delete relation_updater_;
  relation_updater_ = new Relation_Updater(db_dir_, meta);
  
  if (dispatcher_client)
  {
    delete transaction;
    transaction = 0;
    Logger logger(dispatcher_client->get_db_dir());
    logger.annotated_log("write_commit() start");
    dispatcher_client->write_commit();
    rename((dispatcher_client->get_db_dir() + "osm_base_version.shadow").c_str(),
	   (dispatcher_client->get_db_dir() + "osm_base_version").c_str());
    logger.annotated_log("write_commit() end");
    delete dispatcher_client;
    dispatcher_client = 0;
  }
}

Osm_Updater::~Osm_Updater()
{
  delete node_updater_;
  delete update_node_logger_;
  delete way_updater_;
  delete update_way_logger_;
  delete relation_updater_;
  delete update_relation_logger_;
  if (::meta)
    delete ::meta;
  
  if (dispatcher_client)
  {
    if (transaction)
      delete transaction;
    Logger logger(dispatcher_client->get_db_dir());
    logger.annotated_log("write_rollback() start");
    dispatcher_client->write_rollback();
    logger.annotated_log("write_rollback() end");
    delete dispatcher_client;
  }
}
