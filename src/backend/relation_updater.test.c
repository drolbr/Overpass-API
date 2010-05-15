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
      current_way.tags[key] = value;
    else if (current_relation.id > 0)
    {
      current_relation.tags[key] = value;
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
      entry.role = 0;
      current_relation.members.push_back(entry);
      
      member_source_out<<ref<<' '<<entry.type<<' ';
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
      node_updater.update();
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
  
    node_updater.update();
    way_updater.update();
    relation_updater.update();
    
    show_mem_status();

    // check update_members - compare both files for the result
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
	(de_osm3s_file_ids::RELATIONS, false);
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Flat_Iterator
	 it(relations_db.flat_begin()); !(it == relations_db.flat_end()); ++it)
    {
      member_db_out<<it.object().id<<'\t';
      for (int i(0); i < it.object().members.size(); ++i)
	member_db_out<<it.object().members[i].ref<<' '
	    <<it.object().members[i].type<<' ';
      member_db_out<<'\n';
    }
    
    // check update_way_tags_local - compare both files for the result
    Block_Backend< Relation_Tag_Index_Local, Uint32_Index > relations_local_db
	(de_osm3s_file_ids::RELATION_TAGS_LOCAL, false);
    for (Block_Backend< Relation_Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(relations_local_db.flat_begin());
         !(it == relations_local_db.flat_end()); ++it)
    {
      tags_local_out<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_way_tags_local - compare both files for the result
    Block_Backend< Relation_Tag_Index_Global, Uint32_Index > relations_global_db
	(de_osm3s_file_ids::RELATION_TAGS_GLOBAL, false);
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

/*
ohne DB:
13601 (a.out) R 13490 13600 13490 34828 13600 4194304 385 0 0 0 0 0 0 0 20 0 1 0 9140981 3502080 268 4294967295 134512640 134847943 3215148320 3215145768 3086119952 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 112334 0 0 0 1688 46 0 0 20 0 1 0 9140981 320638976 63336 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 128007 0 0 0 3192 74 0 0 20 0 1 0 9140981 362168320 70816 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 156111 0 0 0 4825 91 0 0 20 0 1 0 9140981 429277184 82535 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 166138 0 0 0 6469 121 0 0 20 0 1 0 9140981 429277184 92562 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 173200 0 0 0 8115 146 0 0 20 0 1 0 9140981 429277184 99624 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 213108 0 0 0 9775 184 0 0 20 0 1 0 9140981 563494912 106763 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 220168 0 0 0 11425 207 0 0 20 0 1 0 9140981 563494912 113823 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 227227 0 0 0 13049 234 0 0 20 0 1 0 9140981 563494912 120882 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13601 (a.out) R 13490 13600 13490 34828 13600 4202496 234304 0 0 0 14674 255 0 0 20 0 1 0 9140981 563494912 127959 4294967295 134512640 134847943 3215148320 3215145336 3086119952 0 0 0 0 0 0 0 17 0 0 0 0 0 0

mit DB:
13848 (a.out) R 13490 13847 13490 34828 13847 4194304 387 0 0 0 0 0 0 0 20 0 1 0 9172925 3633152 269 4294967295 134512640 134978487 3213093680 3213091144 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 179431 0 0 0 3420 1391 0 0 20 0 1 0 9172925 591425536 129458 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 206349 0 0 0 7081 2653 0 0 20 0 1 0 9172925 600076288 133978 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 229421 0 0 0 10760 3544 0 0 20 0 1 0 9172925 734298112 157050 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 239449 0 0 0 14539 4368 0 0 20 0 1 0 9172925 734433280 167078 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 246544 0 0 0 18743 5036 0 0 20 0 1 0 9172925 734568448 174173 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 286485 0 0 0 23314 6057 0 0 20 0 1 0 9172925 868921344 181345 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 293545 0 0 0 28012 7093 0 0 20 0 1 0 9172925 868921344 188405 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 300604 0 0 0 33056 8261 0 0 20 0 1 0 9172925 868921344 195464 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n13848 (a.out) R 13490 13847 13490 34828 13847 4202496 314016 0 0 0 38335 9394 0 0 20 0 1 0 9172925 868921344 202541 4294967295 134512640 134978487 3213093680 3213090744 3086337040 0 0 0 0 0 0 0 17 1 0 0 0 0 0

mit Node-Tag-Vector statt -Map, Index-Kontrolle entfernt:
14284 (a.out) R 13490 14283 13490 34828 14283 4194304 387 0 0 0 0 0 0 0 20 0 1 0 9242841 3629056 268 4294967295 134512640 134971651 3221093072 3221090520 3086595088 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 146607 0 0 0 3343 1704 0 0 20 0 1 0 9242841 515813376 117004 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 147802 0 0 0 6974 2874 0 0 20 0 1 0 9242841 496934912 112400 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 185511 0 0 0 10651 3760 0 0 20 0 1 0 9242841 501915648 117075 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 250908 0 0 0 14526 4679 0 0 20 0 1 0 9242841 511496192 121310 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 327188 0 0 0 18695 5485 0 0 20 0 1 0 9242841 511496192 121310 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 425919 0 0 0 23199 6532 0 0 20 0 1 0 9242841 511496192 121310 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 549063 0 0 0 27934 7539 0 0 20 0 1 0 9242841 511496192 121310 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 0 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 690527 0 0 0 32945 8531 0 0 20 0 1 0 9242841 511496192 121310 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 1 0 0 0 0 0
n14284 (a.out) R 13490 14283 13490 34828 14283 4202496 860098 0 0 0 38267 9630 0 0 20 0 1 0 9242841 512847872 121608 4294967295 134512640 134971651 3221093072 3221090088 3086595088 0 0 0 0 0 0 0 17 0 0 0 0 0 0
*/
