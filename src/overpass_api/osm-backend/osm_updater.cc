/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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
#include "way_updater.h"
#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
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

  string data_version;
  
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
    if (current_node.id.val() > 0)
      current_node.tags.push_back(make_pair(key, value));
    else if (current_way.id.val() > 0)
      current_way.tags.push_back(make_pair(key, value));
    else if (current_relation.id.val() > 0)
      current_relation.tags.push_back(make_pair(key, value));
  }


  inline void nd_start(const char **attr)
  {
    if (current_way.id.val() > 0)
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
    if (current_relation.id.val() > 0)
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
    current_node.id = 0u;
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
    current_way.id = 0u;
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
    current_relation.id = 0u;
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
      state = IN_RELATIONS;
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
			  Update_Way_Logger& update_way_logger,
			  Update_Relation_Logger& update_relation_logger)
{
  Resource_Manager rman(transaction);
  map< Uint31_Index, vector< Relation_Skeleton > > relations;

  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
      it = update_relation_logger.insert_begin(); it != update_relation_logger.insert_end(); ++it)
    relations[it->second.first.index].push_back(it->second.first);
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
      it = update_relation_logger.keep_begin(); it != update_relation_logger.keep_end(); ++it)
    relations[it->second.first.index].push_back(it->second.first);
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
      it = update_relation_logger.erase_begin(); it != update_relation_logger.erase_end(); ++it)
    relations[it->second.first.index].push_back(it->second.first);

  vector< Way::Id_Type > way_ids;

  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.insert_begin(); it != update_way_logger.insert_end(); ++it)
    way_ids.push_back(it->second.first.id);
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.keep_begin(); it != update_way_logger.keep_end(); ++it)
    way_ids.push_back(it->second.first.id);
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.erase_begin(); it != update_way_logger.erase_end(); ++it)
    way_ids.push_back(it->second.first.id);

  map< Uint31_Index, vector< Way_Skeleton > > ways =
    relation_way_members(0, rman, relations, 0, &way_ids, true);
  
  map< uint32, vector< Node::Id_Type > > ways_by_idx;
  set< Uint31_Index > meta_idx_set;
  for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator it = ways.begin();
       it != ways.end(); ++it)
  {
    meta_idx_set.insert(Uint31_Index(it->first.val()));
    for (vector< Way_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      update_way_logger.keeping(it->first, *it2);
      ways_by_idx[it->first.val()].push_back(it2->id);
    }
  }
  
  set< pair< Tag_Index_Local, Tag_Index_Local > > tag_range_set
      = make_range_set(collect_coarse(ways_by_idx));  
  Block_Backend< Tag_Index_Local, Uint32_Index > ways_db
      (transaction.data_index(osm_base_settings().WAY_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
      it(ways_db.range_begin
         (Default_Range_Iterator< Tag_Index_Local >(tag_range_set.begin()),
          Default_Range_Iterator< Tag_Index_Local >(tag_range_set.end())));
      !(it == ways_db.range_end()); ++it)
    update_way_logger.keeping(it.index(), it.object());
  
  {
    Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way::Id_Type > > meta_db
        (transaction.data_index(meta_settings().WAYS_META));
    for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Way::Id_Type > >::Discrete_Iterator
        it(meta_db.discrete_begin(meta_idx_set.begin(), meta_idx_set.end()));
        !(it == meta_db.discrete_end()); ++it)  
      update_way_logger.keeping(it.index(), it.object());
  }

  
  ways.clear();
  
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.insert_begin(); it != update_way_logger.insert_end(); ++it)
    ways[it->second.first.index].push_back(it->second.first);
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.keep_begin(); it != update_way_logger.keep_end(); ++it)
    ways[it->second.first.index].push_back(it->second.first);
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.erase_begin(); it != update_way_logger.erase_end(); ++it)
    ways[it->second.first.index].push_back(it->second.first);
    
  vector< Node::Id_Type > node_ids;

  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.insert_begin(); it != update_node_logger.insert_end(); ++it)
    node_ids.push_back(it->second.first.id);
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.keep_begin(); it != update_node_logger.keep_end(); ++it)
    node_ids.push_back(it->second.first.id);
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.erase_begin(); it != update_node_logger.erase_end(); ++it)
    node_ids.push_back(it->second.first.id);
  
  sort(node_ids.begin(), node_ids.end());
  node_ids.erase(unique(node_ids.begin(), node_ids.end()), node_ids.end());
  
  map< Uint32_Index, vector< Node_Skeleton > > kept_nodes =
      way_members(0, rman, ways, 0, &node_ids, true);
  map< Uint32_Index, vector< Node_Skeleton > > rel_based_nodes =
      relation_node_members(0, rman, relations, 0, &node_ids, true);
      
  indexed_set_union(kept_nodes, rel_based_nodes);

  map< uint32, vector< Node::Id_Type > > nodes_by_idx;
  meta_idx_set.clear();
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator it = kept_nodes.begin();
       it != kept_nodes.end(); ++it)
  {
    meta_idx_set.insert(Uint31_Index(it->first.val()));
    for (vector< Node_Skeleton >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
    {
      update_node_logger.keeping(it->first, *it2);
      nodes_by_idx[it->first.val()].push_back(it2->id);
    }
  }
  
  // DEBUG begin
//   node_ids.clear();
//   for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
//       it = update_node_logger.insert_begin(); it != update_node_logger.insert_end(); ++it)
//     node_ids.push_back(it->second.first.id);
//   for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
//       it = update_node_logger.keep_begin(); it != update_node_logger.keep_end(); ++it)
//     node_ids.push_back(it->second.first.id);
//   for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
//       it = update_node_logger.erase_begin(); it != update_node_logger.erase_end(); ++it)
//     node_ids.push_back(it->second.first.id);
//   
//   for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
//       it = update_way_logger.insert_begin(); it != update_way_logger.insert_end(); ++it)
//   {
//     for (vector< Node::Id_Type >::const_iterator it2 = it->second.first.nds.begin();
// 	 it2 != it->second.first.nds.end(); ++it2)
//     {
//       if (!binary_search(node_ids.begin(), node_ids.end(), *it2))
// 	cerr<<it->second.id<<'\t'<<*it2<<'\n';
//     }
//   }
  // DEBUG end
  
  tag_range_set = make_range_set(collect_coarse(nodes_by_idx));  
  Block_Backend< Tag_Index_Local, Uint32_Index > nodes_db
      (transaction.data_index(osm_base_settings().NODE_TAGS_LOCAL));
  for (Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
      it(nodes_db.range_begin
         (Default_Range_Iterator< Tag_Index_Local >(tag_range_set.begin()),
          Default_Range_Iterator< Tag_Index_Local >(tag_range_set.end())));
      !(it == nodes_db.range_end()); ++it)
    update_node_logger.keeping(it.index(), it.object());
  
  {
    Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node::Id_Type > > meta_db
        (transaction.data_index(meta_settings().NODES_META));
    for (Block_Backend< Uint31_Index, OSM_Element_Metadata_Skeleton< Node::Id_Type > >::Discrete_Iterator
        it(meta_db.discrete_begin(meta_idx_set.begin(), meta_idx_set.end()));
        !(it == meta_db.discrete_end()); ++it)  
      update_node_logger.keeping(it.index(), it.object());
  }
}


