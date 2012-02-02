#include "processed_input.h"
#include "../expat/expat_justparse_interface.h"
#include "../expat/escape_xml.h"

#include <algorithm>

/**
Relations:
- Will be considered if and only if the key "route" is set to one of the values
"bus", "tram", "light_rail", "subway", or "rail".
- If a "ref" and "network" is set, then exactly the relations with this ref and network
are considered as relations. All other are considered as correspondence data only.
- The key "direction" can be set to both to enforce the treatment of the relation as
two-directional relation. Otherwise, the direction (one- or two-directional) is guessed
as unidiectional unless at least one node member with role "forward" or "backward" is present.
- The keys "to" and "from" are read from the relation and written into the headline.
- The key "color" is read from the relation. If it is not present, a default color is assigned.
- The keys "operates_Mo_Fr", "operates_Sa", "operates_Su", "day_on", and "opening_hours"
affect the assumed operation times. Only "opening_hours" is recommended.

__Members:
- Only members of type "node" are considered, their order matters.
- Members with role "forward" are only added in the forward direction.
- Members with role "backward" are only added in the backward direction. They are only
visible if the relation turns out to be bidirectional.
- Members with any other role are added in both directions if the relation turns out to
be bidirectional.


Nodes:
- Will be considered as stop if the key "highway" is set to "bus_stop" or "tram_stop", or the key
"railway" is set to "tram_stop" or "station" or the key "public_transport" is set to
"stop_position".
- The value of the attribute "name" is rendered as the textual description of the node.
- Additionally, nodes that fulfill the description of a display class are added to appear with
the correspondence stops.


Joining process:
...
*/

using namespace std;

namespace
{
  map< unsigned int, NamedNode > nodes;
  NamedNode nnode;
  unsigned int id;
  
  unsigned int parse_status;
  const unsigned int IN_NODE = 1;
  const unsigned int IN_RELATION = 2;
  bool is_stop = false;
  bool is_route = false;  

  vector< Relation >* relations;
  vector< Relation > correspondences;
  vector< DisplayNode > display_nodes;
  vector< Display_Class >* display_classes;
  string* pivot_ref;
  string* pivot_network;
  Relation relation;
}

const double PI = acos(0)*2;

double spat_distance(const NamedNode& nnode1, const NamedNode& nnode2)
{
  return acos(sin(nnode1.lat/180.0*PI)*sin(nnode2.lat/180.0*PI)
      + sin(nnode1.lon/180.0*PI)*cos(nnode1.lat/180.0*PI)
        *sin(nnode2.lon/180.0*PI)*cos(nnode2.lat/180.0*PI)
      + cos(nnode1.lon/180.0*PI)*cos(nnode1.lat/180.0*PI)
        *cos(nnode2.lon/180.0*PI)*cos(nnode2.lat/180.0*PI))
      /PI*20000000;
}

bool Timespan::operator<(const Timespan& a) const
{
  if (begin < a.begin)
    return true;
  if (begin > a.begin)
    return false;
  return end < a.end;
}
  
bool Timespan::operator==(const Timespan& a) const
{
  return ((begin == a.begin) && (end == a.end));
}
  
string Timespan::hh_mm() const
{
  string result("##:##-##:##");
  int hours(begin/60%24);
  int minutes(begin%60);
  result[0] = hours/10 + 48;
  result[1] = hours%10 + 48;
  result[3] = minutes/10 + 48;
  result[4] = minutes%10 + 48;
  hours = end/60%24;
  minutes = end%60;
  if ((end%(24*60) == 0) && (end != begin))
    hours = 24;
  result[6] = hours/10 + 48;
  result[7] = hours%10 + 48;
  result[9] = minutes/10 + 48;
  result[10] = minutes%10 + 48;
  return result;
}

bool RelationHumanId_Comparator::operator()
    (const RelationHumanId& rhi_1, const RelationHumanId& rhi_2)
{
  unsigned int numval_1(0), numval_2(0);
  
  unsigned int pos(0);
  while ((pos < rhi_1.ref.size()) && (!isdigit(rhi_1.ref[pos])))
    ++pos;
  while ((pos < rhi_1.ref.size()) && (isdigit(rhi_1.ref[pos])))
  {
    numval_1 = numval_1*10 + (rhi_1.ref[pos] - 48);
    ++pos;
  }
  
  pos = 0;
  while ((pos < rhi_2.ref.size()) && (!isdigit(rhi_2.ref[pos])))
    ++pos;
  while ((pos < rhi_2.ref.size()) && (isdigit(rhi_2.ref[pos])))
  {
    numval_2 = numval_2*10 + (rhi_2.ref[pos] - 48);
    ++pos;
  }
  
  if (numval_1 < numval_2)
    return true;
  else if (numval_1 > numval_2)
    return false;
  
  return (rhi_1.ref < rhi_2.ref);
}

