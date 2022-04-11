/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAP_TO_AREA_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAP_TO_AREA_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"


class Map_To_Area_Statement : public Output_Statement
{
  public:
    Map_To_Area_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                          Parsed_Query& global_settings);
    virtual std::string get_name() const { return "map-to-area"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Map_To_Area_Statement() {}
    static Generic_Statement_Maker< Map_To_Area_Statement > statement_maker;

    static bool is_used() { return is_used_; }

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<map-to-area"
          + (input != "_" ? std::string(" from=\"") + input + "\"" : "")
          + dump_xml_result_name()
          + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string& indent) const { return dump_subquery_map_ql(indent, false); }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_subquery_map_ql(indent, true); }

    std::string dump_subquery_map_ql(const std::string& indent, bool pretty) const
    {
      return indent + (input != "_" ? "." + input + " " : "") + "map_to_area"
          + dump_ql_result_name() + ";";
    }

  private:
    std::string input;

    static bool is_used_;
};

#endif
