#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "union.h"

using namespace std;

void Union_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
}

void Union_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if (statement->get_result_name() != "")
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Union_Statement::forecast()
{
/*  int node_count(0);
  int way_count(0);
  int relation_count(0);
  int area_count(0);
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->forecast(mysql);
    const Set_Forecast& sf_in(declare_read_set((*it)->get_result_name()));
    node_count += sf_in.node_count;
    way_count += sf_in.way_count;
    relation_count += sf_in.relation_count;
    area_count += sf_in.area_count;
  }
  
  Set_Forecast& sf_out(declare_write_set(output));
  
  sf_out.node_count = node_count;
  sf_out.way_count = way_count;
  sf_out.relation_count = relation_count;
  sf_out.area_count = area_count;
  declare_used_time(0);
  finish_statement_forecast();
    
  display_full();
  display_state();*/
}

void Union_Statement::execute(Resource_Manager& rman)
{
  stopwatch.start();

  Set base_set;
  rman.push_reference(base_set);
  map< Uint32_Index, vector< Node_Skeleton > >& nodes(base_set.nodes);
  map< Uint31_Index, vector< Way_Skeleton > >& ways(base_set.ways);
  map< Uint31_Index, vector< Relation_Skeleton > >& relations(base_set.relations);
  map< Uint31_Index, vector< Area_Skeleton > >& areas(base_set.areas);
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    stopwatch.stop(Stopwatch::NO_DISK);
    (*it)->execute(rman);
    stopwatch.skip();
    stopwatch.sum((*it)->stopwatch);
    
    Set& summand(rman.sets()[(*it)->get_result_name()]);

    for (map< Uint32_Index, vector< Node_Skeleton > >::iterator
        it(summand.nodes.begin()); it != summand.nodes.end(); ++it)
    {
      sort(it->second.begin(), it->second.end());
      vector< Node_Skeleton > other(nodes[it->first]);
      nodes[it->first].clear();
      set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
	    back_inserter(nodes[it->first]));
    }
    for (map< Uint31_Index, vector< Way_Skeleton > >::iterator
        it(summand.ways.begin()); it != summand.ways.end(); ++it)
    {
      sort(it->second.begin(), it->second.end());
      vector< Way_Skeleton > other(ways[it->first]);
      ways[it->first].clear();
      set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
	    back_inserter(ways[it->first]));
    }
    for (map< Uint31_Index, vector< Relation_Skeleton > >::iterator
        it(summand.relations.begin()); it != summand.relations.end(); ++it)
    {
      sort(it->second.begin(), it->second.end());
      vector< Relation_Skeleton > other(relations[it->first]);
      relations[it->first].clear();
      set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
	    back_inserter(relations[it->first]));
    }
    for (map< Uint31_Index, vector< Area_Skeleton > >::iterator
      it(summand.areas.begin()); it != summand.areas.end(); ++it)
    {
      sort(it->second.begin(), it->second.end());
      vector< Area_Skeleton > other(areas[it->first]);
      areas[it->first].clear();
      set_union(it->second.begin(), it->second.end(), other.begin(), other.end(),
		back_inserter(areas[it->first]));
    }
  }
  
  rman.sets()[output] = base_set;
  
  stopwatch.stop(Stopwatch::NO_DISK);
  stopwatch.report(get_name());
  rman.pop_reference();
  rman.health_check(*this);
}
