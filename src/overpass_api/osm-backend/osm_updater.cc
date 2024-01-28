/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "node_updater.h"
#include "osm_updater.h"
#include "relation_updater.h"
#include "tags_updater.h"
#include "way_updater.h"
#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher_client.h"
#include "../../template_db/random_file.h"
#include "../../template_db/transaction.h"
#include "../core/settings.h"
#include "../data/abstract_processing.h"
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


/**
 * Tests the library node_updater, way_updater and relation_updater
 * with a sample OSM file
 */

namespace
{
  Node_Updater* node_updater(0);
  Node current_node;
  Way_Updater* way_updater(0);
  Way current_way;
  Relation_Updater* relation_updater(0);
  Relation current_relation;
  int state;
  const int IN_NODES = 1;
  const int IN_WAYS = 2;
  const int IN_RELATIONS = 3;
  int modify_mode = 0;
  const int DELETE = 1;
  uint flush_limit = 4*1024*1024;
  OSM_Element_Metadata* meta;

  uint32 osm_element_count;
  Osm_Backend_Callback* callback(0);
  Cpu_Stopwatch* cpu_stopwatch(0);

  std::string data_version;

  inline void tag_start(const char **attr)
  {
    std::string key(""), value("");
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	key = attr[i+1];
      if (!strcmp(attr[i], "v"))
	value = attr[i+1];
    }
    if (current_node.id.val() > 0)
      current_node.tags.push_back(std::make_pair(key, value));
    else if (current_way.id.val() > 0)
      current_way.tags.push_back(std::make_pair(key, value));
    else if (current_relation.id.val() > 0)
      current_relation.tags.push_back(std::make_pair(key, value));
  }


  inline void nd_start(const char **attr)
  {
    if (current_way.id.val() > 0)
    {
      Uint64 ref;
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "ref"))
	  ref = atoll(attr[i+1]);
      }
      current_way.nds.push_back(ref);
    }
  }


  inline void member_start(const char **attr)
  {
    if (current_relation.id.val() > 0)
    {
      Uint64 ref;
      std::string type, role;
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "ref"))
	  ref = atoll(attr[i+1]);
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

    Node::Id_Type id;
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
        meta->timestamp = Timestamp(
            atol(attr[i+1]), //year
            atoi(attr[i+1]+5), //month
            atoi(attr[i+1]+8), //day
            atoi(attr[i+1]+11), //hour
            atoi(attr[i+1]+14), //minute
            atoi(attr[i+1]+17) //second
            ).timestamp;
      }
      if (meta && (!strcmp(attr[i], "changeset")))
	meta->changeset = atoi(attr[i+1]);
      if (meta && (!strcmp(attr[i], "user")))
	meta->user_name = attr[i+1];
      if (meta && (!strcmp(attr[i], "uid")))
	meta->user_id = atoi(attr[i+1]);
    }
    if (lat >= -90. && lat <= 90. && lon >= -180. && lon <= 180.)
      current_node = Node(id, lat, lon);
    else
      current_node = Node(id, 100., 200.);
  }


  inline void node_end()
  {
    if (modify_mode == DELETE)
      node_updater->set_id_deleted(current_node.id, meta);
    else
      node_updater->set_node(current_node, meta);
    if (osm_element_count >= flush_limit)
    {
      callback->node_elapsed(current_node.id);
      node_updater->update(callback, cpu_stopwatch, true);
      callback->parser_started();
      osm_element_count = 0;
    }
    current_node.id = Node::Id_Type();
  }


  inline void way_start(const char **attr)
  {
    if (state == IN_NODES)
    {
      callback->nodes_finished();
      node_updater->update(callback, cpu_stopwatch, false);
      //way_updater->update_moved_idxs(callback, node_updater->get_moved_nodes(), update_way_logger);
      callback->parser_started();
      osm_element_count = 0;
      state = IN_WAYS;
    }
    else if (state == 0)
      state = IN_WAYS;
    if (meta)
      *meta = OSM_Element_Metadata();

    Way::Id_Type id;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoll(attr[i+1]);
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
    current_way = Way(id.val());
  }


  inline void way_end()
  {
    if (modify_mode == DELETE)
      way_updater->set_id_deleted(current_way.id, meta);
    else
      way_updater->set_way(current_way, meta);
    if (osm_element_count >= flush_limit)
    {
      callback->way_elapsed(current_way.id);
      way_updater->update(callback, cpu_stopwatch, true,
                          node_updater->get_new_skeletons(), node_updater->get_attic_skeletons(),
                          node_updater->get_new_attic_skeletons());
      callback->parser_started();
      osm_element_count = 0;
    }
    current_way.id = 0u;
  }


  inline void relation_end()
  {
    if (modify_mode == DELETE)
      relation_updater->set_id_deleted(current_relation.id, meta);
    else
      relation_updater->set_relation(current_relation, meta);
    if (osm_element_count >= flush_limit)
    {
      callback->relation_elapsed(current_relation.id);
      relation_updater->update(callback, cpu_stopwatch,
                          node_updater->get_new_skeletons(), node_updater->get_attic_skeletons(),
                          node_updater->get_new_attic_skeletons(),
                          way_updater->get_new_skeletons(), way_updater->get_attic_skeletons(),
                          way_updater->get_new_attic_skeletons());
      callback->parser_started();
      osm_element_count = 0;
    }
    current_relation.id = 0u;
  }


  inline void relation_start(const char **attr)
  {
    if (state == IN_NODES)
    {
      callback->nodes_finished();
      node_updater->update(callback, cpu_stopwatch, false);
//       relation_updater->update_moved_idxs
//           (node_updater->get_moved_nodes(), way_updater->get_moved_ways(), update_relation_logger);
      callback->parser_started();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == IN_WAYS)
    {
      callback->ways_finished();
      way_updater->update(callback, cpu_stopwatch, false,
                          node_updater->get_new_skeletons(), node_updater->get_attic_skeletons(),
                          node_updater->get_new_attic_skeletons());
//       relation_updater->update_moved_idxs
//           (node_updater->get_moved_nodes(), way_updater->get_moved_ways(), update_relation_logger);
      callback->parser_started();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == 0)
      state = IN_RELATIONS;
    if (meta)
      *meta = OSM_Element_Metadata();

    Relation::Id_Type id;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoll(attr[i+1]);
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
    current_relation = Relation(id.val());
  }
}


