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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__MAKE_H


#include "statement.h"

#include <map>
#include <string>
#include <vector>


class Make_Statement : public Output_Statement
{
  public:
    Make_Statement(int line_number_, const std::map< std::string, std::string >& attributes,
                     Parsed_Query& global_settings);
    virtual std::string get_name() const { return "make"; }
    virtual void execute(Resource_Manager& rman);
    virtual ~Make_Statement();    
    static Generic_Statement_Maker< Make_Statement > statement_maker;
    
    std::string get_source_name() const { return input; }

  private:
    std::string input;
    std::string type;
};


#endif
