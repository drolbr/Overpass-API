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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H

#include <map>
#include <string>
#include <vector>
#include "../data/collect_members.h"
#include "../data/utils.h"
#include "../frontend/output_handler.h"
#include "statement.h"


class Collection_Print_Target;


class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_, const map< string, string >& attributes, Parsed_Query& global_settings);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Print_Statement();

    static Generic_Statement_Maker< Print_Statement > statement_maker;
    
    void set_collect_lhs();
    void set_collect_rhs(bool add_deletion_information);
    
    static std::string mode_string_xml(Output_Mode mode)
    {
      if ((mode & (Output_Mode::VERSION | Output_Mode::META)) == (Output_Mode::VERSION | Output_Mode::META))
        return " mode=\"meta\"";
      else if ((mode & (Output_Mode::MEMBERS | Output_Mode::TAGS)) == (Output_Mode::MEMBERS | Output_Mode::TAGS))
        return "";
      else if ((mode & Output_Mode::TAGS) == Output_Mode::TAGS)
        return " mode=\"tags\"";
      else if ((mode & Output_Mode::MEMBERS) == Output_Mode::MEMBERS)
        return " mode=\"skeleton\"";
      else if ((mode & Output_Mode::ID) == Output_Mode::ID)
        return " mode=\"ids_only\"";
      
      return " mode=\"count\"";
    }
    
    static std::string mode_string_ql(Output_Mode mode)
    {
      if ((mode & (Output_Mode::VERSION | Output_Mode::META)) == (Output_Mode::VERSION | Output_Mode::META))
        return " meta";
      else if ((mode & (Output_Mode::MEMBERS | Output_Mode::TAGS)) == (Output_Mode::MEMBERS | Output_Mode::TAGS))
        return "";
      else if ((mode & Output_Mode::TAGS) == Output_Mode::TAGS)
        return " tags";
      else if ((mode & Output_Mode::MEMBERS) == Output_Mode::MEMBERS)
        return " skel";
      else if ((mode & Output_Mode::ID) == Output_Mode::ID)
        return " ids";
      
      return " count";
    }
    
    static std::string geometry_string_xml(Output_Mode mode)
    {
      if ((mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
          == (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
        return " geometry=\"full\"";
      else if ((mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
          == (Output_Mode::BOUNDS | Output_Mode::CENTER))
        return " geometry=\"bounds\"";
      else if ((mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
          == Output_Mode::CENTER)      
        return " geometry=\"center\"";
      
      return "";
    }
    
    static std::string geometry_string_ql(Output_Mode mode)
    {
      if ((mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
          == (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
        return " geom";
      else if ((mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
          == (Output_Mode::BOUNDS | Output_Mode::CENTER))
        return " bb";
      else if ((mode & (Output_Mode::GEOMETRY | Output_Mode::BOUNDS | Output_Mode::CENTER))
          == Output_Mode::CENTER)      
        return " center";
      
      return "";
    }
    
    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<print"
          + (input != "_" ? std::string(" from=\"") + input + "\"" : "")
          + mode_string_xml(mode)
          + (order == order_by_id ? "" : " order=\"quadtile\"")
          + (limit == numeric_limits< unsigned int >::max() ? "" : " limit=\"" + ::to_string(limit) + "\"")
          + geometry_string_xml(mode)
          + (south > north ? "" : " s=\"" + to_string(south) + "\"")
          + (south > north ? "" : " w=\"" + to_string(west) + "\"")
          + (south > north ? "" : " n=\"" + to_string(north) + "\"")
          + (south > north ? "" : " e=\"" + to_string(east) + "\"")
          + "/>\n";
    }
  
    virtual std::string dump_compact_ql(const std::string& indent) const { return dump_subquery_map_ql(indent, false); }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_subquery_map_ql(indent, true); }
    
    std::string dump_subquery_map_ql(const std::string& indent, bool pretty) const
    {
      return indent + (input != "_" ? "." + input + " " : "") + "out"
          + mode_string_ql(mode)
          + (order == order_by_id ? "" : " qt")
          + (limit == numeric_limits< unsigned int >::max() ? "" : " " + ::to_string(limit))
          + geometry_string_ql(mode)
          + (south > north ? "" : "(" + to_string(south) + "," + to_string(west) + ","
              + to_string(north) + "," + to_string(east) + ")");
    }
    
  private:
    string input;
    Output_Mode mode;
    enum { order_by_id, order_by_quadtile } order;
    unsigned int limit;
    Collection_Print_Target* collection_print_target;
    enum { dont_collect, collect_lhs, collect_rhs } collection_mode;
    bool add_deletion_information;
    
    double south;
    double north;
    double west;
    double east;

    virtual void execute_comparison(Resource_Manager& rman);
};


#endif
