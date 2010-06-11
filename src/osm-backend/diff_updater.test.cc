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
  Ofstream_Collection node_db_out(get_basedir() + "after_node_", "_db.csv");
  Ofstream_Collection node_tags_local_out(get_basedir() + "after_node_tags_", "_local.csv");
  Ofstream_Collection node_tags_global_out(get_basedir() + "after_node_tags_", "_global.csv");
    
  Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
      (*de_osm3s_file_ids::NODES, false);
  for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
      it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
  {
    ofstream* out(node_db_out.get(it.object().id / 5000000));
    (*out)<<it.object().id<<'\t'<<setprecision(10)
	<<Node::lat(it.index().val(), it.object().ll_lower)<<'\t'
	<<Node::lon(it.index().val(), it.object().ll_lower)<<'\n';
  }
    
    // check update_node_tags_local - compare both files for the result
    Block_Backend< Tag_Index_Local, Uint32_Index > nodes_local_db
	(*de_osm3s_file_ids::NODE_TAGS_LOCAL, false);
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(nodes_local_db.flat_begin());
         !(it == nodes_local_db.flat_end()); ++it)
    {
      ofstream* out(node_tags_local_out.get(it.object().val() / 5000000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_node_tags_global - compare both files for the result
    Block_Backend< Tag_Index_Global, Uint32_Index > nodes_global_db
	(*de_osm3s_file_ids::NODE_TAGS_GLOBAL, false);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
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
  Ofstream_Collection way_db_out(get_basedir() + "after_way_", "_db.csv");
  Ofstream_Collection way_tags_local_out(get_basedir() + "after_way_tags_", "_local.csv");
  Ofstream_Collection way_tags_global_out(get_basedir() + "after_way_tags_", "_global.csv");
    
  // check update_members - compare both files for the result
  Block_Backend< Uint31_Index, Way_Skeleton > ways_db
      (*de_osm3s_file_ids::WAYS, false);
  for (Block_Backend< Uint31_Index, Way_Skeleton >::Flat_Iterator
      it(ways_db.flat_begin()); !(it == ways_db.flat_end()); ++it)
  {
    ofstream* out(way_db_out.get(it.object().id / 1000000));
    (*out)<<it.object().id<<'\t';
    for (uint i(0); i < it.object().nds.size(); ++i)
      (*out)<<it.object().nds[i]<<' ';
    (*out)<<'\n';
  }
    
    // check update_way_tags_local - compare both files for the result
    Block_Backend< Tag_Index_Local, Uint32_Index > ways_local_db
	(*de_osm3s_file_ids::WAY_TAGS_LOCAL, false);
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(ways_local_db.flat_begin());
         !(it == ways_local_db.flat_end()); ++it)
    {
      ofstream* out(way_tags_local_out.get(it.object().val() / 1000000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_way_tags_global - compare both files for the result
    Block_Backend< Tag_Index_Global, Uint32_Index > ways_global_db
	(*de_osm3s_file_ids::WAY_TAGS_GLOBAL, false);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
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
  Ofstream_Collection relation_db_out(get_basedir() + "after_relation_", "_db.csv");
  Ofstream_Collection relation_tags_local_out(get_basedir() + "after_relation_tags_", "_local.csv");
  Ofstream_Collection relation_tags_global_out(get_basedir() + "after_relation_tags_", "_global.csv");
    
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
      ofstream* out(relation_db_out.get(it.object().id / 200000));
      (*out)<<it.object().id<<'\t';
      for (uint i(0); i < it.object().members.size(); ++i)
	(*out)<<it.object().members[i].ref<<' '
	    <<it.object().members[i].type<<' '
	    <<roles[it.object().members[i].role]<<' ';
      (*out)<<'\n';
    }
    
    // check update_relation_tags_local - compare both files for the result
    Block_Backend< Tag_Index_Local, Uint32_Index > relations_local_db
	(*de_osm3s_file_ids::RELATION_TAGS_LOCAL, false);
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(relations_local_db.flat_begin());
         !(it == relations_local_db.flat_end()); ++it)
    {
      ofstream* out(relation_tags_local_out.get(it.object().val() / 200000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_relation_tags_global - compare both files for the result
    Block_Backend< Tag_Index_Global, Uint32_Index > relations_global_db
	(*de_osm3s_file_ids::RELATION_TAGS_GLOBAL, false);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
	 it(relations_global_db.flat_begin());
         !(it == relations_global_db.flat_end()); ++it)
    {
      ofstream* out(relation_tags_global_out.get(it.object().val() / 200000));
      (*out)<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
}

int main(int argc, char* args[])
{
  try
  {
    show_mem_status();
    
    current_node = Node(160621, 51.23, 7.05);
    current_node.tags.push_back(make_pair("highway", "bus_stop"));
    current_node.tags.push_back(make_pair("name", "Lienhardtplatz"));
    node_updater.set_node(current_node);
    
    current_node = Node(160622, 51.23, 7.052);
    current_node.tags.push_back(make_pair("highway", "bus_stop"));
    current_node.tags.push_back(make_pair("name", "Lienhardtplatz 2"));
    node_updater.set_node(current_node);
    
    current_node = Node(160623, 51.23, 7.053);
    current_node.tags.push_back(make_pair("highway", "bus_stop"));
    current_node.tags.push_back(make_pair("name", "Lienhardtplatz 3"));
    node_updater.set_node(current_node);
    
    current_node = Node(160624, 51.23, 7.054);
    current_node.tags.push_back(make_pair("highway", "bus_stop"));
    current_node.tags.push_back(make_pair("name", "Lienhardtplatz 4"));
    node_updater.set_node(current_node);
    
    current_way = Way(8237924);
    current_way.tags.push_back(make_pair("test", "Value 1"));
    current_way.nds.push_back(160621);
    current_way.nds.push_back(160622);
    current_way.nds.push_back(160623);
    current_way.nds.push_back(160624);
    way_updater.set_way(current_way);
    
    current_way = Way(8237925);
    current_way.tags.push_back(make_pair("test", "Value 2"));
    current_way.nds.push_back(160621);
    current_way.nds.push_back(160622);
    current_way.nds.push_back(160623);
    current_way.nds.push_back(160624);
    way_updater.set_way(current_way);
    
    current_way = Way(8237926);
    current_way.tags.push_back(make_pair("test", "Value 3"));
    current_way.nds.push_back(160621);
    current_way.nds.push_back(160622);
    current_way.nds.push_back(160623);
    current_way.nds.push_back(160624);
    way_updater.set_way(current_way);
    
    current_way = Way(8237927);
    current_way.tags.push_back(make_pair("test", "Value 4"));
    current_way.nds.push_back(160621);
    current_way.nds.push_back(160622);
    current_way.nds.push_back(160623);
    current_way.nds.push_back(160624);
    way_updater.set_way(current_way);
    
    current_relation = Relation(163298);
    Relation_Entry entry;
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("forward");
    current_relation.members.push_back(entry);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("forward");
    current_relation.members.push_back(entry);
    entry.ref = 160621;
    entry.type = Relation_Entry::NODE;
    entry.role = relation_updater.get_role_id("");
    current_relation.members.push_back(entry);
    current_relation.tags.push_back(make_pair("type", "test_relation_1"));
    relation_updater.set_relation(current_relation);
      
    current_relation = Relation(163299);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("forward");
    current_relation.members.push_back(entry);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("backward");
    current_relation.members.push_back(entry);
    entry.ref = 160621;
    entry.type = Relation_Entry::NODE;
    entry.role = relation_updater.get_role_id("");
    current_relation.members.push_back(entry);
    current_relation.tags.push_back(make_pair("type", "test_relation_2"));
    relation_updater.set_relation(current_relation);
    
    current_relation = Relation(163300);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("backward");
    current_relation.members.push_back(entry);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("backward");
    current_relation.members.push_back(entry);
    entry.ref = 160621;
    entry.type = Relation_Entry::NODE;
    entry.role = relation_updater.get_role_id("");
    current_relation.members.push_back(entry);
    current_relation.tags.push_back(make_pair("type", "test_relation_3"));
    relation_updater.set_relation(current_relation);
    
    node_updater.set_id_deleted(160621);
    
    current_node = Node(160623, 51.235, 7.053);
    current_node.tags.push_back(make_pair("highway", "bus_stop"));
    current_node.tags.push_back(make_pair("name", "Neuer Name"));
    node_updater.set_node(current_node);
    
    node_updater.set_id_deleted(160624);
    
    current_way = Way(8237924);
    current_way.tags.push_back(make_pair("test", "Value 1"));
    current_way.nds.push_back(160621);
    current_way.nds.push_back(160622);
    current_way.nds.push_back(160623);
    current_way.nds.push_back(160624);
    way_updater.set_way(current_way);
    
    way_updater.set_id_deleted(8237925);
    
    current_way = Way(8237926);
    current_way.tags.push_back(make_pair("test_2", "New Value"));
    current_way.nds.push_back(160622);
    current_way.nds.push_back(160623);
    current_way.nds.push_back(160622);
    way_updater.set_way(current_way);
    
    way_updater.set_id_deleted(8237927);
    
    current_relation = Relation(163298);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("forward");
    current_relation.members.push_back(entry);
    entry.ref = 8237924;
    entry.type = Relation_Entry::WAY;
    entry.role = relation_updater.get_role_id("forward");
    current_relation.members.push_back(entry);
    entry.ref = 160622;
    entry.type = Relation_Entry::NODE;
    entry.role = relation_updater.get_role_id("");
    current_relation.members.push_back(entry);
    current_relation.tags.push_back(make_pair("type", "route"));
    relation_updater.set_relation(current_relation);
    
    relation_updater.set_id_deleted(163300);
    
    node_updater.update();
    dump_nodes();
    
    way_updater.update();
    dump_ways();
    
    relation_updater.update();
    dump_relations();

    show_mem_status();
  }
  catch (File_Error e)
  {
    cerr<<"File error caught: "
	<<e.error_number<<' '<<e.filename<<' '<<e.origin<<'\n';
  }
  
  return 0;
}
