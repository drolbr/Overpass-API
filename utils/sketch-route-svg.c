/*****************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the license contained in the
 * COPYING file that comes with the expat distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../expat_justparse_interface.h"

using namespace std;

struct NamedNode
{
  public:
    NamedNode() : lat(100.0), lon(200.0), name("") {}
    
    double lat, lon;
    string name;
};


struct Stop
{
  public:
    Stop() : id(0), type(0) {}
    
    unsigned int id;
    int type;
    
    static const int BOTH = 0;
    static const int FORWARD = 1;
    static const int BACKWARD = 2;
};

map< unsigned int, NamedNode > nodes;
NamedNode nnode;

list< unsigned int > stops;
list< unsigned int > stops_back;
unsigned int id;
string rel_ref, rel_from, rel_to;
string rel_color("blue");
int direction(0);

bool inRelation(false);
int rel_count(0);

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
    if (key == "name")
      nnode.name = value;
    if ((key == "ref") && inRelation)
      rel_ref = value;
    if ((key == "to") && inRelation)
    {
      if (rel_count == 1)
	rel_to = value;
      else
	rel_from = value;
    }
    if ((key == "color") && inRelation)
      rel_color = value;
  }
  else if (!strcmp(el, "node"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atol(attr[i+1]);
      else if (!strcmp(attr[i], "lat"))
	nnode.lat = atof(attr[i+1]);
      else if (!strcmp(attr[i], "lon"))
	nnode.lon = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "member"))
  {
    unsigned int ref(0);
    string type, role;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "ref"))
	ref = atol(attr[i+1]);
      else if (!strcmp(attr[i], "type"))
	type = attr[i+1];
      else if (!strcmp(attr[i], "role"))
	role = attr[i+1];
    }
    if (type == "node")
    {
      if (rel_count == 1)
	stops.push_back(ref);
      else
	stops_back.push_front(ref);
    }
  }
  else if (!strcmp(el, "relation"))
  {
    inRelation = true;
    ++rel_count;
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    nnode.name = escape_xml(nnode.name);
    nodes[id] = nnode;
  }
  else if (!strcmp(el, "relation"))
  {
    inRelation = false;
  }
}

int main(int argc, char *argv[])
{
  //reading the main document
  parse(stdin, start, end);
  
  rel_ref = escape_xml(rel_ref);
  rel_from = escape_xml(rel_from);
  rel_to = escape_xml(rel_to);
  rel_color = escape_xml(rel_color);

  /* make a common stoplist from both relations */
  list< Stop > stoplist;
  if (rel_count == 1)
  {
    for (list< unsigned int >::const_iterator it(stops.begin()); it != stops.end(); ++it)
    {
      Stop stop;
      stop.id = *it;
      stop.type = Stop::BOTH;
      stoplist.push_back(stop);
    }
  }
  else
  {
    list< unsigned int >::const_iterator back_it(stops_back.begin());
    for (list< unsigned int >::const_iterator it(stops.begin()); it != stops.end(); ++it)
    {
      if (back_it == stops_back.end())
      {
	Stop stop;
	stop.id = *it;
	stop.type = Stop::FORWARD;
	stoplist.push_back(stop);
      }
      else if (nodes[*back_it].name == nodes[*it].name)
      {
	Stop stop;
	stop.id = *it;
	stop.type = Stop::BOTH;
	stoplist.push_back(stop);
	++back_it;
      }
      else
      {
	list< unsigned int >::const_iterator back_it2(back_it);
	while ((back_it2 != stops_back.end()) && (nodes[*back_it2].name != nodes[*it].name))
	  ++back_it2;
	if (back_it2 != stops_back.end())
	{
	  while (back_it != back_it2)
	  {
	    Stop stop;
	    stop.id = *back_it;
	    stop.type = Stop::BACKWARD;
	    stoplist.push_back(stop);
	    ++back_it;
	  }
	  Stop stop;
	  stop.id = *it;
	  stop.type = Stop::BOTH;
	  stoplist.push_back(stop);
	  ++back_it;
	}
	else
	{
	  Stop stop;
	  stop.id = *it;
	  stop.type = Stop::FORWARD;
	  stoplist.push_back(stop);
	}
      }
    }
  }
  
  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      <<"<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
      <<"     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
      <<"     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
      <<"     version=\"1.1\" baseProfile=\"full\"\n"
      <<"     width=\"700px\" height=\"330px\">\n"
      <<"\n";
  
  if (rel_count == 1)
    cout<<"<title>Line "<<rel_ref<<" to "<<rel_to<<"</title>\n"
	<<"<desc>Line "<<rel_ref<<" to "<<rel_to<<"</desc>\n"
	<<"\n"
/*	<<"<defs>\n"
	<<"  <style type=\"text/css\">\n"
	<<"    <![CDATA[\n"
	<<"      path {fill:"<<rel_color<<"; stroke:none;}\n"
	<<"    ]]>\n"
	<<"  </style>\n"
	<<"</defs>\n"*/
	<<"\n"
	<<"<text x=\"30\" y=\"60\" font-family=\"verdana\"><tspan font-size=\"32px\" fill=\""<<rel_color<<"\">"<<rel_ref<<"</tspan>\n"
	<<"  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
	<<"  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
	<<"  <tspan font-size=\"24px\">"<<rel_to<<"</tspan></text>\n"
	<<"\n"
	<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";
  else
    cout<<"<title>Line "<<rel_ref<<" from "<<rel_from<<" to "<<rel_to<<"</title>\n"
	<<"<desc>Line "<<rel_ref<<" from "<<rel_from<<" to "<<rel_to<<"</desc>\n"
	<<"\n"
