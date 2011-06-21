#ifndef DE__OSM3S___PT_DIAGRAMS__TEST_OUTPUT_H
#define DE__OSM3S___PT_DIAGRAMS__TEST_OUTPUT_H
#include "read_input.h"

using namespace std;

void dump_osm_data(const OSMData& current_data);
void sketch_unscaled_osm_data(const OSMData& current_data);
void sketch_osm_data(const OSMData& current_data, double pivot_lon, double m_per_pixel);
#endif
