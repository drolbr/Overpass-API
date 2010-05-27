#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../backend/block_backend.h"
#include "../backend/random_file.h"
#include "../core/settings.h"
#include "print.h"

using namespace std;

const unsigned int PRINT_IDS = 1;
const unsigned int PRINT_COORDS = 2;
const unsigned int PRINT_NDS = 4;
const unsigned int PRINT_MEMBERS = 8;
const unsigned int PRINT_TAGS = 16;

const unsigned int ORDER_BY_ID = 1;
const unsigned int ORDER_BY_QUADTILE = 2;

void Print_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["mode"] = "body";
  attributes["order"] = "id";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["from"];
  
  if (attributes["mode"] == "ids_only")
    mode = PRINT_IDS;
  else if (attributes["mode"] == "skeleton")
    mode = PRINT_IDS|PRINT_COORDS|PRINT_NDS|PRINT_MEMBERS;
  else if (attributes["mode"] == "body")
    mode = PRINT_IDS|PRINT_COORDS|PRINT_NDS|PRINT_MEMBERS|PRINT_TAGS;
  else
  {
    mode = 0;
    ostringstream temp;
    temp<<"For the attribute \"mode\" of the element \"print\""
	<<" the only allowed values are \"ids_only\", \"skeleton\" or \"body\".";
    //add_static_error(temp.str());
  }
  if (attributes["order"] == "id")
    order = ORDER_BY_ID;
  else if (attributes["order"] == "quadtile")
    order = ORDER_BY_QUADTILE;
  else
  {
    order = 0;
    ostringstream temp;
    temp<<"For the attribute \"order\" of the element \"print\""
    <<" the only allowed values are \"id\" or \"quadtile\".";
    //add_static_error(temp.str());
  }
}

void Print_Statement::forecast()
{
/*  const Set_Forecast& sf_in(declare_read_set(input));
    
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
  display_state();*/
}

/*void out_node(const Node& node, bool complete = true)
{
  User_Output& out(get_output());
  
  out.print("<node id=\"");
  out.print(node.id);
  out.print("\" lat=\"");
  out.print(12, ((double)(node.lat))/10000000);
  out.print("\" lon=\"");
  out.print(12, ((double)(node.lon))/10000000);
  out.print("\"");
  out.print((complete ? "/>" : ">"));
  out.print("\n");
}

void out_way(const Way& way, bool complete = true)
{
  User_Output& out(get_output());
  
  if (way.members.size() == 0)
  {
    out.print("<way id=\"");
    out.print((long long)way.id);
    out.print("\"");
    out.print((complete ? "/>" : ">"));
    out.print("\n");
  }
  else
  {
    out.print("<way id=\"");
    out.print((long long)way.id);
    out.print("\">\n");
    for (vector< uint32 >::const_iterator it2(way.members.begin());
	 it2 != way.members.end(); ++it2)
    {
      out.print("  <nd ref=\"");
      out.print((long long)*it2);
      out.print("\"/>\n");
    }
    if (complete)
      out.print("</way>\n");
  }
}

void out_relation
    (const Relation_& rel, const vector< string >& role_cache,
     bool complete = true)
{
  User_Output& out(get_output());
  
  if ((rel.data.size() == 0) && (complete))
  {
    out.print("<relation id=\"");
    out.print((long long)rel.head);
    out.print("\"/>\n");
  }
  else
  {
    out.print("<relation id=\"");
    out.print((long long)rel.head);
    out.print("\">\n");
    for (vector< Relation_Member >::const_iterator it2(rel.data.begin());
         it2 != rel.data.end(); ++it2)
    {
      out.print("  <member type=\"");
      out.print(types_lowercase[it2->type]);
      out.print("\" ref=\"");
      out.print((long long)it2->id);
      out.print("\" role=\"");
      out.print(escape_xml(role_cache[it2->role+1]));
      out.print("\"/>\n");
    }
    if (complete)
      out.print("</relation>\n");
  }
}

void out_area(const Area& area, bool complete = true)
{
  User_Output& out(get_output());
  
  if (complete)
  {
    out.print("<area id=\"");
    out.print((long long)area.id);
    out.print("\"/>\n");
  }
  else
  {
    out.print("<area id=\"");
    out.print((long long)area.id);
    out.print("\">\n");
      //TODO: temporary output
*	for (set< Line_Segment >::const_iterator it2(it->segments.begin());
    it2 != it->segments.end(); ++it2)
    out<<"  <vx west=\""<<it2->west_lat<<' '<<it2->west_lon
    <<"\" east=\""<<it2->east_lat<<' '<<it2->east_lon<<"\"/>\n";
    out<<"</area>\n";*
  }
}*/

//print_nodes_by_quadtile(m, coarse, m<v<v<p*>>>)

void print_node(uint32 ll_upper, const Node_Skeleton& skel,
		const vector< pair< string, string > >& tags)
{
  cout<<"  <node id=\""<<skel.id<<"\" lat=\""<<setprecision(10)
      <<Node::lat(ll_upper, skel.ll_lower)<<"\" lon=\""
      <<Node::lon(ll_upper, skel.ll_lower)<<"\">\n";
  for (vector< pair< string, string > >::const_iterator it(tags.begin());
      it != tags.end(); ++it)
    cout<<"    <tag k=\""<<it->first<<"\" v=\""<<it->second<<"\"/>\n";
  cout<<"  </node>\n";
}

