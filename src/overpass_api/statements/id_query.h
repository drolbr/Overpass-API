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

    static Generic_Statement_Maker< Id_Query_Statement > statement_maker;

    virtual Query_Constraint* get_query_constraint();
    virtual const std::set<Uint64::Id_Type> & get_ref_ids() { return ref_ids; }
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
      return indent + "<id-query"
          + std::string(" type=\"") + to_string(type) + "\""
          + (lower.val() == upper.val()-1 ? std::string(" ref=\"") + ::to_string(lower.val()) + "\"" : "")
          + (lower.val() != upper.val()-1 ? std::string(" lower=\"") + ::to_string(lower.val()) + "\"" : "")
          + (lower.val() != upper.val()-1 ? std::string(" upper=\"") + ::to_string(upper.val()-1) + "\"" : "")
          + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return to_string(type) + dump_ql_in_query("") + dump_ql_result_name();
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }
    virtual std::string dump_ql_in_query(const std::string& indent) const
    {
      return std::string("(")
          + (lower.val() == upper.val()-1 ? ::to_string(lower.val()) : "")
          + ")";
    }

  private:
    int type;

    std::set<Uint64::Id_Type> ref_ids;
    std::vector< Query_Constraint* > constraints;
    Uint64 ref, lower, upper;

    static bool area_query_exists_;
};

#endif
