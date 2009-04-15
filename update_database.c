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
#include "relation_strings_file_io.h"
#include "script_datatypes.h"
#include "way_strings_file_io.h"

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

void load_member_roles(map< string, uint >& member_roles)
{
  member_roles.clear();

  int data_fd = open64(((string)DATADIR + MEMBER_ROLES_FILENAME).c_str(), O_RDONLY);
  if (data_fd < 0)
    throw File_Error(errno, ((string)DATADIR + MEMBER_ROLES_FILENAME), "prepare_caches:1");
  
  uint pos(0);
  uint16 size(0);
  char* buf = (char*) malloc(65536);
  while (read(data_fd, &size, 2))
  {
    read(data_fd, buf, size);
    member_roles[string(buf, size)] = pos++;
  }
  
  close(data_fd);
}

//-----------------------------------------------------------------------------

uint edit_status(0);
const uint CREATE = 1;
const uint DELETE = 2;
const uint MODIFY = 3;
uint32 current_node(0);
uint32 current_ll_idx(0);

set< uint32 > t_delete_nodes;
set< Node > new_nodes;
map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > > new_nodes_tags;
map< pair< string, string >, pair< uint32, uint32 >* > new_node_tags_ids;

Way_ current_way(0, 0);
set< uint32 > t_delete_ways;
set< Way_ > new_ways_floating;
map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > > new_ways_tags;
map< pair< string, string >, pair< uint32, uint32 >* > new_way_tags_ids;

Relation_ current_relation(0);
set< uint32 > t_delete_relations;
set< Relation_ > new_relations;
map< string, uint > member_roles;
map< uint32, vector< pair< uint32, uint32 >* > > new_relations_tags;
map< pair< string, string >, pair< uint32, uint32 >* > new_relation_tags_ids;

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
  {
    string key(""), value("");
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	key = attr[i+1];
      if (!strcmp(attr[i], "v"))
	value = attr[i+1];
    }
    if (current_node)
    {
      pair< uint32, uint32 >* coord_id(NULL);
      if (new_node_tags_ids.find(make_pair< string, string >(key, value)) == new_node_tags_ids.end())
      {
	coord_id = new pair< uint32, uint32 >((current_ll_idx & 0xffffff00), 0xffffffff);
        new_node_tags_ids[make_pair< string, string >(key, value)] = coord_id;
      }
      else
      {
        coord_id = new_node_tags_ids.find(make_pair< string, string >(key, value))->second;
	if (coord_id->first != (current_ll_idx & 0xffffff00))
	  coord_id->first = 0xffffffff;
      }
      new_nodes_tags[make_pair< uint32, uint32 >(current_node, current_ll_idx)].push_back(coord_id);
    }
    else if (current_way.head.second)
    {
      pair< uint32, uint32 >* coord_id(NULL);
      if (new_way_tags_ids.find(make_pair< string, string >(key, value)) == new_way_tags_ids.end())
      {
        coord_id = new pair< uint32, uint32 >(0, 0xffffffff);
        new_way_tags_ids[make_pair< string, string >(key, value)] = coord_id;
      }
      else
        coord_id = new_way_tags_ids.find(make_pair< string, string >(key, value))->second;
      new_ways_tags[make_pair< uint32, uint32 >(current_way.head.second, 0)].push_back(coord_id);
    }
    else if (current_relation.head)
    {
      pair< uint32, uint32 >* coord_id(NULL);
      if (new_relation_tags_ids.find(make_pair< string, string >(key, value)) ==
          new_relation_tags_ids.end())
      {
        coord_id = new pair< uint32, uint32 >(0, 0xffffffff);
        new_relation_tags_ids[make_pair< string, string >(key, value)] = coord_id;
      }
      else
        coord_id = new_relation_tags_ids.find(make_pair< string, string >(key, value))->second;
      new_relations_tags[current_relation.head].push_back(coord_id);
    }
  }
  else if (!strcmp(el, "nd"))
  {
    if (current_way.head.second)
    {
      unsigned int ref(0);
      for (unsigned int i(0); attr[i]; i += 2)
      {
        if (!strcmp(attr[i], "ref"))
          ref = atoi(attr[i+1]);
      }
      current_way.data.push_back(ref);
    }
  }
  else if (!strcmp(el, "member"))
  {
    if (current_relation.head)
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
      uint role_id(0);
      map< string, uint >::const_iterator it(member_roles.find(role));
      if (it != member_roles.end())
        role_id = it->second;
      else
      {
        role_id = member_roles.size();
        member_roles.insert(make_pair< string, uint >(role, role_id));
      }
      if (type == "node")
        current_relation.data.push_back(Relation_Member(ref, Relation_Member::NODE, role_id));
      else if (type == "way")
        current_relation.data.push_back(Relation_Member(ref, Relation_Member::WAY, role_id));
      else if (type == "relation")
        current_relation.data.push_back(Relation_Member(ref, Relation_Member::RELATION, role_id));
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
    t_delete_nodes.insert(id);
    if ((edit_status == CREATE) || (edit_status == MODIFY))
    {
      new_nodes.insert(Node(id, lat, lon));
      current_node = id;
      current_ll_idx = ll_idx(lat, lon);
    }
  }
  else if (!strcmp(el, "way"))
  {
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
        id = atoi(attr[i+1]);
    }
    t_delete_ways.insert(id);
    if ((edit_status == CREATE) || (edit_status == MODIFY))
      current_way.head.second = id;
  }
  else if (!strcmp(el, "relation"))
  {
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
        id = atoi(attr[i+1]);
    }
    t_delete_relations.insert(id);
    if ((edit_status == CREATE) || (edit_status == MODIFY))
      current_relation.head = id;
  }
  else if (!strcmp(el, "create"))
    edit_status = CREATE;
  else if (!strcmp(el, "modify"))
    edit_status = MODIFY;
  else if (!strcmp(el, "delete"))
    edit_status = DELETE;
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
    current_node = 0;
  else if (!strcmp(el, "way"))
  {
    if (!(current_way.data.empty()))
      new_ways_floating.insert(current_way);
    current_way.head.second = 0;
    current_way.data.clear();
  }
  else if (!strcmp(el, "relation"))
  {
    if (!(current_relation.data.empty()))
      new_relations.insert(current_relation);
    current_relation.head = 0;
    current_relation.data.clear();
  }
  else if (!strcmp(el, "create"))
    edit_status = 0;
  else if (!strcmp(el, "modify"))
    edit_status = 0;
  else if (!strcmp(el, "delete"))
    edit_status = 0;
}