void node_start(const char *el, const char **attr)
{
  if (sigterm_status())
    XML_StopParser(get_current_parser(), XML_FALSE);

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
  if (sigterm_status())
    XML_StopParser(get_current_parser(), XML_FALSE);

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
  if (sigterm_status())
    XML_StopParser(get_current_parser(), XML_FALSE);

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
  if (sigterm_status())
    XML_StopParser(get_current_parser(), XML_FALSE);

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
    node_updater->update(callback, cpu_stopwatch, false);
    //way_updater->update_moved_idxs(callback, node_updater->get_moved_nodes(), update_way_logger);
    state = IN_WAYS;
  }
  if (state == IN_WAYS)
  {
    way_updater->update(callback, cpu_stopwatch, false,
                        node_updater->get_new_skeletons(), node_updater->get_attic_skeletons(),
                        node_updater->get_new_attic_skeletons());
//     relation_updater->update_moved_idxs
//         (node_updater->get_moved_nodes(), way_updater->get_moved_ways(), update_relation_logger);
    state = IN_RELATIONS;
  }
  if (state == IN_RELATIONS)
    relation_updater->update(callback, cpu_stopwatch,
                          node_updater->get_new_skeletons(), node_updater->get_attic_skeletons(),
                          node_updater->get_new_attic_skeletons(),
                          way_updater->get_new_skeletons(), way_updater->get_attic_skeletons(),
                          way_updater->get_new_attic_skeletons());

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

