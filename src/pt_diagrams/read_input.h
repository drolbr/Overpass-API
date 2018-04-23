/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
 *
 * This file is part of Overpass_API.
 *
 * Overpass_API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Overpass_API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DE__OSM3S___PT_DIAGRAMS__READ_INPUT_H
#define DE__OSM3S___PT_DIAGRAMS__READ_INPUT_H

#include <map>
#include <string>
#include <vector>

using namespace std;

typedef unsigned int uint32;

class OSMElement
{
  public:
    virtual ~OSMElement() {}
    virtual void operator()() const {}
};

struct Node : public OSMElement
{
  map< string, string > tags;
  double lat;
  double lon;
};

struct Way : public OSMElement
{
  map< string, string > tags;
  vector< Node* > nds;
};

struct Relation : public OSMElement
{
  map< string, string > tags;
  vector< pair< OSMElement*, string > > members;
};

struct RelMember
{
  int type;
  uint32 ref;
  string role;

  static const int NODE = 1;
  static const int WAY = 2;
  static const int RELATION = 3;
};

struct OSMData
{
  ~OSMData();

  map< uint32, Node* > nodes;
  map< uint32, Way* > ways;
  map< uint32, Relation* > relations;
};

const OSMData& read_osm();

#endif
