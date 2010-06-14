/*****************************************************************
 * Must be used with Expat compiled for UTF-8 output.
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
#include "../expat_justparse_interface.h"

using namespace std;

const double PI = acos(0)*2;

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
  return "<text x=\"0\" y=\"0\" transform=\"translate($hpos;,$vpos;)\""
  " font-family=\"Liberation Sans, sans-serif\" font-size=\"$stop_fontsize;px\""
  " text-anchor=\"middle\" fill=\"$color;\">$ref;</text>\n"
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

const vector< unsigned int >& default_colors()
{
  static vector< unsigned int > colors;
  if (colors.empty())
  {
/*    colors.push_back(0x777777);
    colors.push_back(0xff0000);
    colors.push_back(0xff00ff);
    colors.push_back(0x7700aa);
    colors.push_back(0x0000aa);
    colors.push_back(0x00dddd);
    colors.push_back(0x00ff00);
    colors.push_back(0x008800);
    colors.push_back(0x999900);
    colors.push_back(0xff7700);*/
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

struct DisplayNode
{
  public:
    DisplayNode() : node(), limit(-1.0) {}
    
    NamedNode node;
    double limit;
};

double spat_distance(const NamedNode& nnode1, const NamedNode& nnode2)
{
  return acos(sin(nnode1.lat/180.0*PI)*sin(nnode2.lat/180.0*PI)
      + sin(nnode1.lon/180.0*PI)*cos(nnode1.lat/180.0*PI)
        *sin(nnode2.lon/180.0*PI)*cos(nnode2.lat/180.0*PI)
      + cos(nnode1.lon/180.0*PI)*cos(nnode1.lat/180.0*PI)
        *cos(nnode2.lon/180.0*PI)*cos(nnode2.lat/180.0*PI))
      /PI*20000000;
}

/* represents a weekly timespan */
struct Timespan
{
  Timespan(unsigned int begin_d, unsigned int begin_h, unsigned int begin_m,
	   unsigned int end_d, unsigned int end_h, unsigned int end_m)
  : begin(begin_d*60*24 + begin_h*60 + begin_m),
    end(end_d*60*24 + end_h*60 + end_m) {}
  
  bool operator<(const Timespan& a) const
  {
    if (begin < a.begin)
      return true;
    if (begin > a.begin)
      return false;
    return end < a.end;
  }
  
  bool operator==(const Timespan& a) const
  {
    return ((begin == a.begin) && (end == a.end));
  }
  
  string hh_mm() const
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
  
  // values are minutes since monday midnight
  unsigned int begin;
  unsigned int end;
};

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
    if ((start_time >= end_time) || (end_time > 60*24))
      continue;
    
    for (unsigned int day(begin_day); day <= end_day; ++day)
      timespans.push_back(Timespan
	  (day, start_hour, start_minute, day, end_hour, end_minute));
  }
}

struct Relation
{
  public:
    Relation() : forward_stops(), backward_stops(), ref(""), network(""),
      color(""), from(""), to(""), direction(0), opening_hours() {}
    
    vector< unsigned int > forward_stops;
    vector< unsigned int > backward_stops;
    
    string ref;
    string network;
    string color;
    string from;
    string to;
    int direction;
    const static int FORWARD = 1;
    const static int BACKWARD = 2;
    const static int BOTH = 4;

    vector< Timespan > opening_hours;
};

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

struct RelationHumanId
{
  public:
    RelationHumanId() : ref(""), color("") {}
    RelationHumanId(string ref_, string color_) : ref(ref_), color(color_) {}
    
    string ref;
    string color;
};

inline bool operator<(const RelationHumanId& rhi_1, const RelationHumanId& rhi_2)
{
  return (rhi_1.ref < rhi_2.ref);
}

struct RelationHumanId_Comparator
{
  bool operator()(const RelationHumanId& rhi_1, const RelationHumanId& rhi_2)
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
};

struct Stop
{
  public:
    Stop() : name(""), used_by(used_by_size) {}
    
    string name;
    
    vector< int > used_by;
    static unsigned int used_by_size;
    
