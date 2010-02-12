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

template< class Inserted >
class Replacer
{
  public:
    Replacer(string match_, Inserted insert_) : match(match_), insert(insert_) {}
    
    // Returns the string template, where all occurrences of match are
    // replaced by insert
    string apply(string source)
    {
      ostringstream result("");
      string::size_type pos(0), found(source.find(match));
      while (found != string::npos)
      {
	result<<source.substr(pos, found - pos);
	result<<insert;
	pos = found + match.length();
	found = source.find(match, pos);
      }
      result<<source.substr(pos);
      
      return result.str();
    }
    
  private:
    string match;
    Inserted insert;
};

string frame_template()
{
  return "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
    "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
    "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
    "     version=\"1.1\" baseProfile=\"full\"\n"
    "     width=\"700px\" height=\"330px\">\n"
    "\n"
    "<headline/>\n"
    "\n"
    "<stops-diagram/>\n"
    "\n"
    "</svg>\n";
}

string from_to_headline_template()
{
  return "<title>Line &rel_ref; from &rel_from; to &rel_to;</title>\n"
      "<desc>Line &rel_ref; from &rel_from; to &rel_to;</desc>\n"
      "\n"
      "<text x=\"30\" y=\"30\" font-family=\"verdana\"><tspan font-size=\"32px\" fill=\"none\">&rel_ref;</tspan>\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">from</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"24px\">&rel_from;</tspan></text>\n"
      "<text x=\"30\" y=\"60\" font-family=\"verdana\"><tspan font-size=\"32px\" fill=\"none\">&rel_ref;</tspan>\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"24px\">&rel_to;</tspan></text>\n"
      "<text x=\"30\" y=\"45\" font-family=\"verdana\" font-size=\"32px\" fill=\"&rel_color;\">&rel_ref;</text>\n"
      "\n";
}

string only_to_headline_template()
{
  return "<title>Line &rel_ref; to &rel_to;</title>\n"
      "<desc>Line &rel_ref; to &rel_to;</desc>\n"
      "\n"
      "<text x=\"30\" y=\"60\" font-family=\"verdana\"><tspan font-size=\"32px\" fill=\"&rel_color;\">&rel_ref;</tspan>\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"24px\">&rel_to;</tspan></text>\n"
      "\n";
}

string line_template()
{
  return "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"7px\""
      " points=\"&hmin; &vpos;, &hmax; &vpos;\"/>\n";
}

string bidirectional_stop_template()
{
  return "<circle cx=\"&hpos;\" cy=\"&vpos;\" r=\"7\" fill=\"&rel_color;\"/>\n"
      "<text x=\"0\" y=\"-10\" transform=\"translate(&hpos;,&vpos;) rotate(-45,0,-10)\""
      " font-family=\"verdana\" font-size=\"&stop_fontsize;\">&stopname;</text>\n"
      "\n";
}

string forward_stop_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 0 14,0 Z\" style=\"fill:&rel_color;\""
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"2px\" points=\"-6 14, 2 14\"" 
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<path d=\"M 0,9 l 0,10 l 6,-5 Z\" style=\"fill:&rel_color;\" transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<text x=\"0\" y=\"-10\" transform=\"translate(&hpos;,&vpos;) rotate(-45,0,-10)\""
      " font-family=\"verdana\" font-size=\"&stop_fontsize;\">&stopname;</text>\n"
      "\n";
}

