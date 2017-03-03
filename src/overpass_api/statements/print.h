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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PRINT_H

#include <map>
#include <string>
#include <vector>
#include "../data/collect_members.h"
#include "../frontend/output_handler.h"
#include "statement.h"


class Collection_Print_Target;


class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_, const map< string, string >& attributes, Parsed_Query& global_settings);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Print_Statement();

    static Generic_Statement_Maker< Print_Statement > statement_maker;
    
    void set_collect_lhs();
    void set_collect_rhs(bool add_deletion_information);
    
  private:
    string input;
    Output_Mode mode;
    enum { order_by_id, order_by_quadtile } order;
    unsigned int limit;
    Collection_Print_Target* collection_print_target;
    enum { dont_collect, collect_lhs, collect_rhs } collection_mode;
    bool add_deletion_information;
    
    double south;
    double north;
    double west;
    double east;

    virtual void execute_comparison(Resource_Manager& rman);
};


#endif
