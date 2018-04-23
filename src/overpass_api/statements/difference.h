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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__DIFFERENCE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__DIFFERENCE_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"

class Difference_Statement : public Output_Statement
{
  public:
    Difference_Statement(int line_number_, const std::map< std::string, std::string >& input_attributes,
                         Parsed_Query& global_settings);
    virtual void add_statement(Statement* statement, std::string text);
    virtual std::string get_name() const { return "difference"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Difference_Statement() {}

    static Generic_Statement_Maker< Difference_Statement > statement_maker;

    virtual std::string dump_xml(const std::string& indent) const
    {
      std::string result = indent + "<difference" + dump_xml_result_name() + ">\n";

      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += *it ? (*it)->dump_xml(indent + "  ") : "";

      return result + indent + "</difference>\n";
    }

    virtual std::string dump_compact_ql(const std::string& indent) const
    {
      std::string result = "(";

      std::vector< Statement* >::const_iterator it = substatements.begin();
      if (it != substatements.end())
      {
        result += (*it)->dump_compact_ql(indent) + ";";
        for (++it; it != substatements.end(); ++it)
          result += "-" + (*it)->dump_compact_ql(indent) + ";";
      }
      result += ")";

      return result + dump_ql_result_name() + ";";
    }

    virtual std::string dump_pretty_ql(const std::string& indent) const
    {
      std::string result = indent + "(";

      std::vector< Statement* >::const_iterator it = substatements.begin();
      if (it != substatements.end())
      {
        result += "\n" + (*it)->dump_pretty_ql(indent + "  ") + ";";
        for (++it; it != substatements.end(); ++it)
          result += "\n" + indent + "  -\n" + (*it)->dump_pretty_ql(indent + "  ") + ";";
      }
      result += "\n)";

      return result + dump_ql_result_name() + ";";
    }

  private:
    std::vector< Statement* > substatements;
};


#endif
