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
#include "user_interface.h"
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
    temp<<"For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\" or \"body\".";
    add_static_error(temp.str());
  }
}

void Print_Statement::forecast(MYSQL* mysql)
{
  const Set_Forecast& sf_in(declare_read_set(input));
    
  if (mode == PRINT_IDS_ONLY)
    declare_used_time(1 + sf_in.node_count/100 + sf_in.way_count/100 + sf_in.relation_count/100
	+ sf_in.area_count/20);
  else if (mode == PRINT_SKELETON)
    declare_used_time(1 + sf_in.node_count/50 + sf_in.way_count/5 + sf_in.relation_count/5
	+ sf_in.area_count/5);
  else if (mode == PRINT_BODY)
    declare_used_time(10 + sf_in.node_count + sf_in.way_count + sf_in.relation_count
	+ sf_in.area_count);
  finish_statement_forecast();
    
  display_full();
  display_state();
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
    for (vector< uint32 >::const_iterator it2(way.members.begin());
	 it2 != way.members.end(); ++it2)
      cout<<"  <nd ref=\""<<*it2<<"\"/>\n";
    if (complete)
      cout<<"</way>\n";
  }
}

void out_relation
    (const Relation_& rel, const vector< string >& role_cache,
     bool complete = true)
{
  if ((rel.data.size() == 0) && (complete))
    cout<<"<relation id=\""<<rel.head<<"\"/>\n";
  else
  {
    cout<<"<relation id=\""<<rel.head<<"\">\n";
    for (vector< Relation_Member >::const_iterator it2(rel.data.begin());
         it2 != rel.data.end(); ++it2)
    {
      cout<<"  <member type=\""<<types_lowercase[it2->type]
	  <<"\" ref=\""<<it2->id<<"\" role=\"";
      escape_xml(cout, role_cache[it2->role+1]);
      cout<<"\"/>\n";
    }
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
      for (set< Relation_ >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
	cout<<"<relation id=\""<<it->head<<"\"/>\n";
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
      for (set< Relation_ >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
	out_relation(*it, role_cache);
      for (set< Area >::const_iterator it(mit->second.get_areas().begin());
	   it != mit->second.get_areas().end(); ++it)
	out_area(*it);
    }
    else if (mode == PRINT_BODY)
    {
      set< Node >::const_iterator it_nodes(mit->second.get_nodes().begin());
      while (it_nodes != mit->second.get_nodes().end())
      {
        set< Node >::const_iterator it(it_nodes);
        vector< vector< pair< string, string > > > tags;
	multiNode_to_kvs_query(mit->second.get_nodes(), it_nodes, tags);
	vector< vector< pair< string, string > > >::const_iterator tit(tags.begin());
        while (it != it_nodes)
        {
          if (tit->empty())
            out_node(*it);
          else
          {
            out_node(*it, false);
            for (vector< pair< string, string > >::const_iterator tit2(tit->begin());
                 tit2 != tit->end(); ++tit2)
	    {
	      cout<<"  <tag k=\"";
	      escape_xml(cout, tit2->first);
	      cout<<"\" v=\"";
	      escape_xml(cout, tit2->second);
	      cout<<"\"/>\n";
	    }
	    cout<<"</node>\n";
	  }
          ++it;
          ++tit;
        }
      }
      set< Way >::const_iterator it_ways(mit->second.get_ways().begin());
      while (it_ways != mit->second.get_ways().end())
      {
	set< Way >::const_iterator it(it_ways);
	vector< vector< pair< string, string > > > tags;
	multiWay_to_kvs_query(mit->second.get_ways(), it_ways, tags);
	vector< vector< pair< string, string > > >::const_iterator tit(tags.begin());
	while (it != it_ways)
	{
	  if (tit->empty())
	    out_way(*it);
	  else
	  {
	    out_way(*it, false);
	    for (vector< pair< string, string > >::const_iterator tit2(tit->begin());
			tit2 != tit->end(); ++tit2)
	    {
	      cout<<"  <tag k=\"";
	      escape_xml(cout, tit2->first);
	      cout<<"\" v=\"";
	      escape_xml(cout, tit2->second);
	      cout<<"\"/>\n";
	    }
	    cout<<"</way>\n";
	  }
	  ++it;
	  ++tit;
	}
      }
      set< Relation_ >::const_iterator it_relations(mit->second.get_relations().begin());
      while (it_relations != mit->second.get_relations().end())
      {
	set< Relation_ >::const_iterator it(it_relations);
	vector< vector< pair< string, string > > > tags;
	multiRelation_to_kvs_query(mit->second.get_relations(), it_relations, tags);
	vector< vector< pair< string, string > > >::const_iterator tit(tags.begin());
	while (it != it_relations)
	{
	  if (tit->empty())
	    out_relation(*it, role_cache);
	  else
	  {
	    out_relation(*it, role_cache, false);
	    for (vector< pair< string, string > >::const_iterator tit2(tit->begin());
			tit2 != tit->end(); ++tit2)
	    {
	      cout<<"  <tag k=\"";
	      escape_xml(cout, tit2->first);
	      cout<<"\" v=\"";
	      escape_xml(cout, tit2->second);
	      cout<<"\"/>\n";
	    }
	    cout<<"</relation>\n";
	  }
	  ++it;
	  ++tit;
	}
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
	mysql_free_result(result);
      }
    }
  }
}
