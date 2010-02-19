/*****************************************************************
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

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
    "     width=\"700px\" height=\"495px\">\n"
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
      "<text x=\"30\" y=\"30\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"none\">&rel_ref;</tspan>\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">from</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"24px\">&rel_from;</tspan></text>\n"
      "<text x=\"30\" y=\"60\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"none\">&rel_ref;</tspan>\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"24px\">&rel_to;</tspan></text>\n"
      "<text x=\"30\" y=\"45\" font-family=\"Liberation Sans, sans-serif\" font-size=\"32px\" fill=\"&rel_color;\">&rel_ref;</text>\n"
      "\n";
}

string only_to_headline_template()
{
  return "<title>Line &rel_ref; to &rel_to;</title>\n"
      "<desc>Line &rel_ref; to &rel_to;</desc>\n"
      "\n"
      "<text x=\"30\" y=\"60\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"&rel_color;\">&rel_ref;</tspan>\n"
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

string stop_name_template()
{
  return "<text x=\"0\" y=\"-10\" transform=\"translate(&hpos;,&vpos;) rotate(-45,0,-10)\""
      " font-family=\"Liberation Sans, sans-serif\" font-size=\"&stop_fontsize;\">&stopname;</text>\n"
      "\n";
}

string bidirectional_stop_template()
{
  return "<circle cx=\"&hpos;\" cy=\"&vpos;\" r=\"7\" fill=\"&rel_color;\"/>\n"
      "\n";
}

string forward_stop_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 0 14,0 Z\" style=\"fill:&rel_color;\""
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"2px\" points=\"0 -8, 6 -8\"" 
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<path d=\"M 4,-4 l 0,-8 l 6,4 Z\" style=\"fill:&rel_color;\" transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "\n";
}

string backward_stop_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 1 14,0 Z\" style=\"fill:&rel_color;\""
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"2px\" points=\"-6 8, 0 8\"" 
      " transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "<path d=\"M -4,4 l 0,8 l -6,-4 Z\" style=\"fill:&rel_color;\" transform=\"translate(&hpos;,&vpos;)\"/>\n"
      "\n";
}

//-----------------------------------------------------------------------------

vector< unsigned int > longest_ascending_subsequence(const vector< unsigned int >& sequence)
{
  vector< vector< unsigned int > > sublists;
  
  vector< unsigned int >::const_iterator it(sequence.begin());
  if (it != sequence.end())
  {
    vector< unsigned int > sublist;
    sublist.push_back(*it);
    sublists.push_back(sublist);
    ++it;
  }
  while (it != sequence.end())
  {
    unsigned int i(sublists.size());
    while (i > 0)
    {
      --i;
      
      if (*it > sublists[i].back())
      {
	if (sublists.size() == i+1)
	{
	  sublists.push_back(sublists[i]);
	  sublists[i+1].push_back(*it);
	}
	else if (*it < sublists[i+1].back())
	{
	  sublists[i+1] = sublists[i];
	  sublists[i+1].push_back(*it);
	}
      }
      else if (*it < sublists[i].back())
      {
	if (sublists[i].size() == 1)
	  sublists[i][0] = *it;
	else if (*it > sublists[i][sublists[i].size()-2])
	  sublists[i][sublists[i].size()-1] = *it;
      }
    }
    
    ++it;
  }
  
  return sublists.back();
}

vector< unsigned int > longest_descending_subsequence(const vector< unsigned int >& sequence)
{
  vector< vector< unsigned int > > sublists;
  
  vector< unsigned int >::const_reverse_iterator it(sequence.rbegin());
  if (it != sequence.rend())
  {
    vector< unsigned int > sublist;
    sublist.push_back(*it);
    sublists.push_back(sublist);
    ++it;
  }
  while (it != sequence.rend())
  {
    unsigned int i(sublists.size());
    while (i > 0)
    {
      --i;
      
      if (*it > sublists[i].back())
      {
	if (sublists.size() == i+1)
	{
	  sublists.push_back(sublists[i]);
	  sublists[i+1].push_back(*it);
	}
	else if (*it < sublists[i+1].back())
	{
	  sublists[i+1] = sublists[i];
	  sublists[i+1].push_back(*it);
	}
      }
      else if (*it < sublists[i].back())
      {
	if (sublists[i].size() == 1)
	  sublists[i][0] = *it;
	else if (*it > sublists[i][sublists[i].size()-2])
	  sublists[i][sublists[i].size()-1] = *it;
      }
    }
    
    ++it;
  }
  
  return sublists.back();
}

//-----------------------------------------------------------------------------

struct NamedNode
{
  public:
    NamedNode() : lat(100.0), lon(200.0), name("") {}
    
    double lat, lon;
    string name;
};

struct Relation
{
  public:
    Relation() : forward_stops(), backward_stops(), ref(""), color("blue"),
	     from(""), to(""), direction(0) {}
    
    vector< unsigned int > forward_stops;
    vector< unsigned int > backward_stops;
    
    string ref;
    string color;
    string from;
    string to;
    int direction;
    const static int FORWARD = 1;
    const static int BACKWARD = 2;
    const static int BOTH = 3;
};

struct Stop
{
  public:
    Stop() : name(""), used_by() {}
    
    string name;
    vector< int > used_by;
    const static int FORWARD = 1;
    const static int BACKWARD = 2;
    const static int BOTH = 3;
};

map< unsigned int, NamedNode > nodes;
NamedNode nnode;
unsigned int id;

vector< Relation > relations;
Relation relation;

unsigned int parse_status;
const unsigned int IN_NODE = 1;
const unsigned int IN_RELATION = 2;

bool doubleread_rel;
// int rel_count(0);
// int doubleread_rel(0);
// const int TIME_ORDERED = 1;
// const int SPACE_ORDERED = 2;

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
    if ((key == "name") && (parse_status == IN_NODE))
      nnode.name = escape_xml(value);
    if ((key == "ref") && (parse_status == IN_RELATION))
      relation.ref = escape_xml(value);
    if ((key == "to") && (parse_status == IN_RELATION))
      relation.to = escape_xml(value);
    if ((key == "from") && (parse_status == IN_RELATION))
      relation.from = escape_xml(value);
    if ((key == "color") && (parse_status == IN_RELATION))
      relation.color = escape_xml(value);
  }
  else if (!strcmp(el, "node"))
  {
    parse_status = IN_NODE;
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
      if (role.substr(0, 7) == "forward")
      {
	relation.forward_stops.push_back(ref);
	relation.direction = Relation::BOTH;
      }
      else if (role.substr(0, 8) == "backward")
      {
	relation.backward_stops.push_back(ref);
	relation.direction = Relation::BOTH;
      }
      else
      {
	relation.forward_stops.push_back(ref);
	relation.backward_stops.push_back(ref);
      }
    }
  }
  else if (!strcmp(el, "relation"))
  {
    parse_status = IN_RELATION;
    relation = Relation();
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    nodes[id] = nnode;
    parse_status = 0;
  }
  else if (!strcmp(el, "relation"))
  {
    parse_status = 0;
    relations.push_back(relation);
  }
}

int main(int argc, char *argv[])
{
  doubleread_rel = false;
  if (argc >= 2)
  {
    if (!strcmp("--backspace", argv[1]))
      doubleread_rel = true;
    else if (!strcmp("--backtime", argv[1]))
      doubleread_rel = true;
  }
  
  // read the XML input
  parse_status = 0;
  parse(stdin, start, end);

  // make a common stoplist from both relations
  list< Stop > stoplist;
  if (relations.size() == 0)
  {
    cout<<Replacer< string >("<headline/>", "").apply
	(Replacer< string >("<stops-diagram/>", "<text>No relation found</text>").apply(frame_template()));
    return 0;
  }
  
//   for (vector< Relation >::const_iterator it(relations.begin()); it != relations.end(); ++it)
//   {
//     cerr<<"from: "<<it->from<<'\n';
//     cerr<<"to: "<<it->from<<'\n';
//     for (vector< unsigned int >::const_iterator iit(it->forward_stops.begin());
//         iit != it->forward_stops.end(); ++iit)
//       cerr<<nodes[*iit].lat<<' '<<nodes[*iit].name<<'\n';
//     cerr<<'\n';
//     for (vector< unsigned int >::const_iterator iit(it->backward_stops.begin());
//     iit != it->backward_stops.end(); ++iit)
//     cerr<<nodes[*iit].lat<<' '<<nodes[*iit].name<<'\n';
//     cerr<<'\n';
//   }

  relation = relations.front();
  for(vector< unsigned int >::const_iterator it(relation.forward_stops.begin());
      it != relation.forward_stops.end(); ++it)
  {
    Stop stop;
    stop.used_by.resize(relations.size());
    if (nodes[*it].lat <= 90.0)
    {
      stop.name = nodes[*it].name;
      stop.used_by[0] = Stop::FORWARD;
    }
    stoplist.push_back(stop);
  }
  if (relations.begin()->direction == 0)
    relations.begin()->direction = Relation::FORWARD;

//   for (list< Stop >::const_iterator it(stoplist.begin());
//       it != stoplist.end(); ++it)
//   {
//     cerr<<it->name<<' '<<it->used_by.size()<<'\n';
//   }
//   cerr<<'\n';

  vector< Relation >::iterator rit(relations.begin());
  unsigned int rel_count(1);
  ++rit;
  while (rit != relations.end())
  {
    map< string, unsigned int > stopdict;
    for (unsigned int i(0); i < rit->forward_stops.size(); ++i)
    {
      if (nodes[rit->forward_stops[i]].lat <= 90.0)
	stopdict[nodes[rit->forward_stops[i]].name] = i+1;
    }
    
    vector< unsigned int > indices_of_present_stops;
    for (list< Stop >::const_iterator it(stoplist.begin());
        it != stoplist.end(); ++it)
    {
      if (stopdict[it->name] > 0)
	indices_of_present_stops.push_back(stopdict[it->name]);
    }
    
    vector< unsigned int > ascending(longest_ascending_subsequence
        (indices_of_present_stops));
    vector< unsigned int > descending(longest_descending_subsequence
        (indices_of_present_stops));
    
    if (ascending.size() > descending.size())
    {
      if (rit->direction == 0)
	rit->direction = Relation::FORWARD;
      
      vector< unsigned int >::const_iterator sit(ascending.begin());
      unsigned int last_idx(1);
      for (list< Stop >::iterator it(stoplist.begin());
      it != stoplist.end(); ++it)
      {
	if (stopdict[it->name] == *sit)
	{
	  // insert stops that aren't yet inserted
	  while (*sit > last_idx)
	  {
	    Stop stop;
	    stop.used_by.resize(relations.size());
	    if (nodes[rit->forward_stops[last_idx-1]].lat <= 90.0)
	    {
	      stop.name = nodes[rit->forward_stops[last_idx-1]].name;
	      stop.used_by[rel_count] = Stop::FORWARD;
	      stoplist.insert(it, stop);
	    }
	    ++last_idx;
	  }
	  ++last_idx;
	  
	  // match the current stop
	  it->used_by[rel_count] = Stop::FORWARD;;
	  
	  if (++sit == ascending.end())
	    break;
	}
      }
      
      // insert stops at the end
      while (last_idx < rit->forward_stops.size())
      {
	Stop stop;
	stop.used_by.resize(relations.size());
	if (nodes[rit->forward_stops[last_idx-1]].lat <= 90.0)
	{
	  stop.name = nodes[rit->forward_stops[last_idx-1]].name;
	  stop.used_by[rel_count] = Stop::FORWARD;
	  stoplist.push_back(stop);
	}
	++last_idx;
      }
    }
    else
    {
      if (rit->direction == 0)
	rit->direction = Relation::BACKWARD;
      
      vector< unsigned int >::const_reverse_iterator sit(descending.rbegin());
      unsigned int last_idx(rit->forward_stops.size());
      for (list< Stop >::iterator it(stoplist.begin());
	  it != stoplist.end(); ++it)
      {
	if (stopdict[it->name] == *sit)
	{
	  // insert stops that aren't yet inserted
	  while (*sit < last_idx)
	  {
	    Stop stop;
	    stop.used_by.resize(relations.size());
	    if (nodes[rit->forward_stops[last_idx-1]].lat <= 90.0)
	    {
	      stop.name = nodes[rit->forward_stops[last_idx-1]].name;
	      stop.used_by[rel_count] = Stop::BACKWARD;
	      stoplist.insert(it, stop);
	    }
	    --last_idx;
	  }
	  --last_idx;
	  
	  // match the current stop
	  it->used_by[rel_count] = Stop::BACKWARD;;
	  
	  if (++sit == descending.rend())
	    break;
	}
      }
      
      // insert stops at the end
      while (last_idx > 0)
      {
	Stop stop;
	stop.used_by.resize(relations.size());
	if (nodes[rit->forward_stops[last_idx-1]].lat <= 90.0)
	{
	  stop.name = nodes[rit->forward_stops[last_idx-1]].name;
	  stop.used_by[rel_count] = Stop::BACKWARD;
	  stoplist.push_back(stop);
	}
	--last_idx;
      }
    }
    
    ++rit;
    ++rel_count;
  }
  
/*  for (list< Stop >::const_iterator it(stoplist.begin());
  it != stoplist.end(); ++it)
  {
    cerr<<it->name<<' '<<it->used_by.size()<<'\n';
  }
  cerr<<'\n';*/
  
  rit = relations.begin();
  rel_count = 0;
  while (rit != relations.end())
  {
    if (!(rit->direction == Relation::BOTH) && (!doubleread_rel))
    {
      ++rit;
      ++rel_count;
      continue;
    }
    
    map< string, unsigned int > stopdict;
    for (unsigned int i(0); i < rit->backward_stops.size(); ++i)
    {
      if (nodes[rit->backward_stops[i]].lat <= 90.0)
	stopdict[nodes[rit->backward_stops[i]].name] = i+1;
    }
    
    vector< unsigned int > indices_of_present_stops;
    for (list< Stop >::const_iterator it(stoplist.begin());
    it != stoplist.end(); ++it)
    {
      if (stopdict[it->name] > 0)
	indices_of_present_stops.push_back(stopdict[it->name]);
    }
    
    vector< unsigned int > ascending(longest_ascending_subsequence
    (indices_of_present_stops));
    vector< unsigned int > descending(longest_descending_subsequence
    (indices_of_present_stops));
    
    if (ascending.size() > descending.size())
    {
      vector< unsigned int >::const_iterator sit(ascending.begin());
      unsigned int last_idx(1);
      for (list< Stop >::iterator it(stoplist.begin());
      it != stoplist.end(); ++it)
      {
	if (stopdict[it->name] == *sit)
	{
	  // insert stops that aren't yet inserted
	  while (*sit > last_idx)
	  {
	    Stop stop;
	    stop.used_by.resize(relations.size());
	    if (nodes[rit->backward_stops[last_idx-1]].lat <= 90.0)
	    {
	      stop.name = nodes[rit->backward_stops[last_idx-1]].name;
	      stop.used_by[rel_count] = Stop::BACKWARD;
	      stoplist.insert(it, stop);
	    }
	    ++last_idx;
	  }
	  ++last_idx;
	  
	  // match the current stop
	  it->used_by[rel_count] |= Stop::BACKWARD;
	  
	  if (++sit == ascending.end())
	    break;
	}
      }
      
      // insert stops at the end
      while (last_idx < rit->backward_stops.size())
      {
	Stop stop;
	stop.used_by.resize(relations.size());
	if (nodes[rit->backward_stops[last_idx-1]].lat <= 90.0)
	{
	  stop.name = nodes[rit->backward_stops[last_idx-1]].name;
	  stop.used_by[rel_count] = Stop::BACKWARD;
	  stoplist.push_back(stop);
	}
	++last_idx;
      }
    }
    else
    {
      vector< unsigned int >::const_reverse_iterator sit(descending.rbegin());
      unsigned int last_idx(rit->backward_stops.size());
      for (list< Stop >::iterator it(stoplist.begin());
      it != stoplist.end(); ++it)
      {
	if (stopdict[it->name] == *sit)
	{
	  // insert stops that aren't yet inserted
	  while (*sit < last_idx)
	  {
	    Stop stop;
	    stop.used_by.resize(relations.size());
	    if (nodes[rit->backward_stops[last_idx-1]].lat <= 90.0)
	    {
	      stop.name = nodes[rit->backward_stops[last_idx-1]].name;
	      stop.used_by[rel_count] = Stop::FORWARD;
	      stoplist.insert(it, stop);
	    }
	    --last_idx;
	  }
	  --last_idx;
	  
	  // match the current stop
	  it->used_by[rel_count] |= Stop::FORWARD;;
	  
	  if (++sit == descending.rend())
	    break;
	}
      }
      
      // insert stops at the end
      while (last_idx > 0)
      {
	Stop stop;
	stop.used_by.resize(relations.size());
	if (nodes[rit->backward_stops[last_idx-1]].lat <= 90.0)
	{
	  stop.name = nodes[rit->backward_stops[last_idx-1]].name;
	  stop.used_by[rel_count] = Stop::FORWARD;
	  stoplist.push_back(stop);
	}
	--last_idx;
      }
    }
    
    ++rit;
    ++rel_count;
  }