string backward_stop_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 1 14,0 Z\" style=\"fill:&rel_color;\""
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"2px\" points=\"6 14, -2 14\"" 
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<path d=\"M 0,9 l 0,10 l -6,-5 Z\" style=\"fill:&rel_color;\" transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<text x=\"0\" y=\"-10\" transform=\"translate(&hpos;,&vpos;) rotate(-45,0,-10)\""
      " font-family=\"verdana\" font-size=\"&stop_fontsize;\">&stopname;</text>\n"
      "\n";
}

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
bool doubleread_rel(false);

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
    if ((key == "from") && inRelation)
    {
      if (doubleread_rel)
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
      if (doubleread_rel)
      {
	if (role.substr(0, 7) == "forward")
	  stops.push_back(ref);
	else if (role.substr(0, 8) == "backward")
	  stops_back.push_front(ref);
	else
	{
	  stops.push_back(ref);
	  stops_back.push_front(ref);
	}
      }
      else if (rel_count == 1)
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
  if (argc >= 2)
  {
    if (!strcmp("--doubleread-rel", argv[1]))
      doubleread_rel = true;
  }
  
  //reading the main document
  parse(stdin, start, end);
  
  rel_ref = escape_xml(rel_ref);
  rel_from = escape_xml(rel_from);
  rel_to = escape_xml(rel_to);
  rel_color = escape_xml(rel_color);

  /* make a common stoplist from both relations */
  list< Stop > stoplist;
  if ((rel_count == 1) && (!doubleread_rel))
  {
    list< unsigned int >::const_iterator it(stops.begin());
    if (it != stops.end())
    {
      Stop stop;
      stop.id = *it;
      stop.type = Stop::BOTH;
      stoplist.push_back(stop);
    }
    while (it != stops.end())
    {
      Stop stop;
      stop.id = *it;
      stop.type = Stop::FORWARD;
      stoplist.push_back(stop);
      ++it;
    }
    if (!stoplist.empty())
      stoplist.back().type = Stop::BOTH;
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
  
  string headline;
  if ((rel_count == 1) && (!doubleread_rel))
    headline = only_to_headline_template();
  else
    headline = from_to_headline_template();
  
  headline = Replacer< string >("&rel_from;", rel_from).apply
	    (Replacer< string >("&rel_to;", rel_to).apply
	    (Replacer< string >("&rel_ref;", rel_ref).apply
	    (Replacer< string >("&rel_color;", rel_color).apply(headline))));
  
  ostringstream result("");
  
  if ((stoplist.size() > 1) && (stoplist.size() <= 21))
  {
    result<<Replacer< string >("&rel_color;", rel_color).apply
	    (Replacer< double >("&hmin;",  30).apply
	    (Replacer< double >("&hmax;", 510).apply
	    (Replacer< double >("&vpos;", 300).apply(line_template()))));
    
    double pos(30);
    double stop_distance(0);
  
    stop_distance = 480.0/(stoplist.size()-1);
  
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      NamedNode nnode(nodes[it->id]);
      if (nnode.lat <= 90.0)
      {
	string stop_template;
	if (it->type == Stop::BOTH)
	  stop_template = bidirectional_stop_template();
	else if (it->type == Stop::FORWARD)
	  stop_template = forward_stop_template();
	else if (it->type == Stop::BACKWARD)
	  stop_template = backward_stop_template();
	
	result<<Replacer< string >("&stopname;", nnode.name).apply
	    (Replacer< string >("&rel_color;", rel_color).apply
	    (Replacer< string >("&stop_fontsize;", "16px").apply
	    (Replacer< double >("&hpos;", pos).apply
	    (Replacer< double >("&vpos;", 300).apply(stop_template)))));	
      }
      pos += stop_distance;
    }
  }
  else if (stoplist.size() > 21)
  {
    result<<Replacer< string >("&rel_color;", rel_color).apply
	    (Replacer< double >("&hmin;",  30).apply
	    (Replacer< double >("&hmax;", 530).apply
	    (Replacer< double >("&vpos;", 170).apply(line_template()))));
    result<<Replacer< string >("&rel_color;", rel_color).apply
	    (Replacer< double >("&hmin;",  30).apply
	    (Replacer< double >("&hmax;", 510).apply
	    (Replacer< double >("&vpos;", 300).apply(line_template()))))<<'\n';
    
    unsigned int count(0);
    double pos(30), vpos(170);
    double stop_distance(0);
  
    stop_distance = 480.0/((stoplist.size()+1)/2-1);
  
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      NamedNode nnode(nodes[it->id]);
      if (nnode.lat <= 90.0)
      {
	string stop_template;
	if (it->type == Stop::BOTH)
	  stop_template = bidirectional_stop_template();
	else if (it->type == Stop::FORWARD)
	  stop_template = forward_stop_template();
	else if (it->type == Stop::BACKWARD)
	  stop_template = backward_stop_template();
	
	result<<Replacer< string >("&stopname;", nnode.name).apply
	    (Replacer< string >("&rel_color;", rel_color).apply
	    (Replacer< string >("&stop_fontsize;", "10px").apply
	    (Replacer< double >("&hpos;", pos).apply
	    (Replacer< double >("&vpos;", vpos).apply(stop_template)))));
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
  
  cout<<Replacer< string >("<headline/>", headline).apply
      (Replacer< string >("<stops-diagram/>", result.str()).apply(frame_template()));
  
  return 0;
}