Osm_Updater::Osm_Updater(Osm_Backend_Callback* callback_, const std::string& data_version_,
			 Database_Meta_State meta_, unsigned int flush_limit_)
  : dispatcher_client(0), meta(Database_Meta_State::only_data)
{
  dispatcher_client = new Dispatcher_Client(osm_base_settings().shared_name);
  Logger logger(dispatcher_client->get_db_dir());
  logger.annotated_log("write_start() start version='" + data_version_ + '\'');
  dispatcher_client->write_start();
  logger.annotated_log("write_start() end");
  transaction = new Nonsynced_Transaction(Access_Mode::writeable, true, dispatcher_client->get_db_dir(), "");
  {
    std::ofstream version((dispatcher_client->get_db_dir()
        + "osm_base_version.shadow").c_str());
    version<<data_version_<<'\n';
  }

  meta = meta_.value_or_autodetect(dispatcher_client->get_db_dir());

  node_updater_ = new Node_Updater(*transaction, meta);
  way_updater_ = new Way_Updater(*transaction, meta);
  relation_updater_ = new Relation_Updater(*transaction, meta);
  flush_limit = flush_limit_;

  data_version = data_version_;

  state = 0;
  osm_element_count = 0;
  node_updater = node_updater_;
  way_updater = way_updater_;
  relation_updater = relation_updater_;
  callback = callback_;
  callback->set_db_dir(dispatcher_client->get_db_dir());
  cpu_stopwatch = new Cpu_Stopwatch();
  cpu_stopwatch->start_cpu_timer(0);
  if (meta)
    ::meta = new OSM_Element_Metadata();
}

Osm_Updater::Osm_Updater
    (Osm_Backend_Callback* callback_, std::string db_dir, const std::string& data_version_,
     Database_Meta_State meta_, unsigned int flush_limit_)
  : transaction(0), dispatcher_client(0), db_dir_(db_dir), meta(Database_Meta_State::only_data)
{
  if (file_present(db_dir + osm_base_settings().shared_name))
    throw Context_Error("File " + db_dir + osm_base_settings().shared_name + " present, "
        "which indicates a running dispatcher. Delete file if no dispatcher is running.");

  {
    std::ofstream version((db_dir + "osm_base_version").c_str());
    version<<data_version_<<'\n';
  }
  
  meta = meta_.value_or_autodetect(db_dir);

  node_updater_ = new Node_Updater(db_dir, meta);
  way_updater_ = new Way_Updater(db_dir, meta);
  relation_updater_ = new Relation_Updater(db_dir, meta);
  flush_limit = flush_limit_;

  data_version = data_version_;

  state = 0;
  osm_element_count = 0;
  node_updater = node_updater_;
  way_updater = way_updater_;
  relation_updater = relation_updater_;
  callback = callback_;
  if (meta)
    ::meta = new OSM_Element_Metadata();

  signal(SIGTERM, sigterm);
}

void Osm_Updater::flush()
{
  delete node_updater_;
  node_updater_ = new Node_Updater(db_dir_, meta);
  delete way_updater_;
  way_updater_ = new Way_Updater(db_dir_, meta);
  delete relation_updater_;
  relation_updater_ = new Relation_Updater(db_dir_, meta);
  if (cpu_stopwatch)
    cpu_stopwatch->stop_cpu_timer(0);
  std::vector< uint64 > cpu_runtime = cpu_stopwatch ? cpu_stopwatch->cpu_time() : std::vector< uint64 >();

  if (dispatcher_client)
  {
    delete transaction;
    transaction = 0;

    Logger logger(dispatcher_client->get_db_dir());
    std::ostringstream out;
    out<<"write_commit() start "<<global_read_counter();
    for (std::vector< uint64 >::const_iterator it = cpu_runtime.begin(); it != cpu_runtime.end(); ++it)
      out<<' '<<*it;
    logger.annotated_log(out.str());

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
  delete way_updater_;
  delete relation_updater_;
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
