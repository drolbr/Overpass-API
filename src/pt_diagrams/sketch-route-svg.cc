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
#include "processed_input.h"

using namespace std;

string frame_template()
{
  return "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
    "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
    "     xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
    "     version=\"1.1\" baseProfile=\"full\"\n"
    "     width=\"$width;px\" height=\"$height;px\">\n"
    "\n"
    "<headline/>\n"
    "\n"
    "<stops-diagram/>\n"
    "\n"
    "</svg>\n";
}

string from_to_headline_template()
{
  return "<title>Line $rel_ref; $tr_from; $rel_from; $tr_to; $rel_to;</title>\n"
      "<desc>Line $rel_ref; $tr_from; $rel_from; $tr_to; $rel_to;</desc>\n"
      "\n"
      "<text x=\"30\" y=\"30\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"none\">$rel_ref;</tspan>\n"
      "  <tspan font-size=\"16px\" dx=\"20\">$tr_from;</tspan>\n"
      "  $from_tail;</text>\n"
      "<text x=\"30\" y=\"60\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"none\">$rel_ref;</tspan>\n"
      "  <tspan font-size=\"16px\" dx=\"20\">$tr_to;</tspan>\n"
      "  $to_tail;</text>\n"
      "<text x=\"30\" y=\"45\" font-family=\"Liberation Sans, sans-serif\" font-size=\"32px\" fill=\"$rel_color;\">$rel_ref;</text>\n"
      "\n";
}

string alternate_headline_extension_template()
{
  return "  $head;\n"
      "  <tspan font-size=\"16px\" dx=\"20\">$tr_or;</tspan>\n"
      "  $tail;";
}

string destination_headline_template()
{
  return "<tspan font-size=\"24px\" dx=\"20\">$content;</tspan>";
}

string operation_time_template()
{
  return "<text x=\"30\" y=\"90\" font-family=\"Liberation Sans, sans-serif\""
      " font-size=\"16px\">$tr_operates;: $timespans;</text>\n";
}

string only_to_headline_template()
{
  return "<title>Line $rel_ref; $tr_to; $rel_to;</title>\n"
      "<desc>Line $rel_ref; $tr_to; $rel_to;</desc>\n"
      "\n"
      "<text x=\"30\" y=\"60\" font-family=\"Liberation Sans, sans-serif\"><tspan font-size=\"32px\" fill=\"$rel_color;\">$rel_ref;</tspan>\n"
      "  <tspan font-size=\"16px\" dx=\"20\">$tr_to;</tspan>\n"
      "  <tspan font-size=\"24px\">$rel_to;</tspan></text>\n"
      "\n";
}

string line_template()
{
  return "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"7px\""
      " points=\"$hmin; $vpos;, $hmax; $vpos;\"/>\n";
}

string left_join_template()
{
  return "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"7px\""
  " points=\"$htop; $vpos_join;, $hmin; $vpos_self;, $hmax; $vpos_self;\"/>\n";
}

string right_join_template()
{
  return "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"7px\""
  " points=\"$hmin; $vpos_self;, $hmax; $vpos_self;, $htop; $vpos_join;\"/>\n";
}

string stop_name_template()
{
  return "<text x=\"0\" y=\"-10\" transform=\"translate($hpos;,$vpos;) rotate(-45,0,-10)\""
      " font-family=\"Liberation Sans, sans-serif\" font-size=\"$stop_fontsize;px\">$stopname;</text>\n"
      "\n";
}

string terminus_name_template()
{
  return "<text x=\"0\" y=\"-10\" transform=\"translate($hpos;,$vpos;) rotate(-45,0,-10)\""
      " font-family=\"Liberation Sans, sans-serif\" font-size=\"$stop_fontsize;px\""
      " font-weight=\"bold\">$stopname;</text>\n"
      "\n";
}

string stop_background_template()
{
  return "<polyline fill=\"none\" stroke=\"gray\" stroke-width=\"1px\""
  " points=\"$hpos; $vpos_upper;, $hpos; $vpos_lower;\"/>\n";
}

string bidirectional_stop_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"7\" fill=\"$rel_color;\"/>\n"
      "\n";
}

string forward_stop_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 0 14,0 Z\" style=\"fill:$rel_color;\""
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"0 -8, 6 -8\"" 
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<path d=\"M 4,-4 l 0,-8 l 6,4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
      "\n";
}

string backward_stop_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 1 14,0 Z\" style=\"fill:$rel_color;\""
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"-6 8, 0 8\"" 
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<path d=\"M -4,4 l 0,8 l -6,-4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
      "\n";
}

string bidirectional_terminus_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"9\" fill=\"$rel_color;\"/>\n"
      "\n";
}

string forward_terminus_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"9\" fill=\"$rel_color;\"/>\n"
      "\n"
      /*"<path d=\"M -9,0 a 9 9 0 0 0 18,0 Z\" style=\"fill:$rel_color;\""
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"2 -8, 8 -8\"" 
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<path d=\"M 6,-4 l 0,-8 l 6,4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
      "\n"*/;
}

string backward_terminus_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"9\" fill=\"$rel_color;\"/>\n"
      "\n"
      /*"<path d=\"M -9,0 a 9 9 0 0 1 18,0 Z\" style=\"fill:$rel_color;\""
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"-8 8, -2 8\"" 
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<path d=\"M -6,4 l 0,8 l -6,-4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
      "\n"*/;
}

string bidirectional_hub_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"7\" fill=\"white\" stroke=\"$rel_color;\" stroke-width=\"2px\"/>\n"
  "\n";
}

string forward_hub_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 0 14,0 Z\" style=\"fill:white; stroke:$rel_color;; stroke-width:2px;\""
  " transform=\"translate($hpos;,$vpos;)\"/>\n"
  "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"0 -8, 6 -8\"" 
  " transform=\"translate($hpos;,$vpos;)\"/>\n"
  "<path d=\"M 4,-4 l 0,-8 l 6,4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
  "\n";
}

