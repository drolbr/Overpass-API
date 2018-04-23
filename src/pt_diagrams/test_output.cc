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

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "read_input.h"
#include "test_output.h"

using namespace std;

namespace
{
  struct Bbox
  {
    double north, south, east, west;
    Bbox(double north_, double south_, double east_, double west_)
      : north(north_), south(south_), east(east_), west(west_) {}
  };

  Bbox calc_bbox(const OSMData& current_data)
  {
    double north(-90.0), south(90.0), east(-180.0), west(180.0);
    for (map< uint32, Node* >::const_iterator it(current_data.nodes.begin());
    it != current_data.nodes.end(); ++it)
    {
      if (it->second->lat < south)
	south = it->second->lat;
      if (it->second->lat > north)
	north = it->second->lat;
      if (it->second->lon < west)
	west = it->second->lon;
      if (it->second->lon > east)
	east = it->second->lon;
    }

    return Bbox(north, south, east, west);
  }
}

void dump_osm_data(const OSMData& current_data)
{
  cout<<"Nodes:\n";
  for (map< uint32, Node* >::const_iterator it(current_data.nodes.begin());
      it != current_data.nodes.end(); ++it)
  {
    cout<<it->first<<'\t'<<it->second->lat<<'\t'<<it->second->lon<<'\n';
    for (map< string, string >::const_iterator it2(it->second->tags.begin());
        it2 != it->second->tags.end(); ++it2)
      cout<<'\t'<<it2->first<<'\t'<<it2->second<<'\n';
  }
  cout<<'\n';

  cout<<"Ways:\n";
  for (map< uint32, Way* >::const_iterator it(current_data.ways.begin());
      it != current_data.ways.end(); ++it)
  {
    cout<<it->first<<"\n\t";
    for (vector< Node* >::const_iterator it2(it->second->nds.begin());
        it2 != it->second->nds.end(); ++it2)
      cout<<"("<<(*it2)->lat<<", "<<(*it2)->lon<<") ";
    cout<<'\n';
    for (map< string, string >::const_iterator it2(it->second->tags.begin());
        it2 != it->second->tags.end(); ++it2)
      cout<<'\t'<<it2->first<<'\t'<<it2->second<<'\n';
  }
  cout<<'\n';

  cout<<"Relations:\n";
  for (map< uint32, Relation* >::const_iterator it(current_data.relations.begin());
      it != current_data.relations.end(); ++it)
  {
    cout<<it->first<<"\n\t";
    for (vector< pair< OSMElement*, string > >::const_iterator
        it2(it->second->members.begin()); it2 != it->second->members.end(); ++it2)
    {
      const Node* node;
      const Way* way;
      const Relation* relation;
      cout<<'(';
      const OSMElement* elem(static_cast< const OSMElement* >(it2->first));
      if (node = dynamic_cast< const Node* >(elem))
	cout<<'('<<node->lat<<", "<<node->lon<<')';
      else if (way = dynamic_cast< const Way* >(elem))
      {
        for (vector< Node* >::const_iterator it3(way->nds.begin());
            it3 != way->nds.end(); ++it3)
          cout<<"("<<(*it3)->lat<<", "<<(*it3)->lon<<") ";
      }
      else if (relation = dynamic_cast< const Relation* >(elem))
	cout<<"<relation>";
      cout<<") "<<it2->second<<' ';
    }
    cout<<'\n';
    for (map< string, string >::const_iterator it2(it->second->tags.begin());
        it2 != it->second->tags.end(); ++it2)
      cout<<'\t'<<it2->first<<'\t'<<it2->second<<'\n';
  }
  cout<<'\n';
}

