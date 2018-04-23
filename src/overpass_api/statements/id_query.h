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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__ID_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__ID_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "../data/utils.h"
#include "statement.h"


class Id_Query_Statement : public Output_Statement
{
  public:
    Id_Query_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                       Parsed_Query& global_settings);
    virtual std::string get_name() const { return "id-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Id_Query_Statement();

    struct Statement_Maker : public Generic_Statement_Maker< Id_Query_Statement >
    {
      Statement_Maker() : Generic_Statement_Maker< Id_Query_Statement >("id-query") {}
    };
    static Statement_Maker statement_maker;

    struct Criterion_Maker : public Statement::Criterion_Maker
    {
      virtual bool can_standalone(const std::string& type) { return type != "nwr"; }
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
      Criterion_Maker() { Statement::maker_by_ql_criterion()["id"] = this; }
    };
    static Criterion_Maker criterion_maker;

    virtual Query_Constraint* get_query_constraint();

    const std::vector< uint64 >& get_refs() { return refs; }
    int get_type() const { return type; }

    static bool area_query_exists() { return area_query_exists_; }

    static std::string to_string(int type)
    {
      if (type == Statement::NODE)
        return "node";
      else if (type == Statement::WAY)
        return "way";
      else if (type == Statement::RELATION)
        return "relation";

      return "area";
    }

    virtual std::string dump_xml(const std::string& indent) const
    {
      std::vector< uint64 >::const_iterator it = refs.begin();

      std::string result = indent + "<id-query" + std::string(" type=\"") + to_string(type) + "\"";
      if (!refs.empty())
        result += " ref=\"" + ::to_string(refs[0]) + "\"";
      for (uint i = 1; i < refs.size(); ++i)
        result += " ref_" + ::to_string(i) + "=\"" + ::to_string(refs[i]) + "\"";

      return result + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return to_string(type) + dump_ql_in_query("") + dump_ql_result_name() + ";";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }
    virtual std::string dump_ql_in_query(const std::string& indent) const
    {
      std::vector< uint64 >::const_iterator it = refs.begin();

      std::string result = "(";
      if (it != refs.end())
        result += ::to_string(*it++);
      while (it != refs.end())
        result += "," + ::to_string(*it++);
      result += ")";

      return result;
    }

  private:
    int type;

    std::vector< uint64 > refs;
    std::vector< Query_Constraint* > constraints;

    static bool area_query_exists_;
};

#endif
