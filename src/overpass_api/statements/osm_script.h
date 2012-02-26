/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__OSM_SCRIPT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__OSM_SCRIPT_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Output_Handle;

class Osm_Script_Statement : public Statement
{
  public:
    Osm_Script_Statement(int line_number_, const map< string, string >& input_attributes);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "osm-script"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Osm_Script_Statement();
    
    static Generic_Statement_Maker< Osm_Script_Statement > statement_maker;
    
    const string& get_type() const { return type; }
    void set_factory(Statement::Factory* factory_) { factory = factory_; }

    string adapt_url(const string& url) const;
    string get_output() const;
    uint32 get_written_elements_count() const;
    
    void set_template_names(const string& node_template_name_,
			    const string& way_template_name_,
			    const string& relation_template_name_)
    {
      node_template_name = node_template_name_;
      way_template_name = way_template_name_;
      relation_template_name = relation_template_name_;
    }

    /*    string get_rule_name() { return name; }
    int get_rule_replace() { return replace; }
    int get_rule_version() { return version; }
    void set_database_id(uint database_id_) { database_id = database_id_; }*/
    
  private:
    vector< Statement* > substatements;
    uint32 max_allowed_time;
    uint64 max_allowed_space;
    string type;
    Output_Handle* output_handle;
    Statement::Factory* factory;
    string node_template_name;
    string way_template_name;
    string relation_template_name;
};

#endif
