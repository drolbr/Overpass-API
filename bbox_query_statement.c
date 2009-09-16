#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "user_interface.h"
#include "bbox_query_statement.h"

#include <mysql.h>

using namespace std;

void Bbox_Query_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["into"] = "_";
  attributes["s"] = "";
  attributes["n"] = "";
  attributes["w"] = "";
  attributes["e"] = "";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["into"];
  double s_d(atof(attributes["s"].c_str()));
  if ((s_d < -90.0) || (s_d > 90.0) || (attributes["s"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"s\" of the element \"bbox-query\""
	<<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  double n_d(atof(attributes["n"].c_str()));
  if ((n_d < -90.0) || (n_d > 90.0) || (attributes["n"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"n\" of the element \"bbox-query\""
	<<" the only allowed values are floats between -90.0 and 90.0.";
    add_static_error(temp.str());
  }
  double w_d(atof(attributes["w"].c_str()));
  if ((w_d < -180.0) || (w_d > 180.0) || (attributes["w"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"w\" of the element \"bbox-query\""
	<<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  double e_d(atof(attributes["e"].c_str()));
  if ((e_d < -180.0) || (e_d > 180.0) || (attributes["e"] == ""))
  {
    ostringstream temp;
    temp<<"For the attribute \"e\" of the element \"bbox-query\""
	<<" the only allowed values are floats between -180.0 and 180.0.";
    add_static_error(temp.str());
  }
  south = (int)(s_d * 10000000 + 0.5);
  north = (int)(n_d * 10000000 + 0.5);
  west = (int)(w_d * 10000000 + 0.5);
  east = (int)(e_d * 10000000 + 0.5);
}

void Bbox_Query_Statement::indices_of_bbox
    (int32 south, int32 north, int32 west, int32 east)
{
  if (east < west)
  {
    indices_of_bbox(south, north, east, 180*10*1000*1000);
    east = -180*10*1000*1000;
  }
  
  int size(64*256*256*256 - 1);
  vector< pair< int, int > > pending;
  pending.push_back(make_pair(-90*10*1000*1000, -128*256*256*256));
  pending.push_back(make_pair(-90*10*1000*1000, 0));
  while (size >= 256*256 - 1)
  {
    const char INSIDE = 1;
    const char BORDER = 2;
    vector< pair< int, int > > next;
    for (vector< pair< int, int > >::const_iterator it(pending.begin()); it != pending.end(); ++it)
    {
      char lower(0), upper(0), left(0), right(0);
      if (south <= it->first)
      {
	if (north <= it->first + size)
	  lower = ((north >= it->first) ? BORDER : 0);
	else
	{
	  lower = INSIDE;
	  upper = ((north < it->first + 2*size + 1) ? BORDER : INSIDE);
	}
      }
      else
      {
	if (south <= it->first + size)
	{
	  lower = BORDER;
	  if (north > it->first + size)
	    upper = ((north < it->first + 2*size + 1) ? BORDER : INSIDE);
	}
	else
	  upper = ((south <= it->first + 2*size + 1) ? BORDER : 0);
      }
      if (west <= it->second)
      {
	if (east <= it->second + size)
	  left = ((east >= it->second) ? BORDER : 0);
	else
	{
	  left = INSIDE;
	  right = ((east < it->second + 2*size + 1) ? BORDER : INSIDE);
	}
      }
      else
      {
	if (west <= it->second + size)
	{
	  left = BORDER;
	  if (east > it->second + size)
	    right = ((east < it->second + 2*size + 1) ? BORDER : INSIDE);
	}
	else
	  right = ((west <= it->second + 2*size + 1) ? BORDER : 0);
      }
      if ((lower == INSIDE) && (left == INSIDE))
	in_inside.insert(make_pair< int, int >
	    (ll_idx(it->first, it->second), ll_idx(it->first + size, it->second + size)));
      else if ((lower) && (left))
	next.push_back(*it);
      if ((lower == INSIDE) && (right == INSIDE))
	in_inside.insert(make_pair< int, int >
	    (ll_idx(it->first, it->second + size + 1),
	     ll_idx(it->first + size, it->second + 2*size + 1)));
      else if ((lower) && (right))
	next.push_back(make_pair< int, int >
	    (it->first, it->second + size + 1));
      if ((upper == INSIDE) && (left == INSIDE))
	in_inside.insert(make_pair< int, int >
	    (ll_idx(it->first + size + 1, it->second),
	     ll_idx(it->first + 2*size + 1, it->second + size)));
      else if ((upper) && (left))
	next.push_back(make_pair< int, int >
	    (it->first + size + 1, it->second));
      if ((upper == INSIDE) && (right == INSIDE))
	in_inside.insert(make_pair< int, int >
	    (ll_idx(it->first + size + 1, it->second + size + 1),
	     ll_idx(it->first + 2*size + 1, it->second + 2*size + 1)));
      else if ((upper) && (right))
	next.push_back(make_pair< int, int >
	    (it->first + size + 1, it->second + size + 1));
    }
    pending = next;
    size = size/2;
  }
  for (vector< pair< int, int > >::const_iterator it(pending.begin()); it != pending.end(); ++it)
    in_border.insert(make_pair< int, int >
	(ll_idx(it->first, it->second), ll_idx(it->first + size, it->second + size)));
}

void Bbox_Query_Statement::forecast(MYSQL* mysql)
{
  Set_Forecast& sf_out(declare_write_set(output));
  
  indices_of_bbox(south, north, west, east);
  
  sf_out.node_count = multiRange_to_count_query(in_inside, in_border);
  declare_used_time(1000 + sf_out.node_count/100);
  finish_statement_forecast();
  
  display_full();
  display_state();
}

bool Bbox_Query_Statement::is_contained(const Node& node)
{
  if (west <= east)
  {
    if ((node.lat >= south) && (node.lat <= north) &&
	 (node.lon >= west) && (node.lon <= east))
      return true;
  }
  else
  {
    if ((node.lat >= south) && (node.lat <= north) &&
	 ((node.lon >= west) || (node.lon <= east)))
      return true;
  }
  return false;
}

void Bbox_Query_Statement::get_nodes(MYSQL* mysql, set< Node >& nodes)
{
  indices_of_bbox(south, north, west, east);
  set< Node > on_border;
  multiRange_to_multiNode_query(in_inside, in_border, nodes, on_border);
  for (set< Node >::const_iterator it(on_border.begin()); it != on_border.end(); ++it)
  {
    if (is_contained(*it))
      nodes.insert(*it);
  }
}

uint32 Bbox_Query_Statement::prepare_split(MYSQL* mysql)
{
  indices_of_bbox(south, north, west, east);
  
  multiRange_to_split_query(in_inside, in_border, in_inside_v, in_border_v);
  
  return in_inside_v.size();
}

void Bbox_Query_Statement::get_nodes(MYSQL* mysql, set< Node >& nodes, uint32 part)
{
  indices_of_bbox(south, north, west, east);
  set< Node > on_border;
  multiRange_to_multiNode_query(in_inside_v[part], in_border_v[part], nodes, on_border);
  for (set< Node >::const_iterator it(on_border.begin()); it != on_border.end(); ++it)
  {
    if (is_contained(*it))
      nodes.insert(*it);
  }
}

void Bbox_Query_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  set< Node >* nodes(&(maps[output].get_nodes_handle()));
  set< Way >* ways(&(maps[output].get_ways_handle()));
  set< Relation_ >* relations(&(maps[output].get_relations_handle()));
  set< Area >* areas(&(maps[output].get_areas_handle()));
  nodes->clear();
  ways->clear();
  relations->clear();
  areas->clear();
  
  get_nodes(mysql, *nodes);
}
