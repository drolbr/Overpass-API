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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COMPLETE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COMPLETE_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"


class Complete_Statement : public Output_Statement
{
  public:
  Complete_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings);
    virtual void add_statement(Statement* statement, std::string text);
    virtual std::string get_name() const { return "complete"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Complete_Statement() {}
    
    static Generic_Statement_Maker< Complete_Statement > statement_maker;
    
    virtual std::string dump_xml(const std::string& indent) const
    {
      std::string result = indent + "<complete"
          + (input != "_" ? " from=\"" + input + "\"" : "")
          + (output_iteration != "_" ? " into=\"" + output_iteration + "\"" : "") + ">\n";

      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += *it ? (*it)->dump_xml(indent + "  ") : "";

      // TODO: "-> .output_complete" is still missing
      return result + indent + "</complete>\n";
    }

    virtual std::string dump_compact_ql(const std::string& indent) const
    {
      std::string result = indent + "complete"
          + (input != "_" ? "." + input : "") + (output_iteration != "_" ? "->." + output_iteration : "") + "(";

      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += (*it)->dump_compact_ql(indent) + ";";
      result += ")";
      // TODO: "-> .output_complete" is still missing

      return result;
    }

    virtual std::string dump_pretty_ql(const std::string& indent) const
    {
      std::string result = indent + "complete"
          + (input != "_" ? "." + input : "") + (output_iteration != "_" ? "->." + output_iteration : "") + "(";

      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += "\n" + (*it)->dump_pretty_ql(indent + "  ") + ";";
      result += "\n)";
      // TODO: "-> .output_complete" is still missing

      return result;
    }

  private:
    std::string input, output_iteration, output_complete;
    std::vector< Statement* > substatements;
};

#endif