unsigned int Stop::used_by_size = 0;

Correspondence_Data::Correspondence_Data(const vector< Relation >& correspondences,
    const map< unsigned int, NamedNode >& nodes_,
    const vector< DisplayNode > display_nodes_,
    double walk_limit_for_changes_)
    : what_calls_here(), nodes(nodes_),
      display_nodes(display_nodes_),
      walk_limit_for_changes(walk_limit_for_changes_)
{
  // create the dictionary bus_stop -> ref
  for (vector< Relation >::const_iterator rit(correspondences.begin());
      rit != correspondences.end(); ++rit)
  {
    for (vector< unsigned int >::const_iterator it(rit->forward_stops.begin());
         it != rit->forward_stops.end(); ++it)
    {
      if ((nodes.find(*it) != nodes.end()) && (nodes.find(*it)->second.lat <= 90.0))
	what_calls_here[*it].insert(RelationHumanId(rit->ref, rit->color));
    }
    for (vector< unsigned int >::const_iterator it(rit->backward_stops.begin());
        it != rit->backward_stops.end(); ++it)
    {
      if ((nodes.find(*it) != nodes.end()) && (nodes.find(*it)->second.lat <= 90.0))
	what_calls_here[*it].insert(RelationHumanId(rit->ref, rit->color));
    }
  }
}

set< RelationHumanId > Correspondence_Data::correspondences_at(const NamedNode& nnode) const
{
  set< RelationHumanId > result;
  
  for (map< unsigned int, set< RelationHumanId > >::const_iterator it(what_calls_here.begin());
      it != what_calls_here.end(); ++it)
  {
    if ((nodes.find(it->first) != nodes.end()) &&
      (spat_distance(nnode, nodes.find(it->first)->second) < walk_limit_for_changes))
    {
      for (set< RelationHumanId >::const_iterator iit(it->second.begin());
          iit != it->second.end();  ++iit)
	result.insert(*iit);
    }
  }
  return result;
}

set< RelationHumanId >& Correspondence_Data::display_nodes_at
    (const NamedNode& nnode, set< RelationHumanId >& result) const
{
  for (vector< DisplayNode >::const_iterator it(display_nodes.begin());
      it != display_nodes.end(); ++it)
  {
    if (spat_distance(nnode, it->node) < it->limit)
      result.insert(RelationHumanId(it->node.name, "#777777"));
  }
  return result;
}

void Stoplist::populate_stop
    (Stop& stop, const NamedNode& nnode, int rel_index, int mode, bool additive,
     const Correspondence_Data& cors_data)
{
  if (nnode.lat <= 90.0)
  {
    stop.name = nnode.name;
    if (additive)
      stop.used_by[rel_index] |= mode;
    else
      stop.used_by[rel_index] = mode;
    
    set< RelationHumanId > cors(cors_data.correspondences_at(nnode));
    cors_data.display_nodes_at(nnode, cors);
    stop.correspondences.insert(cors.begin(), cors.end());
  }
}

void Stoplist::push_back
    (const NamedNode& nnode, int rel_index, int mode,
     const Correspondence_Data& cors_data)
{
  Stop stop;
  populate_stop(stop, nnode, rel_index, mode, false, cors_data);
  stops.push_back(stop);
}

void Stoplist::insert
	(list< Stop >::iterator it, const NamedNode& nnode, int rel_index, int mode,
	 const Correspondence_Data& cors_data)
{
  Stop stop;
  populate_stop(stop, nnode, rel_index, mode, false, cors_data);
  stops.insert(it, stop);
}

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

