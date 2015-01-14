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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COORD_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COORD_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Coord_Query_Statement : public Output_Statement
{
  public:
    Coord_Query_Statement(int line_number_, const map< string, string >& attributes,
                          Parsed_Query& global_settings);
    virtual string get_name() const { return "coord-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Coord_Query_Statement() {}    
    static Generic_Statement_Maker< Coord_Query_Statement > statement_maker;
    
    static int check_segment
        (uint32 a_lat, int32 a_lon, uint32 b_lat, int32 b_lon,
         uint32 coord_lat, int32 coord_lon);
    //static uint32 shifted_lat(uint32 ll_index, uint64 coord);
    //static int32 lon_(uint32 ll_index, uint64 coord);
    static int check_area_block
        (uint32 ll_index, const Area_Block& area_block,
	 uint32 coord_lat, int32 coord_lon);
    
    // Used as bitmasks.
    const static int HIT = 1;
    const static int TOGGLE_EAST = 2;
    const static int TOGGLE_WEST = 4;
    const static int INTERSECT = 8;
    
    static bool is_used() { return is_used_; }
  
  private:
    string input;
    double lat, lon;
    
    static bool is_used_;
};

#endif
