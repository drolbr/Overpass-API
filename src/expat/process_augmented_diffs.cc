/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of PT_Diagrams.
*
* PT_Diagrams is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PT_Diagrams is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with PT_Diagrams.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../expat/escape_xml.h"
#include "../expat/expat_justparse_interface.h"
#include "../overpass_api/core/datatypes.h"
#include "../overpass_api/data/geometry.h"

using namespace std;


struct Tag
{
  Tag(string name_) : name(name_) {}
  
  map< string, string > attributes;
  vector< Tag* > children;
  string name;
};


vector< Tag* > stack;
Tag* root;


void start(const char* el, const char** attr)
{
  Tag* tag = new Tag(el);
  
  for (unsigned int i(0); attr[i]; i += 2)
  {
    pair< string, string > attribute;
    attribute.first = attr[i];
    attribute.second = attr[i+1];
    tag->attributes.insert(attribute);
  }
  
  if (!stack.empty())
    stack.back()->children.push_back(tag);
  else
    root = tag;
  
  stack.push_back(tag);
}


void end(const char *el)
{
  stack.pop_back();
}


//Bitfield for actions to keep
const int INSIDE = 1;
const int ERASE = 2;
const int INSERT = 4;


const Tag* obtain_data_from_action(const Tag& tag, int flag = 0)
{
  if (tag.attributes.find("type")->second == "create")
    return tag.children[0];
  else if (tag.attributes.find("type")->second == "delete")
    return tag.children[0]->children[0];
  else if (tag.attributes.find("type")->second == "modify")
  {
    if (flag == ERASE)
      return tag.children[0]->children[0];
    else
      return tag.children[1]->children[0];
  }
  else if (tag.attributes.find("type")->second == "info")
    return tag.children[0];
  
  return 0;
}


void print_tag_firstversion(const Tag& tag, string indent)
{
  if (tag.name == "action")
  {
    map< string, string >::const_iterator type_it = tag.attributes.find("type");
    if (type_it == tag.attributes.end())
      return;
    else if (type_it->second == "create") {
      cout<<indent<<"<insert>\n";
      for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
        print_tag_firstversion(**it, indent + "  ");
      cout<<indent<<"</insert>\n";
    }
    else if (type_it->second == "info")
    {
      cout<<indent<<"<keep>\n";
      for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
        print_tag_firstversion(**it, indent + "  ");
      cout<<indent<<"</keep>\n";
    }
    else if (type_it->second == "modify")
    {
      cout<<indent<<"<erase>\n";
      print_tag_firstversion(*tag.children[0]->children[0], indent + "  ");
      cout<<indent<<"</erase>\n";
      cout<<indent<<"<insert>\n";
      print_tag_firstversion(*tag.children[1]->children[0], indent + "  ");
      cout<<indent<<"</insert>\n";
    }
    else if (type_it->second == "delete")
    {
      cout<<indent<<"<erase>\n";
      print_tag_firstversion(*tag.children[0]->children[0], indent + "  ");
      cout<<indent<<"</erase>\n";
    }
    return;
  }
  else if (tag.name == "bounds")
    return;
  else if (tag.name == "osmAugmentedDiff")
  {
    cout<<indent<<"<"<<tag.name;
    for (map< string, string >::const_iterator it = tag.attributes.begin();
         it != tag.attributes.end(); ++it)
    {
      if (it->first != "format")
        cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
    }
    if (tag.children.empty())
      cout<<"/>\n";
    else
    {
      cout<<">\n";
      for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
        print_tag_firstversion(**it, indent + "  ");
      cout<<indent<<"</"<<tag.name<<">\n";
    }
  }
  else if (tag.name == "note")
  {
    cout<<"<note>The data included in this document is from www.openstreetmap.org. "
          "The data is made available under ODbL.</note>\n";
    return;
  }
  
  cout<<indent<<"<"<<tag.name;
  for (map< string, string >::const_iterator it = tag.attributes.begin();
       it != tag.attributes.end(); ++it)
     cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
  if (tag.children.empty())
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
      print_tag_firstversion(**it, indent + "  ");
    cout<<indent<<"</"<<tag.name<<">\n";
  }
}


void print_tag_idsorted(const Tag& tag, string indent,
    bool bbox,
    map< Node::Id_Type, int >& node_actions,
    map< Way::Id_Type, int >& way_actions,
    map< Relation::Id_Type, int >& relation_actions);


