/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__BASIC_FORMATS_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__BASIC_FORMATS_H


#include "../core/datatypes.h"


std::string iso_string(uint64 timestamp);


int decode_hex(const std::string& representation);


std::string api_key_from_hex(const std::string& hex);


typedef enum { http_get, http_post, http_head, http_options } Http_Methods;


void write_html_header
    (const std::string& timestamp, const std::string& area_timestamp, uint write_mime, bool write_js_init,
     bool write_remarks);


#endif
