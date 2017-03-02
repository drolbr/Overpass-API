/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AREA_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AREA_QUERY_H

#include "../data/collect_members.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Area_Query_Statement : public Output_Statement
{
  public:
    Area_Query_Statement(int line_number_, const map< string, string >& attributes,
                         Query_Constraint* bbox_limitation = 0);
    virtual string get_name() const { return "area-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Area_Query_Statement();    
    static Generic_Statement_Maker< Area_Query_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    void get_ranges
      (set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       set< Uint31_Index >& area_block_req,
       Resource_Manager& rman);

    void get_ranges
      (const map< Uint31_Index, vector< Area_Skeleton > >& input_areas,
       set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       set< Uint31_Index >& area_block_req,
       Resource_Manager& rman);

    void collect_nodes
      (const set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       const set< Uint31_Index >& req,
       vector< Node::Id_Type >* ids,
       map< Uint32_Index, vector< Node_Skeleton > >& nodes,
       Resource_Manager& rman);
       
    template< typename Node_Skeleton >
    void collect_nodes
      (map< Uint32_Index, vector< Node_Skeleton > >& nodes,
       const set< Uint31_Index >& req, bool add_border,
       Resource_Manager& rman);
       
    template< typename Way_Skeleton >
    void collect_ways
      (const Way_Geometry_Store& way_geometries,
       map< Uint31_Index, vector< Way_Skeleton > >& ways,
       const set< Uint31_Index >& req, bool add_border,
       const Statement& query, Resource_Manager& rman);

    bool areas_from_input() const { return (submitted_id == 0); }
    long long get_submitted_id() const { return submitted_id; }
    string get_input() const { return input; }
    
    static bool is_used() { return is_used_; }
  
  private:
    string input;
    long long submitted_id;
    vector< Area_Skeleton::Id_Type > area_id;    
    static bool is_used_;
    vector< Query_Constraint* > constraints;
};


int intersects_inner(const Area_Block& string_a, const Area_Block& string_b);

void has_inner_points(const Area_Block& string_a, const Area_Block& string_b, int& inside);

#endif
