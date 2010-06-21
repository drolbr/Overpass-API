#ifndef CGI_HELPER
#define CGI_HELPER

#include <string>

using namespace std;

string cgi_get_to_text();

string cgi_post_to_text();

string decode_cgi_to_plain(const string& raw, int& error);

#endif
