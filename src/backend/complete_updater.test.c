#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../dispatch/settings.h"
#include "../expat/expat_justparse_interface.h"
#include "node_updater.h"
#include "random_file.h"
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
uint32 csv_count;
ofstream* node_source_out;
ofstream* node_tags_source_out;
ofstream* way_source_out;
ofstream* way_tags_source_out;
ofstream* relation_source_out;
ofstream* relation_tags_source_out;

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

struct Ofstream_Collection
{
  vector< ofstream* > streams;
  string prefix;
  string postfix;
  
  Ofstream_Collection(string prefix_, string postfix_)
  : prefix(prefix_), postfix(postfix_) {}
  
  ofstream* get(uint32 i)
  {
    while (streams.size() <= i)
    {
      ostringstream buf("");
      buf<<streams.size();
      streams.push_back(new ofstream((prefix + buf.str() + postfix).c_str()));
    }
    return streams[i];
  }
  
  ~Ofstream_Collection()
  {
    for (vector< ofstream* >::iterator it(streams.begin());
	 it != streams.end(); ++it)
    {
      (*it)->close();
      delete (*it);
    }
  }
};

void dump_nodes()
{
  Ofstream_Collection node_db_out(get_basedir() + "node_", "_db.csv");
  Ofstream_Collection node_tags_local_out(get_basedir() + "node_tags_", "_local.csv");
  Ofstream_Collection node_tags_global_out(get_basedir() + "node_tags_", "_global.csv");
    
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (de_osm3s_file_ids::NODES, false);
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
      it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
  {
    ofstream* out(node_db_out.get(it.object().id / 5000000));
    (*out)<<it.object().id<<'\t'<<setprecision(10)
	<<Node::lat(it.index().val(), it.object().ll_lower)<<'\t'
	<<Node::lon(it.index().val(), it.object().ll_lower)<<'\n';
  }
    
    // check update_node_tags_local - compare both files for the result
    Block_Backend< Node_Tag_Index_Local, Uint32_Index > nodes_local_db
	(de_osm3s_file_ids::NODE_TAGS_LOCAL, false);
    for (Block_Backend< Node_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(nodes_local_db.flat_begin());
         !(it == nodes_local_db.flat_end()); ++it)
    {
      ofstream* out(node_tags_local_out.get(it.object().val() / 5000000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_node_tags_global - compare both files for the result
    Block_Backend< Node_Tag_Index_Global, Uint32_Index > nodes_global_db
	(de_osm3s_file_ids::NODE_TAGS_GLOBAL, false);
    for (Block_Backend< Node_Tag_Index_Global, Uint32_Index >::Flat_Iterator
	 it(nodes_global_db.flat_begin());
         !(it == nodes_global_db.flat_end()); ++it)
    {
      ofstream* out(node_tags_global_out.get(it.object().val() / 5000000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
}

void dump_ways()
{
  Ofstream_Collection way_db_out(get_basedir() + "way_", "_db.csv");
  Ofstream_Collection way_tags_local_out(get_basedir() + "way_tags_", "_local.csv");
  Ofstream_Collection way_tags_global_out(get_basedir() + "way_tags_", "_global.csv");
    
  // check update_members - compare both files for the result
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (de_osm3s_file_ids::WAYS, false);
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
      it(ways_db.flat_begin()); !(it == ways_db.flat_end()); ++it)
  {
    ofstream* out(way_db_out.get(it.object().id / 1000000));
    (*out)<<it.object().id<<'\t';
    for (int i(0); i < it.object().nds.size(); ++i)
      (*out)<<it.object().nds[i]<<' ';
    (*out)<<'\n';
  }
    
    // check update_way_tags_local - compare both files for the result
    Block_Backend< Way_Tag_Index_Local, Uint32_Index > ways_local_db
	(de_osm3s_file_ids::WAY_TAGS_LOCAL, false);
    for (Block_Backend< Way_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(ways_local_db.flat_begin());
         !(it == ways_local_db.flat_end()); ++it)
    {
      ofstream* out(way_tags_local_out.get(it.object().val() / 1000000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_way_tags_global - compare both files for the result
    Block_Backend< Way_Tag_Index_Global, Uint32_Index > ways_global_db
	(de_osm3s_file_ids::WAY_TAGS_GLOBAL, false);
    for (Block_Backend< Way_Tag_Index_Global, Uint32_Index >::Flat_Iterator
	 it(ways_global_db.flat_begin());
         !(it == ways_global_db.flat_end()); ++it)
    {
      ofstream* out(way_tags_global_out.get(it.object().val() / 1000000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
}

void dump_relations()
{
    Ofstream_Collection relation_db_out(get_basedir() + "relation_", "_db.csv");
    Ofstream_Collection relation_tags_local_out(get_basedir() + "relation_tags_", "_local.csv");
    Ofstream_Collection relation_tags_global_out(get_basedir() + "relation_tags_", "_global.csv");
    
    // prepare check update_members - load roles
    map< uint32, string > roles;
    Block_Backend< Uint32_Index, String_Object > roles_db
      (de_osm3s_file_ids::RELATION_ROLES, true);
    for (Block_Backend< Uint32_Index, String_Object >::Flat_Iterator
        it(roles_db.flat_begin()); !(it == roles_db.flat_end()); ++it)
      roles[it.index().val()] = it.object().val();
    
    // check update_members - compare both files for the result
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
	(de_osm3s_file_ids::RELATIONS, false);
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
	 it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
    {
      ofstream* out(relation_db_out.get(it.object().id / 500000));
      (*out)<<it.object().id<<'\t';
      for (int i(0); i < it.object().members.size(); ++i)
	(*out)<<it.object().members[i].ref<<' '
	    <<it.object().members[i].type<<' '
	    <<roles[it.object().members[i].role]<<' ';
      (*out)<<'\n';
    }
    
    // check update_relation_tags_local - compare both files for the result
    Block_Backend< Relation_Tag_Index_Local, Uint32_Index > relations_local_db
	(de_osm3s_file_ids::RELATION_TAGS_LOCAL, false);
    for (Block_Backend< Relation_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(relations_local_db.flat_begin());
         !(it == relations_local_db.flat_end()); ++it)
    {
      ofstream* out(relation_tags_local_out.get(it.object().val() / 500000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_relation_tags_global - compare both files for the result
    Block_Backend< Relation_Tag_Index_Global, Uint32_Index > relations_global_db
	(de_osm3s_file_ids::RELATION_TAGS_GLOBAL, false);
    for (Block_Backend< Relation_Tag_Index_Global, Uint32_Index >::Flat_Iterator
	 it(relations_global_db.flat_begin());
         !(it == relations_global_db.flat_end()); ++it)
    {
      ofstream* out(relation_tags_global_out.get(it.object().val() / 500000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
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
    {
      current_node.tags.push_back(make_pair(key, value));
      (*node_tags_source_out)<<current_node.id<<'\t'<<key<<'\t'<<value<<'\n';
    }
    else if (current_way.id > 0)
    {
      current_way.tags.push_back(make_pair(key, value));
      (*way_tags_source_out)<<current_way.id<<'\t'<<key<<'\t'<<value<<'\n';
    }
    else if (current_relation.id > 0)
    {
      current_relation.tags.push_back(make_pair(key, value));
      (*relation_tags_source_out)<<current_relation.id<<'\t'<<key<<'\t'<<value<<'\n';
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
      
      (*way_source_out)<<ref<<' ';
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
      
      (*relation_source_out)<<ref<<' '<<entry.type<<' '<<role<<' ';
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
    
    if (current_node.id / 5000000 > csv_count)
    {
      node_source_out->close();
      delete(node_source_out);
      node_tags_source_out->close();
      delete(node_tags_source_out);
      
      ++csv_count;
      ostringstream buf("");
      buf<<csv_count;
      node_source_out = new ofstream
	  ((get_basedir() + "node_" + buf.str() + "_source.csv").c_str());
      node_tags_source_out = new ofstream
	  ((get_basedir() + "node_tags_" + buf.str() + "_source.csv").c_str());
    }
    (*node_source_out)<<id<<'\t'<<setprecision(10)<<lat<<'\t'<<lon<<'\n';
  }
  else if (!strcmp(el, "way"))
  {
    if (state == IN_NODES)
    {
      node_source_out->close();
      delete(node_source_out);
      node_tags_source_out->close();
      delete(node_tags_source_out);
      
      node_updater.update();
      show_mem_status();
      dump_nodes();
      osm_element_count = 0;
      state = IN_WAYS;
      csv_count = 0;
    }
    
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
    current_way = Way(id);
    
    if (current_way.id / 1000000 > csv_count)
    {
      way_source_out->close();
      delete(way_source_out);
      way_tags_source_out->close();
      delete(way_tags_source_out);
      
      ++csv_count;
      ostringstream buf("");
      buf<<csv_count;
      way_source_out = new ofstream
	  ((get_basedir() + "way_" + buf.str() + "_source.csv").c_str());
      way_tags_source_out = new ofstream
	  ((get_basedir() + "way_tags_" + buf.str() + "_source.csv").c_str());
    }
    (*way_source_out)<<id<<'\t';
  }
  else if (!strcmp(el, "relation"))
  {
    if (state == IN_NODES)
    {
      node_source_out->close();
      delete(node_source_out);
      node_tags_source_out->close();
      delete(node_tags_source_out);
      
      node_updater.update();
      show_mem_status();
      dump_nodes();
      dump_ways();
      show_mem_status();
      osm_element_count = 0;
      state = IN_RELATIONS;
      csv_count = 0;
    }
    else if (state == IN_WAYS)
    {
      way_source_out->close();
      delete(way_source_out);
      way_tags_source_out->close();
      delete(way_tags_source_out);
      
      way_updater.update();
      show_mem_status();
      dump_ways();
      show_mem_status();
      osm_element_count = 0;
      state = IN_RELATIONS;
      csv_count = 0;
    }
    
    unsigned int id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atoi(attr[i+1]);
    }
    current_relation = Relation(id);
    
    if (current_relation.id / 500000 > csv_count)
    {
      relation_source_out->close();
      delete(relation_source_out);
      relation_tags_source_out->close();
      delete(relation_tags_source_out);
      
      ++csv_count;
      ostringstream buf("");
      buf<<csv_count;
      relation_source_out = new ofstream
	  ((get_basedir() + "relation_" + buf.str() + "_source.csv").c_str());
      relation_tags_source_out = new ofstream
	  ((get_basedir() + "relation_tags_" + buf.str() + "_source.csv").c_str());
    }
    (*relation_source_out)<<id<<'\t';
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

    (*way_source_out)<<'\n';
    
    if (osm_element_count >= 4*1024*1024)
    {
      way_updater.update(true);
      show_mem_status();
      osm_element_count = 0;
    }
  }
  else if (!strcmp(el, "relation"))
  {
    relation_updater.set_relation(current_relation);
    current_relation.id = 0;
    
    (*relation_source_out)<<'\n';
    
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
    node_source_out = new ofstream
	((get_basedir() + "node_0_source.csv").c_str());
    node_tags_source_out = new ofstream
	((get_basedir() + "node_tags_0_source.csv").c_str());
    way_source_out = new ofstream
	((get_basedir() + "way_0_source.csv").c_str());
    way_tags_source_out = new ofstream
	((get_basedir() + "way_tags_0_source.csv").c_str());
    relation_source_out = new ofstream
	((get_basedir() + "relation_0_source.csv").c_str());
    relation_tags_source_out = new ofstream
	((get_basedir() + "relation_tags_0_source.csv").c_str());
    
    show_mem_status();
    
    osm_element_count = 0;
    state = 0;
    //reading the main document
    parse(stdin, start, end);
  
    if (state == IN_NODES)
    {
      node_source_out->close();
      delete(node_source_out);
      node_tags_source_out->close();
      delete(node_tags_source_out);
      
      node_updater.update();
      show_mem_status();
      dump_nodes();
      dump_ways();
    }
    else if (state == IN_WAYS)
    {
      way_source_out->close();
      delete(way_source_out);
      way_tags_source_out->close();
      delete(way_tags_source_out);
      
      way_updater.update();
      show_mem_status();
      dump_ways();
    }
    else if (state == IN_RELATIONS)
    {
      relation_source_out->close();
      delete(relation_source_out);
      relation_tags_source_out->close();
      delete(relation_tags_source_out);
      
      relation_updater.update();
      show_mem_status();
    }
    dump_relations();
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
