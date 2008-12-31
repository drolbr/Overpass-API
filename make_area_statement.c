#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "cgi-helper.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "make_area_statement.h"

#include <mysql.h>

using namespace std;

void Make_Area_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  output = attributes["into"];
}

void Make_Area_Statement::add_statement(Statement* statement)
{
  substatement_error(get_name(), statement);
}

inline void area_insert(Area& area, int lat1, int lon1, int lat2, int lon2)
{
  if (lon1 < lon2)
  {
    unsigned int div((lon2 - lon1)/100000+1);
    for (unsigned int i(0); i < div; ++i)
      area.segments.insert(Line_Segment
	  (lat1+((long long)(lat2-lat1))*i/div, lon1+((long long)(lon2-lon1))*i/div,
	   lat1+((long long)(lat2-lat1))*(i+1)/div, lon1+((long long)(lon2-lon1))*(i+1)/div));
  }
  else
  {
    unsigned int div((lon1 - lon2)/100000+1);
    for (unsigned int i(0); i < div; ++i)
    {
      area.segments.insert(Line_Segment
	  (lat2+((long long)(lat1-lat2))*i/div, lon2+((long long)(lon1-lon2))*i/div,
	   lat2+((long long)(lat1-lat2))*(i+1)/div, lon2+((long long)(lon1-lon2))*(i+1)/div));
    }
  }
}

void Make_Area_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node > nodes;
  set< Way > ways;
  set< Relation > relations;
  set< Area > areas;
  if (input == output)
  {
    nodes = maps[output].get_nodes();
    ways = maps[output].get_ways();
    relations = maps[output].get_relations();
    areas = maps[output].get_areas();
  }
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
  {
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  const set< Node >& in_nodes(mit->second.get_nodes());
  
  Area area(next_area_id--);
  set< int > node_parity_control;
  for (set< Way >::const_iterator it(mit->second.get_ways().begin());
       it != mit->second.get_ways().end(); ++it)
  {
    set< Node >::const_iterator onit(in_nodes.end());
    vector< int >::const_iterator iit(it->members.begin());
    if (iit != it->members.end())
    {
      if (node_parity_control.find(*iit) != node_parity_control.end())
	node_parity_control.erase(*iit);
      else
	node_parity_control.insert(*iit);
      onit = in_nodes.find(Node(*iit, 0, 0));
      if (onit == in_nodes.end())
      {
	cout<<"Error: Node "<<*iit<<" referred by way "<<it->id
	    <<" is not contained in set \""<<input<<"\".\n";
	maps[output] = Set(nodes, ways, relations, areas);
	return;
      }
      ++iit;
    }
    for (; iit != it->members.end(); ++iit)
    {
      set< Node >::const_iterator nnit(in_nodes.find(Node(*iit, 0, 0)));
      if (nnit == in_nodes.end())
      {
	cout<<"Error: Node "<<*iit<<" referred by way "<<it->id
	    <<" is not contained in set \""<<input<<"\".\n";
	maps[output] = Set(nodes, ways, relations, areas);
	return;
      }
      if (calc_idx(onit->lat) != calc_idx(nnit->lat))
      {
	if (calc_idx(onit->lat) < calc_idx(nnit->lat))
	{
	  area_insert(area, onit->lat, onit->lon, (calc_idx(onit->lat)+1)*10000000,
		      onit->lon + (long long)(nnit->lon - onit->lon)
			  *((calc_idx(onit->lat)+1)*10000000 - onit->lat)/(nnit->lat - onit->lat));
	  for (int i(calc_idx(onit->lat)+1); i < calc_idx(nnit->lat); ++i)
	    area_insert(area, i*10000000,
			onit->lon + (long long)(nnit->lon - onit->lon)
			    *(i*10000000 - onit->lat)/(nnit->lat - onit->lat),
			      (i+1)*10000000,
			onit->lon + (long long)(nnit->lon - onit->lon)
			    *((i+1)*10000000 - onit->lat)/(nnit->lat - onit->lat));
	  area_insert(area, calc_idx(nnit->lat)*10000000,
		      onit->lon + (long long)(nnit->lon - onit->lon)
			  *(calc_idx(nnit->lat)*10000000 - onit->lat)/(nnit->lat - onit->lat),
			    nnit->lat, nnit->lon);
	}
	else
	{
	  area_insert(area, nnit->lat, nnit->lon, (calc_idx(nnit->lat)+1)*10000000,
		      nnit->lon + (long long)(onit->lon - nnit->lon)
			  *((calc_idx(nnit->lat)+1)*10000000 - nnit->lat)/(onit->lat - nnit->lat));
	  for (int i(calc_idx(nnit->lat)+1); i < calc_idx(onit->lat); ++i)
	    area_insert(area, i*10000000,
			nnit->lon + (long long)(onit->lon - nnit->lon)
			    *(i*10000000 - nnit->lat)/(onit->lat - nnit->lat),
			      (i+1)*10000000,
			       nnit->lon + (long long)(onit->lon - nnit->lon)
				   *((i+1)*10000000 - nnit->lat)/(onit->lat - nnit->lat));
	  area_insert(area, calc_idx(onit->lat)*10000000,
		      nnit->lon + (long long)(onit->lon - nnit->lon)
			  *(calc_idx(onit->lat)*10000000 - nnit->lat)/(onit->lat - nnit->lat),
			    onit->lat, onit->lon);
	}
      }
      else
	area_insert(area, onit->lat, onit->lon, nnit->lat, nnit->lon);
      onit = nnit;
    }
    if (it->members.size() > 0)
    {
      if (node_parity_control.find(onit->id) != node_parity_control.end())
	node_parity_control.erase(onit->id);
      else
	node_parity_control.insert(onit->id);
    }
  }
  if (node_parity_control.size() > 0)
  {
    cout<<"Error: Node "<<*(node_parity_control.begin())
	<<" is contained in an odd number of ways.\n";
    maps[output] = Set(nodes, ways, relations, areas);
    return;
  }
  
  areas.insert(area);
  maps[output] = Set(nodes, ways, relations, areas);
}
