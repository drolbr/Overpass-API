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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__NEWER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__NEWER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "../data/utils.h"
#include "../frontend/basic_formats.h"
#include "statement.h"


class Newer_Statement : public Statement
{
  public:
    Newer_Statement(int line_number_, const map< string, string >& input_attributes,
                    Parsed_Query& global_settings);
    virtual string get_name() const { return "newer"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Newer_Statement();
    
    static Generic_Statement_Maker< Newer_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    uint64 get_timestamp() const { return than_timestamp; }
  
    virtual std::string dump_xml(const std::string& indent) const
    {
      return indent + "<newer"
          + (than_timestamp != NOW ? std::string(" than=\"") + iso_string(than_timestamp) + "\"" : "")
          + "/>\n";
    }
  
    virtual std::string dump_compact_ql(const std::string&) const
    {
      return "node" + dump_ql_in_query("");
    }
    virtual std::string dump_ql_in_query(const std::string&) const
    {
      return std::string("(newer:")
          + (than_timestamp != NOW ? std::string("\"") + iso_string(than_timestamp) + "\"" : "")
          + ")";
    }
    virtual std::string dump_pretty_ql(const std::string& indent) const { return dump_compact_ql(indent); }

  private:
    uint64 than_timestamp;
    vector< Query_Constraint* > constraints;
};

#endif
