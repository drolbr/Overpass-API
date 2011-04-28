#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "../frontend/output.h"
#include "node_updater.h"

using namespace std;

/**
 * Tests the library node_updater with a sample OSM file
 */

Node_Updater* node_updater;
Node current_node;
//vector< pair< uint32, uint32 > > id_idxs;
ofstream* coord_source_out;
ofstream* tags_source_out;
Osm_Backend_Callback* callback;
uint32 DEBUG_nodes_count, DEBUG_update_count, DEBUG_nodes_8, DEBUG_nodes_64;

uint32 osm_element_count;

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
  {
    if (current_node.id > 0)
    {
      string key(""), value("");
      for (unsigned int i(0); attr[i]; i += 2)
      {
	if (!strcmp(attr[i], "k"))
	  key = attr[i+1];
	if (!strcmp(attr[i], "v"))
	  value = attr[i+1];
      }
      current_node.tags.push_back(make_pair(key, value));
      
      *tags_source_out<<current_node.id<<'\t'<<key<<'\t'<<value<<'\n';
    }
  }
  else if (!strcmp(el, "node"))
  {
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
    //id_idxs.push_back(make_pair(id, Node::ll_upper(lat, lon)));
    
    *coord_source_out<<id<<'\t'<<fixed<<setprecision(7)<<lat<<'\t'<<lon<<'\n';
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    node_updater->set_node(current_node);
    
    if (osm_element_count >= 4*1024*1024)
    {
      callback->node_elapsed(current_node.id);
      node_updater->update(callback, true);
      callback->parser_started();
      osm_element_count = 0;
    }
    
    current_node.id = 0;
  }
  ++osm_element_count;
}

void cleanup_files(const File_Properties& file_properties, bool cleanup_map)
{
  remove((file_properties.get_file_base_name() +
          file_properties.get_data_suffix() +
	  file_properties.get_index_suffix()).c_str());
  remove((file_properties.get_file_base_name() +
          file_properties.get_data_suffix()).c_str());
  if (cleanup_map)
  {
    remove((file_properties.get_file_base_name() +
        file_properties.get_id_suffix()).c_str());
    remove((file_properties.get_file_base_name() +
	file_properties.get_id_suffix() + file_properties.get_index_suffix()).c_str());
  }
}

int main(int argc, char* args[])
{
  Node_Updater node_updater_;
  node_updater = &node_updater_;
  
  try
  {
    coord_source_out = new ofstream((get_basedir() + "coord_source.csv").c_str());
    tags_source_out = new ofstream((get_basedir() + "tags_source.csv").c_str());
    
    ofstream coord_db_out((get_basedir() + "coord_db.csv").c_str());
    ofstream tags_local_out((get_basedir() + "tags_local.csv").c_str());
    ofstream tags_global_out((get_basedir() + "tags_global.csv").c_str());
    callback = get_verbatim_callback();
    
    DEBUG_nodes_count = 0;
    DEBUG_update_count = 0;
    DEBUG_nodes_8 = 0;
    DEBUG_nodes_64 = 0;
    osm_element_count = 0;
    //reading the main document
    callback->parser_started();
    parse(stdin, start, end);
  
    callback->nodes_finished();
    node_updater->update(callback);
    
    delete coord_source_out;
    delete tags_source_out;
    
    // check update_node_ids
/*    uint32 false_count(0);
    Random_File< Uint32_Index > random(de_osm3s_file_ids::NODES, true);
    for (vector< pair< uint32, uint32 > >::const_iterator it(id_idxs.begin());
	 it != id_idxs.end(); ++it)
    {
      if (it->second != random.get(it->first).val())
      {
	cout<<it->first<<'\t'<<it->second<<'\t'
	    <<random.get(it->first).val()<<'\n';
	++false_count;
      }
    }
    cout<<'('<<id_idxs.size()<<" nodes checked, "
	<<false_count<<" are inconsistent)\n";*/
    
    // check update_coords - compare both files for the result
    Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
	(*de_osm3s_file_ids::NODES, false, false);
    for (Block_Backend< Uint32_Index, Node_Skeleton >::Flat_Iterator
	 it(nodes_db.flat_begin()); !(it == nodes_db.flat_end()); ++it)
    {
      coord_db_out<<it.object().id<<'\t'<<fixed<<setprecision(7)
	  <<Node::lat(it.index().val(), it.object().ll_lower)<<'\t'
	  <<Node::lon(it.index().val(), it.object().ll_lower)<<'\n';
    }
    
    // check update_node_tags_local - compare both files for the result
    Block_Backend< Tag_Index_Local, Uint32_Index > nodes_local_db
	(*de_osm3s_file_ids::NODE_TAGS_LOCAL, false, false);
    for (Block_Backend< Tag_Index_Local, Uint32_Index >::Flat_Iterator
	 it(nodes_local_db.flat_begin()); !(it == nodes_local_db.flat_end()); ++it)
    {
      tags_local_out<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
    
    // check update_node_tags_global - compare both files for the result
    Block_Backend< Tag_Index_Global, Uint32_Index > nodes_global_db
	(*de_osm3s_file_ids::NODE_TAGS_GLOBAL, false, false);
    for (Block_Backend< Tag_Index_Global, Uint32_Index >::Flat_Iterator
	 it(nodes_global_db.flat_begin()); !(it == nodes_global_db.flat_end()); ++it)
    {
      tags_global_out<<it.object().val()<<'\t'
	  <<it.index().key<<'\t'<<it.index().value<<'\n';
    }
  }
  catch (File_Error e)
  {
    report_file_error(e);
  }
  
  cleanup_files(*de_osm3s_file_ids::NODES, true);
  cleanup_files(*de_osm3s_file_ids::NODE_TAGS_LOCAL, true);
  cleanup_files(*de_osm3s_file_ids::NODE_TAGS_GLOBAL, true);
  
  return 0;
}
