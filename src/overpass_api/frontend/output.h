#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__OUTPUT_H

#include <iostream>
#include <string>

#include "../../template_db/types.h"
#include "../core/datatypes.h"

using namespace std;

string escape_xml(const string& s);

Osm_Backend_Callback* get_verbatim_callback();
Osm_Backend_Callback* get_quiet_callback();

void report_file_error(const File_Error& e);

#endif
