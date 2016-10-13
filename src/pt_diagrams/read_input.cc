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

#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../expat/expat_justparse_interface.h"

#include "read_input.h"

using namespace std;

typedef unsigned int uint32;

OSMData::~OSMData()
{
  for (map< uint32, Node* >::const_iterator it(nodes.begin());
      it != nodes.end(); ++it)
    delete it->second;
  for (map< uint32, Way* >::const_iterator it(ways.begin());
      it != ways.end(); ++it)
    delete it->second;
  for (map< uint32, Relation* >::const_iterator it(relations.begin());
      it != relations.end(); ++it)
    delete it->second;
}

namespace
{
  OSMData current_data;
  map< string, string > current_tags;
  map< uint32, vector< uint32 > > pending_nds;
  map< uint32, vector< RelMember > > pending_members;
  int current_elem;
}

void start(const char *el, const char **attr)
{
  if (!strcmp(el, "tag"))
  {
    string key, value;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	key = attr[i+1];
      else if (!strcmp(attr[i], "v"))
	value = attr[i+1];
    }
    current_tags.insert
      (make_pair((string)key, (string)value));
  }
  else if (!strcmp(el, "node"))
  {
    Node* node = new Node;
    uint32 id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atol(attr[i+1]);
      else if (!strcmp(attr[i], "lat"))
	node->lat = atof(attr[i+1]);
      else if (!strcmp(attr[i], "lon"))
	node->lon = atof(attr[i+1]);
    }
    current_elem = id;
    bool inserted(current_data.nodes.insert
        (make_pair(id, node)).second);
    if (!inserted)
      delete node;
  }
  else if (!strcmp(el, "way"))
  {
    Way* way = new Way;
    uint32 id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atol(attr[i+1]);
    }
    current_elem = id;
    bool inserted(current_data.ways.insert
        (make_pair(id, way)).second);
    if (!inserted)
      delete way;
  }
  else if (!strcmp(el, "nd"))
  {
    uint32 ref(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "ref"))
	ref = atol(attr[i+1]);
    }
    pending_nds[current_elem].push_back(ref);
  }
  else if (!strcmp(el, "relation"))
  {
    Relation* rel = new Relation;
    uint32 id(0);
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atol(attr[i+1]);
    }
    current_elem = id;
    bool inserted(current_data.relations.insert
        (make_pair(id, rel)).second);
    if (!inserted)
      delete rel;
  }
  else if (!strcmp(el, "member"))
  {
    pending_members[current_elem].push_back(RelMember());
    RelMember& rel_member(pending_members[current_elem].back());
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "ref"))
	rel_member.ref = atol(attr[i+1]);
      else if (!strcmp(attr[i], "type"))
      {
	string type(attr[i+1]);
	if (type == "node")
	  rel_member.type = RelMember::NODE;
	else if (type == "way")
	  rel_member.type = RelMember::WAY;
	else if (type == "relation")
	  rel_member.type = RelMember::RELATION;
      }
      else if (!strcmp(attr[i], "role"))
	rel_member.role = attr[i+1];
    }
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    current_data.nodes[current_elem]->tags = current_tags;
    current_tags.clear();
  }
  else if (!strcmp(el, "way"))
  {
    current_data.ways[current_elem]->tags = current_tags;
    current_tags.clear();
  }
  else if (!strcmp(el, "relation"))
  {
    current_data.relations[current_elem]->tags = current_tags;
    current_tags.clear();
  }
}

const OSMData& read_osm()
{
  // read the XML input
  parse(stdin, start, end);
  
  for (map< uint32, Way* >::const_iterator it(current_data.ways.begin());
      it != current_data.ways.end(); ++it)
  {
    vector< uint32 >& nds(pending_nds[it->first]);
    for (vector< uint32 >::const_iterator it2(nds.begin()); it2 != nds.end(); ++it2)
    {
      map< uint32, Node* >::const_iterator nit(current_data.nodes.find(*it2));
      if (nit == current_data.nodes.end())
      {
	cerr<<"Error: Node "<<*it2<<" referenced by way "<<it->first
	    <<"but not contained in the source file.\n";
	// better throw an exception
      }
      else
        it->second->nds.push_back(nit->second);
    }
  }
  
  for (map< uint32, Relation* >::const_iterator it(current_data.relations.begin());
      it != current_data.relations.end(); ++it)
  {
    vector< RelMember >& members(pending_members[it->first]);
    for (vector< RelMember >::const_iterator it2(members.begin());
        it2 != members.end(); ++it2)
    {
      if (it2->type == RelMember::NODE)
      {
	map< uint32, Node* >::const_iterator nit(current_data.nodes.find(it2->ref));
	if (nit == current_data.nodes.end())
	{
	  cerr<<"Error: Node "<<it2->ref<<" referenced by relation "<<it->first
	  <<" but not contained in the source file.\n";
	  // throw an exception
	}
	else
	  it->second->members.push_back(make_pair(nit->second, it2->role));
      }
      else if (it2->type == RelMember::WAY)
      {
	map< uint32, Way* >::const_iterator nit(current_data.ways.find(it2->ref));
	if (nit == current_data.ways.end())
	{
	  cerr<<"Error: Way "<<it2->ref<<" referenced by relation "<<it->first
	  <<" but not contained in the source file.\n";
	  // throw an exception
	}
	else
	  it->second->members.push_back(make_pair(nit->second, it2->role));
      }
      if (it2->type == RelMember::RELATION)
      {
	map< uint32, Relation* >::const_iterator nit(current_data.relations.find(it2->ref));
	if (nit == current_data.relations.end())
	{
	  cerr<<"Error: Relation "<<it2->ref<<" referenced by relation "<<it->first
	  <<" but not contained in the source file.\n";
	  // throw an exception
	}
	else
	  it->second->members.push_back(make_pair(nit->second, it2->role));
      }
    }
  }

  return current_data;
}
