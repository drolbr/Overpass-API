#ifndef CGI_HELPER
#define CGI_HELPER

#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;

string cgi_form_to_text();
void return_error(const string& error, int current_line_number = -1);
string get_xml_raw(int& line_offset);

#endif