void print_subtags(const Tag& tag, string indent,
    bool bbox,
    map< Node::Id_Type, int >& node_actions,
    map< Way::Id_Type, int >& way_actions,
    map< Relation::Id_Type, int >& relation_actions)
{
  if (tag.children.empty())
    cout<<"/>\n";
  else
  {
    cout<<">\n";
    for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
      print_tag_idsorted(**it, indent + "  ", bbox, node_actions, way_actions, relation_actions);
    cout<<indent<<"</"<<tag.name<<">\n";
  }
}


void print_tag_idsorted(const Tag& tag, string indent,
    bool bbox,
    map< Node::Id_Type, int >& node_actions,
    map< Way::Id_Type, int >& way_actions,
    map< Relation::Id_Type, int >& relation_actions)
{
  if (bbox && tag.name =="action")
  {
    const Tag* osm_tag = obtain_data_from_action(tag);
    
    int action = 0;
    if (osm_tag->name == "node" &&
        node_actions.find(Node::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str()))) != node_actions.end())
      action = node_actions.find(Node::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str())))->second;
    else if (osm_tag->name == "way" &&
        way_actions.find(Way::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str()))) != way_actions.end())
      action = way_actions.find(Way::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str())))->second;
    if (osm_tag->name == "relation" &&
        relation_actions.find(Relation::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str()))) !=
        relation_actions.end())
      action = relation_actions.find(Relation::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str())))->second;
    
    if (tag.attributes.find("type")->second == "create")
    {
      if (action & (INSERT | INSIDE))
      {
        cout<<indent<<"<"<<tag.name;
        for (map< string, string >::const_iterator it = tag.attributes.begin();
            it != tag.attributes.end(); ++it)
          cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
        print_subtags(tag, indent, bbox, node_actions, way_actions, relation_actions);
      }
      return;
    }
    else if (tag.attributes.find("type")->second == "delete")
    {
      if (action & (ERASE | INSIDE))
      {
        cout<<indent<<"<"<<tag.name;
        for (map< string, string >::const_iterator it = tag.attributes.begin();
            it != tag.attributes.end(); ++it)
          cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
        print_subtags(tag, indent, bbox, node_actions, way_actions, relation_actions);
      }
      return;
    }
    else if (tag.attributes.find("type")->second == "info")
    {
      Tag modified_tag = tag;
      if (action & INSIDE)
	; // Nothing needs be changed
      else if (action & ERASE)
      {
	if (action & INSERT)
	  ; // Nothing needs be changed
	else
	{
	  modified_tag.attributes["type"] = "delete";
	  modified_tag.children.clear();

	  Tag old_tag("old");
	  old_tag.children.push_back(tag.children[0]);
	  modified_tag.children.push_back(&old_tag);
	  
	  Tag new_tag("new");
	  Tag copied_tag = *tag.children[0];
	  copied_tag.children.clear();
	  copied_tag.attributes.erase("lat");
	  copied_tag.attributes.erase("lon");
	  copied_tag.attributes["visible"] = "false";
	  new_tag.children.push_back(&copied_tag);
	  modified_tag.children.push_back(&new_tag);
	  
          cout<<indent<<"<"<<tag.name;
          for (map< string, string >::const_iterator it = modified_tag.attributes.begin();
              it != modified_tag.attributes.end(); ++it)
            cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
          print_subtags(modified_tag, indent, bbox, node_actions, way_actions, relation_actions);
          return;
	}
      }
      else
      {
	if (action & INSERT)
	  modified_tag.attributes["type"] = "create";
	else
	  return;
      }
	
      cout<<indent<<"<"<<tag.name;
      for (map< string, string >::const_iterator it = modified_tag.attributes.begin();
          it != modified_tag.attributes.end(); ++it)
        cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
      print_subtags(modified_tag, indent, bbox, node_actions, way_actions, relation_actions);
      return;
    }
    else if (tag.attributes.find("type")->second == "modify")
    {
      Tag modified_tag = tag;
      if (action & INSIDE)
	; // Nothing needs be changed
      else if (action & ERASE)
      {
	if (action & INSERT)
	  ; // Nothing needs be changed
	else
	{
	  modified_tag.attributes["type"] = "delete";
	  modified_tag.children.resize(1);
	  
	  Tag new_tag("new");
	  Tag copied_tag = *tag.children[1]->children[0];
	  copied_tag.children.clear();
	  copied_tag.attributes.erase("lat");
	  copied_tag.attributes.erase("lon");
	  copied_tag.attributes["visible"] = "false";
	  new_tag.children.push_back(&copied_tag);
	  modified_tag.children.push_back(&new_tag);
	  
          cout<<indent<<"<"<<tag.name;
          for (map< string, string >::const_iterator it = modified_tag.attributes.begin();
              it != modified_tag.attributes.end(); ++it)
            cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
          print_subtags(modified_tag, indent, bbox, node_actions, way_actions, relation_actions);
          return;
	}
      }
      else
      {
	if (action & INSERT)
	{
	  modified_tag.attributes["type"] = "create";
	  modified_tag.children[0] = tag.children[1]->children[0];
	  modified_tag.children.resize(1);
	}
	else
	  return;
      }
	
      cout<<indent<<"<"<<tag.name;
      for (map< string, string >::const_iterator it = modified_tag.attributes.begin();
          it != modified_tag.attributes.end(); ++it)
        cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
      print_subtags(modified_tag, indent, bbox, node_actions, way_actions, relation_actions);
      return;
    }
  }
  else if (tag.name == "note")
  {
    cout<<"<note>The data included in this document is from www.openstreetmap.org. "
          "The data is made available under ODbL.</note>\n";
    return;
  }
  
  cout<<indent<<"<"<<tag.name;
  for (map< string, string >::const_iterator it = tag.attributes.begin();
       it != tag.attributes.end(); ++it)
     cout<<" "<<it->first<<"=\""<<escape_xml(it->second)<<"\"";
  print_subtags(tag, indent, bbox, node_actions, way_actions, relation_actions);
}


