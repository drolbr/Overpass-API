#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__CGI_HELPER_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__CGI_HELPER_H

#include <string>

using namespace std;

string cgi_get_to_text();

string cgi_post_to_text();

string decode_cgi_to_plain(const string& raw, int& error, string& jsonp);

#endif
