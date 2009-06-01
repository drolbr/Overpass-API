#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../expat_justparse_interface.h"
#include "../script_datatypes.h"
#include "file_types.h"
#include "node_strings_file_io.h"
#include "raw_file_db.h"
#include "relation_strings_file_io.h"
#include "way_strings_file_io.h"

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

void multiNode_to_multiWay_query
(const set< Node >& source_1, const set< Node >& source_2, set< Way_ >& result_set)
{
  set< Way_Storage::Index > ll_idxs;
  for (set< Node >::const_iterator it(source_1.begin()); it != source_1.end(); ++it)
  {
    Way_Storage::Index ll_idx_(ll_idx(it->lat, it->lon));
    ll_idxs.insert(ll_idx_);
    ll_idxs.insert(0x88000000 | (ll_idx_>>8));
    ll_idxs.insert(0x88880000 | (ll_idx_>>16));
    ll_idxs.insert(0x88888800 | (ll_idx_>>24));
  }
  ll_idxs.insert(0x00000001);
  ll_idxs.insert(0x88000000);
  ll_idxs.insert(0x88880000);
  ll_idxs.insert(0x88888800);
  ll_idxs.insert(0x88888888);
  
  set< Way_ > result;
  Indexed_Ordered_Id_To_Many_Index_Reader< Way_Storage, set< Way_Storage::Index >, set< Way_ > >
    reader(ll_idxs, result);
  select_with_idx(reader);
  
  for (set< Way_ >::const_iterator it(result.begin()); it != result.end(); ++it)
  {
    bool is_referred(false);
    for (vector< Way_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
    {
      if ((source_1.find(Node(*it2, 0, 0)) != source_1.end()) ||
          (source_2.find(Node(*it2, 0, 0)) != source_2.end()))
      {
        is_referred = true;
        break;
      }
    }
    if (!is_referred)
      continue;
    result_set.insert(*it);
  }
}

void load_member_roles(map< string, uint >& member_roles)
{
  member_roles.clear();

  int data_fd = open64((DATADIR + db_subdir + MEMBER_ROLES_FILENAME).c_str(),
                       O_RDONLY);
  if (data_fd < 0)
    throw File_Error(errno, (DATADIR + db_subdir + MEMBER_ROLES_FILENAME),
                     "load_member_roles:1");
  
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

void dump_member_roles(const map< string, uint >& member_roles)
{
  vector< string > roles_by_id(member_roles.size());
  for (map< string, uint >::const_iterator it(member_roles.begin());
       it != member_roles.end(); ++it)
    roles_by_id[it->second] = it->first;
  
  int dest_fd = open64((DATADIR + db_subdir + MEMBER_ROLES_FILENAME).c_str(),
                       O_WRONLY|O_CREAT|O_TRUNC,
                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  close(dest_fd);
  
  dest_fd = open64((DATADIR + db_subdir + MEMBER_ROLES_FILENAME).c_str(),
                   O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (dest_fd < 0)
    throw File_Error(errno, DATADIR + db_subdir + MEMBER_ROLES_FILENAME,
                     "dump_member_roles:1");
  
  for (vector< string >::const_iterator it(roles_by_id.begin());
       it != roles_by_id.end(); ++it)
  {
    uint16 size(it->size());
    write(dest_fd, &(size), sizeof(uint16));
    write(dest_fd, it->data(), size);
  }
  
  close(dest_fd);
}

//-----------------------------------------------------------------------------

uint edit_status(0);
const uint CREATE = 1;
const uint DELETE = 2;
const uint MODIFY = 3;
uint32 current_node(0);
uint32 current_ll_idx(0);

set< uint32 > delete_node_ids;
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
    delete_node_ids.insert(id);
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

string db_subdir;

int main(int argc, char *argv[])
{
  bool intermediate_run(false);
  
  int argpos(1);
  while (argpos < argc)
  {
    intermediate_run |= ((string)"--intermediate" == argv[argpos]);
    if (!(strncmp(argv[argpos], "--db=", 5)))
    {
      db_subdir = ((string)argv[argpos]).substr(5);
      if ((db_subdir.size() > 0) && (db_subdir[db_subdir.size()-1] != '/'))
        db_subdir += '/';
    }
    ++argpos;
  }
  
  set< Node > delete_nodes;
  set< pair< uint32, uint32 > > moved_local_ids;
  
  //cerr<<(uintmax_t)time(NULL)<<'\n';
  
  try
  {
    cerr<<'a';
    //load existing member roles
    load_member_roles(member_roles);
    
    cerr<<'b';
    //reading the main document
    parse(stdin, start, end);
    
    cerr<<'c';
    //save updated member roles
    dump_member_roles(member_roles);
    member_roles.clear();
    
    cerr<<'d';
    //retrieving old coordinates of the nodes that will be deleted
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    Node_Id_Node_By_Id_Reader reader(delete_node_ids, delete_nodes);
    select_by_id< Node_Id_Node_By_Id_Reader >(reader);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    vector< uint32 > local_id_idx;
    vector< uint32 > spatial_boundaries;
    node_string_delete_insert(new_node_tags_ids, moved_local_ids, local_id_idx, spatial_boundaries);
    //cerr<<(uintmax_t)time(NULL)<<'\n';

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
    
    map< uint32, set< uint32 > > moved_ids;
    {
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Tag_Id_Node_Local_Updater id_node_local_updater
	  (local_ids, moved_ids, local_id_idx, spatial_boundaries);
      delete_insert< Tag_Id_Node_Local_Updater >(id_node_local_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Tag_Id_Node_Local_Updater >(id_node_local_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }
    
    cerr<<'e';
    //retrieving old coordinates of the nodes that appear in local ids which are moving to global
    set< uint32 > t_move_involved_nodes;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
	 it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
	t_move_involved_nodes.insert(*it2);
    }
    set< Node > move_involved_nodes;
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    Node_Id_Node_By_Id_Reader reader2(t_move_involved_nodes, move_involved_nodes);
    select_by_id< Node_Id_Node_By_Id_Reader >(reader2);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'f';
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
    
    cerr<<'g';
    map< uint32, set< uint32 > > deleted_nodes_ids;
    {
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Tag_Node_Id_Updater node_id_updater
	  (global_nodes_to_be_edited, deleted_nodes_ids);
      delete_insert< Tag_Node_Id_Updater >(node_id_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Tag_Node_Id_Updater >(node_id_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }
    
    cerr<<'h';
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
    
    cerr<<'i';
    //give id_global_updater a local scope to save memory
/*    {
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Tag_Id_Node_Global_Updater id_global_updater(global_ids_to_be_edited);
      delete_insert< Tag_Id_Node_Global_Updater >(id_global_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Tag_Id_Node_Global_Updater >(id_global_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }*/
    
    new_nodes_tags.clear();
    for (map< pair< string, string >, pair< uint32, uint32 >* >::iterator it(new_node_tags_ids.begin());
         it != new_node_tags_ids.end(); ++it)
      delete(it->second);
    new_node_tags_ids.clear();
    
    cerr<<'j';
    //give node_updater a local scope to save memory
    {
      //updating the nodes file
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Node_Id_Node_Updater node_updater(delete_nodes, new_nodes);
      delete_insert< Node_Id_Node_Updater >(node_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Node_Id_Node_Updater >(node_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      update_id_index< Node_Id_Node_Updater >(node_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }
    
    if (!intermediate_run)
      node_tag_id_statistics_remake();
    
    cerr<<'k';
    //collect ways that may be affected by moving nodes
    //patch any way that is not sceduled to be deleted into new_ways_floating
    //cerr<<(uintmax_t)time(NULL)<<" (entering ways)"<<'\n';
    set< Way_ > touched_ways;
    vector< Way_ > way_coords;
    cerr<<'k';
    multiNode_to_multiWay_query(delete_nodes, new_nodes, touched_ways);
    cerr<<'k';
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    for (set< Way_ >::const_iterator it(touched_ways.begin());
         it != touched_ways.end(); ++it)
    {
      if (t_delete_ways.find(it->head.second) == t_delete_ways.end())
      {
        t_delete_ways.insert(it->head.second);
        new_ways_floating.insert(*it);
        way_coords.push_back(*it);
      }
    }
    
    cerr<<'l';
    //get the tags for the node move affected ways
    set< uint32 > way_idxs;
    for (vector< Way_ >::const_iterator it(way_coords.begin()); it != way_coords.end(); ++it)
      way_idxs.insert(it->head.first & 0xffffff00);
    
    map< uint32, set< uint32 > > local_way_to_id;
    for (vector< Way_ >::const_iterator it(way_coords.begin()); it != way_coords.end(); ++it)
      local_way_to_id[it->head.second] = set< uint32 >();
    
    map< uint32, set< uint32 > > global_way_to_id;
    set< uint32 > global_way_idxs;
    for (vector< Way_ >::const_iterator it(way_coords.begin()); it != way_coords.end(); ++it)
    {
      global_way_to_id[it->head.second] = set< uint32 >();
      global_way_idxs.insert(it->head.first);
    }
    
    cerr<<'m';
    Tag_MultiWay_Id_Local_Reader local_reader_2(local_way_to_id, way_idxs);
    select_with_idx< Tag_MultiWay_Id_Local_Reader >(local_reader_2);
    Tag_Way_Id_Reader global_reader(global_way_to_id, global_way_idxs);
    select_with_idx< Tag_Way_Id_Reader >(global_reader);
    
    cerr<<'n';
    set< uint32 > ids_global;
    for (map< uint32, set< uint32 > >::const_iterator it(global_way_to_id.begin());
         it != global_way_to_id.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
        ids_global.insert(*it2);
    }
    sort(way_coords.begin(), way_coords.end(), Way_Less_By_Id());
    map< uint32, uint32 > ids_local;
    for (map< uint32, set< uint32 > >::const_iterator it(local_way_to_id.begin());
         it != local_way_to_id.end(); ++it)
    {
      const Way_& cur_wy(*(lower_bound
                           (way_coords.begin(), way_coords.end(), Way_(0, it->first), Way_Less_By_Id())));
      uint32 ll_idx_(cur_wy.head.first & 0xffffff00);
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
        ids_local[*it2] = ll_idx_;
    }
    map< uint32, pair< string, string > > kvs;
    
    select_way_ids_to_kvs(ids_local, ids_global, kvs);
    
    cerr<<'o';
    //insert found tags properly into new_way_tags and new_way_tag_ids
    for (vector< Way_ >::const_iterator it(way_coords.begin()); it != way_coords.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(local_way_to_id[it->head.second].begin());
           it2 != local_way_to_id[it->head.second].end(); ++it2)
      {
        pair< uint32, uint32 >* coord_id(NULL);
        if (new_way_tags_ids.find(kvs[*it2]) == new_way_tags_ids.end())
        {
          coord_id = new pair< uint32, uint32 >(0, 0xffffffff);
          new_way_tags_ids[kvs[*it2]] = coord_id;
        }
        else
          coord_id = new_way_tags_ids.find(kvs[*it2])->second;
        new_ways_tags[make_pair< uint32, uint32 >(it->head.second, 0)].push_back(coord_id);
      }
      for (set< uint32 >::const_iterator it2(global_way_to_id[it->head.second].begin());
           it2 != global_way_to_id[it->head.second].end(); ++it2)
      {
        pair< uint32, uint32 >* coord_id(NULL);
        if (new_way_tags_ids.find(kvs[*it2]) == new_way_tags_ids.end())
        {
          coord_id = new pair< uint32, uint32 >(0, 0xffffffff);
          new_way_tags_ids[kvs[*it2]] = coord_id;
        }
        else
          coord_id = new_way_tags_ids.find(kvs[*it2])->second;
        new_ways_tags[make_pair< uint32, uint32 >(it->head.second, 0)].push_back(coord_id);
      }
    }
    
    cerr<<'p';
    //computing coordinates of the new ways
    //query used nodes
    //cerr<<(uintmax_t)time(NULL)<<'\n';
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
    
    cerr<<'q';
    //retrieving old coordinates of the ways that will be deleted
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    set< Way_ > delete_ways;
    Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< uint32 >, set< Way_ > >
      way_id_reader(t_delete_ways, delete_ways);
    select_by_id(way_id_reader);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'r';
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
        uint32 ll_idx_(0x00000001);
        if (node_it != used_nodes.end())
        {
          ll_idx_ = ll_idx(node_it->lat, node_it->lon);
	  //otherwise this node is referenced but does not exist and is treated as
          //lying on position 0x00000001
        }
        if (position == 0)
          position = ll_idx_;
        else
          bitmask |= (position ^ ll_idx_);
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
    
    cerr<<'s';
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

    cerr<<'t';
    //updating the string dictionary of the way tags
    moved_local_ids.clear();
    local_id_idx.clear();
    spatial_boundaries.clear();
    way_string_delete_insert(new_way_tags_ids, moved_local_ids, local_id_idx, spatial_boundaries);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'u';
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
    
    cerr<<'v';
    {
      //updating of the local id to way data
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      moved_ids.clear();
      Tag_Id_Way_Local_Updater id_way_local_updater
	  (local_ids, moved_ids, local_id_idx, spatial_boundaries);
      delete_insert< Tag_Id_Way_Local_Updater >(id_way_local_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Tag_Id_Way_Local_Updater >(id_way_local_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }
    
    cerr<<'w';
    //retrieving old coordinates of the ways that appear in local ids which are moving to global
    set< uint32 > t_move_involved_ways;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
        t_move_involved_ways.insert(*it2);
    }
    set< Way_ > move_involved_ways;
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    Indexed_Ordered_Id_To_Many_By_Id_Reader< Way_Storage, set< uint32 >, set< Way_ > >
      way_id_reader2(t_move_involved_ways, move_involved_ways);
    select_by_id(way_id_reader2);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    map< Way_::Id, Way_::Index > move_involved_ways_by_id;
    for (set< Way_ >::const_iterator it(move_involved_ways.begin()); it != move_involved_ways.end(); ++it)
      move_involved_ways_by_id[it->head.second] = it->head.first;
    
    cerr<<'x';
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
    
    cerr<<'y';
    map< uint32, set< uint32 > > deleted_ways_ids;
    {
      //updating of the global way to id data
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Tag_Way_Id_Updater way_id_updater
	  (global_ways_to_be_edited, deleted_ways_ids);
      delete_insert< Tag_Way_Id_Updater >(way_id_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Tag_Way_Id_Updater >(way_id_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }
    
    cerr<<'z';
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
    
    cerr<<'A';
/*    {
      //updating of the global id to way data
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Tag_Id_Way_Global_Updater id_way_global_updater(global_ids_to_be_edited);
      delete_insert< Tag_Id_Way_Global_Updater >(id_way_global_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index< Tag_Id_Way_Global_Updater >(id_way_global_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }*/
    
    new_ways_tags.clear();
    for (map< pair< string, string >, pair< uint32, uint32 >* >::iterator it(new_way_tags_ids.begin());
         it != new_way_tags_ids.end(); ++it)
      delete(it->second);
    new_way_tags_ids.clear();
    
    cerr<<'B';
    {
      //updating the ways file
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      Indexed_Ordered_Id_To_Many_Updater< Way_Storage, set< Way_ > > way_updater(delete_ways, new_ways);
      delete_insert(way_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      make_block_index(way_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
      update_id_index(way_updater);
      //cerr<<(uintmax_t)time(NULL)<<'\n';
    }
    
    if (!intermediate_run)
      way_tag_id_statistics_remake();
  
    //computing coordinates of the new relations
    
    cerr<<'C';
    //retrieving old coordinates of the relations that will be deleted
    //cerr<<(uintmax_t)time(NULL)<<" (entering relations)"<<'\n';
    set< Relation_ > delete_relations;
    Indexed_Ordered_Id_To_Many_By_Id_Reader< Relation_Storage, set< uint32 >, set< Relation_ > >
      relation_id_reader(t_delete_relations, delete_relations);
    select_by_id(relation_id_reader);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'D';
    //calculate for each relation tag its index
    for (map< uint32, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_relations_tags.begin()); it != new_relations_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0)
          (*it2)->first = (it->first & 0xffffff00);
        else if ((*it2)->first != (it->first & 0xffffff00))
          (*it2)->first = 0xffffffff;
      }
    }
    
    cerr<<'E';
    //updating the string dictionary of the relation tags
    moved_local_ids.clear();
    local_id_idx.clear();
    spatial_boundaries.clear();
    relation_string_delete_insert(new_relation_tags_ids, moved_local_ids, local_id_idx, spatial_boundaries);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'F';
    //preparing the update of the local id to relation data
    set< uint32 > delete_relation_idxs;
    map< Relation_::Id, Relation_::Index > delete_relations_by_id;
    for (set< Relation_ >::const_iterator it(delete_relations.begin()); it != delete_relations.end(); ++it)
    {
      delete_relation_idxs.insert(it->head & 0xffffff00);
      delete_relations_by_id[it->head] = it->head;
    }
    local_ids.clear();
    Tag_Id_MultiRelation_Local_Reader local_id_relation_reader(local_ids, delete_relations_by_id, delete_relation_idxs);
    select_with_idx< Tag_Id_MultiRelation_Local_Reader >(local_id_relation_reader);
    for (set< pair< uint32, uint32 > >::const_iterator it(moved_local_ids.begin());
         it != moved_local_ids.end(); ++it)
      local_ids[*it].second.insert(0);
    for (map< uint32, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_relations_tags.begin()); it != new_relations_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first != 0xffffffff)
          local_ids[**it2].second.insert(it->first);
      }
    }
    
    cerr<<'G';
    //updating of the local id to relation data
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    moved_ids.clear();
    Tag_Id_Relation_Local_Updater id_relation_local_updater
      (local_ids, moved_ids, local_id_idx, spatial_boundaries);
    delete_insert< Tag_Id_Relation_Local_Updater >(id_relation_local_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Id_Relation_Local_Updater >(id_relation_local_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'H';
    //retrieving old coordinates of the relations that appear in local ids which are moving to global
    set< uint32 > t_move_involved_relations;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
        t_move_involved_relations.insert(*it2);
    }
    set< Relation_ > move_involved_relations;
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    Indexed_Ordered_Id_To_Many_By_Id_Reader< Relation_Storage, set< uint32 >, set< Relation_ > >
      relation_id_reader2(t_move_involved_relations, move_involved_relations);
    select_by_id(relation_id_reader2);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    map< Relation_::Id, Relation_::Index > move_involved_relations_by_id;
    for (set< Relation_ >::const_iterator it(move_involved_relations.begin());
         it != move_involved_relations.end(); ++it)
      move_involved_relations_by_id[it->head] = it->head;
    
    cerr<<'I';
    //preparing the update of the global relation to id data
    map< pair< uint32, uint32 >, pair< set< uint32 >, uint > > global_relations_to_be_edited;
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin()); it2 != it->second.end(); ++it2)
      {
        map< Relation_::Id, Relation_::Index >::const_iterator nit(move_involved_relations_by_id.find(*it2));
        if (nit == move_involved_relations_by_id.end())
          continue;
        pair< set< uint32 >, uint >& tail(global_relations_to_be_edited[make_pair< uint32, uint32 >
          (nit->second, nit->first)]);
        tail.first.insert(it->first);
        tail.second = Tag_Relation_Id_Updater::UPDATE;
      }
    }
    for (set< Relation_ >::const_iterator it(delete_relations.begin());
         it != delete_relations.end(); ++it)
      global_relations_to_be_edited[make_pair< uint32, uint32 >
                               (it->head, it->head)] = make_pair< set< uint32 >, uint >
      (set< uint32 >(), Tag_Relation_Id_Updater::DELETE);
    for (map< uint32, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_relations_tags.begin()); it != new_relations_tags.end(); ++it)
    {
      set< uint32 > global_ids;
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0xffffffff)
          global_ids.insert((*it2)->second);
      }
      global_relations_to_be_edited[make_pair< uint32, uint32 >
                               (it->first, it->first)] = make_pair< set< uint32 >, uint >
        (global_ids, Tag_Relation_Id_Updater::INSERT);
    }
    
    cerr<<'J';
    //updating of the global relation to id data
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    map< uint32, set< uint32 > > deleted_relations_ids;
    Tag_Relation_Id_Updater relation_id_updater
      (global_relations_to_be_edited, deleted_relations_ids);
    delete_insert< Tag_Relation_Id_Updater >(relation_id_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Relation_Id_Updater >(relation_id_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'K';
    //preparing the update of the global id to relation data
    global_ids_to_be_edited.clear();
    for (map< uint32, set< uint32 > >::const_iterator it(moved_ids.begin());
         it != moved_ids.end(); ++it)
    {
      global_ids_to_be_edited[it->first] = make_pair< set< uint32 >, set< uint32 > >
        (set< uint32 >(), set< uint32 >());
      set< uint32 >& inserted_relations(global_ids_to_be_edited[it->first].second);
      for (set< uint32 >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if (deleted_relations_ids.find(*it2) == deleted_relations_ids.end())
          inserted_relations.insert(*it2);
      }
    }
    for (map< uint32, set< uint32 > >::const_iterator it(deleted_relations_ids.begin());
         it != deleted_relations_ids.end(); ++it)
    {
      for (set< uint32 >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
        global_ids_to_be_edited[*it2].first.insert(it->first);
    }
    for (map< uint32, vector< pair< uint32, uint32 >* > >::const_iterator
         it(new_relations_tags.begin()); it != new_relations_tags.end(); ++it)
    {
      for (vector< pair< uint32, uint32 >* >::const_iterator it2(it->second.begin());
           it2 != it->second.end(); ++it2)
      {
        if ((*it2)->first == 0xffffffff)
        {
          pair< set< uint32 >, set< uint32 > >& tail(global_ids_to_be_edited[(*it2)->second]);
          if (tail.first.find(it->first) == tail.first.end())
            tail.second.insert(it->first);
          else
            tail.first.erase(it->first);
        }
      }
    }
    
    cerr<<'L';
    //updating of the global id to relation data
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    Tag_Id_Relation_Global_Updater id_relation_global_updater(global_ids_to_be_edited);
    delete_insert< Tag_Id_Relation_Global_Updater >(id_relation_global_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index< Tag_Id_Relation_Global_Updater >(id_relation_global_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    new_relations_tags.clear();
    for (map< pair< string, string >, pair< uint32, uint32 >* >::iterator
         it(new_relation_tags_ids.begin());
         it != new_relation_tags_ids.end(); ++it)
      delete(it->second);
    new_relation_tags_ids.clear();
    
    cerr<<'M';
    //updating the relations file
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    Indexed_Ordered_Id_To_Many_Updater< Relation_Storage, set< Relation_ > >
      relation_updater(delete_relations, new_relations);
    delete_insert(relation_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index(relation_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    update_id_index(relation_updater);
    //cerr<<(uintmax_t)time(NULL)<<'\n';
    
    cerr<<'N';
    if (!intermediate_run)
      relation_tag_id_statistics_remake();
  }
  catch(File_Error e)
  {
    cerr<<"\nopen64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  //cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
