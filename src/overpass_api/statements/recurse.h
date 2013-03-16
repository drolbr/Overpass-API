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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__RECURSE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__RECURSE_H

#include "query.h"
#include "statement.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

class Recurse_Statement : public Statement
{
  public:
    Recurse_Statement(int line_number_, const map< string, string >& input_attributes);
    virtual string get_name() const { return "recurse"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Recurse_Statement();
    
    static Generic_Statement_Maker< Recurse_Statement > statement_maker;

    virtual Query_Constraint* get_query_constraint();
    unsigned int get_type() const { return type; }
    string get_input() const { return input; }
    
    const string* get_role() const { return (restrict_to_role ? &role : 0); }
    
  private:
    string input, output;
    unsigned int type;
    string role;
    bool restrict_to_role;
    vector< Query_Constraint* > constraints;
};
      
struct Order_By_Node_Id
{
  bool operator() (const pair< Uint32_Index, const Node_Skeleton* >& a,
		   const pair< Uint32_Index, const Node_Skeleton* >& b)
  {
    return (a.second->id < b.second->id);
  }
};

struct Order_By_Way_Id
{
  bool operator() (const pair< Uint31_Index, const Way_Skeleton* >& a,
		   const pair< Uint31_Index, const Way_Skeleton* >& b)
  {
    return (a.second->id < b.second->id);
  }
};


#endif
