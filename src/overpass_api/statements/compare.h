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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COMPARE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COMPARE_H


#include "statement.h"


class Set_Comparison;


/* === The statement <em>compare</em> ===

''since v0.7.55''

The statement <em>compare</em> computes the diff of the data of two timestamps.
That diff can consist of any elements as well as only those with specific properties.

It can only be used in diff mode.
In other modes its behaviour is undefined,
and in future versions it might be a syntax error to have it elsewhere.

In the first run of a diff query it returns an empty set.
In the second run of a diff query it returns the difference of the elements.
Currently, the only purpose of such a difference is to feed it into an output statement.

The base syntax is

  compare;

In addition, an input and/or output set can be specified:

  .<Set> compare->.<Set>;

*/

class Compare_Statement : public Output_Statement
{
public:
  Compare_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings);
  virtual ~Compare_Statement();
  virtual std::string get_name() const { return "compare"; }
  virtual void execute(Resource_Manager& rman);

  virtual void set_collect_lhs();
  virtual void set_collect_rhs(bool add_deletion_information);
    
  static Generic_Statement_Maker< Compare_Statement > statement_maker;

  virtual std::string dump_xml(const std::string& indent) const
  {
    std::string result = indent + "<compare"
      + (input != "_" ? std::string(" from=\"") + input + "\"" : "");
      
    return result + dump_xml_result_name() + "/>\n";
  }

  virtual std::string dump_compact_ql(const std::string& indent) const
  {
    return (input != "_" ? std::string(".") + input + " " : "")
        + "compare" + dump_ql_result_name();
  }

  virtual std::string dump_pretty_ql(const std::string& indent) const
  {
    return indent + (input != "_" ? std::string(".") + input + " " : "")
        + "compare" + dump_ql_result_name();
  }

private:
  std::string input;
  Set_Comparison* set_comparison;
  enum { dont_collect, collect_lhs, collect_rhs } collection_mode;
  bool add_deletion_information;
};


#endif
