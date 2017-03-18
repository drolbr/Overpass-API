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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__USER_INTERFACE_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__USER_INTERFACE_H

#include <string>

#include "../core/datatypes.h"
#include "web_output.h"


std::map< std::string, std::string > get_xml_cgi(
    const std::string& content, Error_Output* error_output, uint32 max_input_size,
    Http_Methods& http_method, std::string& allow_header, bool& has_origin, bool is_cgi);

std::string get_xml_console(Error_Output* error_output, uint32 max_input_size = 1048576);

uint32 probe_client_token();

std::string probe_client_identifier();

#endif
