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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COORD_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COORD_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "../data/utils.h"
#include "statement.h"


class Coord_Query_Statement : public Output_Statement
{
  public:
    Coord_Query_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                          Parsed_Query& global_settings);
    virtual std::string get_name() const { return "coord-query"; }
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

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<coord-query"
          + (input != "_" ? std::string(" from=\"") + input + "\"" : "")
          + (lat != 100. ? std::string(" lat=\"") + to_string(lat) + "\"" : "")
          + (lon != 200. ? std::string(" lon=\"") + to_string(lon) + "\"" : "")
          + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return (input != "_" ? std::string(".") + input + " " : "")
          + "is_in"
          + (lat != 100. ? std::string("(") + to_string(lat) : "")
          + (lon != 200. ? std::string(",") + to_string(lon) : "")
          + (lat != 100. ? ")" : "") + dump_ql_result_name();
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }

  private:
    std::string input;
    double lat, lon;

    static bool is_used_;
};

#endif
