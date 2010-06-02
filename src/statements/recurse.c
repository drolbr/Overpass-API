#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../backend/block_backend.h"
#include "../backend/random_file.h"
#include "../core/settings.h"
#include "recurse.h"

using namespace std;

const unsigned int RECURSE_RELATION_RELATION = 1;
const unsigned int RECURSE_RELATION_BACKWARDS = 2;
const unsigned int RECURSE_RELATION_WAY = 3;
const unsigned int RECURSE_RELATION_NODE = 4;
const unsigned int RECURSE_WAY_NODE = 5;
const unsigned int RECURSE_WAY_RELATION = 6;
const unsigned int RECURSE_NODE_RELATION = 7;
const unsigned int RECURSE_NODE_WAY = 8;

void Recurse_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  attributes["type"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
  
  if (attributes["type"] == "relation-relation")
    type = RECURSE_RELATION_RELATION;
  else if (attributes["type"] == "relation-backwards")
    type = RECURSE_RELATION_BACKWARDS;
  else if (attributes["type"] == "relation-way")
    type = RECURSE_RELATION_WAY;
  else if (attributes["type"] == "relation-node")
    type = RECURSE_RELATION_NODE;
  else if (attributes["type"] == "way-node")
    type = RECURSE_WAY_NODE;
  else if (attributes["type"] == "way-relation")
    type = RECURSE_WAY_RELATION;
  else if (attributes["type"] == "node-relation")
    type = RECURSE_NODE_RELATION;
  else if (attributes["type"] == "node-way")
    type = RECURSE_NODE_WAY;
  else
  {
    type = 0;
    ostringstream temp;
    temp<<"For the attribute \"type\" of the element \"recurse\""
	<<" the only allowed values are \"relation-relation\", \"relation-backwards\","
	<<"\"relation-way\", \"relation-node\", \"way-node\", \"way-relation\","
	<<"\"node-relation\" or \"node-way\".";
    //add_static_error(temp.str());
  }
}

