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

set< int32 > t_delete_nodes;

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
  }
  else if (!strcmp(el, "way"))
  {
  }
  else if (!strcmp(el, "relation"))
  {
  }
}

void end(const char *el)
{
  if (!strcmp(el, "nd"))
  {
  }
  else if (!strcmp(el, "node"))
  {
  }
  else if (!strcmp(el, "way"))
  {
  }
  else if (!strcmp(el, "relation"))
  {
  }
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
    
    for (set< Node >::const_iterator it(delete_nodes.begin());
	 it != delete_nodes.end(); ++it)
      cout<<it->id<<'\t'<<it->lat<<'\t'<<it->lon<<'\n';
  }
  catch(File_Error e)
  {
    cerr<<"\nopen64: "<<e.error_number<<'\n';
  }
  
  cerr<<'\n'<<(uintmax_t)time(NULL)<<'\n';
  return 0;
}
