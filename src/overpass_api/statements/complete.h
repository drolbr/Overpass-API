/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht, 2014 mmd
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COMPLETE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COMPLETE_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"

using namespace std;

class Complete_Statement : public Output_Statement
{
  public:
  Complete_Statement(int line_number_, const map< string, string >& attributes,
                     Query_Constraint* bbox_limitation = 0);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "complete"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Complete_Statement() {}
    
    static Generic_Statement_Maker< Complete_Statement > statement_maker;
    
  private:
    string input, output_iteration, output_complete;
    vector< Statement* > substatements;
};

#endif
