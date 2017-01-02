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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__ID_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__ID_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Id_Query_Statement : public Output_Statement
{
  public:
    Id_Query_Statement(int line_number_, const map< string, string >& attributes,
                       Query_Constraint* bbox_limitation = 0);
    virtual string get_name() const { return "id-query"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Id_Query_Statement();
    
    static Generic_Statement_Maker< Id_Query_Statement > statement_maker;

    virtual Query_Constraint* get_query_constraint();
    virtual const set<Uint64::Id_Type> & get_ref_ids() { return ref_ids; }
    int get_type() const { return type; }
    
    static bool area_query_exists() { return area_query_exists_; }
    
  private:
    int type;
    set<Uint64::Id_Type> ref_ids;
    vector< Query_Constraint* > constraints;
    
    static bool area_query_exists_;
};

#endif