int process_relation
    (const Relation& rel, const map< unsigned int, NamedNode >& nodes,
     unsigned int rel_count, int direction_const, int forward_backward,
     Stoplist& stoplist, const Correspondence_Data& cors_data)
{
  vector< unsigned int > const* rel_stops;
  if (forward_backward == Relation::FORWARD)
    rel_stops = &(rel.forward_stops);
  else if (forward_backward == Relation::BACKWARD)
    rel_stops = &(rel.backward_stops);
  else
    return direction_const;
  
  multimap< string, unsigned int > stopdict;
  for (unsigned int i(0); i < rel_stops->size(); ++i)
  {
    if ((nodes.find((*rel_stops)[i]) != nodes.end())
	 && (nodes.find((*rel_stops)[i])->second.lat <= 90.0))
      stopdict.insert(make_pair(nodes.find((*rel_stops)[i])->second.name, i+1));
  }
    
  vector< unsigned int > indices_of_present_stops;
  for (list< Stop >::const_iterator it(stoplist.stops.begin());
       it != stoplist.stops.end(); ++it)
  {
    for (multimap< string, unsigned int >::const_iterator
	 iit(stopdict.lower_bound(it->name)); iit != stopdict.upper_bound(it->name); ++iit)
      indices_of_present_stops.push_back(iit->second);
  }
    
  vector< unsigned int > ascending(longest_ascending_subsequence
      (indices_of_present_stops));
  vector< unsigned int > descending(longest_descending_subsequence
      (indices_of_present_stops));
  
  if (ascending.size() > descending.size())
  {
    if (direction_const == 0)
      direction_const = Stop::FORWARD;
    
    vector< unsigned int >::const_iterator sit(ascending.begin());
    unsigned int last_idx(1);
    for (list< Stop >::iterator it(stoplist.stops.begin());
	 it != stoplist.stops.end(); ++it)
    {
      bool sit_found(false);
      for (multimap< string, unsigned int >::const_iterator
	   iit(stopdict.lower_bound(it->name)); iit != stopdict.upper_bound(it->name); ++iit)
	sit_found |= (iit->second == *sit);
      if (sit_found)
      {
	  // insert stops that aren't yet inserted
	while (*sit > last_idx)
	{
	  if (nodes.find((*rel_stops)[last_idx-1]) != nodes.end())
	    stoplist.insert(it, nodes.find((*rel_stops)[last_idx-1])->second,
			    rel_count, direction_const, cors_data);
	  ++last_idx;
	}
	  
	// match the current stop
	if (nodes.find((*rel_stops)[last_idx-1]) != nodes.end())
	  stoplist.populate_stop(*it, nodes.find((*rel_stops)[last_idx-1])->second,
				  rel_count, direction_const, true, cors_data);
	  
	++last_idx;
	  
	if (++sit == ascending.end())
	  break;
      }
    }
      
      // insert stops at the end
    while (last_idx <= rel_stops->size())
    {
      if (nodes.find((*rel_stops)[last_idx-1]) != nodes.end())
	stoplist.push_back(nodes.find((*rel_stops)[last_idx-1])->second,
			   rel_count, direction_const, cors_data);
      ++last_idx;
    }
    
    return Relation::FORWARD;
  }
  else if (descending.size() > 0)
  {
    if (direction_const == 0)
      direction_const = Stop::BACKWARD;
    
    vector< unsigned int >::const_reverse_iterator sit(descending.rbegin());
    unsigned int last_idx(rel_stops->size());
    for (list< Stop >::iterator it(stoplist.stops.begin());
	 it != stoplist.stops.end(); ++it)
    {
      bool sit_found(false);
      for (multimap< string, unsigned int >::const_iterator
	   iit(stopdict.lower_bound(it->name)); iit != stopdict.upper_bound(it->name); ++iit)
	sit_found |= (iit->second == *sit);
      if (sit_found)
      {
	// insert stops that aren't yet inserted
	while (*sit < last_idx)
	{
	  if (nodes.find((*rel_stops)[last_idx-1]) != nodes.end())
	    stoplist.insert(it, nodes.find((*rel_stops)[last_idx-1])->second,
			    rel_count, direction_const, cors_data);
	  --last_idx;
	}
	  
	// match the current stop
	if (nodes.find((*rel_stops)[last_idx-1]) != nodes.end())
	  stoplist.populate_stop(*it, nodes.find((*rel_stops)[last_idx-1])->second,
				rel_count, direction_const, true, cors_data);

	--last_idx;
	
	if (++sit == descending.rend())
	  break;
      }
    }
      
    // insert stops at the end
    while (last_idx > 0)
    {
      if (nodes.find((*rel_stops)[last_idx-1]) != nodes.end())
	stoplist.push_back(nodes.find((*rel_stops)[last_idx-1])->second,
			     rel_count, direction_const, cors_data);
      --last_idx;
    }
    
    return Relation::BACKWARD;
  }
  
  return direction_const;
}

