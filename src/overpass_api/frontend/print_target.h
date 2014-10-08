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

#ifndef PRINT_TARGET
#define PRINT_TARGET

#include "../statements/osm_script.h"
#include "../statements/print.h"

#include <string>

using namespace std;

class Output_Handle
{
  public:
    Output_Handle(string type_)
      : type(type_), mode(0), print_target(0), written_elements_count(0), first_id(0) {}
    ~Output_Handle();
    
    Print_Target& get_print_target(uint32 mode, Transaction& transaction);
    void set_templates(const string& node_template_, const string& way_template_,
		       const string& relation_template_)
    {
      node_template = node_template_;
      way_template = way_template_;
      relation_template = relation_template_;
    }

    // Output_Handle takes ownership of the provided Print_Target
    void set_print_target(Print_Target* print_target_)
    {
      print_target = print_target_;
    }

    string adapt_url(const string& url) const;
    string get_output() const;
    uint32 get_written_elements_count() const;

    void set_categories(const vector< Category_Filter >& categories_) { categories = categories_; }
    void set_csv_settings(const Csv_Settings& csv_settings_) { csv_settings = csv_settings_; }
    
    void print_bounds(double south, double west, double north, double east);

  private:
    string type;
    uint32 mode;
    Print_Target* print_target;
    string output;
    uint32 written_elements_count;
    string first_type;
    uint32 first_id;
    string node_template;
    string way_template;
    string relation_template;
    vector< Category_Filter > categories;
    Csv_Settings csv_settings;
};

string::size_type find_block_end(string data, string::size_type pos);

#endif