int main(int argc, char *argv[])
{
  set< Node > delete_nodes;
  set< pair< uint32, uint32 > > moved_local_ids;
  
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  try
  {
    //load existing member roles
    load_member_roles(member_roles);
    
    //reading the main document
    parse(stdin, start, end);
    
    for (set< uint32 >::const_iterator it(t_delete_relations.begin());
         it != t_delete_relations.end(); ++it)
      cout<<*it<<'\n';
    cout<<"---\n";
    for (set< Relation_ >::const_iterator it(new_relations.begin()); it != new_relations.end(); ++it)
    {
      cout<<it->head<<'\n';
      for (vector< Relation_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
        cout<<'\t'<<it2->id<<'\t'<<it2->type<<'\t'<<it2->role<<'\n';
    }
    cout<<"---\n";
    for (map< string, uint >::const_iterator it(member_roles.begin()); it != member_roles.end(); ++it)
      cout<<'['<<it->first<<"]["<<it->second<<"]\n";
    cout<<"---\n";
    for (map< uint32, vector< pair< uint32, uint32 >* > >::const_iterator it(new_relations_tags.begin());
         it != new_relations_tags.end(); ++it)
    {
      cout<<it->first<<'\n';
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
        cout<<'\t'<<(*it2)<<'\n';
    }
    cout<<"---\n";
    for (map< pair< string, string >, pair< uint32, uint32 >* >::const_iterator
         it(new_relation_tags_ids.begin());
         it != new_relation_tags_ids.end(); ++it)
    {
      cout<<'['<<it->first.first<<"]["<<it->first.second<<"]\n";
      cout<<'\t'<<it->second<<'\n';
    }
    return 0;
    
    //retrieving old coordinates of the nodes that will be deleted
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Node_Id_Node_By_Id_Reader reader(t_delete_nodes, delete_nodes);
    select_by_id< Node_Id_Node_By_Id_Reader >(reader);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    vector< uint32 > local_id_idx;
    vector< uint32 > spatial_boundaries;
    node_string_delete_insert(new_node_tags_ids, moved_local_ids, local_id_idx, spatial_boundaries);
    cerr<<(uintmax_t)time(NULL)<<'\n';

    set< uint32 > delete_node_idxs;
    for (set< Node >::const_iterator it(delete_nodes.begin()); it != delete_nodes.end(); ++it)
      delete_node_idxs.insert(ll_idx(it->lat, it->lon) & 0xffffff00);
    map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > > local_ids;
    Tag_Id_MultiNode_Local_Reader local_reader(local_ids, delete_nodes, delete_node_idxs);
    select_with_idx< Tag_Id_MultiNode_Local_Reader >(local_reader);
    for (set< pair< uint32, uint32 > >::const_iterator it(moved_local_ids.begin());
         it != moved_local_ids.end(); ++it)
      local_ids[*it].second.insert(0);
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_nodes_tags.begin()); it != new_nodes_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first != 0xffffffff)
          local_ids[**it2].second.insert(it->first.first);
      }
    }
    
    cerr<<(uintmax_t)time(NULL)<<'\n';
    map< uint32, set< uint32 > > moved_ids;
    Tag_Id_Node_Local_Updater id_node_local_updater
	(local_ids, moved_ids, local_id_idx, spatial_boundaries);
    delete_insert< Tag_Id_Node_Local_Updater >(id_node_local_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Id_Node_Local_Updater >(id_node_local_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    //retrieving old coordinates of the nodes that appear in local ids which are moving to global
    set< uint32 > t_move_involved_nodes;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
	 it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
	t_move_involved_nodes.insert(*it2);
    }
    set< Node > move_involved_nodes;
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Node_Id_Node_By_Id_Reader reader2(t_move_involved_nodes, move_involved_nodes);
    select_by_id< Node_Id_Node_By_Id_Reader >(reader2);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    map< pair< uint32, uint32 >, pair< set< uint32 >, uint > > global_nodes_to_be_edited;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
	 it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      {
	set< Node >::const_iterator nit(move_involved_nodes.find(Node(*it2, 0, 0)));
	if (nit == move_involved_nodes.end())
	  continue;
	pair< set< uint32 >, uint >& tail(global_nodes_to_be_edited[make_pair< uint32, uint32 >
	    (ll_idx(nit->lat, nit->lon), nit->id)]);
	tail.first.insert(it->first);
	tail.second = Tag_Node_Id_Updater::UPDATE;
      }
    }
    for (set< Node >::const_iterator it(delete_nodes.begin());
	 it != delete_nodes.end(); ++it)
      global_nodes_to_be_edited[make_pair< uint32, uint32 >
	  (ll_idx(it->lat, it->lon), it->id)] = make_pair< set< uint32 >, uint >
	      (set< uint32 >(), Tag_Node_Id_Updater::DELETE);
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
	 it(new_nodes_tags.begin()); it != new_nodes_tags.end(); ++it)
    {
      set< uint32 > global_ids;
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
	   it2 != it->second.end(); ++it2)
      {
	if ((*it2)->first == 0xffffffff)
	  global_ids.insert((*it2)->second);
      }
      global_nodes_to_be_edited[make_pair< uint32, uint32 >
	  (it->first.second, it->first.first)] = make_pair< set< uint32 >, uint >
	      (global_ids, Tag_Node_Id_Updater::INSERT);
    }
    
    cerr<<(uintmax_t)time(NULL)<<'\n';
    map< uint32, set< uint32 > > deleted_nodes_ids;
    Tag_Node_Id_Updater node_id_updater
	(global_nodes_to_be_edited, deleted_nodes_ids);
    delete_insert< Tag_Node_Id_Updater >(node_id_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Node_Id_Updater >(node_id_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    map< uint32, pair< set< uint32 >, set< uint32 > > > global_ids_to_be_edited;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
	 it != moved_ids.end(); ++it)
    {
      global_ids_to_be_edited[it->first] = make_pair< set< uint32 >, set< uint32 > >
        (set< uint32 >(), set< uint32 >());
      set< uint32 >& inserted_nodes(global_ids_to_be_edited[it->first].second);
      for (set< uint32 >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if (deleted_nodes_ids.find(*it2) == deleted_nodes_ids.end())
          inserted_nodes.insert(*it2);
      }
    }
    for (map< uint32, set< uint32 > >::const_iterator it(deleted_nodes_ids.begin());
	 it != deleted_nodes_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
        global_ids_to_be_edited[*it2].first.insert(it->first);
    }
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_nodes_tags.begin()); it != new_nodes_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0xffffffff)
        {
          pair< set< uint32 >, set< uint32 > >& tail(global_ids_to_be_edited[(*it2)->second]);
          if (tail.first.find(it->first.first) == tail.first.end())
            tail.second.insert(it->first.first);
          else
            tail.first.erase(it->first.first);
        }
      }
    }
    
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Tag_Id_Node_Global_Updater id_global_updater(global_ids_to_be_edited);
    delete_insert< Tag_Id_Node_Global_Updater >(id_global_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Id_Node_Global_Updater >(id_global_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    new_nodes_tags.clear();
    for (map< pair< string, string >, pair< uint32, uint32 >* >::iterator it(new_node_tags_ids.begin());
         it != new_node_tags_ids.end(); ++it)
      delete(it->second);
    new_node_tags_ids.clear();
    
    //updating the nodes file
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Node_Id_Node_Updater node_updater(delete_nodes, new_nodes);
    delete_insert< Node_Id_Node_Updater >(node_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Node_Id_Node_Updater >(node_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    update_id_index< Node_Id_Node_Updater >(node_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    node_tag_id_statistics_remake();
    
/*    vector< uint32 > local_id_idx;
    vector< uint32 > spatial_boundaries;
    map< pair< uint32, uint32 >, pair< set< uint32 >, set< uint32 > > > local_ids;
    map< uint32, set< uint32 > > moved_ids;
    map< uint32, pair< set< uint32 >, set< uint32 > > > global_ids_to_be_edited;*/
    
    //computing coordinates of the new ways
    //query used nodes
    set< Node > used_nodes;
    set< uint32 > used_nodes_ids;
    for (set< Way_ >::const_iterator it(new_ways_floating.begin());
         it != new_ways_floating.end(); ++it)
    {
      for (vector< Way_::Data >::const_iterator it2((*it).data.begin());
           it2 != (*it).data.end(); ++it2)
        used_nodes_ids.insert(*it2);
    }
    Node_Id_Node_By_Id_Reader nodes_reader(used_nodes_ids, used_nodes);
    select_by_id< Node_Id_Node_By_Id_Reader >(nodes_reader);
    used_nodes_ids.clear();
    
    //retrieving old coordinates of the ways that will be deleted
    cerr<<(uintmax_t)time(NULL)<<'\n';
    set< Way_ > delete_ways;
    Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< uint32 >, set< Way_ > >
      way_id_reader(t_delete_ways, delete_ways);
    select_by_id(way_id_reader);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    //calculate for each way its index
    set< Way_ > new_ways;
    for (set< Way_ >::const_iterator it(new_ways_floating.begin()); it != new_ways_floating.end(); ++it)
    {
      Way_::Index bitmask(0), position(0);
      Way_ way(*it);
      for (vector< Way_::Data >::const_iterator it2(way.data.begin());
           it2 != way.data.end(); ++it2)
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
      way.head.first = position;
      new_ways.insert(way);
      map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
        tit(new_ways_tags.find(make_pair< uint32, uint32 >(way.head.second, 0)));
      if (tit != new_ways_tags.end())
      {
        new_ways_tags[make_pair< uint32, uint32 >(way.head.second, position)] = tit->second;
        new_ways_tags.erase(make_pair< uint32, uint32 >(way.head.second, 0));
      }
      
    }
    new_ways_floating.clear();
    
    //calculate for each way tag its index
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_ways_tags.begin()); it != new_ways_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0)
          (*it2)->first = (it->first.second & 0xffffff00);
        else if ((*it2)->first != (it->first.second & 0xffffff00))
          (*it2)->first = 0xffffffff;
      }
    }

    //updating the string dictionary of the way tags
    moved_local_ids.clear();
    local_id_idx.clear();
    spatial_boundaries.clear();
    way_string_delete_insert(new_way_tags_ids, moved_local_ids, local_id_idx, spatial_boundaries);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    //preparing the update of the local id to way data
    set< uint32 > delete_way_idxs;
    map< Way_::Id, Way_::Index > delete_ways_by_id;
    for (set< Way_ >::const_iterator it(delete_ways.begin()); it != delete_ways.end(); ++it)
    {
      delete_way_idxs.insert(it->head.first & 0xffffff00);
      delete_ways_by_id[it->head.second] = it->head.first;
    }
    local_ids.clear();
    Tag_Id_MultiWay_Local_Reader local_id_way_reader(local_ids, delete_ways_by_id, delete_way_idxs);
    select_with_idx< Tag_Id_MultiWay_Local_Reader >(local_id_way_reader);
    for (set< pair< uint32, uint32 > >::const_iterator it(moved_local_ids.begin());
         it != moved_local_ids.end(); ++it)
      local_ids[*it].second.insert(0);
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_ways_tags.begin()); it != new_ways_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first != 0xffffffff)
          local_ids[**it2].second.insert(it->first.first);
      }
    }
    
    //updating of the local id to way data
    cerr<<(uintmax_t)time(NULL)<<'\n';
    moved_ids.clear();
    Tag_Id_Way_Local_Updater id_way_local_updater
      (local_ids, moved_ids, local_id_idx, spatial_boundaries);
    delete_insert< Tag_Id_Way_Local_Updater >(id_way_local_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Id_Way_Local_Updater >(id_way_local_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    //retrieving old coordinates of the ways that appear in local ids which are moving to global
    set< uint32 > t_move_involved_ways;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
        t_move_involved_ways.insert(*it2);
    }
    set< Way_ > move_involved_ways;
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< uint32 >, set< Way_ > >
      way_id_reader2(t_move_involved_ways, move_involved_ways);
    select_by_id(way_id_reader2);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    map< Way_::Id, Way_::Index > move_involved_ways_by_id;
    for (set< Way_ >::const_iterator it(move_involved_ways.begin()); it != move_involved_ways.end(); ++it)
      move_involved_ways_by_id[it->head.second] = it->head.first;
    
    //preparing the update of the global way to id data
    map< pair< uint32, uint32 >, pair< set< uint32 >, uint > > global_ways_to_be_edited;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      {
        map< Way_::Id, Way_::Index >::const_iterator nit(move_involved_ways_by_id.find(*it2));
        if (nit == move_involved_ways_by_id.end())
          continue;
        pair< set< uint32 >, uint >& tail(global_ways_to_be_edited[make_pair< uint32, uint32 >
          (nit->second, nit->first)]);
        tail.first.insert(it->first);
        tail.second = Tag_Way_Id_Updater::UPDATE;
      }
    }
    for (set< Way_ >::const_iterator it(delete_ways.begin());
         it != delete_ways.end(); ++it)
      global_ways_to_be_edited[make_pair< uint32, uint32 >
                                (it->head.first, it->head.second)] = make_pair< set< uint32 >, uint >
      (set< uint32 >(), Tag_Way_Id_Updater::DELETE);
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_ways_tags.begin()); it != new_ways_tags.end(); ++it)
    {
      set< uint32 > global_ids;
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0xffffffff)
          global_ids.insert((*it2)->second);
      }
      global_ways_to_be_edited[make_pair< uint32, uint32 >
                                (it->first.second, it->first.first)] = make_pair< set< uint32 >, uint >
        (global_ids, Tag_Way_Id_Updater::INSERT);
    }
    
    //updating of the global way to id data
    cerr<<(uintmax_t)time(NULL)<<'\n';
    map< uint32, set< uint32 > > deleted_ways_ids;
    Tag_Way_Id_Updater way_id_updater
      (global_ways_to_be_edited, deleted_ways_ids);
    delete_insert< Tag_Way_Id_Updater >(way_id_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Way_Id_Updater >(way_id_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    //preparing the update of the global id to way data
    global_ids_to_be_edited.clear();
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      global_ids_to_be_edited[it->first] = make_pair< set< uint32 >, set< uint32 > >
        (set< uint32 >(), set< uint32 >());
      set< uint32 >& inserted_ways(global_ids_to_be_edited[it->first].second);
      for (set< uint32 >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if (deleted_ways_ids.find(*it2) == deleted_ways_ids.end())
          inserted_ways.insert(*it2);
      }
    }
    for (map< uint32, set< uint32 > >::const_iterator it(deleted_ways_ids.begin());
         it != deleted_ways_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
        global_ids_to_be_edited[*it2].first.insert(it->first);
    }
    for (map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_ways_tags.begin()); it != new_ways_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0xffffffff)
        {
          pair< set< uint32 >, set< uint32 > >& tail(global_ids_to_be_edited[(*it2)->second]);
          if (tail.first.find(it->first.first) == tail.first.end())
            tail.second.insert(it->first.first);
          else
            tail.first.erase(it->first.first);
        }
      }
    }
    
    //updating of the global id to way data
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Tag_Id_Way_Global_Updater id_way_global_updater(global_ids_to_be_edited);
    delete_insert< Tag_Id_Way_Global_Updater >(id_way_global_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Id_Way_Global_Updater >(id_way_global_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    new_ways_tags.clear();
    for (map< pair< string, string >, pair< uint32, uint32 >* >::iterator it(new_way_tags_ids.begin());
         it != new_way_tags_ids.end(); ++it)
      delete(it->second);
    new_way_tags_ids.clear();
    
    //updating the ways file
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Indexed_Ordered_Id_To_Many_Updater< Way_Storage, set< Way_ > > way_updater(delete_ways, new_ways);
    delete_insert(way_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index(way_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    update_id_index(way_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
    way_tag_id_statistics_remake();
  }
  catch(File_Error e)
  {
    cerr<<"\nopen64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
