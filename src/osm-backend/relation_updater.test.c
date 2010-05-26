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
 * Tests the library node_updater with a sample OSM file
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
ofstream member_source_out((get_basedir() + "member_source.csv").c_str());
ofstream tags_source_out((get_basedir() + "tags_source.csv").c_str());

uint32 osm_element_count;

void show_mem_status()
{
  ostringstream proc_file_name_("");
  proc_file_name_<<"/proc/"<<getpid()<<"/stat";
  ifstream stat(proc_file_name_.str().c_str());
  while (stat.good())
  {
    string line;
    getline(stat, line);
    cerr<<line;
  }
  cerr<<'\n';
}

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
    {
      current_relation.tags.push_back(make_pair(key, value));
      tags_source_out<<current_relation.id<<'\t'<<key<<'\t'<<value<<'\n';
    }
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
      
      member_source_out<<ref<<' '<<entry.type<<' '<<role<<' ';
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
      show_mem_status();
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
      show_mem_status();
      osm_element_count = 0;
      state = IN_RELATIONS;
    }
    else if (state == IN_WAYS)
    {
      way_updater.update();
      show_mem_status();
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
    
    member_source_out<<id<<'\t';
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    node_updater.set_node(current_node);
    current_node.id = 0;

    if (osm_element_count >= 4*1024*1024)
    {
      node_updater.update(true);
      show_mem_status();
      osm_element_count = 0;
    }
  }
  else if (!strcmp(el, "way"))
  {
    way_updater.set_way(current_way);
    current_way.id = 0;

    if (osm_element_count >= 4*1024*1024)
    {
      way_updater.update();
      show_mem_status();
      osm_element_count = 0;
    }
  }
  else if (!strcmp(el, "relation"))
  {
    relation_updater.set_relation(current_relation);
    current_relation.id = 0;
    
    member_source_out<<'\n';
    
    if (osm_element_count >= 4*1024*1024)
    {
      relation_updater.update();
      show_mem_status();
      osm_element_count = 0;
    }
  }
  ++osm_element_count;
}

int main(int argc, char* args[])
{
  try
  {
    ofstream member_db_out((get_basedir() + "member_db.csv").c_str());
    ofstream tags_local_out((get_basedir() + "tags_local.csv").c_str());
    ofstream tags_global_out((get_basedir() + "tags_global.csv").c_str());
    
    show_mem_status();
    
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
    
    show_mem_status();

    // prepare check update_members - load roles
    map< uint32, string > roles;
    Block_Backend< Uint32_Index, String_Object > roles_db
      (*de_osm3s_file_ids::RELATION_ROLES, true);
    for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
        it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
      roles[it.index().val()] = it.object().val();
    
    // check update_members - compare both files for the result
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
	(*de_osm3s_file_ids::RELATIONS, false);
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
	 it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
    {
      member_db_out<<it.object().id<<'\t';
      for (uint i(0); i < it.object().members.size(); ++i)
	member_db_out<<it.object().members[i].ref<<' '
	    <<it.object().members[i].type<<' '
	    <<roles[it.object().members[i].role]<<' ';
      member_db_out<<'\n';
    }
    
    // check update_way_tags_local - compare both files for the result
    Block_Backend< Relation_Tag_Index_Local, Uint32_Index > relations_local_db
	(*de_osm3s_file_ids::RELATION_TAGS_LOCAL, false);
    for (Block_Backend< Relation_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(relations_local_db.flat_begin());
         !(it == relations_local_db.flat_end()); ++it)
    {
      tags_local_out<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_way_tags_local - compare both files for the result
    Block_Backend< Relation_Tag_Index_Global, Uint32_Index > relations_global_db
	(*de_osm3s_file_ids::RELATION_TAGS_GLOBAL, false);
    for (Block_Backend< Relation_Tag_Index_Global, Uint32_Index >::Flat_Iterator
	 it(relations_global_db.flat_begin());
         !(it == relations_global_db.flat_end()); ++it)
    {
      tags_global_out<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