void detect_osm_tags
    (const Tag& tag,
    map< Node::Id_Type, const Tag* >& nodes,
    map< Way::Id_Type, const Tag* >& ways,
    map< Relation::Id_Type, const Tag* >& relations)
{
  if (tag.name =="action")
  {
    const Tag* osm_tag = obtain_data_from_action(tag);
    
    if (osm_tag->name == "node")
        nodes[Node::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str()))] = &tag;
    else if (osm_tag->name == "way")
        ways[Way::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str()))] = &tag;
    if (osm_tag->name == "relation")
        relations[Relation::Id_Type(atoll(osm_tag->attributes.find("id")->second.c_str()))] = &tag;
  }
  
  for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
    detect_osm_tags(**it, nodes, ways, relations);
}


bool check_bbox_node(const Tag& tag, 
    double south, double north, double east, double west)
{
  double lat = atof(tag.attributes.find("lat")->second.c_str());
  double lon = atof(tag.attributes.find("lon")->second.c_str());
  if (west <= east)
    return (west <= lon && lon <= east && south <= lat && lat <= north);
  else
    return ((west <= lon || lon <= east) && south <= lat && lat <= north);
}


bool check_bbox_way(const Tag& tag, 
    double south, double north, double east, double west)
{
  vector< Tag* >::const_iterator it = tag.children.begin();
  while (it != tag.children.end() && (*it)->name != "nd")
    ++it;
  if (it == tag.children.end())
    return false;
  
  double last_lat = atof((*it)->attributes.find("lat")->second.c_str());
  double last_lon = atof((*it)->attributes.find("lon")->second.c_str());
  if (west <= east)
  {
    if (west <= last_lon && last_lon <= east && south <= last_lat && last_lat <= north)
      return true;
  }
  else
  {
    if ((west <= last_lon || last_lon <= east) && south <= last_lat && last_lat <= north)
      return true;
  }

  while (it != tag.children.end())
  {
    if ((*it)->name == "nd")
    {
      double lat = atof((*it)->attributes.find("lat")->second.c_str());
      double lon = atof((*it)->attributes.find("lon")->second.c_str());
      
      if (west <= east)
      {
        if (west <= lon && lon <= east && south <= lat && lat <= north)
          return true;
      }
      else
      {
        if ((west <= lon || lon <= east) && south <= lat && lat <= north)
          return true;
      }
      
      if (segment_intersects_bbox
          (last_lat, last_lon, lat, lon, south, north, west, east))
	return true;
      
      last_lat = lat;
      last_lon = lon;
    }
    ++it;
  }

  return false;
}


