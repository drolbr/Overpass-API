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
int32 current_node(0);

set< int32 > t_delete_nodes;
set< Node > new_nodes;
map< int32, map< string, string > > new_nodes_tags;

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
      new_nodes_tags[current_node][key] = value;
  }
  else if (!strcmp(el, "nd"))
  {
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
    }
  }
  else if (!strcmp(el, "way"))
  {
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
  {
    current_node = 0;
  }
  else if (!strcmp(el, "way"))
  {
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
  
  cerr<<(uintmax_t)time(NULL)<<'\n';
  
  try
  {
    //reading the main document
    parse(stdin, start, end);
    
    //retrieving old coordinates of the nodes that will be deleted
    Node_Id_Node_By_Id_Reader reader(t_delete_nodes, delete_nodes);
    select_by_id< Node_Id_Node_By_Id_Reader >(reader);
    
    //updating the nodes file
    Node_Id_Node_Updater node_updater(delete_nodes, new_nodes);
    delete_insert< Node_Id_Node_Updater >(node_updater);
    
    map< pair< string, string >, set< uint32 > > kv_local_ids;
    map< pair< string, string >, set< uint32 > > kv_global_ids;
    for (map< int32, map< string, string > >::const_iterator it(new_nodes_tags.begin());
	 it != new_nodes_tags.end(); ++it)
    {
      for (map< string, string >::const_iterator it2(it->second.begin());
	   it2 != it->second.end(); ++it2)
      {
	kv_local_ids.insert(make_pair< pair< string, string >, set< uint32 > >(*it2, set< uint32 >()));
	kv_global_ids.insert(make_pair< pair< string, string >, set< uint32 > >(*it2, set< uint32 >()));
      }
    }
    
/*    for (set< Node >::const_iterator it(delete_nodes.begin());
	 it != delete_nodes.end(); ++it)
      cout<<it->id<<'\t'<<it->lat<<'\t'<<it->lon<<'\n';
    cout<<"---\n";
    for (set< Node >::const_iterator it(new_nodes.begin());
	 it != new_nodes.end(); ++it)
      cout<<it->id<<'\t'<<it->lat<<'\t'<<it->lon<<'\n';
    for (map< int32, map< string, string > >::const_iterator it(new_nodes_tags.begin());
	 it != new_nodes_tags.end(); ++it)
    {
      cout<<it->first<<'\n';
      for (map< string, string >::const_iterator it2(it->second.begin());
	   it2 != it->second.end(); ++it2)
	cout<<'['<<it2->first<<"]["<<it2->second<<"]\n";
    }
    cout<<"---\n";
    for (map< pair< string, string >, set< uint32 > >::const_iterator it(kv_local_ids.begin());
	 it != kv_local_ids.end(); ++it)
      cout<<'['<<it->first.first<<"]["<<it->first.second<<"]\n";
    cout<<"---\n";
    for (map< pair< string, string >, set< uint32 > >::const_iterator it(kv_global_ids.begin());
	 it != kv_global_ids.end(); ++it)
      cout<<'['<<it->first.first<<"]["<<it->first.second<<"]\n";*/
  }
  catch(File_Error e)
  {
    cerr<<"\nopen64: "<<e.error_number<<'\n';
  }
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