void Recurse_Statement::forecast()
{
/*  Set_Forecast sf_in(declare_read_set(input));
  Set_Forecast& sf_out(declare_write_set(output));
    
  if (type == RECURSE_RELATION_RELATION)
  {
    sf_out.relation_count = sf_in.relation_count;
    declare_used_time(100*sf_in.relation_count);
  }
  else if (type == RECURSE_RELATION_BACKWARDS)
  {
    sf_out.relation_count = sf_in.relation_count;
    declare_used_time(2000);
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    sf_out.way_count = 22*sf_in.relation_count;
    declare_used_time(100*sf_in.relation_count);
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    sf_out.node_count = 2*sf_in.relation_count;
    declare_used_time(100*sf_in.relation_count);
  }
  else if (type == RECURSE_WAY_NODE)
  {
    sf_out.node_count = 28*sf_in.way_count;
    declare_used_time(50*sf_in.way_count);
  }
  else if (type == RECURSE_WAY_RELATION)
  {
    sf_out.relation_count = sf_in.way_count/10;
    declare_used_time(2000);
  }
  else if (type == RECURSE_NODE_WAY)
  {
    sf_out.way_count = sf_in.node_count/2;
    declare_used_time(sf_in.node_count/1000); //TODO
  }
  else if (type == RECURSE_NODE_RELATION)
  {
    sf_out.relation_count = sf_in.node_count/100;
    declare_used_time(2000);
  }
    
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

void Recurse_Statement::execute(map< string, Set >& maps)
{
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(maps[output].nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways(maps[output].ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(maps[output].relations);
  //set< Area >& areas(maps[output].areas);
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
  {
    nodes.clear();
    ways.clear();
    relations.clear();
    //areas.clear();
    
    return;
  }
  
  if (type == RECURSE_RELATION_RELATION)
  {
    set< Uint31_Index > req;
    vector< uint32 > ids;
    
    {
      Random_File< Uint31_Index > random(*de_osm3s_file_ids::RELATIONS, false);
      for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
	   it(mit->second.relations.begin()); it != mit->second.relations.end(); ++it)
      {
	for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
	    it2 != it->second.end(); ++it2)
	{
	  for (vector< Relation_Entry >::const_iterator it3(it2->members.begin());
		      it3 != it2->members.end(); ++it3)
	  {
	    if (it3->type == Relation_Entry::RELATION)
	    {
	      req.insert(random.get(it3->ref));
	      ids.push_back(it3->ref);
	    }
	  }
	}
      }
    }
    sort(ids.begin(), ids.end());
    
    nodes.clear();
    ways.clear();
    relations.clear();
    //areas.clear();
  
    Block_Backend< Uint31_Index, Relation_Skeleton > relations_db
	(*de_osm3s_file_ids::RELATIONS, false);
    for (Block_Backend< Uint31_Index, Relation_Skeleton >::Discrete_Iterator
	 it(relations_db.discrete_begin(req.begin(), req.end()));
	 !(it == relations_db.discrete_end()); ++it)
    {
      if (binary_search(ids.begin(), ids.end(), it.object().id))
	relations[it.index()].push_back(it.object());
    }
  }
  else if (type == RECURSE_RELATION_BACKWARDS)
  {
  }
  else if (type == RECURSE_RELATION_WAY)
  {
    set< Uint31_Index > req;
    vector< uint32 > ids;
    
    {
      Random_File< Uint31_Index > random(*de_osm3s_file_ids::WAYS, false);
      for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
	   it(mit->second.relations.begin()); it != mit->second.relations.end(); ++it)
      {
	for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
	    it2 != it->second.end(); ++it2)
	{
	  for (vector< Relation_Entry >::const_iterator it3(it2->members.begin());
		      it3 != it2->members.end(); ++it3)
	  {
	    if (it3->type == Relation_Entry::WAY)
	    {
	      req.insert(random.get(it3->ref));
	      ids.push_back(it3->ref);
	    }
	  }
	}
      }
    }
    sort(ids.begin(), ids.end());
    
    nodes.clear();
    ways.clear();
    relations.clear();
    //areas.clear();
  
    Block_Backend< Uint31_Index, Way_Skeleton > ways_db
	(*de_osm3s_file_ids::WAYS, false);
    for (Block_Backend< Uint31_Index, Way_Skeleton >::Discrete_Iterator
	 it(ways_db.discrete_begin(req.begin(), req.end()));
	 !(it == ways_db.discrete_end()); ++it)
    {
      if (binary_search(ids.begin(), ids.end(), it.object().id))
	ways[it.index()].push_back(it.object());
    }
  }
  else if (type == RECURSE_RELATION_NODE)
  {
    set< Uint32_Index > req;
    vector< uint32 > ids;
    
    {
      Random_File< Uint32_Index > random(*de_osm3s_file_ids::NODES, false);
      for (map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator
	   it(mit->second.relations.begin()); it != mit->second.relations.end(); ++it)
      {
	for (vector< Relation_Skeleton >::const_iterator it2(it->second.begin());
	    it2 != it->second.end(); ++it2)
	{
	  for (vector< Relation_Entry >::const_iterator it3(it2->members.begin());
		      it3 != it2->members.end(); ++it3)
	  {
	    if (it3->type == Relation_Entry::NODE)
	    {
	      req.insert(random.get(it3->ref));
	      ids.push_back(it3->ref);
	    }
	  }
	}
      }
    }
    sort(ids.begin(), ids.end());
    
    nodes.clear();
    ways.clear();
    relations.clear();
    //areas.clear();
  
    Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
	(*de_osm3s_file_ids::NODES, false);
    for (Block_Backend< Uint32_Index, Node_Skeleton >::Discrete_Iterator
	 it(nodes_db.discrete_begin(req.begin(), req.end()));
	 !(it == nodes_db.discrete_end()); ++it)
    {
      if (binary_search(ids.begin(), ids.end(), it.object().id))
	nodes[it.index()].push_back(it.object());
    }
  }
  else if (type == RECURSE_WAY_NODE)
  {
    set< Uint32_Index > req;
    vector< uint32 > ids;
    
    {
      Random_File< Uint32_Index > random(*de_osm3s_file_ids::NODES, false);
      for (map< Uint31_Index, vector< Way_Skeleton > >::const_iterator
	   it(mit->second.ways.begin()); it != mit->second.ways.end(); ++it)
      {
	for (vector< Way_Skeleton >::const_iterator it2(it->second.begin());
	    it2 != it->second.end(); ++it2)
	{
	  for (vector< uint32 >::const_iterator it3(it2->nds.begin());
		      it3 != it2->nds.end(); ++it3)
	  {
	    req.insert(random.get(*it3));
	    ids.push_back(*it3);
	  }
	}
      }
    }
    sort(ids.begin(), ids.end());
    
    nodes.clear();
    ways.clear();
    relations.clear();
    //areas.clear();
  
    Block_Backend< Uint32_Index, Node_Skeleton > nodes_db
	(*de_osm3s_file_ids::NODES, false);
    for (Block_Backend< Uint32_Index, Node_Skeleton >::Discrete_Iterator
	 it(nodes_db.discrete_begin(req.begin(), req.end()));
	 !(it == nodes_db.discrete_end()); ++it)
    {
      if (binary_search(ids.begin(), ids.end(), it.object().id))
	nodes[it.index()].push_back(it.object());
    }
  }
  else if (type == RECURSE_WAY_RELATION)
  {
  }
  else if (type == RECURSE_NODE_WAY)
  {
  }
  else if (type == RECURSE_NODE_RELATION)
  {
  }
}
