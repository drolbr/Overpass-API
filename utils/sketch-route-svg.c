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

map< unsigned int, NamedNode > nodes;
NamedNode nnode;

list< unsigned int > stops;
unsigned int id;
string rel_ref, rel_to;
string rel_color("blue");
int direction(0);

bool inRelation(false);

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
      rel_to = value;
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
      if (direction == 0)
	stops.push_back(ref);
      else if (direction == 1)
      {
	if ((role == "forward_stop") || (role == "forward_stop_0"))
	  stops.push_back(ref);
      }
      else if (direction == -1)
      {
	if ((role == "backward_stop") || (role == "backward_stop_0"))
	  stops.push_front(ref);
      }
    }
  }
  else if (!strcmp(el, "relation"))
  {
    inRelation = true;
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
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
  
  cout<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      <<"<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
      <<"     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
      <<"     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
      <<"     version=\"1.1\" baseProfile=\"full\"\n"
      <<"     width=\"700px\" height=\"330px\">\n"
      <<"\n"
      <<"<title>Line "<<rel_ref<<" to "<<rel_to<<"</title>\n"
      <<"<desc>Line "<<rel_ref<<" to "<<rel_to<<"</desc>\n"
      <<"\n"
/*      <<"<defs>\n"
      <<"  <style type=\"text/css\">\n"
      <<"    <![CDATA[\n"
      <<"      path {fill:"<<ref_color<<"; stroke:none;}\n"
      <<"    ]]>\n"
      <<"  </style>\n"
      <<"</defs>\n"
      <<"\n"*/
      <<"<text x=\"30\" y=\"60\" font-family=\"verdana\"><tspan font-size=\"32px\">"<<rel_ref<<"</tspan>\n"
      <<"  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      <<"  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      <<"  <tspan font-size=\"24px\">"<<rel_to<<"</tspan></text>\n"
      <<"\n"
      <<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";
  
  if ((stops.size() > 1) && (stops.size() <= 21))
  {
    cout<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";
    
    double pos(30);
    double stop_distance(0);
  
    stop_distance = 480.0/(stops.size()-1);
  
    for (list< unsigned int >::const_iterator it(stops.begin()); it != stops.end(); ++it)
    {
      NamedNode nnode(nodes[*it]);
      if (nnode.lat <= 90.0)
      {
	cout<<"<circle cx=\""<<pos<<"\" cy=\"300\" r=\"7\" fill=\""<<rel_color<<"\"/>\n"
	    <<"<text x=\""<<pos<<"\" y=\"290\" transform=\"rotate(-45,"<<pos<<",290)\" font-family=\"verdana\" font-size=\"16px\">"<<nnode.name<<"</text>\n";
      }
      pos += stop_distance;
    }
  }
  else if (stops.size() > 21)
  {
    cout<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 170, 530 170\"/>\n"
    	<<"<polyline fill=\"none\" stroke=\""<<rel_color<<"\" stroke-width=\"7px\" points=\"30 300, 510 300\"/>\n";

    
    unsigned int count(0);
    double pos(30), vpos(170);
    double stop_distance(0);
  
    stop_distance = 480.0/((stops.size()+1)/2-1);
  
    for (list< unsigned int >::const_iterator it(stops.begin()); it != stops.end(); ++it)
    {
      NamedNode nnode(nodes[*it]);
      if (nnode.lat <= 90.0)
      {
	cout<<"<circle cx=\""<<pos<<"\" cy=\""<<vpos<<"\" r=\"7\" fill=\""<<rel_color<<"\"/>\n"
	    <<"<text x=\""<<pos<<"\" y=\""<<vpos-10<<"\" transform=\"rotate(-45,"<<pos<<","<<vpos-10<<")\" font-family=\"verdana\" font-size=\"10px\">"<<nnode.name<<"</text>\n";
      }
      pos += stop_distance;
      if (++count >= (stops.size()+1)/2)
      {
	count = 0;
	vpos = 300;
	stop_distance = 480.0/(stops.size()/2);
	pos = 30 + stop_distance;
      }
    }
  }
  
  cout<<"</svg>\n";
  
  return 0;
}
