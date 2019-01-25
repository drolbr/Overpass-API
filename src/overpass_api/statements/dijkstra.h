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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__DIJKSTRA_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__DIJKSTRA_H


#include "set_prop.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


/* === The statement <em>dijkstra</em> ===

Experimental. Not intended for general use.
*/

class Dijkstra_Statement : public Output_Statement
{
public:
  Dijkstra_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "dijkstra"; }
  virtual void execute(Resource_Manager& rman);
  static Generic_Statement_Maker< Dijkstra_Statement > statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<dijkstra"
        + (input != "_" ? " from=\"" + input + "\"" : "");
    return result + dump_xml_result_name() + "/>\n";
  }

  virtual std::string dump_compact_ql(const std::string& indent) const
  {
    std::string result = indent + (input == "_" ? "" : "." + input + " ") + "dijkstra";
    return result + dump_ql_result_name() + ";";
  }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  {
    return dump_compact_ql(indent);
  }

private:
  std::string input;
};


#endif