bool have_uniform_operation_times(const Relation& a, const Relation& b)
{
  if (a.opening_hours.size() != b.opening_hours.size())
    return false;
  for(unsigned int i(0); i < a.opening_hours.size(); ++i)
  {
    if (!(a.opening_hours[i] == b.opening_hours[i]))
      return false;
  }
  return true;
}

bool have_intersecting_operation_times(const Relation& a, const Relation& b)
{
  vector< Timespan >::const_iterator ita(a.opening_hours.begin());
  vector< Timespan >::const_iterator itb(b.opening_hours.begin());
  
  while ((ita != a.opening_hours.end()) && (itb != b.opening_hours.end()))
  {
    if (ita->begin < itb->begin)
    {
      if (ita->end > itb->begin)
	return true;
      ++ita;
    }
    else
    {
      if (itb->end > ita->begin)
	return true;
      ++itb;
    }
  }
  return false;
}

void parse_timespans(vector< Timespan >& timespans, string data)
{
  static map< string, unsigned int > weekdays;
  if (weekdays.empty())
  {
    weekdays["Mo"] = 0;
    weekdays["Tu"] = 1;
    weekdays["We"] = 2;
    weekdays["Th"] = 3;
    weekdays["Fr"] = 4;
    weekdays["Sa"] = 5;
    weekdays["Su"] = 6;
    weekdays[""] = 8;
  }
  
  string::size_type pos(0);
  while ((pos < data.size()) && isspace(data[pos]))
    ++pos;
  
  string begin_wd(data.substr(pos, 2)), end_wd;
  
  while ((pos < data.size()) && isspace(data[pos]))
    ++pos;
  if (!((pos < data.size()) && isalpha(data[pos])))
    return;
  pos += 2;
  if ((pos < data.size()) && (data[pos] == '-'))
  {
    ++pos;
    end_wd = data.substr(pos, 2);
    pos += 2;
  }
  
  if (weekdays.find(begin_wd) == weekdays.end())
    return;
  if (weekdays.find(end_wd) == weekdays.end())
    return;
  unsigned int begin_day(weekdays[begin_wd]), end_day(weekdays[end_wd]);
  if (end_wd == "")
    end_day = begin_day;
  if (begin_day > end_day)
    return;
  
  while ((pos < data.size()) && isspace(data[pos]))
    ++pos;
  while ((pos < data.size()) && isdigit(data[pos]))
  {
    unsigned int start_hour(0), start_minute(0), end_hour(0), end_minute(0);
    while ((pos < data.size()) && isdigit(data[pos]))
    {
      start_hour = start_hour*10 + (data[pos] - 48);
      ++pos;
    }
    ++pos;
    while ((pos < data.size()) && isdigit(data[pos]))
    {
      start_minute = start_minute*10 + (data[pos] - 48);
      ++pos;
    }
    while ((pos < data.size()) && !isdigit(data[pos]))
      ++pos;
    while ((pos < data.size()) && isdigit(data[pos]))
    {
      end_hour = end_hour*10 + (data[pos] - 48);
      ++pos;
    }
    ++pos;
    while ((pos < data.size()) && isdigit(data[pos]))
    {
      end_minute = end_minute*10 + (data[pos] - 48);
      ++pos;
    }
    while ((pos < data.size()) && !isdigit(data[pos]))
      ++pos;
    
    unsigned int start_time(start_hour*60 + start_minute);
    unsigned int end_time(end_hour*60 + end_minute);
    if (end_time > 60*24)
      continue;
    unsigned int end_day_offset = 0;
    if (start_time >= end_time)
      end_day_offset = 1;
    
    for (unsigned int day(begin_day); day <= end_day; ++day)
    {
      timespans.push_back(Timespan
	  (day, start_hour, start_minute, day + end_day_offset, end_hour, end_minute));
    }
  }
}