    set< RelationHumanId > correspondences;
    const static int FORWARD = 1;
    const static int BACKWARD = 2;
    const static int BOTH = 3;
};

unsigned int Stop::used_by_size = 0;

struct Correspondence_Data
{
  public:
    Correspondence_Data(const map< unsigned int, set< RelationHumanId > >& what_calls_here_,
	     const map< unsigned int, NamedNode >& nodes_,
	     const vector< DisplayNode > display_nodes_,
	     double walk_limit_for_changes_)
    : what_calls_here(what_calls_here_), nodes(nodes_),
      display_nodes(display_nodes_),
      walk_limit_for_changes(walk_limit_for_changes_) {}
      
    Correspondence_Data(const vector< Relation >& correspondences,
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
	for(vector< unsigned int >::const_iterator it(rit->forward_stops.begin());
		   it != rit->forward_stops.end(); ++it)
	{
	  if ((nodes.find(*it) != nodes.end()) && (nodes.find(*it)->second.lat <= 90.0))
	    what_calls_here[*it].insert(RelationHumanId(rit->ref, rit->color));
	}
	for(vector< unsigned int >::const_iterator it(rit->backward_stops.begin());
		   it != rit->backward_stops.end(); ++it)
	{
	  if ((nodes.find(*it) != nodes.end()) && (nodes.find(*it)->second.lat <= 90.0))
	    what_calls_here[*it].insert(RelationHumanId(rit->ref, rit->color));
	}
      }
    }
    
    set< RelationHumanId > correspondences_at(const NamedNode& nnode) const
    {
      set< RelationHumanId > result;
      
      for (map< unsigned int, set< RelationHumanId > >::const_iterator it(what_calls_here.begin());
	   it != what_calls_here.end(); ++it)
      {
	if ((nodes.find(it->first) != nodes.end()) &&
		    (spat_distance(nnode, nodes.find(it->first)->second) < walk_limit_for_changes))
	{
	  for (set< RelationHumanId >::const_iterator iit(it->second.begin()); iit != it->second.end(); ++iit)
	    result.insert(*iit);
	}
      }
      return result;
    }
    
    set< RelationHumanId >& display_nodes_at
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
    
    map< unsigned int, set< RelationHumanId > > what_calls_here;
    const map< unsigned int, NamedNode >& nodes;
    vector< DisplayNode > display_nodes;
    const double walk_limit_for_changes;
};

struct Stoplist
{
  public:
    Stoplist() : stops() {}
    
    void populate_stop
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
    
    void push_back
	(const NamedNode& nnode, int rel_index, int mode,
	 const Correspondence_Data& cors_data)
    {
      Stop stop;
      populate_stop(stop, nnode, rel_index, mode, false, cors_data);
      stops.push_back(stop);
    }
    
    void insert
	(list< Stop >::iterator it, const NamedNode& nnode, int rel_index, int mode,
	 const Correspondence_Data& cors_data)
    {
      Stop stop;
      populate_stop(stop, nnode, rel_index, mode, false, cors_data);
      stops.insert(it, stop);
    }
    
    list< Stop > stops;
};

struct Display_Class
{
  Display_Class() : key(""), value(""), text(""), limit(-1) {}
  
  string key;
  string value;
  string text;
  double limit;
};

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

map< unsigned int, NamedNode > nodes;
NamedNode nnode;
unsigned int id;

vector< Relation > relations;
vector< Relation > correspondences;
vector< Display_Class > display_classes;
vector< DisplayNode > display_nodes;
string pivot_ref;
string pivot_network;
Relation relation;

