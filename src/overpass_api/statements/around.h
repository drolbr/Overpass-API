/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "../data/collect_members.h"
#include "../data/way_geometry_store.h"
#include "statement.h"

using namespace std;


struct Prepared_Segment
{
  double first_lat;
  double first_lon;
  double second_lat;
  double second_lon;
  vector< double > first_cartesian;
  vector< double > second_cartesian;
  vector< double > norm;
  
  Prepared_Segment(double first_lat, double first_lon, double second_lat, double second_lon);
};


struct Prepared_Point
{
  double lat;
  double lon;
  vector< double > cartesian;
  
  Prepared_Point(double lat, double lon);
};


class Around_Statement : public Output_Statement
{
  public:
    Around_Statement(int line_number_, const map< string, string >& attributes,
                     Parsed_Query& global_settings);
    virtual string get_name() const { return "around"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Around_Statement();    
    static Generic_Statement_Maker< Around_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    string get_source_name() const { return input; }

    set< pair< Uint32_Index, Uint32_Index > > calc_ranges
        (const Set& input_nodes, Resource_Manager& rman) const;

    void calc_lat_lons(const Set& input_nodes, Statement& query, Resource_Manager& rman);

    bool is_inside(double lat, double lon) const;
    bool is_inside(double first_lat, double first_lon, double second_lat, double second_lon) const;
    bool is_inside(const vector< Quad_Coord >& way_geometry) const;
    
    double get_radius() const { return radius; }
    
    template< typename Node_Skeleton >
    void add_nodes(const map< Uint32_Index, vector< Node_Skeleton > >& nodes);
    
    template< typename Way_Skeleton >
    void add_ways(const map< Uint31_Index, vector< Way_Skeleton > >& ways,
		  const Way_Geometry_Store& way_geometries);
  private:
    string input;
    double radius;
    double lat;
    double lon;
    map< Uint32_Index, vector< pair< double, double > > > radius_lat_lons;
    vector< Prepared_Point > simple_lat_lons;
    vector< Prepared_Segment > simple_segments;
    vector< Query_Constraint* > constraints;
};

#endif