void cleanup_opening_hours(Relation& rel)
{
  sort(rel.opening_hours.begin(), rel.opening_hours.end());
  
  // join overlapping timespans
  unsigned int last_valid(0);
  for (unsigned int i(1); i < rel.opening_hours.size(); ++i)
  {
    if (rel.opening_hours[i].begin > rel.opening_hours[last_valid].end)
    {
      last_valid = i;
      continue;
    }
    if (rel.opening_hours[i].end > rel.opening_hours[last_valid].end)
    {
      rel.opening_hours[last_valid].end = rel.opening_hours[i].end;
      rel.opening_hours[i].begin = 60*24*7;
    }
  }
  sort(rel.opening_hours.begin(), rel.opening_hours.end());
  while ((!rel.opening_hours.empty()) && (rel.opening_hours.back().begin >= 60*24*7))
    rel.opening_hours.pop_back();
}

const vector< unsigned int >& default_colors()
{
  static vector< unsigned int > colors;
  if (colors.empty())
  {
    colors.push_back(0x0000ff);
    colors.push_back(0x00ffff);
    colors.push_back(0x007777);
    colors.push_back(0x00ff00);
    colors.push_back(0xffff00);
    colors.push_back(0x777700);
    colors.push_back(0x777777);
    colors.push_back(0xff0000);
    colors.push_back(0xff00ff);
    colors.push_back(0x770077);
  }
  return colors;
}

char hex(int i)
{
  if (i < 10)
    return (i + '0');
  else
    return (i + 'a' - 10);
}

