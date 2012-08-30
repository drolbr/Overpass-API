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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_DUMP_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_DUMP_H

#include <map>
#include <string>
#include <vector>

using namespace std;

/**
 * The base class for all statements
 */
class Statement_Dump
{
  public:
    struct Factory
    {
      Statement_Dump* create_statement(string element, int line_number,
				       const map< string, string >& attributes);
    };
    
    Statement_Dump(string name_, const map< string, string >& attributes_)
        : name(name_), attributes(attributes_) {}
    ~Statement_Dump();
    
    void add_statement(Statement_Dump* statement, string text);
    string dump_xml() const;
    string dump_pretty_map_ql() const;
    string dump_compact_map_ql() const;
    
    void add_final_text(string text) {}

  private:
    string name;
    map< string, string > attributes;
    vector< Statement_Dump* > substatements;
};

#endif
