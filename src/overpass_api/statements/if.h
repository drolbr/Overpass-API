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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COMPLETE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COMPLETE_H

#include <map>
#include <string>
#include <vector>

#include "evaluator.h"
#include "statement.h"


/* === The block statement <em>if</em> ===

''since v0.7.55''

The block statement <em>if</em> executes its substatements
only if its condition evaluates to boolean true.
This allows e.g. to try more loosely search conditions
if stricter search conditions did not deliver a result.

The statement does not directly interact with any sets.

The base syntax is

  if (<Evaluator>)
  {
    <List of Substatements>
  };

resp.

  if (<Evaluator>)
  {
    <List of Substatements>
  }
  else
  {
    <List of Substatements>
  };

where <Evaluator> is an evaulator and <List of Substatements> is a list of substatements.

*/

class If_Statement : public Statement
{
public:
  If_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings);
  virtual void add_statement(Statement* statement, std::string text);
  virtual std::string get_name() const { return "if"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman);
  virtual ~If_Statement() {}

  struct Statement_Maker : public Generic_Statement_Maker< If_Statement >
  {
    Statement_Maker() : Generic_Statement_Maker< If_Statement >("if") {}
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<if>\n"
          + (criterion ? criterion->dump_xml(indent + "  ") : "");

    for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
      result += *it ? (*it)->dump_xml(indent + "  ") : "";

    if (!else_statements.empty())
    {
      result = indent + "  <else/>\n";
      for (std::vector< Statement* >::const_iterator it = else_statements.begin();
          it != else_statements.end(); ++it)
        result += *it ? (*it)->dump_xml(indent + "  ") : "";
    }

    return result + indent + "</if>\n";
  }

  virtual std::string dump_compact_ql(const std::string& indent) const
  {
    std::string result = indent + "if(" + (criterion ? criterion->dump_compact_ql("") : "") + "){";

    for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
      result += (*it)->dump_compact_ql(indent) + ";";

    if (!else_statements.empty())
    {
      result += "}else{";
      for (std::vector< Statement* >::const_iterator it = else_statements.begin();
          it != else_statements.end(); ++it)
        result += (*it)->dump_compact_ql(indent);
    }
    result += "}";

    return result;
  }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  {
    std::string result = indent + "if (" + (criterion ? criterion->dump_compact_ql("") : "") + ")\n"
        + indent + "{";

    for (std::vector< Statement* >::const_iterator it = substatements.begin(); it != substatements.end(); ++it)
      result += "\n" + (*it)->dump_pretty_ql(indent + "  ") + ";";

    if (!else_statements.empty())
    {
      result += "\n" + indent + "}\n" + indent + "else\n" + indent + "{\n";
      for (std::vector< Statement* >::const_iterator it = else_statements.begin();
          it != else_statements.end(); ++it)
        result += (*it)->dump_compact_ql(indent + "  ");
    }
    result += "\n" + indent + "}";

    return result;
  }

private:
  Evaluator* criterion;
  bool else_reached;
  std::vector< Statement* > substatements;
  std::vector< Statement* > else_statements;
};


class Else_Statement : public Statement
{
public:
  Else_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings) : Statement(line_number_) {}
  virtual std::string get_name() const { return "else"; }
  virtual std::string get_result_name() const { return ""; }
  virtual void execute(Resource_Manager& rman) {}

  struct Statement_Maker : public Generic_Statement_Maker< Else_Statement >
  {
    Statement_Maker() : Generic_Statement_Maker< Else_Statement >("else") {}
  };
  static Statement_Maker statement_maker;

  virtual std::string dump_xml(const std::string& indent) const { return indent + "<else/>\n"; }
  virtual std::string dump_compact_ql(const std::string& indent) const { return "else\n"; }
  virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + "else\n"; }
};


#endif