string default_color(string ref)
{
  unsigned int numval(0), pos(0), color(0);
  while ((pos < ref.size()) && (!isdigit(ref[pos])))
    ++pos;
  while ((pos < ref.size()) && (isdigit(ref[pos])))
  {
    numval = numval*10 + (ref[pos] - 48);
    ++pos;
  }
  
  if (numval == 0)
  {
    for (unsigned int i(0); i < ref.size(); ++i)
      numval += i*(unsigned char)ref[i];
    numval = numval % 1000;
  }
  
  if (numval < 10)
    color = default_colors()[numval];
  else if (numval < 100)
  {
    unsigned int color1 = default_colors()[numval % 10];
    unsigned int color2 = default_colors()[numval/10];
    color = ((((color1 & 0x0000ff)*3 + (color2 & 0x0000ff))/4) & 0x0000ff)
        | ((((color1 & 0x00ff00)*3 + (color2 & 0x00ff00))/4) & 0x00ff00)
	| ((((color1 & 0xff0000)*3 + (color2 & 0xff0000))/4) & 0xff0000);
  }
  else
  {
    unsigned int color1 = default_colors()[numval % 10];
    unsigned int color2 = default_colors()[numval/10%10];
    unsigned int color3 = default_colors()[numval/100%10];
    color = ((((color1 & 0x0000ff)*10 + (color2 & 0x0000ff)*5 + (color3 & 0x0000ff))
	  /16) & 0x0000ff)
        | ((((color1 & 0x00ff00)*10 + (color2 & 0x00ff00)*5 + (color3 & 0x00ff00))
	  /16) & 0x00ff00)
	| ((((color1 & 0xff0000)*10 + (color2 & 0xff0000)*5 + (color3 & 0xff0000))
	  /16) & 0xff0000);
  }
  
  string result("#......");
  result[1] = hex((color & 0xf00000)/0x100000);
  result[2] = hex((color & 0x0f0000)/0x10000);
  result[3] = hex((color & 0x00f000)/0x1000);
  result[4] = hex((color & 0x000f00)/0x100);
  result[5] = hex((color & 0x0000f0)/0x10);
  result[6] = hex(color & 0x00000f);
  return result;
};

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
    if ((key == "network") && (parse_status == IN_RELATION))
      relation.network = escape_xml(value);
    if ((key == "to") && (parse_status == IN_RELATION))
      relation.to = escape_xml(value);
    if ((key == "from") && (parse_status == IN_RELATION))
      relation.from = escape_xml(value);
    if ((key == "color") && (parse_status == IN_RELATION))
      relation.color = escape_xml(value);
    if ((key == "direction") && (value == "both"))
      relation.direction = Relation::BOTH;
    if ((key == "highway") && (value == "bus_stop"))
      is_stop = true;
    if ((key == "highway") && (value == "tram_stop"))
      is_stop = true;
    if ((key == "railway") && (value == "tram_stop"))
      is_stop = true;
    if ((key == "railway") && (value == "station"))
      is_stop = true;
    if ((key == "public_transport") && (value == "stop_position"))
      is_stop = true;
    if ((key == "amenity") && (value == "ferry_terminal"))
      is_stop = true;
    if ((key == "route") && (value == "bus"))
      is_route = true;
    if ((key == "route") && (value == "trolleybus"))
      is_route = true;
    if ((key == "route") && (value == "tram"))
      is_route = true;
    if ((key == "route") && (value == "light_rail"))
      is_route = true;
    if ((key == "route") && (value == "subway"))
      is_route = true;
    if ((key == "route") && (value == "rail"))
      is_route = true;
    if ((key == "route") && (value == "train"))
      is_route = true;
    if ((key == "route") && (value == "ferry"))
      is_route = true;
    if (key == "operates_Mo_Fr")
    {
      for (unsigned int i(0); i < 5; ++i)
	relation.opening_hours.push_back(Timespan
	    (i, atoi(value.substr(0, 2).c_str()), atoi(value.substr(2, 2).c_str()),
	     i, atoi(value.substr(5, 2).c_str()), atoi(value.substr(7, 2).c_str())));
    }
    if (key == "operates_Sa")
    {
      relation.opening_hours.push_back(Timespan
	(5, atoi(value.substr(0, 2).c_str()), atoi(value.substr(2, 2).c_str()),
	 5, atoi(value.substr(5, 2).c_str()), atoi(value.substr(7, 2).c_str())));
    }
    if (key == "operates_Su")
    {
      relation.opening_hours.push_back(Timespan
	(6, atoi(value.substr(0, 2).c_str()), atoi(value.substr(2, 2).c_str()),
	 6, atoi(value.substr(5, 2).c_str()), atoi(value.substr(7, 2).c_str())));
    }
    if (key == "day_on")
    {
      if ((value == "Mo-Fr") || (value == "Mo-Sa") || (value == "Mo-Su"))
	relation.opening_hours.push_back(Timespan(0, 0, 0, 4, 24, 0));
      if ((value == "Sa") || (value == "Mo-Sa") || (value == "Sa-Su")
	|| (value == "Mo-Su"))
	relation.opening_hours.push_back(Timespan(5, 0, 0, 5, 24, 0));
      if ((value == "Su") || (value == "Sa-Su") || (value == "Mo-Su"))
	relation.opening_hours.push_back(Timespan(6, 0, 0, 6, 24, 0));
    }
    if (key == "opening_hours")
    {
      string::size_type pos(0), colon_found(value.find(";"));
      while (colon_found != string::npos)
      {
	parse_timespans(relation.opening_hours, value.substr(pos, colon_found - pos));
	pos = colon_found + 1;
	colon_found = value.find(";", pos);
      }
      parse_timespans(relation.opening_hours, value.substr(pos));
    }
    for (vector< Display_Class >::const_iterator it(display_classes->begin());
	it != display_classes->end(); ++it)
    {
      if ((key == it->key) && ((it->value == "") || (value == it->value)))
      {
	DisplayNode dnode;
	dnode.node = nnode;
	dnode.node.name = it->text;
	dnode.limit = it->limit;
	display_nodes.push_back(dnode);
      }
    }
  }
  else if (!strcmp(el, "node"))
  {
    parse_status = IN_NODE;
    is_stop = false;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "id"))
	id = atol(attr[i+1]);
      else if (!strcmp(attr[i], "lat"))
	nnode.lat = atof(attr[i+1]);
      else if (!strcmp(attr[i], "lon"))
	nnode.lon = atof(attr[i+1]);
    }
    nnode.name = "";
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
    is_route = false;
    relation = Relation();
  }
}

void end(const char *el)
{
  if (!strcmp(el, "node"))
  {
    if (is_stop)
      nodes[id] = nnode;
    parse_status = 0;
  }
  else if (!strcmp(el, "relation"))
  {
    parse_status = 0;
    if (is_route)
    {
      if (relation.color == "")
	relation.color = default_color(relation.ref);
      cleanup_opening_hours(relation);
      if (((*pivot_ref == "") || (relation.ref == *pivot_ref))
	&& ((*pivot_network == "") || (relation.network == *pivot_network)))
	relations->push_back(relation);
      else
	correspondences.push_back(relation);
    }
  }
}