void complete_user_data(Transaction& transaction,
			Update_Node_Logger& update_node_logger,
			Update_Way_Logger& update_way_logger,
			Update_Relation_Logger& update_relation_logger)
{
  map< uint32, string > user_names;
  
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.insert_begin(); it != update_node_logger.insert_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.keep_begin(); it != update_node_logger.keep_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
      it = update_node_logger.erase_begin(); it != update_node_logger.erase_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.insert_begin(); it != update_way_logger.insert_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.keep_begin(); it != update_way_logger.keep_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
      it = update_way_logger.erase_begin(); it != update_way_logger.erase_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
      it = update_relation_logger.insert_begin(); it != update_relation_logger.insert_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
      it = update_relation_logger.keep_begin(); it != update_relation_logger.keep_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
      it = update_relation_logger.erase_begin(); it != update_relation_logger.erase_end(); ++it)
  {
    if (it->second.second)
      user_names[it->second.second->user_id];
  }
   
  Block_Backend< Uint32_Index, User_Data > user_db
      (transaction.data_index(meta_settings().USER_DATA));
  for (Block_Backend< Uint32_Index, User_Data >::Flat_Iterator
      it(user_db.flat_begin()); !(it == user_db.flat_end()); ++it)
  {
    map< uint32, string >::iterator mit = user_names.find(it.object().id);
    if (mit != user_names.end())
      mit->second = it.object().name;
  }

  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator
      it = update_node_logger.insert_begin(); it != update_node_logger.insert_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator
      it = update_node_logger.keep_begin(); it != update_node_logger.keep_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
  for (map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::iterator
      it = update_node_logger.erase_begin(); it != update_node_logger.erase_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  

  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::iterator
      it = update_way_logger.insert_begin(); it != update_way_logger.insert_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::iterator
      it = update_way_logger.keep_begin(); it != update_way_logger.keep_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
  for (map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::iterator
      it = update_way_logger.erase_begin(); it != update_way_logger.erase_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  

  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator
      it = update_relation_logger.insert_begin(); it != update_relation_logger.insert_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator
      it = update_relation_logger.keep_begin(); it != update_relation_logger.keep_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
  for (map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::iterator
      it = update_relation_logger.erase_begin(); it != update_relation_logger.erase_end(); ++it)
  {
    if (it->second.second)
      it->second.second->user_name = user_names[it->second.second->user_id];
  }  
}


void print_meta(OSM_Element_Metadata* meta)
{
  if (!meta)
    return;

  uint32 year = (meta->timestamp)>>26;
  uint32 month = ((meta->timestamp)>>22) & 0xf;
  uint32 day = ((meta->timestamp)>>17) & 0x1f;
  uint32 hour = ((meta->timestamp)>>12) & 0x1f;
  uint32 minute = ((meta->timestamp)>>6) & 0x3f;
  uint32 second = meta->timestamp & 0x3f;
  string timestamp("    -  -  T  :  :  Z");
  timestamp[0] = (year / 1000) % 10 + '0';
  timestamp[1] = (year / 100) % 10 + '0';
  timestamp[2] = (year / 10) % 10 + '0';
  timestamp[3] = year % 10 + '0';
  timestamp[5] = (month / 10) % 10 + '0';
  timestamp[6] = month % 10 + '0';
  timestamp[8] = (day / 10) % 10 + '0';
  timestamp[9] = day % 10 + '0';
  timestamp[11] = (hour / 10) % 10 + '0';
  timestamp[12] = hour % 10 + '0';
  timestamp[14] = (minute / 10) % 10 + '0';
  timestamp[15] = minute % 10 + '0';
  timestamp[17] = (second / 10) % 10 + '0';
  timestamp[18] = second % 10 + '0';
  
  cout<<" version=\""<<meta->version<<"\""
        " timestamp=\""<<timestamp<<"\""
        " changeset=\""<<meta->changeset<<"\""
        " uid=\""<<meta->user_id<<"\""
        " user=\""<<escape_xml(meta->user_name)<<"\"";
}


void print_tags(const vector< pair< string, string > >& tags)
{
  for (vector< pair< string, string > >::const_iterator tit = tags.begin(); tit != tags.end(); ++tit)
    cout<<"    <tag k=\""<<escape_xml(tit->first)<<"\" v=\""<<escape_xml(tit->second)<<"\"/>\n";
}


void print_node(const Node& node, OSM_Element_Metadata* meta)
{
  cout<<
  "  <node id=\""<<node.id.val()<<"\" "
  "lat=\""<<fixed<<setprecision(7)<<::lat(node.index, node.ll_lower_)<<"\" "
  "lon=\""<<fixed<<setprecision(7)<<::lon(node.index, node.ll_lower_)<<"\"";
  print_meta(meta);
  if (node.tags.empty())
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    print_tags(node.tags);
    cout<<"  </node>\n";
  }
}


