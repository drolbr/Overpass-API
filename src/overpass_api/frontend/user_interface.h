#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__USER_INTERFACE_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__USER_INTERFACE_H

#include <iostream>
#include <string>

#include "../core/datatypes.h"

using namespace std;

string get_xml_cgi(Error_Output* error_output, uint32 max_input_size = 1048576);

string get_xml_console(Error_Output* error_output, uint32 max_input_size = 1048576);

#endif
