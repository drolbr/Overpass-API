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

#ifndef FOO
#define FOO

#include <cmath>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

struct NamedNode
{
  public:
    NamedNode() : lat(100.0), lon(200.0), name("") {}
    
    double lat, lon;
    string name;
};

double spat_distance(const NamedNode& nnode1, const NamedNode& nnode2);

struct DisplayNode
{
  public:
    DisplayNode() : node(), limit(-1.0) {}
    
    NamedNode node;
    double limit;
};

/* represents a weekly timespan */
struct Timespan
{
  Timespan(unsigned int begin_d, unsigned int begin_h, unsigned int begin_m,
	   unsigned int end_d, unsigned int end_h, unsigned int end_m)
  : begin(begin_d*60*24 + begin_h*60 + begin_m),
    end(end_d*60*24 + end_h*60 + end_m) {}
  
  bool operator<(const Timespan& a) const;
  bool operator==(const Timespan& a) const;
  string hh_mm() const;
  
  // values are minutes since monday midnight
  unsigned int begin;
  unsigned int end;
};

struct Relation
{
  public:
    Relation() : forward_stops(), backward_stops(), ref(""), network(""),
      color(""), from(""), to(""), direction(0), opening_hours() {}
    
    vector< unsigned int > forward_stops;
    vector< unsigned int > backward_stops;
    
    string name;
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
  bool operator()(const RelationHumanId& rhi_1, const RelationHumanId& rhi_2);
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
	     double walk_limit_for_changes_);
    
    set< RelationHumanId > correspondences_at(const NamedNode& nnode) const;    
    set< RelationHumanId >& display_nodes_at
        (const NamedNode& nnode, set< RelationHumanId >& result) const;
    
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
	 const Correspondence_Data& cors_data);
    
    void push_back
	(const NamedNode& nnode, int rel_index, int mode,
	 const Correspondence_Data& cors_data);
    
    void insert
	(list< Stop >::iterator it, const NamedNode& nnode, int rel_index, int mode,
	 const Correspondence_Data& cors_data);
    
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

Stoplist make_stoplist(double walk_limit_for_changes, bool doubleread_rel,
		       vector< Display_Class >& display_classes_,
		       string pivot_ref_,
		       string pivot_network_,
		       vector< Relation >& relations,
		       bool& have_valid_operation_times,
		       int debug_level);

#endif
