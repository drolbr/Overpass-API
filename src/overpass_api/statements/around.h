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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "../data/collect_members.h"
#include "../data/utils.h"
#include "../data/way_geometry_store.h"
#include "statement.h"


struct Prepared_Segment
{
  double first_lat;
  double first_lon;
  double second_lat;
  double second_lon;
  std::vector< double > first_cartesian;
  std::vector< double > second_cartesian;
  std::vector< double > norm;

  Prepared_Segment(double first_lat, double first_lon, double second_lat, double second_lon);
};


struct Prepared_Point
{
  double lat;
  double lon;
  std::vector< double > cartesian;

  Prepared_Point(double lat, double lon);
};


class Around_Statement : public Output_Statement
{
  public:
    Around_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings);
    virtual std::string get_name() const { return "around"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Around_Statement();
    
    struct Statement_Maker : public Generic_Statement_Maker< Around_Statement >
    {
      virtual bool can_standalone(const std::string& type) { return type == "node"; }
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
      Statement_Maker() : Generic_Statement_Maker< Around_Statement >("around")
      { Statement::maker_by_ql_criterion()["around"] = this; }
    };
    static Statement_Maker statement_maker;

    virtual Query_Constraint* get_query_constraint();

    std::string get_source_name() const { return input; }

    std::set< std::pair< Uint32_Index, Uint32_Index > > calc_ranges
        (const Set& input_nodes, Resource_Manager& rman) const;

    void calc_lat_lons(const Set& input_nodes, Statement& query, Resource_Manager& rman);

    bool is_inside(double lat, double lon) const;
    bool is_inside(double first_lat, double first_lon, double second_lat, double second_lon) const;
    bool is_inside(const std::vector< Quad_Coord >& way_geometry) const;

    double get_radius() const { return radius; }

    template< typename Node_Skeleton >
    void add_nodes(const std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes);

    template< typename Way_Skeleton >
    void add_ways(const std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
		  const Way_Geometry_Store& way_geometries);

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<around"
          + (input != "_" ? std::string(" from=\"") + input + "\"" : "")
          + std::string(" radius=\"") + to_string(radius) + "\""
          + (lat != 100. ? std::string(" lat=\"") + to_string(lat) + "\"" : "")
          + (lon != 200. ? std::string(" lon=\"") + to_string(lon) + "\"" : "")
          + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return "node" + dump_ql_in_query("") + dump_ql_result_name();
    }
    virtual std::string dump_ql_in_query(const std::string&) const
    {
      return std::string("(around")
          + (input != "_" ? std::string(".") + input : "")
          + std::string(":") + to_string(radius)
          + (lat != 100. ? std::string(",") + to_string(lat) : "")
          + (lon != 200. ? std::string(",") + to_string(lon) : "")
          + ")";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }

  private:
    std::string input;
    double radius;
    double lat;
    double lon;
    std::map< Uint32_Index, std::vector< std::pair< double, double > > > radius_lat_lons;
    std::vector< Prepared_Point > simple_lat_lons;
    std::vector< Prepared_Segment > simple_segments;
    std::vector< Query_Constraint* > constraints;
};

#endif
