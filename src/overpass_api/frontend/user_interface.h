#ifndef USER_INTERFACE
#define USER_INTERFACE

#include <iostream>
#include <string>

#include "../core/datatypes.h"

using namespace std;

string get_xml_cgi(Error_Output* error_output, uint32 max_input_size = 1048576);

string get_xml_console(Error_Output* error_output, uint32 max_input_size = 1048576);

#endif
