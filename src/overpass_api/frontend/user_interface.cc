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

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "cgi-helper.h"
#include "../../expat/expat_justparse_interface.h"
#include "user_interface.h"

using namespace std;

namespace
{
  string autocomplete
      (string& input, Error_Output* error_output, uint32 max_input_size)
  {
    unsigned int pos(0), line_number(1);
    while ((pos < input.size()) && (isspace(input[pos])))
    {
      if (input[pos] == '\n')
	++line_number;
      ++pos;
    }
    
    if (pos == input.size())
    {
      if (pos == 0)
      {
	if (error_output)
	  error_output->add_encoding_error("Your input is empty.");
      }
      else
      {
	if (error_output)
	  error_output->add_encoding_error("Your input contains only whitespace.");
      }
      return "";
    }
    
    // pos now points at the first non-whitespace character
    // assert length restriction.
    if (input.size() > max_input_size)
    {
      ostringstream temp;
      temp<<"Input too long (length: "<<input.size()<<", max. allowed: "<<max_input_size<<')';
      if (error_output)
	error_output->add_encoding_error(temp.str());
      return input;
    }
    
    // pos again points at the first non-whitespace character.
    if (input.substr(pos, 1) == "<" && input.substr(pos, 2) != "<?")
    {
      if (input.find("<osm-script") == string::npos)
        // Add a header line, the root tag 'osm-script' and remove trailing whitespace
      {
        ostringstream temp;
        temp<<"Your input starts with a tag but not the root tag. Thus, a line with the\n"
            <<"datatype declaration and a line with the root tag 'osm-script' is\n"
            <<"added. This shifts line numbering by "<<(int)line_number-3<<" line(s).";
        if (error_output)
          error_output->add_encoding_remark(temp.str());
      
        input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-script>\n"
            + input.substr(pos) + "\n</osm-script>\n";
      }
      else
        // Add a header line and remove trailing whitespace.
      {
        ostringstream temp;
        temp<<"Your input contains an 'osm-script' tag. Thus, a line with the\n"
            <<"datatype declaration is added. This shifts line numbering by "
            <<(int)line_number - 2<<" line(s).";
        if (error_output)
          error_output->add_encoding_remark(temp.str());
      
        input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + input.substr(pos);
      }
    }
    
    return input;
  }
}

string get_xml_cgi(Error_Output* error_output, uint32 max_input_size, string& url, bool& redirect,
		   string& template_name, Web_Output::Http_Methods& http_method, string& allow_header,
                   bool& has_origin)
{
  // Check for various HTTP headers
  char* method = getenv("REQUEST_METHOD");
  if (method)
  {
    if (!strncmp(method, "HEAD", 5))
      http_method = Web_Output::http_head;
    else if (!strncmp(method, "OPTIONS", 8))
      http_method = Web_Output::http_options;
  }
  char* allow_header_c = getenv("HTTP_ACCESS_CONTROL_REQUEST_HEADERS");
  allow_header = ((allow_header_c) ? allow_header_c : "");
  char* origin = getenv("HTTP_ORIGIN");
  has_origin = ((origin) && strnlen(origin, 1) > 0);
  
  int line_number(1);
  // If there is nonempty input from GET method, use GET
  string input(cgi_get_to_text());
  unsigned int pos(0);
  while ((pos < input.size()) && (isspace(input[pos])))
  {
    if (input[pos] == '\n')
      ++line_number;
    ++pos;
  }
  
  if (pos == input.size())
  {
    if (http_method == Web_Output::http_options)
      // if we have an OPTIONS request then assume the query is valid
      // As a quick hack set the input to a valid dummy value
      return "out;";

    // otherwise use POST input
    if (pos == 0)
    {
      if (error_output)
        error_output->add_encoding_remark("No input found from GET method. Trying to retrieve input by POST method.");
    }
    else
    {
      if (error_output)
	error_output->add_encoding_remark("Only whitespace found from GET method. Trying to retrieve input by POST method.");
    }
    
    input = cgi_post_to_text();
    pos = 0;
    line_number = 1;
    while ((pos < input.size()) && (isspace(input[pos])))
    {
      if (input[pos] == '\n')
	++line_number;
      ++pos;
    }
  
    if (pos == input.size())
    {
      if (pos == 0)
      {
	if (error_output)
	  error_output->add_encoding_error("Your input is empty.");
      }
      else
      {
	if (error_output)
	  error_output->add_encoding_error("Your input contains only whitespace.");
      }
      return "";
    }
  }
  else
  {
    if (error_output)
      error_output->add_encoding_remark("Found input from GET method. Thus, any input from POST is ignored.");
  }

  // pos now points at the first non-whitespace character
  if (input[pos] != '<')
    // reduce input to the part between "data=" and "&"
    // and remove the character escapings from the form
  {
    if (error_output)
      error_output->add_encoding_remark("The server now removes the CGI character escaping.");
    string jsonp;
    input = decode_cgi_to_plain(input, jsonp, url, redirect, template_name);
    
    // sanity check for template_name
    if (template_name.find("/") != std::string::npos)
    {
      if (error_output)
	error_output->add_encoding_error("Parameter \"template\" must not contain slashes.");
      template_name = "";
    }
    
    // sanity check for jsonp
    for (std::string::size_type i = 0; i < jsonp.size(); ++i)
    {
      if (!isalnum(jsonp[i]) && !(jsonp[i] == '_') && !(jsonp[i] == '.'))
      {
	error_output->add_encoding_error("Parameter \"jsonp\" must contain only letters, digits, or the underscore.");
	jsonp = "";
      }
    }
    
    if (jsonp != "")
      error_output->add_padding(jsonp);
  }
  else
  {
    if (error_output)
      error_output->add_encoding_remark("The first non-whitespace character is '<'. Thus, your input will be interpreted verbatim.");
  }
  
  input = autocomplete(input, error_output, max_input_size);  
  return input;
}


string get_xml_console(Error_Output* error_output, uint32 max_input_size)
{
  if (error_output)
    error_output->add_encoding_remark("Please enter your query and terminate it with CTRL+D.");
  
  // If there is nonempty input from GET method, use GET
  string input("");
  input = cgi_post_to_text();
  input = autocomplete(input, error_output, max_input_size);
  return input;
}


string probe_client_identifier()
{
  char* remote_addr_c = getenv("REMOTE_ADDR");
  if (!remote_addr_c)
    return "";
  
  return string(remote_addr_c);  
}


uint32 probe_client_token()
{
  string ip_addr = probe_client_identifier();
  if (ip_addr == "")
    return 0;
  
  string::size_type pos = ip_addr.find(".");
  string::size_type old_pos = 0;
  uint32 client_token = 0;
  
  // Try IPv4 address format
  while (pos != string::npos)
  {
    client_token = (client_token<<8 |
        atoll(ip_addr.substr(old_pos, pos - old_pos).c_str()));
    old_pos = pos + 1;
    pos = ip_addr.find(".", old_pos);
  }
  client_token = (client_token<<8 |
    atoll(ip_addr.substr(old_pos).c_str()));
  
  return client_token;
}
