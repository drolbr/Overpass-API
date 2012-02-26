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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__CGI_HELPER_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__CGI_HELPER_H

#include <string>

using namespace std;

string cgi_get_to_text();

string cgi_post_to_text();

string decode_cgi_to_plain
    (const string& raw, int& error,
     string& jsonp,
     string& url, bool& redirect,
     string& node_template_name,
     string& way_template_name,
     string& relation_template_name);

#endif
