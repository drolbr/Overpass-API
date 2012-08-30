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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_H

#include <iostream>
#include <string>

#include "../../template_db/types.h"
#include "../core/datatypes.h"

using namespace std;

string escape_xml(const string& s);
string escape_cstr(const string& s);

Osm_Backend_Callback* get_verbatim_callback();
Osm_Backend_Callback* get_quiet_callback();

void report_file_error(const File_Error& e);

#endif
