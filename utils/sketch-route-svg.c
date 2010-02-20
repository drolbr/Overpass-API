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
    "     width=\"&width;px\" height=\"&height;px\">\n"
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
      "  &from_tail;</text>\n"
      "<text x=\"30\" y=\"60\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"none\">&rel_ref;</tspan>\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">to</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  &to_tail;</text>\n"
      "<text x=\"30\" y=\"45\" font-family=\"Liberation Sans, sans-serif\" font-size=\"32px\" fill=\"&rel_color;\">&rel_ref;</text>\n"
      "\n";
}

string alternate_headline_extension_template()
{
  return "  &head;\n"
      "  <tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  <tspan font-size=\"16px\">or</tspan><tspan font-size=\"32px\" fill=\"none\">_</tspan>\n"
      "  &tail;";
}

string destination_headline_template()
{
  return "<tspan font-size=\"24px\">&content;</tspan>";
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

string left_join_template()
{
  return "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"7px\""
  " points=\"&htop; &vpos_join;, &hmin; &vpos_self;, &hmax; &vpos_self;\"/>\n";
}

string right_join_template()
{
  return "<polyline fill=\"none\" stroke=\"&rel_color;\" stroke-width=\"7px\""
  " points=\"&hmin; &vpos_self;, &hmax; &vpos_self;, &htop; &vpos_join;\"/>\n";
}

string stop_name_template()
{
  return "<text x=\"0\" y=\"-10\" transform=\"translate(&hpos;,&vpos;) rotate(-45,0,-10)\""
      " font-family=\"Liberation Sans, sans-serif\" font-size=\"&stop_fontsize;px\">&stopname;</text>\n"
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
  if (it == sequence.end())
    return vector< unsigned int >();
  vector< unsigned int > sublist;
  sublist.push_back(*it);
  sublists.push_back(sublist);
  ++it;
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
  if (it == sequence.rend())
    return vector< unsigned int >();
  vector< unsigned int > sublist;
  sublist.push_back(*it);
  sublists.push_back(sublist);
  ++it;
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
    const static int BOTH = 4;
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
  int argi(1);
  double width(700);
  double height(495);
  double stop_font_size(0);
  int force_rows(0);
  while (argi < argc)
  {
    if (!strncmp("--width=", argv[argi], 8))
      width = atof(((string)(argv[argi])).substr(8).c_str());
    else if (!strncmp("--height=", argv[argi], 9))
      height = atof(((string)(argv[argi])).substr(9).c_str());
    else if (!strncmp("--stop-font-size=", argv[argi], 17))
      stop_font_size = atof(((string)(argv[argi])).substr(17).c_str());
    else if (!strncmp("--rows=", argv[argi], 7))
      force_rows = atoi(((string)(argv[argi])).substr(7).c_str());
    else if (!strcmp("--backspace", argv[argi]))
      doubleread_rel = true;
    else if (!strcmp("--backtime", argv[argi]))
      doubleread_rel = true;
    ++argi;
  }
  
  // read the XML input
  parse_status = 0;
  parse(stdin, start, end);

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

  // make a common stoplist from all relations
  list< Stop > stoplist;
  if (relations.size() == 0)
  {
    cout<<Replacer< double >("&width;", width).apply
	(Replacer< double >("&height;", height).apply
	(Replacer< string >("<headline/>", "").apply
	(Replacer< string >("<stops-diagram/>", "<text x=\"100\" y=\"100\">No relation found</text>").apply(frame_template()))));
    return 0;
  }
  
  // integrate all relations (their forward direction) into the stoplist
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
  relations.begin()->direction |= Relation::FORWARD;

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
      rit->direction |= Relation::FORWARD;
      
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
      while (last_idx <= rit->forward_stops.size())
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
      rit->direction |= Relation::BACKWARD;
      
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
  
  // integrate second direction (where it exists) into stoplist
  rit = relations.begin();
  rel_count = 0;
  while (rit != relations.end())
  {
    if (((rit->direction & Relation::BOTH) == 0) && (!doubleread_rel))
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
    
    int direction_const(0);
    if ((rit->direction & Relation::FORWARD) != 0)
      direction_const = Stop::BACKWARD;
    if ((rit->direction & Relation::BACKWARD) != 0)
      direction_const = Stop::FORWARD;
    
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
	      stop.used_by[rel_count] = direction_const;
	      stoplist.insert(it, stop);
	    }
	    ++last_idx;
	  }
	  ++last_idx;
	  
	  // match the current stop
	  it->used_by[rel_count] |= direction_const;
	  
	  if (++sit == ascending.end())
	    break;
	}
      }
      
      // insert stops at the end
      while (last_idx <= rit->backward_stops.size())
      {
	Stop stop;
	stop.used_by.resize(relations.size());
	if (nodes[rit->backward_stops[last_idx-1]].lat <= 90.0)
	{
	  stop.name = nodes[rit->backward_stops[last_idx-1]].name;
	  stop.used_by[rel_count] = direction_const;
	  stoplist.push_back(stop);
	}
	++last_idx;
      }
    }
    else if (descending.size() > 0)
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
	      stop.used_by[rel_count] = direction_const;
	      stoplist.insert(it, stop);
	    }
	    --last_idx;
	  }
	  --last_idx;
	  
	  // match the current stop
	  it->used_by[rel_count] |= direction_const;
	  
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
	  stop.used_by[rel_count] = direction_const;
	  stoplist.push_back(stop);
	}
	--last_idx;
      }
    }
    
    ++rit;
    ++rel_count;
  }
  
  // unify one-directional relations as good as possible
  multimap< double, pair< int, int > > possible_pairs;
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    if (relations[i].direction != Relation::FORWARD)
      continue;
    for (unsigned int j(0); j < relations.size(); ++j)
    {
      int common(0), total(0);
      if (relations[j].direction != Relation::BACKWARD)
	continue;
      for (list< Stop >::const_iterator it(stoplist.begin());
          it != stoplist.end(); ++it)
      {
	if (it->used_by[i] == Stop::FORWARD)
	{
	  ++total;
	  if (it->used_by[j] == Stop::BACKWARD)
	    ++common;
	}
	else if (it->used_by[j] == Stop::BACKWARD)
	  ++total;
      }
      possible_pairs.insert(make_pair
          ((double)common/(double)total, make_pair(i, j)));
    }
  }
  
  for (multimap< double, pair< int, int > >::const_reverse_iterator
       it(possible_pairs.rbegin()); it != possible_pairs.rend(); ++it)
  {
    if (relations[it->second.second].direction != Relation::BACKWARD)
      continue;
    for (list< Stop >::iterator sit(stoplist.begin());
        sit != stoplist.end(); ++sit)
      sit->used_by[it->second.first] |= sit->used_by[it->second.second];
    relations[it->second.second].direction = 0;
  }

