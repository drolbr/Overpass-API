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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAP_TO_AREA_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAP_TO_AREA_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"


class Map_To_Area_Statement : public Output_Statement
{
  public:
    Map_To_Area_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                          Parsed_Query& global_settings);
    virtual std::string get_name() const { return "map-to-area"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Map_To_Area_Statement() {}
    static Generic_Statement_Maker< Map_To_Area_Statement > statement_maker;

    static bool is_used() { return is_used_; }

  private:
    std::string input;

    static bool is_used_;
};

#endif
