/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of PT_Diagrams.
*
* PT_Diagrams is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PT_Diagrams is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with PT_Diagrams.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE__OSM3S___PT_DIAGRAMS__TEST_OUTPUT_H
#define DE__OSM3S___PT_DIAGRAMS__TEST_OUTPUT_H
#include "read_input.h"

using namespace std;

void dump_osm_data(const OSMData& current_data);
void sketch_unscaled_osm_data(const OSMData& current_data);
void sketch_osm_data(const OSMData& current_data, double pivot_lon, double m_per_pixel);
#endif