bool check_bbox_relation(const Tag& tag,
    const map< Node::Id_Type, const Tag* >& nodes,
    const map< Way::Id_Type, const Tag* >& ways,
    double south, double north, double east, double west, int flag)
{
  for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
  {
    if ((*it)->name == "member")
    {
      if ((*it)->attributes.find("type")->second == "node" && 
	  check_bbox_node(*obtain_data_from_action
	      (*nodes.find(atoll((*it)->attributes.find("ref")->second.c_str()))->second, flag),
	      south, north, east, west))
        return true;
      else if ((*it)->attributes.find("type")->second == "way")
      {
	string s = (*it)->attributes.find("ref")->second;
	map< Way::Id_Type, const Tag* >::const_iterator ptr = ways.find(atoll(s.c_str()));
	cout<<(ptr==ways.end())<<'\n';
	const Tag* action = ptr->second;
	if (check_bbox_way(*obtain_data_from_action
	      (*action, flag),
	      south, north, east, west))
          return true;
      }
    }
  }
  return false;
}


void mark_way(const Tag& tag, int& way_action, map< Node::Id_Type, int >& node_actions, int flags)
{
  way_action |= flags;
  
  for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
  {
    if ((*it)->name == "nd")
      node_actions[atoll((*it)->attributes.find("ref")->second.c_str())] |= flags;
  }
}


void mark_relation(const Tag& tag, int& relation_action,
		   map< Way::Id_Type, int >& way_actions, map< Node::Id_Type, int >& node_actions,
		   const map< Way::Id_Type, const Tag* >& ways,
		   int flags)
{
  relation_action |= flags;
  
  for (vector< Tag* >::const_iterator it = tag.children.begin(); it != tag.children.end(); ++it)
  {
    if ((*it)->name == "member")
    {
      if ((*it)->attributes.find("type")->second == "node")
        node_actions[atoll((*it)->attributes.find("ref")->second.c_str())] |= flags;
      else if ((*it)->attributes.find("type")->second == "way")
	mark_way(*obtain_data_from_action
	             (*ways.find(atoll((*it)->attributes.find("ref")->second.c_str()))->second, flags),
		 way_actions[atoll((*it)->attributes.find("ref")->second.c_str())], node_actions, flags);
    }
  }
}


