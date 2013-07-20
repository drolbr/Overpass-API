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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__BBOX_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__BBOX_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"
    

class Bbox_Query_Statement : public Output_Statement
{
  public:
    Bbox_Query_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "bbox-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Bbox_Query_Statement();    
    static Generic_Statement_Maker< Bbox_Query_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    vector< pair< uint32, uint32 > > calc_ranges()
    {
      return ::calc_ranges(south, north, west, east);
    }
    
    double get_south() const { return south; }
    double get_north() const { return north; }
    double get_west() const { return west; }
    double get_east() const { return east; }

  private:
    unsigned int type;
    double south, north, west, east;
    vector< Query_Constraint* > constraints;
};

#endif