void print_way(const Way& way, OSM_Element_Metadata* meta)
{
  cout<<"  <way id=\""<<way.id.val()<<"\"";
  print_meta(meta);
  cout<<">\n";
  for (vector< Node::Id_Type >::const_iterator it = way.nds.begin(); it != way.nds.end(); ++it)
    cout<<"    <nd ref=\""<<it->val()<<"\"/>\n";
  print_tags(way.tags);
  cout<<"  </way>\n";
}


const char* MEMBER_TYPE[] = { 0, "node", "way", "relation" };

void print_relation(const Relation& relation, OSM_Element_Metadata* meta,
		    const vector< string >& relation_roles)
{
  cout<<"  <relation id=\""<<relation.id.val()<<"\"";
  print_meta(meta);
  cout<<">\n";
  for (vector< Relation_Entry >::const_iterator it = relation.members.begin();
       it != relation.members.end(); ++it)
    cout<<"    <member "
          "type=\""<<MEMBER_TYPE[it->type]<<"\" "
	  "ref=\""<<it->ref.val()<<"\" "
	  "role=\""<<escape_xml(relation_roles[it->role])<<"\"/>\n";
  print_tags(relation.tags);
  cout<<"  </relation>\n";
}


typedef enum { none, erase, keep, insert } Diff_State;


