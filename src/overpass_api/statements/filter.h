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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__FILTER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__FILTER_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"
#include "tag_value.h"


/* == The Query Filter ==

The query filter can be added as condition to a query statement.
It has an evaluator as argument
and it lets pass only those elements for which the expression returns boolean true.

At the moment, the query filter cannot be the only condition in a query.
This is due to implementation reasons and will change in future versions.

It is technically possible to have multiple query filters in a single query.
But it does not make sense:
Their evaulators can be combined with a conjunction in a single query filter.
This has the same semantics and is faster.

Its syntax is

  (if: <Evaluator>)
  
The whitespace is optional.
*/

class Filter_Statement : public Output_Statement
{
  public:
    Filter_Statement(int line_number_, const map< string, string >& attributes,
                         Parsed_Query& global_settings);
    virtual string get_name() const { return "filter"; }
    virtual void add_statement(Statement* statement, string text);
    virtual void execute(Resource_Manager& rman);
    virtual ~Filter_Statement();    
    static Generic_Statement_Maker< Filter_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    Evaluator* get_criterion() { return criterion; }
  
    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<filter>\n"
          + (criterion ? criterion->dump_xml(indent + "  ") : "")
          + indent + "</filter>\n";
    }
  
    virtual std::string dump_compact_ql(const std::string&) const
    {
      return std::string("(if:") + (criterion ? criterion->dump_compact_ql("") : "") + ")";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_compact_ql(indent); }

  private:
    vector< Query_Constraint* > constraints;
    Evaluator* criterion;
};


#endif