/*	<<"<defs>\n"
	<<"  <style type=\"text/css\">\n"
	<<"    <![CDATA[\n"
	<<"      path {fill:"<<rel_color<<"; stroke:none;}\n"
	<<"    ]]>\n"
	<<"  </style>\n"
	<<"</defs>\n"*/
	<<"\n"
	<<"<text x=\"30\" y=\"30\" font-family=\"verdana\"><tspan font-size=\"32px\" fill=\"none\">"<<rel_ref<<"</tspan>\n"
	<<"  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
	<<"  <tspan font-size=\"16px\">from</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
	<<"  <tspan font-size=\"24px\">"<<rel_from<<"</tspan></text>\n"
	<<"<text x=\"30\" y=\"60\" font-family=\"verdana\"><tspan font-size=\"32px\" fill=\"none\">"<<rel_ref<<"</tspan>\n"
	<<"  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
	<<"  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
	<<"  <tspan font-size=\"24px\">"<<rel_to<<"</tspan></text>\n"
	<<"<text x=\"30\" y=\"45\" font-family=\"verdana\" font-size=\"32px\" fill=\""<<rel_color<<"\">"<<rel_ref<<"</text>\n"
	<<"\n"
	<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";
  
  if ((stoplist.size() > 1) && (stoplist.size() <= 21))
  {
    cout<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";
    
    double pos(30);
    double stop_distance(0);
  
    stop_distance = 480.0/(stoplist.size()-1);
  
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      NamedNode nnode(nodes[it->id]);
      if (nnode.lat <= 90.0)
      {
	if (it->type == Stop::BOTH)
	  cout<<"<circle cx=\""<<pos<<"\" cy=\"300\" r=\"7\" fill=\""<<rel_color<<"\"/>\n";
	else if (it->type == Stop::FORWARD)
	  cout<<"<path d=\"M "<<pos - 7<<",300 a 7 7 0 0 0 14,0 Z\" style=\"fill:"<<rel_color<<"\"/>\n"
	      <<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"2px\" points=\""<<pos - 6<<" 314, "<<pos + 2<<" 314\"/>\n"
	      <<"<path d=\"M "<<pos<<",309 l 0,10 l 6,-5 Z\" style=\"fill:"<<rel_color<<"\"/>\n";
	else if (it->type == Stop::BACKWARD)
	  cout<<"<path d=\"M "<<pos - 7<<",300 a 7 7 0 0 1 14,0 Z\" style=\"fill:"<<rel_color<<"\"/>\n"
	      <<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"2px\" points=\""<<pos - 2<<" 314, "<<pos + 6<<" 314\"/>\n"
	      <<"<path d=\"M "<<pos<<",309 l 0,10 l -6,-5 Z\" style=\"fill:"<<rel_color<<"\"/>\n";
	cout<<"<text x=\""<<pos<<"\" y=\"290\" transform=\"rotate(-45,"<<pos<<",290)\" font-family=\"verdana\" font-size=\"16px\">"<<nnode.name<<"</text>\n";
      }
      pos += stop_distance;
    }
  }
  else if (stoplist.size() > 21)
  {
    cout<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 170, 530 170\"/>\n"
    	<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";

    
    unsigned int count(0);
    double pos(30), vpos(170);
    double stop_distance(0);
  
    stop_distance = 480.0/((stoplist.size()+1)/2-1);
  
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      NamedNode nnode(nodes[it->id]);
      if (nnode.lat <= 90.0)
      {
	if (it->type == Stop::BOTH)
	  cout<<"<circle cx=\""<<pos<<"\" cy=\""<<vpos<<"\" r=\"7\" fill=\""<<rel_color<<"\"/>\n";
	else if (it->type == Stop::FORWARD)
	  cout<<"<path d=\"M "<<pos - 7<<","<<vpos<<" a 7 7 0 0 0 14,0 Z\" style=\"fill:"<<rel_color<<"\"/>\n"
	      <<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"2px\" points=\""<<pos - 6<<" "<<vpos + 14<<", "<<pos + 2<<" "<<vpos + 14<<"\"/>\n"
	      <<"<path d=\"M "<<pos<<","<<vpos + 9<<" l 0,10 l 6,-5 Z\" style=\"fill:"<<rel_color<<"\"/>\n";
	else if (it->type == Stop::BACKWARD)
	  cout<<"<path d=\"M "<<pos - 7<<","<<vpos<<" a 7 7 0 0 1 14,0 Z\" style=\"fill:"<<rel_color<<"\"/>\n"
	      <<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"2px\" points=\""<<pos - 2<<" "<<vpos + 14<<", "<<pos + 6<<" "<<vpos + 14<<"\"/>\n"
	      <<"<path d=\"M "<<pos<<","<<vpos + 9<<" l 0,10 l -6,-5 Z\" style=\"fill:"<<rel_color<<"\"/>\n";
	cout<<"<text x=\""<<pos<<"\" y=\""<<vpos-10<<"\" transform=\"rotate(-45,"<<pos<<","<<vpos-10<<")\" font-family=\"verdana\" font-size=\"10px\">"<<nnode.name<<"</text>\n";
      }
      pos += stop_distance;
      if (++count >= (stoplist.size()+1)/2)
      {
	count = 0;
	vpos = 300;
	stop_distance = 480.0/(stoplist.size()/2);
	pos = 30 + stop_distance;
      }
    }
  }
  
  cout<<"</svg>\n";
  
  return 0;
}
