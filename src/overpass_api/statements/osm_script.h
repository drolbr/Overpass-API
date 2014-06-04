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


struct Tag_Filter
{
  string key;
  string value;
  bool straight;  
};

struct Category_Filter
{
  string title;
  string title_key;
  vector< vector< Tag_Filter > > filter_disjunction;
};


class Osm_Script_Statement : public Statement
{
  public:
    Osm_Script_Statement(int line_number_, const map< string, string >& input_attributes,
                         Query_Constraint* bbox_limitation = 0);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "osm-script"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Osm_Script_Statement();
    
    static Generic_Statement_Maker< Osm_Script_Statement > statement_maker;
    
    const string& get_type() const { return type; }
    void set_factory(Statement::Factory* factory_, Output_Handle* output_handle_ = 0)
    {
      factory = factory_;
      output_handle = output_handle_;
    }

    string adapt_url(const string& url) const;
    uint32 get_written_elements_count() const;
    
    void set_template_name(const string& template_name_) { template_name = template_name_; }
    bool template_contains_js() const { return template_contains_js_; }
    void write_output() const;

    void set_categories(const vector< Category_Filter >& categories_) { categories = categories_; }
    
    uint32 get_max_allowed_time() const { return max_allowed_time; }
    uint64 get_max_allowed_space() const { return max_allowed_space; }
    Query_Constraint* get_bbox_limitation() { return bbox_limitation; }
    uint64 get_desired_timestamp() const { return desired_timestamp; }
    
  private:
    vector< Statement* > substatements;
    Query_Constraint* bbox_limitation;
    Bbox_Query_Statement* bbox_statement;
    uint64 desired_timestamp;
    uint64 comparison_timestamp;
    bool add_deletion_information;
    uint32 max_allowed_time;
    uint64 max_allowed_space;
    string type;
    Output_Handle* output_handle;
    Statement::Factory* factory;
    string template_name;
    string header;
    bool template_contains_js_;
    vector< Category_Filter > categories;
};

#endif
