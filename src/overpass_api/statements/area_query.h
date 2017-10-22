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

#include "../data/collect_members.h"
#include "../data/utils.h"
#include "../data/way_geometry_store.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Area_Query_Statement : public Output_Statement
{
  public:
    Area_Query_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                         Parsed_Query& global_settings);
    virtual std::string get_name() const { return "area-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Area_Query_Statement();
    
    struct Statement_Maker : public Generic_Statement_Maker< Area_Query_Statement >
    {
      Statement_Maker() : Generic_Statement_Maker< Area_Query_Statement >("area-query") {}
    };
    static Statement_Maker statement_maker;
    
    struct Criterion_Maker : public Statement::Criterion_Maker
    {
      virtual bool can_standalone(const std::string& type) { return type == "node"; }
      virtual Statement* create_criterion(const Token_Node_Ptr& tree_it,
          const std::string& type, const std::string& into,
          Statement::Factory& stmt_factory, Parsed_Query& global_settings, Error_Output* error_output);
      Criterion_Maker() { Statement::maker_by_ql_criterion()["area"] = this; }
    };
    static Criterion_Maker criterion_maker;

    virtual Query_Constraint* get_query_constraint();

    void get_ranges
      (std::set< std::pair< Uint32_Index, Uint32_Index > >& nodes_req,
       std::set< Uint31_Index >& area_block_req,
       Resource_Manager& rman);

    void get_ranges
      (const std::map< Uint31_Index, std::vector< Area_Skeleton > >& input_areas,
       std::set< std::pair< Uint32_Index, Uint32_Index > >& nodes_req,
       std::set< Uint31_Index >& area_block_req,
       Resource_Manager& rman);

    void collect_nodes
      (const std::set< std::pair< Uint32_Index, Uint32_Index > >& nodes_req,
       const std::set< Uint31_Index >& req,
       std::vector< Node::Id_Type >* ids,
       std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
       Resource_Manager& rman);

    template< typename Node_Skeleton >
    void collect_nodes
      (std::map< Uint32_Index, std::vector< Node_Skeleton > >& nodes,
       const std::set< Uint31_Index >& req, bool add_border,
       Resource_Manager& rman);

    template< typename Way_Skeleton >
    void collect_ways
      (const Way_Geometry_Store& way_geometries,
       std::map< Uint31_Index, std::vector< Way_Skeleton > >& ways,
       const std::set< Uint31_Index >& req, bool add_border,
       const Statement& query, Resource_Manager& rman);

    bool areas_from_input() const { return (submitted_id == 0); }
    std::string get_input() const { return input; }

    static bool is_used() { return is_used_; }

    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<area-query"
          + (input != "_" ? std::string(" from=\"") + input + "\"" : "")
          + (submitted_id > 0 ? std::string(" ref=\"") + to_string(submitted_id) + "\"" : "")
          + dump_xml_result_name() + "/>\n";
    }

    virtual std::string dump_compact_ql(const std::string&) const
    {
      return "node" + dump_ql_in_query("") + dump_ql_result_name();
    }
    virtual std::string dump_ql_in_query(const std::string&) const
    {
      return std::string("(area")
          + (input != "_" ? std::string(".") + input : "")
          + (submitted_id > 0 ? std::string(":") + to_string(submitted_id) : "")
          + ")";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return indent + dump_compact_ql(indent); }

  private:
    std::string input;
    long long submitted_id;
    std::vector< Area_Skeleton::Id_Type > area_id;
    static bool is_used_;
    std::vector< Query_Constraint* > constraints;
};


int intersects_inner(const Area_Block& string_a, const Area_Block& string_b);

void has_inner_points(const Area_Block& string_a, const Area_Block& string_b, int& inside);

#endif