Diff_State change_diff_state(Diff_State state, Diff_State target)
{
  if (state != target)
  {
    if (state == erase)
      cout<<"</erase>\n";
    else if (state == keep)
      cout<<"</keep>\n";
    else if (state == insert)
      cout<<"</insert>\n";
    
    if (target == erase)
      cout<<"<erase>\n";
    else if (target == keep)
      cout<<"<keep>\n";
    else if (target == insert)
      cout<<"<insert>\n";
  }
  return target;
}


void print_augmented_diff(Update_Node_Logger& update_node_logger,
			  Update_Way_Logger& update_way_logger,
			  Update_Relation_Logger& update_relation_logger)
{
  cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<osmAugmentedDiff version=\"0.6\" generator=\"Overpass API\">\n"
  "<note>The data included in this document is from www.openstreetmap.org. "
  "The data is made available under ODbL.</note>\n"
  "<meta osm_base=\""<<data_version<<"\"/>\n\n";

  Diff_State state = none;

  {
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
        erase_it = update_node_logger.erase_begin();
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
        keep_it = update_node_logger.keep_begin();
    map< Node::Id_Type, pair< Node, OSM_Element_Metadata* > >::const_iterator
        insert_it = update_node_logger.insert_begin();
	
    while (true)
    {
      if (erase_it != update_node_logger.erase_end())
      {
	if (keep_it != update_node_logger.keep_end() && keep_it->first < erase_it->first)
	{
	  if (insert_it != update_node_logger.insert_end() && insert_it->first < keep_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_node(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, keep);
	    print_node(keep_it->second.first, keep_it->second.second);
	    ++keep_it;
	  }
	}
	else
	{
	  if (insert_it != update_node_logger.insert_end() && insert_it->first < erase_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_node(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, erase);
	    print_node(erase_it->second.first, erase_it->second.second);
	    ++erase_it;
	  }
	}
      }
      else
      {
	if (keep_it != update_node_logger.keep_end())
	{
	  if (insert_it != update_node_logger.insert_end() && insert_it->first < keep_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_node(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, keep);
	    print_node(keep_it->second.first, keep_it->second.second);
	    ++keep_it;
	  }
	}
	else
	{
	  if (insert_it != update_node_logger.insert_end())
	  {
	    state = change_diff_state(state, insert);
	    print_node(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	    break;
	}
      }
    }
  }

  {
    map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
        erase_it = update_way_logger.erase_begin();
    map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
        keep_it = update_way_logger.keep_begin();
    map< Way::Id_Type, pair< Way, OSM_Element_Metadata* > >::const_iterator
        insert_it = update_way_logger.insert_begin();
	
    while (true)
    {
      if (erase_it != update_way_logger.erase_end())
      {
	if (keep_it != update_way_logger.keep_end() && keep_it->first < erase_it->first)
	{
	  if (insert_it != update_way_logger.insert_end() && insert_it->first < keep_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_way(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, keep);
	    print_way(keep_it->second.first, keep_it->second.second);
	    ++keep_it;
	  }
	}
	else
	{
	  if (insert_it != update_way_logger.insert_end() && insert_it->first < erase_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_way(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, erase);
	    print_way(erase_it->second.first, erase_it->second.second);
	    ++erase_it;
	  }
	}
      }
      else
      {
	if (keep_it != update_way_logger.keep_end())
	{
	  if (insert_it != update_way_logger.insert_end() && insert_it->first < keep_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_way(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, keep);
	    print_way(keep_it->second.first, keep_it->second.second);
	    ++keep_it;
	  }
	}
	else
	{
	  if (insert_it != update_way_logger.insert_end())
	  {
	    state = change_diff_state(state, insert);
	    print_way(insert_it->second.first, insert_it->second.second);
	    ++insert_it;
	  }
	  else
	    break;
	}
      }
    }
  }

  vector< string > relation_roles = relation_updater->get_roles();
  
  {
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
        erase_it = update_relation_logger.erase_begin();
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
        keep_it = update_relation_logger.keep_begin();
    map< Relation::Id_Type, pair< Relation, OSM_Element_Metadata* > >::const_iterator
        insert_it = update_relation_logger.insert_begin();
	
    while (true)
    {
      if (erase_it != update_relation_logger.erase_end())
      {
	if (keep_it != update_relation_logger.keep_end() && keep_it->first < erase_it->first)
	{
	  if (insert_it != update_relation_logger.insert_end() && insert_it->first < keep_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_relation(insert_it->second.first, insert_it->second.second, relation_roles);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, keep);
	    print_relation(keep_it->second.first, keep_it->second.second, relation_roles);
	    ++keep_it;
	  }
	}
	else
	{
	  if (insert_it != update_relation_logger.insert_end() && insert_it->first < erase_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_relation(insert_it->second.first, insert_it->second.second, relation_roles);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, erase);
	    print_relation(erase_it->second.first, erase_it->second.second, relation_roles);
	    ++erase_it;
	  }
	}
      }
      else
      {
	if (keep_it != update_relation_logger.keep_end())
	{
	  if (insert_it != update_relation_logger.insert_end() && insert_it->first < keep_it->first)
	  {
	    state = change_diff_state(state, insert);
	    print_relation(insert_it->second.first, insert_it->second.second, relation_roles);
	    ++insert_it;
	  }
	  else
	  {
	    state = change_diff_state(state, keep);
	    print_relation(keep_it->second.first, keep_it->second.second, relation_roles);
	    ++keep_it;
	  }
	}
	else
	{
	  if (insert_it != update_relation_logger.insert_end())
	  {
	    state = change_diff_state(state, insert);
	    print_relation(insert_it->second.first, insert_it->second.second, relation_roles);
	    ++insert_it;
	  }
	  else
	    break;
	}
      }
    }
  }
  
  change_diff_state(state, none);

  cout<<"\n</osmAugmentedDiff>\n";
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

Osm_Updater::Osm_Updater(Osm_Backend_Callback* callback_, const string& data_version_,
			 bool meta_, bool produce_augmented_diffs)
  : dispatcher_client(0), meta(meta_)
{
  dispatcher_client = new Dispatcher_Client(osm_base_settings().shared_name);
  Logger logger(dispatcher_client->get_db_dir());
  logger.annotated_log("write_start() start version='" + data_version_ + '\'');
  dispatcher_client->write_start();
  logger.annotated_log("write_start() end");
  transaction = new Nonsynced_Transaction
      (true, true, dispatcher_client->get_db_dir(), "");
  {
    ofstream version((dispatcher_client->get_db_dir()
        + "osm_base_version.shadow").c_str());
    version<<data_version_<<'\n';
  }

  node_updater_ = new Node_Updater(*transaction, meta);
  update_node_logger_ = (produce_augmented_diffs ? new Update_Node_Logger() : 0);
  way_updater_ = new Way_Updater(*transaction, meta);
  update_way_logger_ = (produce_augmented_diffs ? new Update_Way_Logger() : 0);
  relation_updater_ = new Relation_Updater(*transaction, meta);
  update_relation_logger_ = (produce_augmented_diffs ? new Update_Relation_Logger() : 0);
  
  data_version = data_version_;

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
    (Osm_Backend_Callback* callback_, string db_dir, const string& data_version_,
     bool meta_, bool produce_augmented_diffs)
  : transaction(0), dispatcher_client(0), db_dir_(db_dir), meta(meta_)
{
  {
    ofstream version((db_dir + "osm_base_version").c_str());
    version<<data_version_<<'\n';
  }
  
  node_updater_ = new Node_Updater(db_dir, meta);
  update_node_logger_ = (produce_augmented_diffs ? new Update_Node_Logger() : 0);
  way_updater_ = new Way_Updater(db_dir, meta);
  update_way_logger_ = (produce_augmented_diffs ? new Update_Way_Logger() : 0);
  relation_updater_ = new Relation_Updater(db_dir, meta);
  update_relation_logger_ = (produce_augmented_diffs ? new Update_Relation_Logger() : 0);

  data_version = data_version_;

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
  if (transaction && update_node_logger && update_way_logger && update_relation_logger)
  {
    collect_kept_members(*transaction, *update_node_logger, *update_way_logger, *update_relation_logger);
    complete_user_data(*transaction, *update_node_logger, *update_way_logger, *update_relation_logger);
    print_augmented_diff(*update_node_logger, *update_way_logger, *update_relation_logger);
  }
  
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