void retrieve_node_tags_quadtile
    (const map< Uint32_Index, vector< Node_Skeleton > >& nodes)
{
  //generate set of relevant coarse indices
  set< uint32 > coarse_indices;
  for (map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      it(nodes.begin()); it != nodes.end(); ++it)
    coarse_indices.insert(it->first.val() & 0xffffff00);
  
  //formulate range query
  set< pair< Tag_Index_Local, Tag_Index_Local > > range_set;
  for (set< uint32 >::const_iterator
    it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    Tag_Index_Local lower, upper;
    lower.index = *it;
    lower.key = "";
    lower.value = "";
    upper.index = (*it) + 1;
    upper.key = "";
    upper.value = "";
    range_set.insert(make_pair(lower, upper));
  }
  
  // iterate over the result
  Block_Backend< Tag_Index_Local, Uint32_Index > nodes_db
      (*de_osm3s_file_ids::NODE_TAGS_LOCAL, true);
  Block_Backend< Tag_Index_Local, Uint32_Index >::Range_Iterator
    tag_it(nodes_db.range_begin
    (Default_Range_Iterator< Tag_Index_Local >(range_set.begin()),
     Default_Range_Iterator< Tag_Index_Local >(range_set.end())));
  map< Uint32_Index, vector< Node_Skeleton > >::const_iterator
      node_it(nodes.begin());
  for (set< uint32 >::const_iterator
    it(coarse_indices.begin()); it != coarse_indices.end(); ++it)
  {
    map< uint32, vector< pair< string, string > > > tags_by_id;
    while ((!(tag_it == nodes_db.range_end())) && (tag_it.index().index == *it))
    {
      tags_by_id[tag_it.object().val()].push_back
          (make_pair(tag_it.index().key, tag_it.index().value));
      ++tag_it;
    }
    
    // print the result
    while ((node_it != nodes.end()) && ((node_it->first.val() & 0xffffff00) == *it))
    {
      for (vector< Node_Skeleton >::const_iterator it2(node_it->second.begin());
          it2 != node_it->second.end(); ++it2)
      print_node(node_it->first.val(), *it2, tags_by_id[it2->id]);
      ++node_it;
    }
  }
};

void Print_Statement::execute(map< string, Set >& maps)
{
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit == maps.end())
    return;
  retrieve_node_tags_quadtile(mit->second.nodes);
  
  /*  const vector< string >& role_cache(get_role_cache());
  User_Output& out(get_output());
  
  map< string, Set >::const_iterator mit(maps.find(input));
  if (mit != maps.end())
  {
    if (mode == PRINT_IDS_ONLY)
    {
      for (set< Node >::const_iterator it(mit->second.get_nodes().begin());
	   it != mit->second.get_nodes().end(); ++it)
      {
	out.print("<node id=\"");
	out.print(it->id);
	out.print("\"/>\n");
      }
      for (set< Way >::const_iterator it(mit->second.get_ways().begin());
	   it != mit->second.get_ways().end(); ++it)
      {
	out.print("<way id=\"");
	out.print((long long)it->id);
	out.print("\"/>\n");
      }
      for (set< Relation_ >::const_iterator it(mit->second.get_relations().begin());
	   it != mit->second.get_relations().end(); ++it)
      {
	out.print("<relation id=\"");
	out.print((long long)it->head);
	out.print("\"/>\n");
      }
      for (set< Area >::const_iterator it(mit->second.get_areas().begin());
	   it != mit->second.get_areas().end(); ++it)
      {
	out.print("<area id=\"");
	out.print((long long)it->id);
	out.print("\"/>\n");
      }
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
	      out.print("  <tag k=\"");
	      out.print(escape_xml(tit2->first));
	      out.print("\" v=\"");
	      out.print(escape_xml(tit2->second));
	      out.print("\"/>\n");
	    }
	    out.print("</node>\n");
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
	      out.print("  <tag k=\"");
	      out.print(escape_xml(tit2->first));
	      out.print("\" v=\"");
	      out.print(escape_xml(tit2->second));
	      out.print("\"/>\n");
	    }
	    out.print("</way>\n");
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
	      out.print("  <tag k=\"");
	      out.print(escape_xml(tit2->first));
	      out.print("\" v=\"");
	      out.print(escape_xml(tit2->second));
	      out.print("\"/>\n");
	    }
	    out.print("</relation>\n");
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
	  uint32 id(atoll(row[0]));
	  while (it2->id < id)
	  {
	    out_area(*it2);
	    ++it2;
	  }
	  out_area(*it2, false);
	  while ((row) && (row[0]) && (it2->id == (uint32)atoll(row[0])))
	  {
	    if ((row[1]) && (row[2]))
	    {
	      out.print("  <tag k=\"");
	      out.print(row[1]);
	      out.print("\" v=\"");
	      out.print(row[2]);
	      out.print("\"/>\n");
	    }
	    row = mysql_fetch_row(result);
	  }
	  out.print("</area>\n");
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
  }*/
}