string backward_hub_template()
{
  return "<path d=\"M -7,0 a 7 7 0 0 1 14,0 Z\" style=\"fill:white; stroke:$rel_color;; stroke-width:2px;\""
  " transform=\"translate($hpos;,$vpos;)\"/>\n"
  "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"-6 8, 0 8\"" 
  " transform=\"translate($hpos;,$vpos;)\"/>\n"
  "<path d=\"M -4,4 l 0,8 l -6,-4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
  "\n";
}

string bidirectional_terminus_hub_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"9\" fill=\"white\" stroke=\"$rel_color;\" stroke-width=\"2px\"/>\n"
  "\n";
}

string forward_terminus_hub_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"9\" fill=\"white\" stroke=\"$rel_color;\" stroke-width=\"2px\"/>\n"
      "\n"
      /*"<path d=\"M -9,0 a 9 9 0 0 0 18,0 Z\" style=\"fill:white; stroke:$rel_color;; stroke-width:2px;\""
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"2 -8, 8 -8\"" 
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<path d=\"M 6,-4 l 0,-8 l 6,4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
      "\n"*/;
}

string backward_terminus_hub_template()
{
  return "<circle cx=\"$hpos;\" cy=\"$vpos;\" r=\"9\" fill=\"white\" stroke=\"$rel_color;\" stroke-width=\"2px\"/>\n"
      "\n"
      /*"<path d=\"M -9,0 a 9 9 0 0 1 18,0 Z\" style=\"fill:white; stroke:$rel_color;; stroke-width:2px;\""
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<polyline fill=\"none\" stroke=\"$rel_color;\" stroke-width=\"2px\" points=\"-8 8, -2 8\"" 
      " transform=\"translate($hpos;,$vpos;)\"/>\n"
      "<path d=\"M -6,4 l 0,8 l -6,-4 Z\" style=\"fill:$rel_color;\" transform=\"translate($hpos;,$vpos;)\"/>\n"
      "\n"*/;
}

string correspondence_row_template()
{
  return "<text x=\"0\" y=\"-10\" transform=\"translate($hpos;,$vpos;) rotate(-45,0,-10)\""
  " font-family=\"Liberation Sans, sans-serif\" font-size=\"$stop_fontsize;px\">\n$data;</text>\n"
  "\n";
}

string correspondence_item_template()
{
  return "  <tspan dx=\"$offset;\" fill=\"$color;\">$ref;</tspan>\n";
}

string correspondence_below_template()
{
  return "<a xlink:href=\"/api/sketch-line?ref=$ref;&amp;network=$network;&amp;correspondences=300\"><text x=\"0\" y=\"0\" transform=\"translate($hpos;,$vpos;)\""
  " font-family=\"Liberation Sans, sans-serif\" font-size=\"$stop_fontsize;px\""
  " text-anchor=\"middle\" fill=\"$color;\">$ref;</text></a>\n"
  "\n";
}

string tr_weekday(unsigned int wd)
{
  static vector< string > tr_weekdays;
  if (tr_weekdays.empty())
  {
    tr_weekdays.push_back("$tr_monday_short;");
    tr_weekdays.push_back("$tr_tuesday_short;");
    tr_weekdays.push_back("$tr_wednesday_short;");
    tr_weekdays.push_back("$tr_thursday_short;");
    tr_weekdays.push_back("$tr_friday_short;");
    tr_weekdays.push_back("$tr_saturday_short;");
    tr_weekdays.push_back("$tr_sunday_short;");
  }
  if (wd < 7)
    return tr_weekdays[wd];
  return "";
}

map< string, string > default_translations()
{
  map< string, string > translations;
  
  translations["$tr_from;"] = "from";
  translations["$tr_to;"] = "to";
  translations["$tr_or;"] = "or";
  translations["$tr_and;"] = "and";
  
  translations["$tr_monday_short;"] = "monday";
  translations["$tr_tuesday_short;"] = "tuesday";
  translations["$tr_wednesday_short;"] = "wednesday";
  translations["$tr_thursday_short;"] = "thursday";
  translations["$tr_friday_short;"] = "friday";
  translations["$tr_saturday_short;"] = "saturday";
  translations["$tr_sunday_short;"] = "sunday";
  
  translations["$tr_operates;"] = "operates";
  
  return translations;
}

//-----------------------------------------------------------------------------

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

string two_digit(int number)
{
  string result("00");
  result[0] = (number / 10) + 48;
  result[1] = (number % 10) + 48;
  return result;
}

string multi_replace(map< string, string > replace, string data)
{
  for (map< string, string >::const_iterator it(replace.begin()); it != replace.end(); ++it)
  {
    if (it->first == "")
      continue;
    ostringstream result("");
    string::size_type pos(0), found(data.find(it->first, 0));
    while (found != string::npos)
    {
      result<<data.substr(pos, found - pos);
      result<<it->second;
      pos = found + it->first.length();
      found = data.find(it->first, pos);
    }
    result<<data.substr(pos);
    
    data = result.str();
  }
  return data;
}

//-----------------------------------------------------------------------------

vector< Display_Class > display_classes;
string pivot_ref;
string pivot_network;

double width(700);
double height(495);
double stop_font_size(0);
int force_rows(0);
double walk_limit_for_changes(300);
unsigned int max_correspondences_per_line(6);
unsigned int max_correspondences_below(0);
map< string, string > name_shortcuts;
map< string, string > translations;
bool doubleread_rel;
int debug_level = 0;

