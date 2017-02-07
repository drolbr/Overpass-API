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
    Bbox_Query_Statement(int line_number_, const map< string, string >& attributes,
                         Parsed_Query& global_settings);
    Bbox_Query_Statement(const Bbox_Double& bbox);
    virtual string get_name() const { return "bbox-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Bbox_Query_Statement();    
    static Generic_Statement_Maker< Bbox_Query_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    const set< pair< Uint32_Index, Uint32_Index > >& get_ranges_32();
    const set< pair< Uint31_Index, Uint31_Index > >& get_ranges_31();
    
    double get_south() const { return south; }
    double get_north() const { return north; }
    double get_west() const { return west; }
    double get_east() const { return east; }
    bool matches_bbox(double lat, double lon) const;
  
    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<bbox-query"
          + " south=\"" + to_string(south) + "\""
          + " west=\"" + to_string(west) + "\""
          + " north=\"" + to_string(north) + "\""
          + " east=\"" + to_string(east) + "\""
          + dump_xml_result_name() + "/>\n";
    }
  
    virtual std::string dump_compact_ql(const std::string&) const
    {
      return std::string("(")
          + to_string(south)
          + "," + to_string(west)
          + "," + to_string(north)
          + "," + to_string(east)
          + ")" + dump_ql_result_name();
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_compact_ql(indent); }

  private:
    double south, north, west, east;
    set< pair< Uint32_Index, Uint32_Index > > ranges_32;
    set< pair< Uint31_Index, Uint31_Index > > ranges_31;
    vector< Query_Constraint* > constraints;
};


inline bool Bbox_Query_Statement::matches_bbox(double lat, double lon) const
{
  return ((lat >= south - 1e-8) && (lat <= north + 1e-8) &&
      (((lon >= west - 1e-8) && (lon <= east + 1e-8)) ||
          ((east < west) && ((lon >= west - 1e-8) || (lon <= east + 1e-8)))));
}


#endif
