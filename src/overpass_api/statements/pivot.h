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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AREA_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AREA_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "../data/utils.h"
#include "statement.h"


class Pivot_Statement : public Output_Statement
{
  public:
    Pivot_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                    Parsed_Query& global_settings);
    virtual std::string get_name() const { return "pivot"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Pivot_Statement();

    struct Statement_Maker : public Generic_Statement_Maker< Pivot_Statement >
    {
      Statement_Maker() : Generic_Statement_Maker< Pivot_Statement >("pivot") {}
    };
    static Statement_Maker statement_maker;
    
    struct Criterion_Maker : public Statement::Criterion_Maker
    {
      virtual bool can_standalone(const std::string& type) { return type == "node"; }
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
      Criterion_Maker() { Statement::maker_by_ql_criterion()["pivot"] = this; }
    };
    static Criterion_Maker criterion_maker;

    virtual Query_Constraint* get_query_constraint();
    std::string get_input() const { return input; }

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<pivot"
          + (input != "_" ? std::string(" from=\"") + input + "\"" : "")
          + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return "node" + dump_ql_in_query("") + dump_ql_result_name();
    }
    virtual std::string dump_ql_in_query(const std::string&) const
    {
      return std::string("(pivot")
          + (input != "_" ? std::string(".") + input : "")
          + ")";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }

  private:
    std::string input;
    std::vector< Query_Constraint* > constraints;
};

#endif