void options_start(const char *el, const char **attr)
{
  if (!strcmp(el, "reduce"))
  {
    string expr, to;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "expr"))
	expr = attr[i+1];
      else if (!strcmp(attr[i], "to"))
	to = attr[i+1];
    }
    name_shortcuts[expr] = to;
  }
  else if (!strcmp(el, "translate"))
  {
    string expr, to;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "expr"))
	expr = attr[i+1];
      else if (!strcmp(attr[i], "to"))
	to = attr[i+1];
    }
    translations[((string)"$tr_") + expr + ";"] = to;
  }
  else if (!strcmp(el, "width"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "px"))
	width = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "height"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "px"))
	height = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "stop-font-size"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "px"))
	stop_font_size = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "force"))
  {
    string expr, to;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "rows"))
	force_rows = atoi(attr[i+1]);
    }
  }
  else if (!strcmp(el, "correspondences"))
  {
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "limit"))
	walk_limit_for_changes = atof(attr[i+1]);
    }
  }
  else if (!strcmp(el, "max-correspondences"))
  {
    string expr, to;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "per-line"))
	max_correspondences_per_line = atoi(attr[i+1]);
      if (!strcmp(attr[i], "below"))
	max_correspondences_below = atoi(attr[i+1]);
    }
  }
  else if (!strcmp(el, "display"))
  {
    Display_Class dc;
    for (unsigned int i(0); attr[i]; i += 2)
    {
      if (!strcmp(attr[i], "k"))
	dc.key = attr[i+1];
      else if (!strcmp(attr[i], "v"))
	dc.value = attr[i+1];
      else if (!strcmp(attr[i], "text"))
	dc.text = attr[i+1];
      else if (!strcmp(attr[i], "limit"))
	dc.limit = atof(attr[i+1]);
    }
    display_classes.push_back(dc);
  }
}

void options_end(const char *el)
{
}

int main(int argc, char *argv[])
{
  translations = default_translations();
  pivot_ref = "";
  walk_limit_for_changes = 300;
  max_correspondences_per_line = 6;
  
  doubleread_rel = false;
  int argi(1);
  // check on early run for options only
  while (argi < argc)
  {
    if (!strncmp("--options=", argv[argi], 10))
    {
      FILE* options_file(fopen(((string)(argv[argi])).substr(10).c_str(), "r"));
      if (!options_file)
      {
	cout<<Replacer< double >("$width;", width).apply
	(Replacer< double >("$height;", height).apply
	(Replacer< string >("<headline/>", "").apply
	(Replacer< string >("<stops-diagram/>", "<text x=\"100\" y=\"100\">"
	"Can't open options file. Try running without \"--options\".</text>").apply
	(frame_template()))));
	return 0;
      }
      parse(options_file, options_start, options_end);
    }
    ++argi;
  }
  // get every other argument
  argi = 1;
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
    else if (!strncmp("--ref=", argv[argi], 6))
      pivot_ref = ((string)(argv[argi])).substr(6);
    else if (!strncmp("--network=", argv[argi], 10))
      pivot_network = ((string)(argv[argi])).substr(10);
    else if (!strncmp("--walk-limit=", argv[argi], 13))
      walk_limit_for_changes = atof(((string)(argv[argi])).substr(13).c_str());
    else if (!strncmp("--max-correspondences-per-line=", argv[argi], 31))
      max_correspondences_per_line = atoi(((string)(argv[argi])).substr(31).c_str());
    else if (!strncmp("--max-correspondences-below=", argv[argi], 28))
      max_correspondences_below = atoi(((string)(argv[argi])).substr(28).c_str());
    else if (!strcmp("--backspace", argv[argi]))
      doubleread_rel = true;
    else if (!strcmp("--backtime", argv[argi]))
      doubleread_rel = true;
    else if (!strncmp("--debug=", argv[argi], 8))
      debug_level = atoi(((string)(argv[argi])).substr(8).c_str());
    ++argi;
  }
  
  // Default rather to a unattractive value than crash
  if (max_correspondences_per_line <= 0)
    max_correspondences_per_line = 1;
  
  bool have_valid_operation_times = false;
  vector< Relation > relations;
  Stoplist stoplist = make_stoplist(walk_limit_for_changes, doubleread_rel,
				    display_classes, pivot_ref, pivot_network,
				    relations, have_valid_operation_times, debug_level);

  // bailout if no relation is found
  if (relations.size() == 0)
  {
    cout<<Replacer< double >("$width;", 700).apply
    (Replacer< double >("$height;", height).apply
    (Replacer< string >("<headline/>", "").apply
    (Replacer< string >("<stops-diagram/>", "<text x=\"100\" y=\"100\">No relation found</text>").apply(frame_template()))));
    return 0;
  }
  
  Relation relation = relations.front();
  
  //desactivated, for debugging purposes only
/*  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<relations[i].direction<<' ';
  cerr<<'\n';
  for (list< Stop >::const_iterator it(stoplist.stops.begin());
  it != stoplist.stops.end(); ++it)
  {
    for (unsigned int i(0); i < relations.size(); ++i)
      cerr<<it->used_by[i]<<' ';
    cerr<<it->name<<'\n';
  }
  cerr<<'\n';*/
  
  // trim the horizontal itinerary lines
  vector< int > first_stop_idx(relations.size());
  vector< int > last_stop_idx(relations.size());
  if (debug_level < 2)
  {
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      if (relations[i].direction == 0)
        continue;
    
      first_stop_idx[i] = 0;
      last_stop_idx[i] = stoplist.stops.size()-1;
      for (list< Stop >::const_iterator it(stoplist.stops.begin());
          it != stoplist.stops.end(); ++it)
      {
	if (it->used_by[i] != 0)
	  break;
	++first_stop_idx[i];
      }
      for (list< Stop >::const_reverse_iterator it(stoplist.stops.rbegin());
          it != stoplist.stops.rend(); ++it)
      {
	if (it->used_by[i] != 0)
	  break;
	--last_stop_idx[i];
      }
    }
  }
  else
  {
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      if (relations[i].direction == 0)
	continue;
      
      first_stop_idx[i] = 0;
      last_stop_idx[i] = stoplist.stops.size()-1;
    }
  }
    
  //desactivated, for debugging purposes only
