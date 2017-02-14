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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__FOREACH_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__FOREACH_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"

using namespace std;

class Foreach_Statement : public Statement
{
  public:
    Foreach_Statement(int line_number_, const map< string, string >& attributes,
                      Parsed_Query& global_settings);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "foreach"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Foreach_Statement() {}
    
    static Generic_Statement_Maker< Foreach_Statement > statement_maker;
    
    virtual std::string dump_xml(const std::string& indent) const
    {
      std::string result = indent + "<foreach"
          + (input != "_" ? " from=\"" + input + "\"" : "")
          + (output != "_" ? " into=\"" + output + "\"" : "") + ">\n";
      
      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += *it ? (*it)->dump_xml(indent + "  ") : "";
      
      return result + indent + "</foreach>\n";
    }
  
    virtual std::string dump_compact_ql(const std::string& indent) const
    {
      std::string result = indent + "foreach"
          + (input != "_" ? "." + input : "") + (output != "_" ? "->." + output : "") + "(";
  
      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += (*it)->dump_compact_ql(indent) + ";";
      result += ")";
  
      return result;
    }
    
    virtual std::string dump_pretty_ql(const std::string& indent) const
    {
      std::string result = indent + "foreach"
          + (input != "_" ? "." + input : "") + (output != "_" ? "->." + output : "") + "(";
    
      for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
        result += "\n" + (*it)->dump_pretty_ql(indent + "  ") + ";";
      result += "\n)";
  
      return result;
    }
    
  private:
    string input, output;
    vector< Statement* > substatements;
};

#endif