/*  for (list< Stop >::const_iterator it(stoplist.begin());
  it != stoplist.end(); ++it)
  {
    cerr<<it->name<<' '<<it->used_by.size()<<'\n';
  }
  cerr<<'\n';*/

  // trim the horizontal itinerary lines
  vector< int > first_stop_idx(relations.size());
  vector< int > last_stop_idx(relations.size());
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    if (relations[i].direction == 0)
      continue;
    
    first_stop_idx[i] = 0;
    last_stop_idx[i] = stoplist.size()-1;
    for (list< Stop >::const_iterator it(stoplist.begin());
        it != stoplist.end(); ++it)
    {
      if (it->used_by[i] != 0)
	break;
      ++first_stop_idx[i];
    }
    for (list< Stop >::const_reverse_iterator it(stoplist.rbegin());
        it != stoplist.rend(); ++it)
    {
      if (it->used_by[i] != 0)
	break;
      --last_stop_idx[i];
    }
  }
  
  // unify itinerary lines with similar begin
  vector< int > left_join_to(relations.size(), -1);
  vector< int > right_join_to(relations.size(), -1);
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    for (unsigned int j(0); j < i; ++j)
    {
      int ijsimilar(0);
      for (list< Stop >::const_iterator it(stoplist.begin());
          it != stoplist.end(); ++it)
      {
	if (it->used_by[i] != it->used_by[j])
	  break;
	++ijsimilar;
      }
      if (ijsimilar > first_stop_idx[i])
      {
	first_stop_idx[i] = ijsimilar;
	left_join_to[i] = j;
      }
    }
  }
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    for (unsigned int j(0); j < i; ++j)
    {
      int ijsimilar(stoplist.size()-1);
      for (list< Stop >::const_reverse_iterator it(stoplist.rbegin());
      it != stoplist.rend(); ++it)
      {
	if (it->used_by[i] != it->used_by[j])
	  break;
	--ijsimilar;
      }
      if (ijsimilar < last_stop_idx[i])
      {
	last_stop_idx[i] = ijsimilar;
	right_join_to[i] = j;
      }
    }
  }
    
  string from_buffer, to_buffer, headline, last_from, last_to;
  headline = from_to_headline_template();
  from_buffer = "&tail;";
  to_buffer = "&tail;";
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    if (relations[i].direction == 0)
      continue;
    if (last_from == "")
      last_from = relations[i].from;
    else if (last_from != relations[i].from)
    {
      from_buffer = Replacer< string >
	  ("&tail;", Replacer< string >("&head;", Replacer< string >
	  ("&content;", last_from).apply(destination_headline_template())).apply
	  (alternate_headline_extension_template())).apply(from_buffer);
      last_from = relations[i].from;
    }
    if (last_to == "")
      last_to = relations[i].to;
    else if (last_to != relations[i].to)
    {
      to_buffer = Replacer< string >
	  ("&tail;", Replacer< string >("&head;", Replacer< string >
	  ("&content;", last_to).apply(destination_headline_template())).apply
	      (alternate_headline_extension_template())).apply(to_buffer);
      last_to = relations[i].to;
    }
  }
  from_buffer = Replacer< string >("&tail;", Replacer< string >
      ("&content;", last_from).apply(destination_headline_template())).apply(from_buffer);
  to_buffer = Replacer< string >("&tail;", Replacer< string >
      ("&content;", last_to).apply(destination_headline_template())).apply(to_buffer);
  
  headline = Replacer< string >("&from_tail;", from_buffer).apply
	    (Replacer< string >("&to_tail;", to_buffer).apply
	    (Replacer< string >("&rel_ref;", relation.ref).apply
	    (Replacer< string >("&rel_color;", relation.color).apply(headline))));
  
  ostringstream result("");
  
  if (stoplist.size() <= 1)
  {
    cout<<Replacer< double >("&width;", width).apply
        (Replacer< double >("&height;", height).apply
        (Replacer< string >("<headline/>", headline).apply
        (Replacer< string >("<stops-diagram/>", "<text>Only one stop found</text>").apply(frame_template()))));
    return 0;
  }
  if (((stoplist.size() <= 21) && (force_rows == 0)) || (force_rows == 1))
  {
    double pos(30);
    double stop_distance(0);
    
    stop_distance = (width - 220)/(stoplist.size()-1);
    
    if (stop_font_size == 0)
      stop_font_size = 16;
    
    int offset(0);
    vector< int > offset_of(relations.size());
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      if (relations[i].direction == 0)
	continue;
      offset_of[i] = 20*(offset++);
      
      double hmin(30 + first_stop_idx[i]*stop_distance);
      double hmax(30 + last_stop_idx[i]*stop_distance);
      
      result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;", hmin).apply
	    (Replacer< double >("&hmax;", hmax).apply
	    (Replacer< double >("&vpos;", height - 115 + offset_of[i]).apply(line_template()))));
      if (left_join_to[i] >= 0)
	result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;", hmin+4-stop_distance/2).apply
	    (Replacer< double >("&hmax;", hmin+2).apply
	    (Replacer< double >("&htop;", hmin-4-stop_distance/2).apply
	    (Replacer< double >("&vpos_self;", height - 115 + offset_of[i]).apply
	    (Replacer< double >("&vpos_join;", height - 115 + offset_of[left_join_to[i]]).apply
	    (left_join_template()))))));
      if (right_join_to[i] >= 0)
	result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;", hmax-2).apply
	    (Replacer< double >("&hmax;", hmax-4+stop_distance/2).apply
	    (Replacer< double >("&htop;", hmax+4+stop_distance/2).apply
	    (Replacer< double >("&vpos_self;", height - 115 + offset_of[i]).apply
	    (Replacer< double >("&vpos_join;", height - 115 + offset_of[left_join_to[i]]).apply
	    (right_join_template()))))));
    }
    
    int stopcount(0);
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      for (unsigned int i(0); i < relations.size(); ++i)
      {
	if (relations[i].direction == 0)
	  continue;
	if (stopcount < first_stop_idx[i])
	  continue;
	if (stopcount > last_stop_idx[i])
	  continue;
	
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
	    (Replacer< double >("&vpos;", height - 115 + offset_of[i]).apply(stop_template)))));	
      }
      ++stopcount;
      
      result<<Replacer< string >("&stopname;", it->name).apply
          (Replacer< string >("&rel_color;", relation.color).apply
          (Replacer< double >("&stop_fontsize;", stop_font_size).apply
          (Replacer< double >("&hpos;", pos).apply
          (Replacer< double >("&vpos;", height - 115).apply(stop_name_template())))));	
      pos += stop_distance;
    }
  }
  else
  {
    if (stop_font_size == 0)
      stop_font_size = 10;
    
    int offset(0);
    vector< int > offset_of(relations.size());
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      if (relations[i].direction == 0)
	continue;
      offset_of[i] = 20*(offset++);
      
      if (first_stop_idx[i] < (int)(stoplist.size()+1)/2)
      {
	double stop_distance((width - 220)/((stoplist.size()+1)/2-1));
	double hmin(30 + first_stop_idx[i]*stop_distance);
	double hmax(30 + last_stop_idx[i]*stop_distance);
	if (last_stop_idx[i] >= (int)(stoplist.size()+1)/2)
	  hmax = width - 170;
	result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;", hmin).apply
	    (Replacer< double >("&hmax;", hmax).apply
	    (Replacer< double >("&vpos;", height/2 - 40 + offset_of[i]).apply(line_template()))));
        if (left_join_to[i] >= 0)
	  result<<Replacer< string >("&rel_color;", relation.color).apply
	      (Replacer< double >("&hmin;", hmin+4-stop_distance/2).apply
	      (Replacer< double >("&hmax;", hmin+2).apply
	      (Replacer< double >("&htop;", hmin-4-stop_distance/2).apply
	      (Replacer< double >("&vpos_self;", height/2 - 40 + offset_of[i]).apply
	      (Replacer< double >("&vpos_join;", height/2 - 40 + offset_of[left_join_to[i]]).apply
	      (left_join_template()))))));
        if ((right_join_to[i] >= 0) && (last_stop_idx[i] < (int)(stoplist.size()+1)/2))
	  result<<Replacer< string >("&rel_color;", relation.color).apply
	      (Replacer< double >("&hmin;", hmax-2).apply
	      (Replacer< double >("&hmax;", hmax-4+stop_distance/2).apply
	      (Replacer< double >("&htop;", hmax+4+stop_distance/2).apply
	      (Replacer< double >("&vpos_self;", height/2 - 40 + offset_of[i]).apply
	      (Replacer< double >("&vpos_join;", height/2 - 40 + offset_of[left_join_to[i]]).apply
	      (right_join_template()))))));
      }
      if (last_stop_idx[i] >= (int)(stoplist.size()+1)/2)
      {
	double stop_distance = (width - 220)/(stoplist.size()/2);
	double hmin(30 + (first_stop_idx[i]-(int)(stoplist.size()-1)/2)*stop_distance);
	double hmax(30 + (last_stop_idx[i]-(int)(stoplist.size()-1)/2)*stop_distance);
	if (first_stop_idx[i] < (int)(stoplist.size()+1)/2)
	  hmin = 30;
	result<<Replacer< string >("&rel_color;", relation.color).apply
	    (Replacer< double >("&hmin;", hmin).apply
	    (Replacer< double >("&hmax;", hmax).apply
	    (Replacer< double >("&vpos;", height - 115 + offset_of[i]).apply(line_template()))))<<'\n';
        if ((left_join_to[i] >= 0) && (first_stop_idx[i] >= (int)(stoplist.size()+1)/2))
	  result<<Replacer< string >("&rel_color;", relation.color).apply
	      (Replacer< double >("&hmin;", hmin+4-stop_distance/2).apply
	      (Replacer< double >("&hmax;", hmin+2).apply
	      (Replacer< double >("&htop;", hmin-4-stop_distance/2).apply
	      (Replacer< double >("&vpos_self;", height - 115 + offset_of[i]).apply
	      (Replacer< double >("&vpos_join;", height - 115 + offset_of[left_join_to[i]]).apply
	      (left_join_template()))))));
        if (right_join_to[i] >= 0)
	  result<<Replacer< string >("&rel_color;", relation.color).apply
	      (Replacer< double >("&hmin;", hmax-2).apply
	      (Replacer< double >("&hmax;", hmax-4-stop_distance/2).apply
	      (Replacer< double >("&htop;", hmax+4+stop_distance/2).apply
	      (Replacer< double >("&vpos_self;", height - 115 + offset_of[i]).apply
	      (Replacer< double >("&vpos_join;", height - 115 + offset_of[left_join_to[i]]).apply
	      (right_join_template()))))));
      }
    }
    
    unsigned int count(0);
    double pos(30), vpos(height/2 - 40);
    double stop_distance(0);
  
    stop_distance = (width - 220)/((stoplist.size()+1)/2-1);
  
    int stopcount(0);
    for (list< Stop >::const_iterator it(stoplist.begin()); it != stoplist.end(); ++it)
    {
      for (unsigned int i(0); i < relations.size(); ++i)
      {
	if (relations[i].direction == 0)
	  continue;
	if (stopcount < first_stop_idx[i])
	  continue;
	if (stopcount > last_stop_idx[i])
	  continue;
	
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
	    (Replacer< double >("&vpos;", vpos + offset_of[i]).apply(stop_template)))));	
      }
      ++stopcount;
      
      result<<Replacer< string >("&stopname;", it->name).apply
          (Replacer< string >("&rel_color;", relation.color).apply
          (Replacer< double >("&stop_fontsize;", stop_font_size).apply
          (Replacer< double >("&hpos;", pos).apply
          (Replacer< double >("&vpos;", vpos).apply(stop_name_template())))));	
      pos += stop_distance;
      if (++count >= (stoplist.size()+1)/2)
      {
	count = 0;
	vpos = height - 115;
	stop_distance = (width - 220)/(stoplist.size()/2);
	pos = 30 + stop_distance;
      }
    }
  }
  
  cout<<Replacer< double >("&width;", width).apply
      (Replacer< double >("&height;", height).apply
      (Replacer< string >("<headline/>", headline).apply
      (Replacer< string >("<stops-diagram/>", result.str()).apply(frame_template()))));
  
  return 0;
}
