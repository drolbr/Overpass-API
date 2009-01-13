#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "print_statement.h"

#include <mysql.h>

using namespace std;

const unsigned int PRINT_IDS_ONLY = 1;
const unsigned int PRINT_SKELETON = 2;
const unsigned int PRINT_BODY = 3;

void Print_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["mode"] = "skeleton";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  
  if (attributes["mode"] == "ids_only")
    mode = PRINT_IDS_ONLY;
  else if (attributes["mode"] == "skeleton")
    mode = PRINT_SKELETON;
  else if (attributes["mode"] == "body")
    mode = PRINT_BODY;
  else
  {
    mode = 0;
    ostringstream temp;
    temp<<"In Line "<<current_line_number()
	<<": For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\" or \"body\".";
    add_static_error(Error(temp.str(), current_line_number()));
  }
}

void out_node(const Node& node, bool complete = true)
{
  cout<<"<node id=\""<<node.id
      <<"\" lat=\""<<setprecision(12)<<((double)(node.lat))/10000000
      <<"\" lon=\""<<setprecision(12)<<((double)(node.lon))/10000000<<'\"'
      <<(complete ? "/>" : ">")<<'\n';
}

void out_way(const Way& way, bool complete = true)
{
  if (way.members.size() == 0)
    cout<<"<way id=\""<<way.id<<'\"'
	<<(complete ? "/>" : ">")<<'\n';
  else
  {
    cout<<"<way id=\""<<way.id<<"\">\n";
    for (vector< int >::const_iterator it2(way.members.begin());
	 it2 != way.members.end(); ++it2)
      cout<<"  <nd ref=\""<<*it2<<"\"/>\n";
    if (complete)
      cout<<"</way>\n";
  }
}

void out_relation(const Relation& rel, const vector< string >& role_cache, bool complete = true)
{
  if ((rel.node_members.size() + rel.way_members.size() + rel.relation_members.size() == 0) && (complete))
    cout<<"<relation id=\""<<rel.id<<"\"/>\n";
  else
  {
    cout<<"<relation id=\""<<rel.id<<"\">\n";
    for (set< pair< int, int > >::const_iterator it2(rel.node_members.begin());
	 it2 != rel.node_members.end(); ++it2)
      cout<<"  <member type=\"node\" ref=\""<<it2->first
	  <<"\" role=\""<<role_cache[it2->second]<<"\"/>\n";
    for (set< pair< int, int > >::const_iterator it2(rel.way_members.begin());
	 it2 != rel.way_members.end(); ++it2)
      cout<<"  <member type=\"way\" ref=\""<<it2->first
	  <<"\" role=\""<<role_cache[it2->second]<<"\"/>\n";
    for (set< pair< int, int > >::const_iterator it2(rel.relation_members.begin());
	 it2 != rel.relation_members.end(); ++it2)
      cout<<"  <member type=\"relation\" ref=\""<<it2->first
	  <<"\" role=\""<<role_cache[it2->second]<<"\"/>\n";
    if (complete)
      cout<<"</relation>\n";
  }
}

void out_area(const Area& area, bool complete = true)
{
  if (complete)
    cout<<"<area id=\""<<area.id<<"\"/>\n";
  else
  {
    cout<<"<area id=\""<<area.id<<"\">\n";
      //TODO: temporary output
/*	for (set< Line_Segment >::const_iterator it2(it->segments.begin());
    it2 != it->segments.end(); ++it2)
    cout<<"  <vx west=\""<<it2->west_lat<<' '<<it2->west_lon
    <<"\" east=\""<<it2->east_lat<<' '<<it2->east_lon<<"\"/>\n";
    cout<<"</area>\n";*/
  }
}

void Print_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  const vector< string >& role_cache(get_role_cache());
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit != maps.end())
  {
    if (mode == PRINT_IDS_ONLY)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); ++it)
	cout<<"<node id=\""<<it->id<<"\"/>\n";
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); ++it)
	cout<<"<way id=\""<<it->id<<"\"/>\n";
      for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
	cout<<"<relation id=\""<<it->id<<"\"/>\n";
      for (set< Area >::const_iterator it(mit->second.get_areas().begin());
	   it != mit->second.get_areas().end(); ++it)
	cout<<"<area id=\""<<it->id<<"\"/>\n";
    }
    else if (mode == PRINT_SKELETON)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); ++it)
	out_node(*it);
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); ++it)
	out_way(*it);
      for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
	out_relation(*it, role_cache);
      for (set< Area >::const_iterator it(mit->second.get_areas().begin());
	   it != mit->second.get_areas().end(); ++it)
	out_area(*it);
    }
    else if (mode == PRINT_BODY)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); )
      {
	set< Node >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select node_tags.id, key_s.key_, value_s.value_ from node_tags "
	    <<"left join key_s on node_tags.key_ = key_s.id "
	    <<"left join value_s on node_tags.value_ = value_s.id "
	    <<"where node_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_nodes().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by node_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_node(*it2);
	    ++it2;
	  }
	  out_node(*it2, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</node>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_node(*it2);
	  ++it2;
	}
	delete result;
      }
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); )
      {
	set< Way >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select way_tags.id, key_s.key_, value_s.value_ from way_tags "
	    <<"left join key_s on way_tags.key_ = key_s.id "
	    <<"left join value_s on way_tags.value_ = value_s.id "
	    <<"where way_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_ways().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by way_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_way(*it2);
	    ++it2;
	  }
	  out_way(*it2, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</way>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_way(*it2);
	  ++it2;
	}
	delete result;
      }
      for (set< Relation >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); )
      {
	set< Relation >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select relation_tags.id, key_s.key_, value_s.value_ from relation_tags "
	    <<"left join key_s on relation_tags.key_ = key_s.id "
	    <<"left join value_s on relation_tags.value_ = value_s.id "
	    <<"where relation_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_relations().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by relation_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_relation(*it2, role_cache);
	    ++it2;
	  }
	  out_relation(*it2, role_cache, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</relation>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_relation(*it2, role_cache);
	  ++it2;
	}
	delete result;
      }
      for (set< Area >::const_iterator it(mit->second.get_areas().begin());
	   it != mit->second.get_areas().end(); )
      {
	set< Area >::const_iterator it2(it);
	ostringstream temp;
	temp<<"select area_tags.id, key_s.key_, value_s.value_ from area_tags "
	    <<"left join key_s on area_tags.key_ = key_s.id "
	    <<"left join value_s on area_tags.value_ = value_s.id "
	    <<"where area_tags.id in ("<<it->id;
	unsigned int i(0);
	while (((++it) != mit->second.get_areas().end()) && (i++ < 10000))
	  temp<<", "<<it->id;
	temp<<") order by area_tags.id";
	MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
	if (!result)
	  return;
	
	MYSQL_ROW row(mysql_fetch_row(result));
	while ((row) && (row[0]))
	{
	  int id(atoi(row[0]));
	  while (it2->id < id)
	  {
	    out_area(*it2);
	    ++it2;
	  }
	  out_area(*it2, false);
	  while ((row) && (row[0]) && (it2->id == atoi(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	      cout<<"  <tag k=\""<<row[1]<<"\" v=\""<<row[2]<<"\"/>\n";
	    row = mysql_fetch_row(result);
	  }
	  cout<<"</area>\n";
	  ++it2;
	}
	while (it2 != it)
	{
	  out_area(*it2);
	  ++it2;
	}
	delete result;
      }
    }
  }
}
