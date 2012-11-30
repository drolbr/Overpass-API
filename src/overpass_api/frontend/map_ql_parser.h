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

#ifndef MAP_QL_PARSER
#define MAP_QL_PARSER

#include "../../expat/map_ql_input.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

using namespace std;

void parse_and_validate_map_ql
    (Statement::Factory& stmt_factory, const string& xml_raw, Error_Output* error_output);

void parse_and_dump_xml_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

void parse_and_dump_compact_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

void parse_and_dump_bbox_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

void parse_and_dump_pretty_from_map_ql
    (const string& xml_raw, Error_Output* error_output);

#endif
