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

Way_ current_way(Way_(0, 0));
set< uint32 > t_delete_ways;
set< Way_ > new_ways_floating;
map< pair< uint32, uint32 >, vector< pair< uint32, uint32 >* > > new_ways_tags;
map< pair< string, string >, pair< uint32, uint32 >* > new_way_tags_ids;

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
      new_ways_tags[make_pair< uint32, uint32 >
                    (current_way.head.second, current_ll_idx)].push_back(coord_id);
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
  if (!strcmp(el, "nd"))
  {
  }
  else if (!strcmp(el, "node"))
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
    //reading the main document
    parse(stdin, start, end);
    
/*    //retrieving old coordinates of the nodes that will be deleted
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
    
    node_tag_id_statistics_remake();*/
    
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
    
    //calculate for each ways its index
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
    }
    new_ways_floating.clear();
    
/*    for (set< Way_ >::const_iterator it(new_ways.begin()); it != new_ways.end(); ++it)
    {
      cout<<hex<<it->head.first<<'\t'<<dec<<it->head.second<<'\n';
      for (vector< Way_::Data >::const_iterator it2(it->data.begin()); it2 != it->data.end(); ++it2)
        cout<<'\t'<<*it2<<'\n';
    }*/
    
    //updating the ways file
    cerr<<(uintmax_t)time(NULL)<<'\n';
    Indexed_Ordered_Id_To_Many_Updater< Way_Storage, set< Way_ > > way_updater(delete_ways, new_ways);
    delete_insert(way_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    make_block_index(way_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    update_id_index(way_updater);
    cerr<<(uintmax_t)time(NULL)<<'\n';
    
/*    way_tag_id_statistics_remake();*/
  }
  catch(File_Error e)
  {
    cerr<<"\nopen64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
