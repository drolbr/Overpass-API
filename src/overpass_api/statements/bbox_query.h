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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__BBOX_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__BBOX_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "../data/utils.h"
#include "statement.h"


class Bbox_Query_Statement : public Output_Statement
{
  public:
    Bbox_Query_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                         Parsed_Query& global_settings);
    Bbox_Query_Statement(const Bbox_Double& bbox);
    virtual std::string get_name() const { return "bbox-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Bbox_Query_Statement();

    struct Statement_Maker : public Generic_Statement_Maker< Bbox_Query_Statement >
    {
      Statement_Maker() : Generic_Statement_Maker< Bbox_Query_Statement >("bbox-query") {}
    };
    static Statement_Maker statement_maker;

    struct Criterion_Maker : public Statement::Criterion_Maker
    {
      virtual bool can_standalone(const std::string& type) { return type == "node"; }
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
      Criterion_Maker() { Statement::maker_by_ql_criterion()["bbox"] = this; }
    };
    static Criterion_Maker criterion_maker;

    virtual Query_Constraint* get_query_constraint();

    const std::set< std::pair< Uint32_Index, Uint32_Index > >& get_ranges_32();
    const std::set< std::pair< Uint31_Index, Uint31_Index > >& get_ranges_31();

    double get_south() const { return south; }
    double get_north() const { return north; }
    double get_west() const { return west; }
    double get_east() const { return east; }
    bool matches_bbox(double lat, double lon) const;

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<bbox-query"
          + " s=\"" + to_string(south) + "\""
          + " w=\"" + to_string(west) + "\""
          + " n=\"" + to_string(north) + "\""
          + " e=\"" + to_string(east) + "\""
          + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return "node" + dump_ql_in_query("") + dump_ql_result_name() + ";";
    }
    virtual std::string dump_ql_in_query(const std::string&) const
    {
      return std::string("(")
          + to_string(south)
          + "," + to_string(west)
          + "," + to_string(north)
          + "," + to_string(east)
          + ")";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }

  private:
    double south, north, west, east;
    std::set< std::pair< Uint32_Index, Uint32_Index > > ranges_32;
    std::set< std::pair< Uint31_Index, Uint31_Index > > ranges_31;
    std::vector< Query_Constraint* > constraints;
};


inline bool Bbox_Query_Statement::matches_bbox(double lat, double lon) const
{
  return ((lat >= south - 1e-8) && (lat <= north + 1e-8) &&
      (((lon >= west - 1e-8) && (lon <= east + 1e-8)) ||
          ((east < west) && ((lon >= west - 1e-8) || (lon <= east + 1e-8)))));
}


#endif