void sketch_unscaled_osm_data(const OSMData& current_data)
{
  ::Bbox bbox(::calc_bbox(current_data));

  //expand the bounding box to avoid elements scratching the frame
  bbox.north += 0.01;
  bbox.south -= 0.01;
  bbox.east += 0.01;
  bbox.west -= 0.01;

  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
  "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
  "     version=\"1.1\" baseProfile=\"full\"\n"
  "     width=\""<<(bbox.east - bbox.west)*10000<<"px\" "
       "height=\""<<(bbox.north - bbox.south)*10000<<"px\">\n"
  "\n";

  for (map< uint32, Node* >::const_iterator it(current_data.nodes.begin());
      it != current_data.nodes.end(); ++it)
  {
    cout<<"<circle cx=\""<<(it->second->lon - bbox.west)*10000
        <<"\" cy=\""<<(bbox.north - it->second->lat)*10000
	<<"\" r=\"6\" fill=\"blue\"/>\n";
  }
  cout<<'\n';

  for (map< uint32, Way* >::const_iterator it(current_data.ways.begin());
      it != current_data.ways.end(); ++it)
  {
    cout<<"<polyline fill=\"none\" stroke=\"green\" stroke-width=\"3px\""
        " points=\"";
    vector< Node* >::const_iterator it2(it->second->nds.begin());
    if (it2 != it->second->nds.end())
    {
      cout<<((*it2)->lon - bbox.west)*10000<<' '
          <<(bbox.north - (*it2)->lat)*10000;
      ++it2;
    }
    for (; it2 != it->second->nds.end(); ++it2)
      cout<<", "<<((*it2)->lon - bbox.west)*10000<<' '
          <<(bbox.north - (*it2)->lat)*10000;
    cout<<"\"/>\n";
  }
  cout<<'\n';

  cout<<"</svg>\n";
}

namespace
{
  double vpos(double lat, double north, double m_per_pixel)
  {
    return (north - lat)*(1000000.0/9.0)/m_per_pixel;
  }

  double hpos(double lat, double lon, double west, double m_per_pixel)
  {
    return (lon - west)*(1000000.0/9.0)/m_per_pixel*cos(lat/90.0*acos(0));
  }
}

void sketch_osm_data(const OSMData& current_data, double pivot_lon, double m_per_pixel)
{
  ::Bbox bbox(::calc_bbox(current_data));

  //expand the bounding box to avoid elements scratching the frame
  bbox.north += 0.01;
  bbox.south -= 0.01;
  bbox.east += 0.01;
  bbox.west -= 0.01;

  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
  "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
  "     version=\"1.1\" baseProfile=\"full\"\n"
  "     width=\""<<hpos(bbox.south, bbox.east, bbox.west, m_per_pixel)<<"px\" "
  "height=\""<<vpos(bbox.south, bbox.north, m_per_pixel)<<"px\">\n"
  "\n";

  for (map< uint32, Node* >::const_iterator it(current_data.nodes.begin());
      it != current_data.nodes.end(); ++it)
  {
    cout<<"<circle cx=\""
        <<hpos(it->second->lat, it->second->lon, bbox.west, m_per_pixel)
        <<"\" cy=\""
	<<vpos(it->second->lat, bbox.north, m_per_pixel)
        <<"\" r=\"6\" fill=\"blue\"/>\n";
  }
  cout<<'\n';

  for (map< uint32, Way* >::const_iterator it(current_data.ways.begin());
      it != current_data.ways.end(); ++it)
  {
    cout<<"<polyline fill=\"none\" stroke=\"aqua\" stroke-width=\"3px\""
          " points=\"";
    vector< Node* >::const_iterator it2(it->second->nds.begin());
    if (it2 != it->second->nds.end())
    {
      cout<<hpos((*it2)->lat, (*it2)->lon, bbox.west, m_per_pixel)<<' '
          <<vpos((*it2)->lat, bbox.north, m_per_pixel);
      ++it2;
    }
    for (; it2 != it->second->nds.end(); ++it2)
    {
      cout<<", "<<hpos((*it2)->lat, (*it2)->lon, bbox.west, m_per_pixel)<<' '
          <<vpos((*it2)->lat, bbox.north, m_per_pixel);
    }
    cout<<"\"/>\n";
  }
  cout<<'\n';

  cout<<"</svg>\n";
}