/*  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<relations[i].direction<<' ';
  cerr<<'\n';
  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<first_stop_idx[i]<<' ';
  cerr<<'\n';
  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<last_stop_idx[i]<<' ';
  cerr<<'\n';
  for (list< Stop >::const_iterator it(stoplist.stops.begin());
  it != stoplist.stops.end(); ++it)
  {
    for (unsigned int i(0); i < relations.size(); ++i)
      cerr<<it->used_by[i]<<' ';
    cerr<<it->name<<'\n';
  }
  cerr<<'\n';*/
  
  // unify itinerary lines with similar begin
  vector< int > left_join_to(relations.size(), -1);
  vector< int > right_join_to(relations.size(), -1);
  if (debug_level < 1)
  {
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      for (unsigned int j(0); j < i; ++j)
      {
	int ijsimilar(0);
	for (list< Stop >::const_iterator it(stoplist.stops.begin());
	    it != stoplist.stops.end(); ++it)
	{
	  if (it->used_by[i] != it->used_by[j])
	    break;
	  if (ijsimilar > last_stop_idx[j])
	    break;
	  ++ijsimilar;
	}
	if (ijsimilar > last_stop_idx[i])
	  relations[i].direction = 0;
	else if (ijsimilar > first_stop_idx[i])
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
	int ijsimilar(stoplist.stops.size()-1);
	for (list< Stop >::const_reverse_iterator it(stoplist.stops.rbegin());
	    it != stoplist.stops.rend(); ++it)
	{
	  if (it->used_by[i] != it->used_by[j])
	    break;
	  if (ijsimilar < first_stop_idx[j])
	    break;
	  --ijsimilar;
	}
	if (ijsimilar < first_stop_idx[i])
	  relations[i].direction = 0;
	else if (ijsimilar < last_stop_idx[i])
	{
	  last_stop_idx[i] = ijsimilar;
	  right_join_to[i] = j;
	}
      }
    }
  }
  
  //desactivated, for debugging purposes only