unsigned int parse_status;
const unsigned int IN_NODE = 1;
const unsigned int IN_RELATION = 2;
bool is_stop = false;
bool is_route = false;

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
    if ((key == "route") && (value == "bus"))
      is_route = true;
    if ((key == "route") && (value == "tram"))
      is_route = true;
    if ((key == "route") && (value == "light_rail"))
      is_route = true;
    if ((key == "route") && (value == "subway"))
      is_route = true;
    if ((key == "route") && (value == "rail"))
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
    for (vector< Display_Class >::const_iterator it(display_classes.begin());
	it != display_classes.end(); ++it)
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
      if (((pivot_ref == "") || (relation.ref == pivot_ref))
	&& ((pivot_network == "") || (relation.network == pivot_network)))
	relations.push_back(relation);
      else
	correspondences.push_back(relation);
    }
  }
}

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
    ++argi;
  }
  
  // read the XML input
  parse_status = 0;
  parse(stdin, start, end);
  
  // bailout if no relation is found
  if (relations.size() == 0)
  {
    cout<<Replacer< double >("$width;", 700).apply
    (Replacer< double >("$height;", height).apply
    (Replacer< string >("<headline/>", "").apply
    (Replacer< string >("<stops-diagram/>", "<text x=\"100\" y=\"100\">No relation found</text>").apply(frame_template()))));
    return 0;
  }
  
  // check whether the relations have uniform operation times
  bool have_valid_operation_times(!relations[0].opening_hours.empty());
  for (unsigned int i(1); i < relations.size(); ++i)
  {
    if (!(have_uniform_operation_times(relations[0], relations[i])))
    {
      have_valid_operation_times = false;
      break;
    }
  }
  if ((have_valid_operation_times) &&
    (!(relations[0].opening_hours.empty())))
  {
    for (int i(0); i < (int)correspondences.size(); ++i)
    {
      if (correspondences[i].opening_hours.empty())
	continue;
      if (!(have_intersecting_operation_times(relations[0], correspondences[i])))
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
  Stop::used_by_size = relations.size();
  
  // make a common stoplist from all relations
  // integrate all relations (their forward direction) into the stoplist
  relation = relations.front();
  for(vector< unsigned int >::const_iterator it(relation.forward_stops.begin());
      it != relation.forward_stops.end(); ++it)
  {
    stoplist.push_back(nodes[*it], 0, Stop::FORWARD, cors_data);
  }
  relations.begin()->direction |= Relation::FORWARD;
  
  vector< Relation >::iterator rit(relations.begin());
  unsigned int rel_count(1);
  ++rit;
  while (rit != relations.end())
  {
    rit->direction |= process_relation
	(*rit, nodes, rel_count, 0, Relation::FORWARD, stoplist, cors_data);
    
    ++rit;
    ++rel_count;
  }
  
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
    
    int direction_const(0);
    if ((rit->direction & Relation::FORWARD) != 0)
      direction_const = Stop::BACKWARD;
    if ((rit->direction & Relation::BACKWARD) != 0)
      direction_const = Stop::FORWARD;
    
    process_relation(*rit, nodes, rel_count, direction_const, Relation::BACKWARD, stoplist, cors_data);
    
    ++rit;
    ++rel_count;
  }
  
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
  
  for (multimap< double, pair< int, int > >::const_reverse_iterator
       it(possible_pairs.rbegin()); it != possible_pairs.rend(); ++it)
  {
    if (relations[it->second.second].direction != Relation::BACKWARD)
      continue;
    for (list< Stop >::iterator sit(stoplist.stops.begin());
        sit != stoplist.stops.end(); ++sit)
      sit->used_by[it->second.first] |= sit->used_by[it->second.second];
    relations[it->second.second].direction = 0;
  }

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
      if ((it != relations[0].opening_hours.end()) && (it->begin < i*24*60))
      {
	Timespan timespan(0, 0, 0, 0, 0, 0);
	timespan.begin = 0;
	timespan.end = it->end % (24*60);
	day.push_back(timespan);
	++it;
      }
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
	timespan.end = 24*60;
	day.push_back(timespan);
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
  
  // count the number of line row that actually appear
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
	      (Replacer< double >("$stop_fontsize;", stop_font_size).apply
	      (Replacer< double >("$hpos;", pos).apply
	      (Replacer< double >("$vpos;", vpos + stop_font_size*(i+1) + 20*offset_of[relations.size()] - 10).apply
	      (correspondence_below_template())))));
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
	  (Replacer< double >("$stop_fontsize;", stop_font_size).apply
	  (Replacer< double >("$hpos;", pos).apply
	  (Replacer< double >("$vpos;", vpos + stop_font_size*(i+1) + 20*offset_of[relations.size()] - 10).apply
	  (correspondence_below_template())))));
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
