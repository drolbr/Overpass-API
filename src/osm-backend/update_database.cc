#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../backend/random_file.h"
#include "../core/settings.h"
#include "../expat/expat_justparse_interface.h"
#include "node_updater.h"
#include "relation_updater.h"
#include "way_updater.h"

using namespace std;

/**
 * Tests the library node_updater, way_updater and relation_updater
 * with a sample OSM file
 */

Node_Updater node_updater;
Node current_node;
Way_Updater way_updater;
Way current_way;
Relation_Updater relation_updater;
Relation current_relation;
int state;
const int IN_NODES = 1;
const int IN_WAYS = 2;
const int IN_RELATIONS = 3;
int modify_mode = 0;
const int DELETE = 1;

uint32 osm_element_count;

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
    if (current_node.id > 0)
      current_node.tags.push_back(make_pair(key, value));
    else if (current_way.id > 0)
      current_way.tags.push_back(make_pair(key, value));
    else if (current_relation.id > 0)
      current_relation.tags.push_back(make_pair(key, value));
  }
  else if (!strcmp(el, "nd"))
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
  else if (!strcmp(el, "member"))
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
      entry.role = relation_updater.get_role_id(role);
      current_relation.members.push_back(entry);
    }
  }
  else if (!strcmp(el, "node"))
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
  else if (!strcmp(el, "way"))
  {
    if (state == IN_NODES)
    {
      node_updater.update();
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
  else if (!strcmp(el, "relation"))
  {
    if (state == IN_NODES)
    {
      node_updater.update();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == IN_WAYS)
    {
      way_updater.update();
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
  else if (!strcmp(el, "delete"))
    modify_mode = DELETE;
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    if (modify_mode == DELETE)
      node_updater.set_id_deleted(current_node.id);
    else
      node_updater.set_node(current_node);
    if (osm_element_count >= 4*1024*1024)
    {
      cerr<<"Id: "<<current_node.id<<' ';
      node_updater.update(true);
      osm_element_count = 0;
    }
    current_node.id = 0;
  }
  else if (!strcmp(el, "way"))
  {
    if (modify_mode == DELETE)
      way_updater.set_id_deleted(current_way.id);
    else
      way_updater.set_way(current_way);
    if (osm_element_count >= 4*1024*1024)
    {
      cerr<<"Id: "<<current_way.id<<' ';
      way_updater.update(true);
      osm_element_count = 0;
    }
    current_way.id = 0;
  }
  else if (!strcmp(el, "relation"))
  {
    if (modify_mode == DELETE)
      relation_updater.set_id_deleted(current_relation.id);
    else
      relation_updater.set_relation(current_relation);
    if (osm_element_count >= 4*1024*1024)
    {
      cerr<<"Id: "<<current_relation.id<<' ';
      relation_updater.update();
      osm_element_count = 0;
    }
    current_relation.id = 0;
  }
  else if (!strcmp(el, "delete"))
    modify_mode = 0;
  ++osm_element_count;
}

int main(int argc, char* argv[])
{
  // read command line arguments
  string db_dir;
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
      set_basedir(db_dir);
    }
    ++argpos;
  }
  
  try
  {
    osm_element_count = 0;
    state = 0;
    //reading the main document
    parse(stdin, start, end);
  
    if (state == IN_NODES)
      node_updater.update();
    else if (state == IN_WAYS)
      way_updater.update();
    else if (state == IN_RELATIONS)
      relation_updater.update();
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
