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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__POLYGON_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__POLYGON_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;


class Polygon_Query_Statement : public Statement
{
  public:
    Polygon_Query_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "polygon-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Polygon_Query_Statement();
    
    static Generic_Statement_Maker< Polygon_Query_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    set< pair< Uint32_Index, Uint32_Index > > calc_ranges();

    void collect_nodes(map< Uint32_Index, vector< Node_Skeleton > >& nodes, bool add_border);
       
    void collect_ways
      (map< Uint31_Index, vector< Way_Skeleton > >& ways,
       map< Uint32_Index, vector< Node_Skeleton > >& way_members_,
       vector< pair< Uint32_Index, const Node_Skeleton* > > way_members_by_id,
       bool add_border,
       Resource_Manager& rman);

  private:
    string output;
    unsigned int type;
    vector< Aligned_Segment > segments;
    vector< Query_Constraint* > constraints;
};

#endif
