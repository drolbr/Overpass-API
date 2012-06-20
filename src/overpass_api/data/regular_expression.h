/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___OVERPASS_API__DATA__REGULAR_EXPRESSION_H
#define DE__OSM3S___OVERPASS_API__DATA__REGULAR_EXPRESSION_H

#include "sys/types.h"
#include "regex.h"

#include <string>

using namespace std;

//-----------------------------------------------------------------------------

class Regular_Expression
{
  public:
    Regular_Expression(const string& regex) { regcomp(&preg, regex.c_str(), REG_EXTENDED|REG_NOSUB); }
    ~Regular_Expression() { regfree(&preg); }
    
    bool matches(const string& line) const
    { return (regexec(&preg, line.c_str(), 0, 0, 0) == 0); }
    
  private:
    regex_t preg;
};

#endif