/*  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<relations[i].direction<<' ';
  cerr<<'\n';
  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<first_stop_idx[i]<<' ';
  cerr<<'\n';
  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<last_stop_idx[i]<<' ';
  cerr<<'\n';
  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<left_join_to[i]<<' ';
  cerr<<'\n';
  for (unsigned int i(0); i < relations.size(); ++i)
    cerr<<right_join_to[i]<<' ';
  cerr<<'\n';
  for (list< Stop >::const_iterator it(stoplist.stops.begin());
  it != stoplist.stops.end(); ++it)
  {
    for (unsigned int i(0); i < relations.size(); ++i)
      cerr<<it->used_by[i]<<' ';
    cerr<<it->name<<'\n';
  }
  cerr<<'\n';*/
  
  string from_buffer, to_buffer, headline, last_from, last_to;
  headline = from_to_headline_template();
  from_buffer = "$tail;";
  to_buffer = "$tail;";
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    if (relations[i].direction == 0)
      continue;
    if (last_from == "")
      last_from = relations[i].from;
    else if (last_from != relations[i].from)
    {
      from_buffer = Replacer< string >
	  ("$tail;", Replacer< string >("$head;", Replacer< string >
	  ("$content;", last_from).apply(destination_headline_template())).apply
	  (alternate_headline_extension_template())).apply(from_buffer);
      last_from = relations[i].from;
    }
    if (last_to == "")
      last_to = relations[i].to;
    else if (last_to != relations[i].to)
    {
      to_buffer = Replacer< string >
	  ("$tail;", Replacer< string >("$head;", Replacer< string >
	  ("$content;", last_to).apply(destination_headline_template())).apply
	      (alternate_headline_extension_template())).apply(to_buffer);
      last_to = relations[i].to;
    }
  }
  from_buffer = Replacer< string >("$tail;", Replacer< string >
      ("$content;", last_from).apply(destination_headline_template())).apply(from_buffer);
  to_buffer = Replacer< string >("$tail;", Replacer< string >
      ("$content;", last_to).apply(destination_headline_template())).apply(to_buffer);
  
  headline = Replacer< string >("$from_tail;", from_buffer).apply
	    (Replacer< string >("$to_tail;", to_buffer).apply
	    (Replacer< string >("$rel_ref;", relation.ref).apply
	    (Replacer< string >("$rel_color;", relation.color).apply(headline))));
  
  // display operation times
  if (have_valid_operation_times)
  {
    string opening_hours_text;
    vector< Timespan > last_day;
    unsigned int range_begin(0);
    vector< Timespan >::const_iterator it(relations[0].opening_hours.begin());
    for (unsigned int i(0); i < 7; ++i)
    {
      vector< Timespan > day;
/*      if ((it != relations[0].opening_hours.end()) && (it->begin < i*24*60))
      {
	Timespan timespan(0, 0, 0, 0, 0, 0);
	timespan.begin = 0;
	timespan.end = it->end % (24*60);
	day.push_back(timespan);
	++it;
      }*/
      while ((it != relations[0].opening_hours.end()) && (it->end <= (i+1)*24*60))
      {
	Timespan timespan(0, 0, 0, 0, 0, 0);
	timespan.begin = it->begin % (24*60);
	timespan.end = it->end % (24*60);
	day.push_back(timespan);
	++it;
      }
      if ((it != relations[0].opening_hours.end()) && (it->begin < (i+1)*24*60))
      {
	Timespan timespan(0, 0, 0, 0, 0, 0);
	timespan.begin = it->begin % (24*60);
	timespan.end = it->end % (24*60)/*24*60*/;
	day.push_back(timespan);
	++it;
      }
      
      if (last_day != day)
      {
	// print last_day
	if (!last_day.empty())
	{
	  if (range_begin < i-1)
	    opening_hours_text += tr_weekday(range_begin) + '-' + tr_weekday(i-1) + ' ';
	  else
	    opening_hours_text += tr_weekday(range_begin) + ' ';
	  for (vector< Timespan >::const_iterator it(last_day.begin());
		      it != last_day.end(); ++it)
	    opening_hours_text += it->hh_mm() + ", ";
	}
	range_begin = i;
	last_day = day;
      }
    }
    // print last day
    if (!last_day.empty())
    {
      if (range_begin < 6)
	opening_hours_text += tr_weekday(range_begin) + '-' + tr_weekday(6) + ' ';
      else
	opening_hours_text += tr_weekday(range_begin) + ' ';
      for (vector< Timespan >::const_iterator it(last_day.begin());
	   it != last_day.end(); ++it)
	opening_hours_text += it->hh_mm() + ", ";
    }
    
    opening_hours_text = opening_hours_text.substr(0, opening_hours_text.size()-2);
    headline += multi_replace(translations,
	     Replacer< string >("$timespans;", opening_hours_text).apply
	    (operation_time_template()));
  }
  
  ostringstream result("");
  ostringstream backlines("");
  
  // count the number of line rows that actually appear
  vector< int > offset_of(relations.size()+1);
  offset_of[0] = 0;
  for (unsigned int i(0); i < relations.size(); ++i)
  {
    if (relations[i].direction != 0)
      offset_of[i+1] = offset_of[i] + 1;
    else
      offset_of[i+1] = offset_of[i];
  }
  
  // calculate extra space used by multiline labels
  vector< unsigned int > extra_rows(stoplist.stops.size()+1);
  unsigned int stop_idx(0);
  extra_rows[0] = 0;
  for (list< Stop >::const_iterator it(stoplist.stops.begin()); it != stoplist.stops.end(); ++it)
  {
    if (it->correspondences.size() > max_correspondences_below)
      extra_rows[stop_idx+1] = extra_rows[stop_idx] +
          (it->correspondences.size()+max_correspondences_per_line-1)
          /max_correspondences_per_line;
    else
      extra_rows[stop_idx+1] = extra_rows[stop_idx];
    ++stop_idx;
  }

  if (stoplist.stops.size() <= 1)
  {
    cout<<multi_replace(translations, Replacer< double >("$width;", 700).apply
        (Replacer< double >("$height;", height).apply
	(Replacer< string >("$rel_from;", relations.begin()->from).apply
	(Replacer< string >("$rel_to;", relations.begin()->to).apply
        (Replacer< string >("<headline/>", headline).apply
        (Replacer< string >("<stops-diagram/>", "<text x=\"100\" y=\"100\">At most one stop found</text>").apply
	(frame_template())))))));
    return 0;
  }
  if (((stoplist.stops.size() <= 21) && (force_rows == 0)) || (force_rows == 1))
  {
    if (stop_font_size == 0)
      stop_font_size = 16;
    
    double stop_distance(0);
    double vpos
        (height - 20*offset_of[relations.size()]
        - stop_font_size*max_correspondences_below);
    
    if (width > 0)
      stop_distance = (width - 220
        - sqrt(2.0)*stop_font_size*extra_rows[stoplist.stops.size()])
	/(stoplist.stops.size()-1);
    else
    {
      stop_distance = stop_font_size*sqrt(2.0)*3;
      width = stop_distance*(stoplist.stops.size()-1) + 220
        + sqrt(2.0)*stop_font_size*extra_rows[stoplist.stops.size()];
    }
    
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      if (relations[i].direction == 0)
	continue;
      
      double hmin(30 + first_stop_idx[i]*stop_distance + extra_rows[first_stop_idx[i]]*sqrt(2.0)*stop_font_size);
      double hmax(30 + last_stop_idx[i]*stop_distance + extra_rows[last_stop_idx[i]]*sqrt(2.0)*stop_font_size);
      
      result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< double >("$hmin;", hmin).apply
	    (Replacer< double >("$hmax;", hmax).apply
	    (Replacer< double >("$vpos;", vpos + 20*offset_of[i]).apply
	    (line_template()))));
      if (left_join_to[i] >= 0)
	result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< double >("$hmin;", hmin+4-stop_distance/2).apply
	    (Replacer< double >("$hmax;", hmin+2).apply
	    (Replacer< double >("$htop;", hmin-4-stop_distance/2).apply
	    (Replacer< double >("$vpos_self;", vpos + 20*offset_of[i]).apply
	    (Replacer< double >("$vpos_join;", vpos + 20*offset_of[left_join_to[i]]).apply
	    (left_join_template()))))));
      if (right_join_to[i] >= 0)
	result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< double >("$hmin;", hmax-2).apply
	    (Replacer< double >("$hmax;", hmax-4+stop_distance/2).apply
	    (Replacer< double >("$htop;", hmax+4+stop_distance/2).apply
	    (Replacer< double >("$vpos_self;", vpos + 20*offset_of[i]).apply
	    (Replacer< double >("$vpos_join;", vpos + 20*offset_of[right_join_to[i]]).apply
	    (right_join_template()))))));
    }
    
    int stopcount(0);
    for (list< Stop >::const_iterator it(stoplist.stops.begin()); it != stoplist.stops.end(); ++it)
    {
      double pos(30 + stopcount*stop_distance + extra_rows[stopcount]*sqrt(2.0)*stop_font_size);
      
      if ((!it->correspondences.empty()) && (it->correspondences.size() <= max_correspondences_below))
	backlines<<Replacer< double >("$hpos;", pos).apply
            (Replacer< double >("$vpos_upper;", vpos - 10).apply
            (Replacer< double >("$vpos_lower;", vpos - 10 + 20*offset_of[relations.size()]).apply
	    (stop_background_template())))<<'\n';
      
      bool is_a_terminus(false);
      for (unsigned int i(0); i < relations.size(); ++i)
      {
	if (relations[i].direction == 0)
	  continue;
	if (stopcount < first_stop_idx[i])
	  continue;
	if (stopcount > last_stop_idx[i])
	  continue;
	
	string stop_template;
	if (it->correspondences.empty())
	{
	  if (((stopcount == first_stop_idx[i]) && (left_join_to[i] < 0)) ||
	    ((stopcount == last_stop_idx[i]) && (right_join_to[i] < 0)))
	  {
	    is_a_terminus = true;
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_terminus_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_terminus_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_terminus_template();
	  }
	  else
	  {
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_stop_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_stop_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_stop_template();
	  }
	}
	else
	{
	  if (((stopcount == first_stop_idx[i]) && (left_join_to[i] < 0)) ||
	    ((stopcount == last_stop_idx[i]) && (right_join_to[i] < 0)))
	  {
	    is_a_terminus = true;
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_terminus_hub_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_terminus_hub_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_terminus_hub_template();
	  }
	  else
	  {
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_hub_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_hub_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_hub_template();
	  }
	}
      
	result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< string >("$stop_fontsize;", "16px").apply
	    (Replacer< double >("$hpos;", pos).apply
	    (Replacer< double >("$vpos;", vpos + 20*offset_of[i]).apply
	    (stop_template))));
      }
      ++stopcount;
      
      string stopname(multi_replace(name_shortcuts, it->name));
      if (is_a_terminus)
	result<<Replacer< string >("$stopname;", stopname).apply
            (Replacer< string >("$rel_color;", relation.color).apply
            (Replacer< double >("$stop_fontsize;", stop_font_size).apply
            (Replacer< double >("$hpos;", pos).apply
            (Replacer< double >("$vpos;", vpos).apply(terminus_name_template())))));
      else
	result<<Replacer< string >("$stopname;", stopname).apply
            (Replacer< string >("$rel_color;", relation.color).apply
            (Replacer< double >("$stop_fontsize;", stop_font_size).apply
            (Replacer< double >("$hpos;", pos).apply
            (Replacer< double >("$vpos;", vpos).apply(stop_name_template())))));

      vector< RelationHumanId > correspondences;
      for (set< RelationHumanId >::const_iterator cit(it->correspondences.begin());
	  cit != it->correspondences.end(); ++cit)
	correspondences.push_back(*cit);
      sort(correspondences.begin(), correspondences.end(), RelationHumanId_Comparator());

      if (it->correspondences.size() <= max_correspondences_below)
      {
	// add correspondences below the line bar
	unsigned int i(0);
	for (vector< RelationHumanId >::const_iterator cit(correspondences.begin());
	    cit != correspondences.end(); ++cit)
	{
	  result<<Replacer< string >("$ref;", cit->ref).apply
	      (Replacer< string >("$color;", cit->color).apply
              (Replacer< string >("$network;", pivot_network).apply
	      (Replacer< double >("$stop_fontsize;", stop_font_size).apply
	      (Replacer< double >("$hpos;", pos).apply
	      (Replacer< double >("$vpos;", vpos + stop_font_size*(i+1) + 20*offset_of[relations.size()] - 10)
                  .apply(correspondence_below_template()))))));
	  ++i;
	}
      }
      else
      {
	// add correspondences to the right of the stop's name
	string corr_buf("");
	unsigned int corr_count(0);
	pos += sqrt(2.0)*stop_font_size;
	for (vector< RelationHumanId >::const_iterator cit(correspondences.begin());
	cit != correspondences.end(); ++cit)
	{
	  corr_buf += Replacer< double >("$offset;", stop_font_size/2.0).apply
	      (Replacer< string >("$ref;", cit->ref).apply
	      (Replacer< string >("$color;", cit->color).apply(correspondence_item_template())));
	  if (++corr_count == max_correspondences_per_line)
	  {
	    result<<Replacer< double >("$stop_fontsize;", stop_font_size).apply
	        (Replacer< double >("$hpos;", pos).apply
	        (Replacer< double >("$vpos;", vpos).apply
	        (Replacer< string >("$data;", corr_buf).apply(correspondence_row_template()))));
	    corr_buf = "";
	    corr_count = 0;
	    pos += sqrt(2.0)*stop_font_size;
	  }
	}
	if (corr_count > 0)
	  result<<Replacer< double >("$stop_fontsize;", stop_font_size).apply
	      (Replacer< double >("$hpos;", pos).apply
	      (Replacer< double >("$vpos;", vpos).apply
	      (Replacer< string >("$data;", corr_buf).apply
	      (correspondence_row_template()))));
      }
    }
  }
  else
  {
    if (stop_font_size == 0)
      stop_font_size = 10;

    if (width == 0)
    {
      double stop_distance(stop_font_size*sqrt(2.0)*3);
      double width_upper(stop_distance*((stoplist.stops.size()+1)/2-1) + 220
        + sqrt(2.0)*stop_font_size*extra_rows[(stoplist.stops.size()+1)/2]);
      double width_lower(stop_distance*(stoplist.stops.size()/2) + 220
        + sqrt(2.0)*stop_font_size*(extra_rows[stoplist.stops.size()] -
	extra_rows[(stoplist.stops.size()-1)/2]));
      if (width_lower < width_upper)
	width = width_upper;
      else
        width = width_lower;
    }
    double stop_distance_upper((width - 220
        - sqrt(2.0)*stop_font_size*extra_rows[(stoplist.stops.size()+1)/2])
	/((stoplist.stops.size()+1)/2-1));
    double stop_distance_lower((width - 220
        - sqrt(2.0)*stop_font_size*(extra_rows[stoplist.stops.size()] - extra_rows[(stoplist.stops.size()-1)/2]))
	/(stoplist.stops.size()/2));
    double vpos_upper(height/2 - 20*offset_of[relations.size()]
	- stop_font_size*max_correspondences_below + 60);
    double vpos_lower(height - 20*offset_of[relations.size()]
	- stop_font_size*max_correspondences_below);
    double vpos(vpos_upper);
	
    for (unsigned int i(0); i < relations.size(); ++i)
    {
      if (relations[i].direction == 0)
	continue;
      
      if (first_stop_idx[i] < (int)(stoplist.stops.size()+1)/2)
      {
	double stop_distance(stop_distance_upper);
	double hmin(30 + first_stop_idx[i]*stop_distance + extra_rows[first_stop_idx[i]]*sqrt(2.0)*stop_font_size);
	double hmax(30 + last_stop_idx[i]*stop_distance + extra_rows[last_stop_idx[i]]*sqrt(2.0)*stop_font_size);
	if (last_stop_idx[i] >= (int)(stoplist.stops.size()+1)/2)
	  hmax = width - 170;
	result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< double >("$hmin;", hmin).apply
	    (Replacer< double >("$hmax;", hmax).apply
	    (Replacer< double >("$vpos;", vpos_upper + 20*offset_of[i]).apply
	    (line_template()))));
        if (left_join_to[i] >= 0)
	  result<<Replacer< string >("$rel_color;", relation.color).apply
	      (Replacer< double >("$hmin;", hmin+4-stop_distance/2).apply
	      (Replacer< double >("$hmax;", hmin+2).apply
	      (Replacer< double >("$htop;", hmin-4-stop_distance/2).apply
	      (Replacer< double >("$vpos_self;", vpos_upper + 20*offset_of[i]).apply
	      (Replacer< double >("$vpos_join;", vpos_upper + 20*offset_of[left_join_to[i]]).apply
	      (left_join_template()))))));
        if ((right_join_to[i] >= 0) && (last_stop_idx[i] < (int)(stoplist.stops.size()+1)/2))
	  result<<Replacer< string >("$rel_color;", relation.color).apply
	      (Replacer< double >("$hmin;", hmax-2).apply
	      (Replacer< double >("$hmax;", hmax-4+stop_distance/2).apply
	      (Replacer< double >("$htop;", hmax+4+stop_distance/2).apply
	      (Replacer< double >("$vpos_self;", vpos_upper + 20*offset_of[i]).apply
	      (Replacer< double >("$vpos_join;", vpos_upper + 20*offset_of[right_join_to[i]]).apply
	      (right_join_template()))))));
      }
      if (last_stop_idx[i] >= (int)(stoplist.stops.size()+1)/2)
      {
	double stop_distance(stop_distance_lower);
	double hmin(30 + (first_stop_idx[i]-(int)(stoplist.stops.size()-1)/2)*stop_distance
	    +  (extra_rows[first_stop_idx[i]] - extra_rows[(stoplist.stops.size()+1)/2])*sqrt(2.0)*stop_font_size);
	double hmax(30 + (last_stop_idx[i]-(int)(stoplist.stops.size()-1)/2)*stop_distance
	    +  (extra_rows[last_stop_idx[i]] - extra_rows[(stoplist.stops.size()+1)/2])*sqrt(2.0)*stop_font_size);
	if (first_stop_idx[i] < (int)(stoplist.stops.size()+1)/2)
	  hmin = 30;
	result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< double >("$hmin;", hmin).apply
	    (Replacer< double >("$hmax;", hmax).apply
	    (Replacer< double >("$vpos;", vpos_lower + 20*offset_of[i]).apply
	    (line_template()))))<<'\n';
        if ((left_join_to[i] >= 0) && (first_stop_idx[i] >= (int)(stoplist.stops.size()+1)/2))
	  result<<Replacer< string >("$rel_color;", relation.color).apply
	      (Replacer< double >("$hmin;", hmin+4-stop_distance/2).apply
	      (Replacer< double >("$hmax;", hmin+2).apply
	      (Replacer< double >("$htop;", hmin-4-stop_distance/2).apply
	      (Replacer< double >("$vpos_self;", vpos_lower + 20*offset_of[i]).apply
	      (Replacer< double >("$vpos_join;", vpos_lower + 20*offset_of[left_join_to[i]]).apply
	      (left_join_template()))))));
        if (right_join_to[i] >= 0)
	  result<<Replacer< string >("$rel_color;", relation.color).apply
	      (Replacer< double >("$hmin;", hmax-2).apply
	      (Replacer< double >("$hmax;", hmax-4+stop_distance/2).apply
	      (Replacer< double >("$htop;", hmax+4+stop_distance/2).apply
	      (Replacer< double >("$vpos_self;", vpos_lower + 20*offset_of[i]).apply
	      (Replacer< double >("$vpos_join;", vpos_lower + 20*offset_of[right_join_to[i]]).apply
	      (right_join_template()))))));
      }
    }
    
    unsigned int count(0);
    double stop_distance(0);
  
    stop_distance = stop_distance_upper;
  
    int stopcount(0);
    for (list< Stop >::const_iterator it(stoplist.stops.begin()); it != stoplist.stops.end(); ++it)
    {
      double pos;
      if (stopcount < (int)(stoplist.stops.size()+1)/2)
	pos = 30 + stopcount*stop_distance + extra_rows[stopcount]*sqrt(2.0)*stop_font_size;
      else
	pos = 30 + (stopcount-(int)(stoplist.stops.size()-1)/2)*stop_distance
	    + (extra_rows[stopcount] - extra_rows[(stoplist.stops.size()+1)/2])*sqrt(2.0)*stop_font_size;
      
      if ((!it->correspondences.empty()) && (it->correspondences.size() <= max_correspondences_below))
        backlines<<Replacer< double >("$hpos;", pos).apply
            (Replacer< double >("$vpos_upper;", vpos - 10).apply
            (Replacer< double >("$vpos_lower;", vpos - 10 + 20*offset_of[relations.size()]).apply
	    (stop_background_template())))<<'\n';
      
      bool is_a_terminus(false);
      for (unsigned int i(0); i < relations.size(); ++i)
      {
	if (relations[i].direction == 0)
	  continue;
	if (stopcount < first_stop_idx[i])
	  continue;
	if (stopcount > last_stop_idx[i])
	  continue;
	
	string stop_template;
	if (it->correspondences.empty())
	{
	  if (((stopcount == first_stop_idx[i]) && (left_join_to[i] < 0)) ||
	    ((stopcount == last_stop_idx[i]) && (right_join_to[i] < 0)))
	  {
	    is_a_terminus = true;
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_terminus_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_terminus_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_terminus_template();
	  }
	  else
	  {
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_stop_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_stop_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_stop_template();
	  }
	}
	else
	{
	  if (((stopcount == first_stop_idx[i]) && (left_join_to[i] < 0)) ||
	    ((stopcount == last_stop_idx[i]) && (right_join_to[i] < 0)))
	  {
	    is_a_terminus = true;
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_terminus_hub_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_terminus_hub_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_terminus_hub_template();
	  }
	  else
	  {
	    if (it->used_by[i] == Stop::BOTH)
	      stop_template = bidirectional_hub_template();
	    else if (it->used_by[i] == Stop::FORWARD)
	      stop_template = forward_hub_template();
	    else if (it->used_by[i] == Stop::BACKWARD)
	      stop_template = backward_hub_template();
	  }
	}
      
	result<<Replacer< string >("$rel_color;", relation.color).apply
	    (Replacer< string >("$stop_fontsize;", "16px").apply
	    (Replacer< double >("$hpos;", pos).apply
	    (Replacer< double >("$vpos;", vpos + 20*offset_of[i]).apply
	    (stop_template))));
      }
      ++stopcount;
      
      string stopname(multi_replace(name_shortcuts, it->name));
      if (is_a_terminus)
        result<<Replacer< string >("$stopname;", stopname).apply
            (Replacer< string >("$rel_color;", relation.color).apply
            (Replacer< double >("$stop_fontsize;", stop_font_size).apply
            (Replacer< double >("$hpos;", pos).apply
            (Replacer< double >("$vpos;", vpos).apply(terminus_name_template())))));
      else
        result<<Replacer< string >("$stopname;", stopname).apply
            (Replacer< string >("$rel_color;", relation.color).apply
            (Replacer< double >("$stop_fontsize;", stop_font_size).apply
            (Replacer< double >("$hpos;", pos).apply
            (Replacer< double >("$vpos;", vpos).apply(stop_name_template())))));	

      vector< RelationHumanId > correspondences;
      for (set< RelationHumanId >::const_iterator cit(it->correspondences.begin());
	  cit != it->correspondences.end(); ++cit)
	correspondences.push_back(*cit);
      sort(correspondences.begin(), correspondences.end(), RelationHumanId_Comparator());

      if (it->correspondences.size() <= max_correspondences_below)
      {
	// add correspondences below the line bar
	unsigned int i(0);
	for (vector< RelationHumanId >::const_iterator cit(correspondences.begin());
	cit != correspondences.end(); ++cit)
	{
	  result<<Replacer< string >("$ref;", cit->ref).apply
	  (Replacer< string >("$color;", cit->color).apply
	  (Replacer< string >("$network;", pivot_network).apply
	  (Replacer< double >("$stop_fontsize;", stop_font_size).apply
	  (Replacer< double >("$hpos;", pos).apply
	  (Replacer< double >("$vpos;", vpos + stop_font_size*(i+1) + 20*offset_of[relations.size()] - 10)
              .apply(correspondence_below_template()))))));
	  ++i;
	}
      }
      else
      {
	// add correspondences to the right of the stop's name
	string corr_buf("");
	unsigned int corr_count(0);
	pos += sqrt(2.0)*stop_font_size;
	for (vector< RelationHumanId >::const_iterator cit(correspondences.begin());
	cit != correspondences.end(); ++cit)
	{
	  corr_buf += Replacer< double >("$offset;", stop_font_size/2.0).apply
	      (Replacer< string >("$ref;", cit->ref).apply
	      (Replacer< string >("$color;", cit->color).apply(correspondence_item_template())));
	  if (++corr_count == max_correspondences_per_line)
	  {
	    result<<Replacer< double >("$stop_fontsize;", stop_font_size).apply
		(Replacer< double >("$hpos;", pos).apply
		(Replacer< double >("$vpos;", vpos).apply
		(Replacer< string >("$data;", corr_buf).apply
		(correspondence_row_template()))));
	    corr_buf = "";
	    corr_count = 0;
	    pos += sqrt(2.0)*stop_font_size;
	  }
	}
	if (corr_count > 0)
	  result<<Replacer< double >("$stop_fontsize;", stop_font_size).apply
	      (Replacer< double >("$hpos;", pos).apply
	      (Replacer< double >("$vpos;", vpos).apply
	      (Replacer< string >("$data;", corr_buf).apply(correspondence_row_template()))));
      }
      
      if (++count >= (stoplist.stops.size()+1)/2)
      {
	count = 0;
	vpos = vpos_lower;
	stop_distance = stop_distance_lower;
      }
    }
  }
  
  cout<<multi_replace(translations, Replacer< double >("$width;", width).apply
      (Replacer< double >("$height;", height).apply
      (Replacer< string >("$rel_from;", relations.begin()->from).apply
      (Replacer< string >("$rel_to;", relations.begin()->to).apply
      (Replacer< string >("<headline/>", headline).apply
      (Replacer< string >("<stops-diagram/>", backlines.str() + result.str()).apply(frame_template())))))));
  
  return 0;
}
