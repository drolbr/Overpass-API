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

#ifndef DE__OSM3S___OVERPASS_API__DISPATCH__SCRIPTING_CORE_H
#define DE__OSM3S___OVERPASS_API__DISPATCH__SCRIPTING_CORE_H

#include "dispatcher_stub.h"
#include "../statements/statement.h"
#include "../../template_db/dispatcher.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


typedef enum
    { parser_execute, parser_dump_xml, parser_dump_pretty_map_ql, parser_dump_compact_map_ql, parser_dump_bbox_map_ql }
    Debug_Level;

bool parse_and_validate
    (Statement::Factory& stmt_factory, Parsed_Query& parsed_query,
     const std::string& xml_raw, Error_Output* error_output, Debug_Level debug_type);

std::vector< Statement* >* get_statement_stack();

meta_modes get_uses_meta_data();

int determine_area_level(Error_Output* error_output, int area_level);

void initialize();

#endif