/*  for (list< Stop >::const_iterator it(stoplist.begin());
  it != stoplist.end(); ++it)
  {
    cerr<<it->name<<' '<<it->used_by.size()<<'\n';
  }
  cerr<<'\n';*/

  string headline;
  headline = from_to_headline_template();
/*  if ((rel_count == 1) && (!doubleread_rel))
    headline = only_to_headline_template();
  else
    headline = from_to_headline_template();*/
  
  headline = Replacer< string >("&rel_from;", relation.from).apply
	    (Replacer< string >("&rel_to;", relation.to).apply
	    (Replacer< string >("&rel_ref;", relation.ref).apply
	    (Replacer< string >("&rel_color;", relation.color).apply(headline))));
  
  ostringstream result("");
  
  if ((stoplist.size() > 1) && (stoplist.size() <= 21))
  {
    for (unsigned int i(0); i < stoplist.begin()->used_by.size(); ++i)
      result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;",  30).apply
	    (Replacer< double >("&hmax;", 510).apply
	    (Replacer< double >("&vpos;", 380 + 20*i).apply(line_template()))));
    
    double pos(30);
    double stop_distance(0);
  
    stop_distance = 480.0/(stoplist.size()-1);
  
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      for (unsigned int i(0); i < it->used_by.size(); ++i)
      {
	string stop_template;
	if (it->used_by[i] == Stop::BOTH)
	  stop_template = bidirectional_stop_template();
	else if (it->used_by[i] == Stop::FORWARD)
	  stop_template = forward_stop_template();
	else if (it->used_by[i] == Stop::BACKWARD)
	  stop_template = backward_stop_template();
      
	result<<Replacer< string >("&stopname;", it->name).apply
	    (Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< string >("&stop_fontsize;", "16px").apply
	    (Replacer< double >("&hpos;", pos).apply
	    (Replacer< double >("&vpos;", 380 + 20*i).apply(stop_template)))));	
      }
      
      result<<Replacer< string >("&stopname;", it->name).apply
          (Replacer< string >("&rel_color;", relation.color).apply
          (Replacer< string >("&stop_fontsize;", "16px").apply
          (Replacer< double >("&hpos;", pos).apply
          (Replacer< double >("&vpos;", 380).apply(stop_name_template())))));	
      pos += stop_distance;
    }
  }
  else if (stoplist.size() > 21)
  {
    for (unsigned int i(0); i < stoplist.begin()->used_by.size(); ++i)
    {
      result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;",  30).apply
	    (Replacer< double >("&hmax;", 530).apply
	    (Replacer< double >("&vpos;", 210 + 20*i).apply(line_template()))));
      result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;",  30).apply
	    (Replacer< double >("&hmax;", 510).apply
	    (Replacer< double >("&vpos;", 380 + 20*i).apply(line_template()))))<<'\n';
    }
    
    unsigned int count(0);
    double pos(30), vpos(210);
    double stop_distance(0);
  
    stop_distance = 480.0/((stoplist.size()+1)/2-1);
  
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      for (unsigned int i(0); i < it->used_by.size(); ++i)
      {
	string stop_template;
	if (it->used_by[i] == Stop::BOTH)
	  stop_template = bidirectional_stop_template();
	else if (it->used_by[i] == Stop::FORWARD)
	  stop_template = forward_stop_template();
	else if (it->used_by[i] == Stop::BACKWARD)
	  stop_template = backward_stop_template();
      
	result<<Replacer< string >("&stopname;", it->name).apply
	    (Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< string >("&stop_fontsize;", "16px").apply
	    (Replacer< double >("&hpos;", pos).apply
	    (Replacer< double >("&vpos;", vpos + 20*i).apply(stop_template)))));	
      }
      
      result<<Replacer< string >("&stopname;", it->name).apply
          (Replacer< string >("&rel_color;", relation.color).apply
          (Replacer< string >("&stop_fontsize;", "10px").apply
          (Replacer< double >("&hpos;", pos).apply
          (Replacer< double >("&vpos;", 380).apply(stop_name_template())))));	
      pos += stop_distance;
      if (++count >= (stoplist.size()+1)/2)
      {
	count = 0;
	vpos = 380;
	stop_distance = 480.0/(stoplist.size()/2);
	pos = 30 + stop_distance;
      }
    }
  }
  
  cout<<Replacer< string >("<headline/>", headline).apply
      (Replacer< string >("<stops-diagram/>", result.str()).apply(frame_template()));
  
  return 0;
}
