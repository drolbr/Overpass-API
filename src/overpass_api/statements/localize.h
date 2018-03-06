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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H


#include "set_prop.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


/* == The statement <em>local</em> ==

The statement <em>local</em> converts the given input into the localized representation of OSM data.
The output parameter controls which classes of data are included.

Without a type parameter local delivers geometry and tags.
This is one object per tagged node
plus one object per part of a way
plus one object per part of a relation geometry.

With the type parameter "ll", it delivers additionally loose objects.
There are one object per way with tags but without node members
and one object per relation member of any relation without node or way members.

With the type parameter "llb", it delivers even more data:
It delivers the objects that associate the OSM element ids to the aforementioned objects.

The base syntax is

  local <Type>

where <Type> is empty, "ll", or "llb". You can also specify other input and/or output sets than "_":

  .<Set> local <Type> ->.<Set>

The first set is the input set,
the second set is the output set.
*/

class Localize_Statement : public Output_Statement
{
public:
  enum Mode { data, also_loose, all };

  Localize_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "localize"; }
  virtual void execute(Resource_Manager& rman);
  static Generic_Statement_Maker< Localize_Statement > statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<localize"
        + (input != "_" ? " from=\"" + input + "\"" : "");
    if (south <= 90.)
      result += " s=\"" + to_string(south) + "\""
          + " w=\"" + to_string(west) + "\""
          + " n=\"" + to_string(north) + "\""
          + " e=\"" + to_string(east) + "\"";
    return result + dump_xml_result_name()
        + (type == data ? "" : (std::string(" type=\"") + (type == also_loose ? "ll" : "llb") + "\"")) + "/>\n";
  }

  virtual std::string dump_compact_ql(const std::string& indent) const
  {
    std::string result = indent + (input == "_" ? "" : "." + input + " ") + "local"
        + (type == data ? "" : (type == also_loose ? " ll" : " llb"));
    if (south <= 90.)
      result += std::string("(")
          + to_string(south)
          + "," + to_string(west)
          + "," + to_string(north)
          + "," + to_string(east)
          + ")";
    return result + dump_ql_result_name();
  }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  {
    return dump_compact_ql(indent);
  }

private:
  std::string input;
  double south, north, west, east;
  Mode type;
};


#endif