Stoplist make_stoplist(double walk_limit_for_changes, bool doubleread_rel,
		       vector< Display_Class >& display_classes_,
		       string pivot_ref_,
		       string pivot_network_,
		       vector< Relation >& relations_,
		       bool& have_valid_operation_times,
		       int debug_level)
{
  relations = &relations_;
  display_classes = &display_classes_;
  pivot_ref = &pivot_ref_;
  pivot_network = &pivot_network_;
  
  // read the XML input
  parse_status = 0;
  parse(stdin, start, end);
  
  // bailout if no relation is found
  if (relations->size() == 0)
    return Stoplist();
  
  // check whether the relations have uniform operation times
  have_valid_operation_times = !relations_[0].opening_hours.empty();
  for (unsigned int i(1); i < relations->size(); ++i)
  {
    if (!(have_uniform_operation_times(relations_[0], relations_[i])))
    {
      have_valid_operation_times = false;
      break;
    }
  }
  if ((have_valid_operation_times) &&
    (!(relations_[0].opening_hours.empty())))
  {
    for (int i(0); i < (int)correspondences.size(); ++i)
    {
      if (correspondences[i].opening_hours.empty())
	continue;
      if (!(have_intersecting_operation_times(relations_[0], correspondences[i])))
      {
	if (i < (int)correspondences.size()-1)
	  correspondences[i] = correspondences[correspondences.size()-1];
	--i;
	correspondences.resize(correspondences.size()-1);
      }
    }
  }
  
  // initialise complex data structures
  Correspondence_Data cors_data
      (correspondences, nodes, display_nodes, walk_limit_for_changes);
  Stoplist stoplist;
  Stop::used_by_size = relations->size();
  
  // make a common stoplist from all relations
  // integrate all relations (their forward direction) into the stoplist
  Relation relation = relations->front();
  for(vector< unsigned int >::const_iterator it(relation.forward_stops.begin());
      it != relation.forward_stops.end(); ++it)
  {
    stoplist.push_back(nodes[*it], 0, Stop::FORWARD, cors_data);
  }
  relations->begin()->direction |= Relation::FORWARD;
  
  vector< Relation >::iterator rit(relations->begin());
  unsigned int rel_count(1);
  ++rit;
  while (rit != relations->end())
  {
    rit->direction |= process_relation
	(*rit, nodes, rel_count, 0, Relation::FORWARD, stoplist, cors_data);
    
    ++rit;
    ++rel_count;
  }
  
  // integrate second direction (where it exists) into stoplist
  rit = relations->begin();
  rel_count = 0;
  while (rit != relations->end())
  {
    if (((rit->direction & Relation::BOTH) == 0) && (!doubleread_rel))
    {
      ++rit;
      ++rel_count;
      continue;
    }
    
    int direction_const(0);
    if ((rit->direction & Relation::FORWARD) != 0)
      direction_const = Stop::BACKWARD;
    if ((rit->direction & Relation::BACKWARD) != 0)
      direction_const = Stop::FORWARD;
    
    process_relation(*rit, nodes, rel_count, direction_const, Relation::BACKWARD, stoplist, cors_data);
    
    ++rit;
    ++rel_count;
  }
  
  // unify one-directional relations as good as possible
  multimap< double, pair< int, int > > possible_pairs;
  if (debug_level < 10)
  {
    for (unsigned int i(0); i < relations->size(); ++i)
    {
      if (relations_[i].direction != Relation::FORWARD)
	continue;
      for (unsigned int j(0); j < relations->size(); ++j)
      {
	int common(0), total(0);
	if (relations_[j].direction != Relation::BACKWARD)
	  continue;
	for (list< Stop >::const_iterator it(stoplist.stops.begin());
	    it != stoplist.stops.end(); ++it)
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
  }
    
  for (multimap< double, pair< int, int > >::const_reverse_iterator
       it(possible_pairs.rbegin()); it != possible_pairs.rend(); ++it)
  {
    if (relations_[it->second.second].direction != Relation::BACKWARD)
      continue;
    for (list< Stop >::iterator sit(stoplist.stops.begin());
        sit != stoplist.stops.end(); ++sit)
      sit->used_by[it->second.first] |= sit->used_by[it->second.second];
    relations_[it->second.second].direction = 0;
  }
  
  return stoplist;
}
