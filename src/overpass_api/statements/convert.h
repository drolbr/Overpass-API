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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__CONVERT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__CONVERT_H


#include "set_prop.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Set_Prop_Statement;


/* == The statement <em>convert</em> ==

The statement <em>convert</em> produces per element in its input set one derived element.
The content of this output element is controlled by the parameters of the statement.

It is necessary to set a fixed type as type for all the generated elements.
After that, an arbitrary number of tags can be set.
In addition, it can be specified to copy all keys from the originating object.
In this case, it is also possible to selectively suppress some tags.

Finally, it is possible to explicitly set the id of the generated objects.
If you do not set an id then an unique id from a global ascending counter is assigned.

The base syntax is

  convert <Type> <List of Tags>

where <List of Tags> is a comma separated list of items,
each of which must be one the following

  <Key> = <Evaluator>
  ::id = <Evaluator>
  :: = <Evaluator>
  !<Key>
*/

class Convert_Statement : public Output_Statement
{
public:
  Convert_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                   Parsed_Query& global_settings);
  virtual std::string get_name() const { return "convert"; }
  virtual void add_statement(Statement* statement, std::string text);
  virtual void execute(Resource_Manager& rman);
  virtual ~Convert_Statement();
  static Generic_Statement_Maker< Convert_Statement > statement_maker;

  std::string get_source_name() const { return input; }

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<convert"
          + (input != "_" ? " from=\"" + input + "\"" : "")
          + dump_xml_result_name() + " type=\"" + type;
    if (evaluators.empty())
      return result + "\"/>\n";
    result += "\">\n";

    for (std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin(); it != evaluators.end(); ++it)
      result += *it ? (*it)->dump_xml(indent + "  ") : "";
    return result + "</convert>\n";
  }

  virtual std::string dump_compact_ql(const std::string& indent) const
  {
    std::string result = indent + (input == "_" ? "" : "." + input + " ")
        + "convert " + type;
    std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin();
    if (it != evaluators.end())
    {
      result += " " + (*it ? (*it)->dump_compact_ql(indent + "  ") : "");
      ++it;
    }
    for (; it != evaluators.end(); ++it)
      result += "," + (*it ? (*it)->dump_compact_ql(indent + "  ") : "");
    return result + dump_ql_result_name();
  }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  {
    std::string result = indent + (input == "_" ? "" : "." + input + " ")
        + "convert " + type;
    std::vector< Set_Prop_Statement* >::const_iterator it = evaluators.begin();
    if (it != evaluators.end())
    {
      result += "\n  " + indent + (*it ? (*it)->dump_pretty_ql(indent + "  ") : "");
      ++it;
    }
    for (; it != evaluators.end(); ++it)
      result += ",\n  " + indent + (*it ? (*it)->dump_pretty_ql(indent + "  ") : "");
    return result + dump_ql_result_name();
  }

private:
  std::string input;
  std::string type;
  std::vector< Set_Prop_Statement* > evaluators;
  Set_Prop_Statement* id_evaluator;
  Set_Prop_Statement* multi_evaluator;
};


#endif
