/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Around_Statement : public Statement
{
  public:
    Around_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "around"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
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
    bool is_inside(const Way_Skeleton& way,
		   const vector< pair< Uint32_Index, const Node_Skeleton* > >& way_members_by_id)
		   const;
    
  private:
    string input, output;
    double radius;
    map< Uint32_Index, vector< pair< double, double > > > radius_lat_lons;
    vector< pair< double, double > > simple_lat_lons;
    vector< pair< pair< double, double >, pair< double, double > > > simple_segments;
    vector< Query_Constraint* > constraints;
};

#endif
