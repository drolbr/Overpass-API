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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__USER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__USER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"


class User_Statement : public Output_Statement
{
  public:
    User_Statement(int line_number_, const map< string, string >& input_attributes,
                   Query_Constraint* bbox_limitation = 0);
    virtual string get_name() const { return "user"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~User_Statement();
    
    static Generic_Statement_Maker< User_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    void calc_ranges
        (set< pair< Uint31_Index, Uint31_Index > >& node_req,
         set< pair< Uint31_Index, Uint31_Index > >& other_req,
         Transaction& transaction);
	 
    // Reads the user id from the database.
    uint32 get_id(Transaction& transaction);
    
    // Works only if get_id(Transaction&) has been called before.
    uint32 get_id() const { return user_id; }
    
  private:
    string input;
    uint32 user_id;
    string user_name;
    string result_type;
    vector< Query_Constraint* > constraints;
};

#endif
