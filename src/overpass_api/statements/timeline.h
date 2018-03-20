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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__TIMELINE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__TIMELINE_H

#include <map>
#include <string>
#include <vector>

#include "evaluator.h"
#include "statement.h"


/* === The statement <em>timeline</em> ===

''since v0.7.55''

The statement <em>timeline</em> takes type and id of an object and optionally version as arguments.
It then returns a derived structure containing the meta information of the specified version.
If no version is specified then one object for each known version is returned.
Each of these objects has tags <em>ref</em>, <em>reftype</em>, and <em>refversion</em> to identify the reference.
In addition, it has tags <em>created</em> and <em>expired</em> that contain the relevant timestamps.

The base syntax is

  timeline(<Type>, <Ref>);

resp.

  timeline(<Type>, <Ref>, <Version>);

where <Type> is one of the three literals node, way, or relation.

In addition, an output set <Set> can be specified:

  timeline(<Type>, <Ref>)->.<Set>;

resp.

  timeline(<Type>, <Ref>, <Version>)->.<Set>;

*/

class Timeline_Statement : public Output_Statement
{
public:
  Timeline_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings);
  virtual std::string get_name() const { return "timeline"; }
  virtual void execute(Resource_Manager& rman);

  static Generic_Statement_Maker< Timeline_Statement > statement_maker;

  static std::string to_string(int type)
  {
    if (type == Statement::NODE)
      return "node";
    else if (type == Statement::WAY)
      return "way";

    return "relation";
  }

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<timeline" + std::string(" type=\"") + to_string(type) + "\""
        " ref=\"" + ::to_string(ref) + "\"";
    if (version)
      result += " version=\"" + ::to_string(version) + "\"";

    return result + dump_xml_result_name() + "/>\n";
  }

  virtual std::string dump_compact_ql(const std::string& indent) const
  {
    std::string result = "timeline(" + to_string(type) + "," + ::to_string(ref);
    if (version)
      result += "," + ::to_string(version);

    return result + ")" + dump_ql_result_name() + ";";
  }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  {
    std::string result = indent + "timeline(" + to_string(type) + ", " + ::to_string(ref);
    if (version)
      result += ", " + ::to_string(version);

    return result + ")" + dump_ql_result_name() + ";";
  }

private:
    int type;
    uint64 ref;
    uint32 version;
};


#endif
