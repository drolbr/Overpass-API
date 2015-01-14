/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__OSM_SCRIPT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__OSM_SCRIPT_H

#include <map>
#include <string>
#include <vector>
#include "bbox_query.h"
#include "statement.h"

using namespace std;

class Output_Handle;

class Osm_Script_Statement : public Statement
{
  public:
    Osm_Script_Statement(int line_number_, const map< string, string >& input_attributes,
                         Parsed_Query& global_settings);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "osm-script"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    
    static Generic_Statement_Maker< Osm_Script_Statement > statement_maker;
    
    void set_factory(Statement::Factory* factory_) { factory = factory_; }

    uint32 get_max_allowed_time() const { return max_allowed_time; }
    uint64 get_max_allowed_space() const { return max_allowed_space; }
    uint64 get_desired_timestamp() const { return desired_timestamp; }
    
  private:
    vector< Statement* > substatements;
    uint64 desired_timestamp;
    uint64 comparison_timestamp;
    bool add_deletion_information;
    uint32 max_allowed_time;
    uint64 max_allowed_space;
    Statement::Factory* factory;
};

#endif
