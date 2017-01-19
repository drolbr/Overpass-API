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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_AREA_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_AREA_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"


// Tries to make an area from the ways in the "from"-set.
// It assumes that
// - all nodes referenced in the ways exist also in the nodes
// - the ways do intersect each other only at nodes that are members of all involved ways
// - the ways can be concatenated such that they form only closed ways.
// It produces the datastructure Area described in script_datatypes.h
// - due to the size restrictions, the algorithm might split up line segments and produce
//   addtional vertices. It does not add nodes for these datastructures to the "into"-set,
//   they are contained only in the Area dataset.
class Make_Area_Statement : public Output_Statement
{
  public:
    Make_Area_Statement(int line_number_, const map< string, string >& attributes,
                        Query_Constraint* bbox_limitation = 0);
    virtual string get_name() const { return "make-area"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Make_Area_Statement() {}    
    static Generic_Statement_Maker< Make_Area_Statement > statement_maker;
    
    static bool is_used() { return is_used_; }
    
  private:
    string input, pivot;
    
    static pair< uint32, Uint64 > detect_pivot(const Set& pivot);
    static Node::Id_Type check_node_parity(const Set& pivot);
    static pair< Node::Id_Type, Way::Id_Type > create_area_blocks
        (map< Uint31_Index, vector< Area_Block > >& areas, bool& wraps_around_date_line,
	 uint32 id, const Set& pivot);
    static uint32 shifted_lat(uint32 ll_index, uint64 coord);
    static int32 lon_(uint32 ll_index, uint64 coord);
    static void add_segment_blocks
        (map< Uint31_Index, vector< Area_Block > >& areas, uint32 id);
	
    static bool is_used_;
};

#endif