void select_osm_tags
    (const map< Node::Id_Type, const Tag* >& nodes,
    const map< Way::Id_Type, const Tag* >& ways,
    const map< Relation::Id_Type, const Tag* >& relations,
    double south, double north, double east, double west,
    map< Node::Id_Type, int >& node_actions,
    map< Way::Id_Type, int >& way_actions,
    map< Relation::Id_Type, int >& relation_actions)
{
  for (map< Node::Id_Type, const Tag* >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
  {
    const Tag tag = *it->second;
    if (tag.attributes.find("type")->second == "create" &&
        check_bbox_node(*tag.children[0], south, north, east, west))
      node_actions[it->first] |= INSERT;
    else if (tag.attributes.find("type")->second == "delete" &&
        check_bbox_node(*tag.children[0]->children[0], south, north, east, west))
      node_actions[it->first] |= ERASE;
    else if (tag.attributes.find("type")->second == "modify")
    {
      if (check_bbox_node(*tag.children[0]->children[0], south, north, east, west))
	node_actions[it->first] |= ERASE;
      if (check_bbox_node(*tag.children[1]->children[0], south, north, east, west))
        node_actions[it->first] |= INSERT;
    }
    else if (tag.attributes.find("type")->second == "info" &&
        check_bbox_node(*tag.children[0], south, north, east, west))
      node_actions[it->first] |= INSIDE;
  }
  
  for (map< Way::Id_Type, const Tag* >::const_iterator it = ways.begin(); it != ways.end(); ++it)
  {
    const Tag tag = *it->second;
    if (tag.attributes.find("type")->second == "create" &&
        check_bbox_way(*tag.children[0], south, north, east, west))
      mark_way(*tag.children[0], way_actions[it->first], node_actions, INSERT);
    else if (tag.attributes.find("type")->second == "delete" &&
        check_bbox_way(*tag.children[0]->children[0], south, north, east, west))
      mark_way(*tag.children[0]->children[0], way_actions[it->first], node_actions, ERASE);
    else if (tag.attributes.find("type")->second == "modify")
    {
      if (check_bbox_way(*tag.children[0]->children[0], south, north, east, west))
        mark_way(*tag.children[0]->children[0], way_actions[it->first], node_actions, ERASE);
      if (check_bbox_way(*tag.children[1]->children[0], south, north, east, west))
        mark_way(*tag.children[1]->children[0], way_actions[it->first], node_actions, INSERT);
    }
    else if (tag.attributes.find("type")->second == "info" &&
        check_bbox_way(*tag.children[0], south, north, east, west))
    {
      mark_way(*tag.children[0], way_actions[it->first], node_actions, ERASE | INSERT);
      way_actions[it->first] |= INSIDE;
    }
  }
  
  for (map< Relation::Id_Type, const Tag* >::const_iterator it = relations.begin();
       it != relations.end(); ++it)
  {
    const Tag tag = *it->second;
    if (tag.attributes.find("type")->second == "create" &&
        check_bbox_relation(*tag.children[0], nodes, ways, south, north, east, west, INSERT))
      mark_relation(*tag.children[0], relation_actions[it->first], way_actions, node_actions, ways, INSERT);
    else if (tag.attributes.find("type")->second == "delete" &&
        check_bbox_relation(*tag.children[0]->children[0], nodes, ways, south, north, east, west, ERASE))
      mark_relation(*tag.children[0]->children[0], relation_actions[it->first],
		    way_actions, node_actions, ways, ERASE);
    else if (tag.attributes.find("type")->second == "modify")
    {
      if (check_bbox_relation(*tag.children[0]->children[0], nodes, ways, south, north, east, west, ERASE))
        mark_relation(*tag.children[0]->children[0], relation_actions[it->first],
		      way_actions, node_actions, ways, ERASE);
      if (check_bbox_relation(*tag.children[1]->children[0], nodes, ways, south, north, east, west, INSERT))
        mark_relation(*tag.children[1]->children[0], relation_actions[it->first],
		      way_actions, node_actions, ways, INSERT);
    }
    else if (tag.attributes.find("type")->second == "info" &&
        check_bbox_relation(*tag.children[0], nodes, ways, south, north, east, west, 0))
    {
      mark_relation(*tag.children[0], relation_actions[it->first], way_actions, node_actions, ways,
		    ERASE | INSERT);
      relation_actions[it->first] |= INSIDE;
    }
  }
}


int main(int argc, char *argv[])
{
  int argi(1);
  enum { first, id_sorted, geo_sorted } target = id_sorted;
  double north(100.0), south(100.0), east(200.0), west(200.0);
  while (argi < argc)
  {
    if (!strncmp("--target=first", argv[argi], 14))
      target = first;
    else if (!strncmp("--north=", argv[argi], 8))
      north = atof(((string)(argv[argi])).substr(8).c_str());
    else if (!strncmp("--south=", argv[argi], 8))
      south = atof(((string)(argv[argi])).substr(8).c_str());
    else if (!strncmp("--west=", argv[argi], 7))
      west = atof(((string)(argv[argi])).substr(7).c_str());
    else if (!strncmp("--east=", argv[argi], 7))
      east = atof(((string)(argv[argi])).substr(7).c_str());
    ++argi;
  }
  
  // read the XML input
  root = 0;
  stack.clear();
  parse(stdin, start, end);
  
  map< Node::Id_Type, const Tag* > nodes;
  map< Way::Id_Type, const Tag* > ways;
  map< Relation::Id_Type, const Tag* > relations;
  
  map< Node::Id_Type, int > node_actions;
  map< Way::Id_Type, int > way_actions;
  map< Relation::Id_Type, int > relation_actions;
  
  if (south != 100.0)
  {
    detect_osm_tags(*root, nodes, ways, relations);
    select_osm_tags(nodes, ways, relations, south, north, east, west,
		    node_actions, way_actions, relation_actions);
  }
  
  //output the converted result
  if (target == first)
  {
    cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    print_tag_firstversion(*root, "");
  }
  else if (target == id_sorted)
  {
    cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    print_tag_idsorted(*root, "", south != 100.0, node_actions, way_actions, relation_actions);
  }
  
  return 0;
}
