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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__CHANGED_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__CHANGED_H

#include <map>
#include <string>
#include <vector>
#include "../data/utils.h"
#include "../frontend/basic_formats.h"
#include "statement.h"


class Changed_Statement : public Output_Statement
{
  public:
    Changed_Statement(int line_number_, const map< string, string >& attributes,
                       Parsed_Query& global_settings);
    virtual string get_name() const { return "changed"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Changed_Statement();
    
    static Generic_Statement_Maker< Changed_Statement > statement_maker;

    virtual Query_Constraint* get_query_constraint();
    uint64 get_since(Resource_Manager& rman) const;
    uint64 get_until(Resource_Manager& rman) const;
    
    static bool area_query_exists() { return area_query_exists_; }
    bool trivial() const { return behave_trivial; }
  
    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<changed"
          + (since != NOW ? std::string(" since=\"") + iso_string(since) + "\"" : "")
          + (until != NOW ? std::string(" until=\"") + iso_string(until) + "\"" : "")
          + dump_xml_result_name() + "/>\n";
    }
  
    virtual std::string dump_compact_ql(const std::string&) const
    {
      return std::string("(changed:")
          + (since != NOW ? std::string("\"") + iso_string(since) + "\"" : "")
          + (until != NOW ? std::string(",\"") + iso_string(until) + "\"" : "")
          + ")" + dump_ql_result_name();
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_compact_ql(indent); }
    
  private:
    uint64 since, until;
    vector< Query_Constraint* > constraints;
    bool behave_trivial;
    
    static bool area_query_exists_;
};

#endif
