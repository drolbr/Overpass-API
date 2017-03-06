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


namespace
{
  std::string autocomplete
      (std::string& input, Error_Output* error_output, uint32 max_input_size)
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
      std::ostringstream temp;
      temp<<"Input Input too long (length: "<<input.size()<<"). The maximum query length is "
          <<(double)max_input_size/1024/1024<<" MB.";
      if (error_output)
	error_output->add_encoding_error(temp.str());
      return input;
    }
    
    // pos again points at the first non-whitespace character.
    if (input.substr(pos, 1) == "<" && input.substr(pos, 2) != "<?")
    {
      if (input.find("<osm-script") == std::string::npos)
        // Add a header line, the root tag 'osm-script' and remove trailing whitespace
      {
        std::ostringstream temp;
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
        std::ostringstream temp;
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

std::map< std::string, std::string > get_xml_cgi(
    Error_Output* error_output, uint32 max_input_size,
    Http_Methods& http_method, std::string& allow_header, bool& has_origin)
{
  // Check for various HTTP headers
  char* method = getenv("REQUEST_METHOD");
  if (method)
  {
    if (!strncmp(method, "HEAD", 5))
      http_method = http_head;
    else if (!strncmp(method, "OPTIONS", 8))
      http_method = http_options;
  }
  char* allow_header_c = getenv("HTTP_ACCESS_CONTROL_REQUEST_HEADERS");
  allow_header = ((allow_header_c) ? allow_header_c : "");
  char* origin = getenv("HTTP_ORIGIN");
  has_origin = ((origin) && strnlen(origin, 1) > 0);
  
  int line_number(1);
  // If there is nonempty input from GET method, use GET
  std::string input(cgi_get_to_text());
  unsigned int pos(0);
  while ((pos < input.size()) && (isspace(input[pos])))
  {
    if (input[pos] == '\n')
      ++line_number;
    ++pos;
  }
  
  std::map< std::string, std::string > decoded;
  
  if (pos == input.size())
  {
    if (http_method == http_options)
    {
      // if we have an OPTIONS request then assume the query is valid
      // As a quick hack set the input to a valid dummy value
      decoded["data"] = "out;";
      return decoded;
    }

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
      decoded["data"] = input;
      return decoded;
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
   decode_cgi_to_plain(input).swap(decoded);
    std::string jsonp = decoded["jsonp"];
    input = decoded["data"];
    
    if (decoded["bbox"] != "")
    {
      const std::string& lonlat = decoded["bbox"];
    
      std::vector< std::string > coords;
      std::string::size_type pos = 0;
      std::string::size_type newpos = lonlat.find(",");
      while (newpos != std::string::npos)
      {
	coords.push_back(lonlat.substr(pos, newpos - pos));
	pos = newpos + 1;
	newpos = lonlat.find(",", pos);
      }
      coords.push_back(lonlat.substr(pos));

      if (coords.size() == 4)
      {
	std::string latlon = coords[1] + "," + coords[0] + "," + coords[3] + "," + coords[2];
      
	pos = input.find("(bbox)");
	while (pos != std::string::npos)
	{
	  input = input.substr(0, pos) + "(" + latlon + ")" + input.substr(pos + 6);
	  pos = input.find("(bbox)");
	}
      
	pos = input.find("[bbox]");
	if (pos != std::string::npos)
	  input = input.substr(0, pos) + "[bbox:" + latlon + "]" + input.substr(pos + 6);
      }
    }
  }
  else
  {
    if (error_output)
      error_output->add_encoding_remark("The first non-whitespace character is '<'. Thus, your input will be interpreted verbatim.");
  }
  
  input = autocomplete(input, error_output, max_input_size);
  decoded["data"] = input;
  return decoded;
}


std::string get_xml_console(Error_Output* error_output, uint32 max_input_size)
{
  if (error_output)
    error_output->add_encoding_remark("Please enter your query and terminate it with CTRL+D.");
  
  // If there is nonempty input from GET method, use GET
  std::string input("");
  input = cgi_post_to_text();
  input = autocomplete(input, error_output, max_input_size);
  return input;
}


std::string probe_client_identifier()
{
  char* remote_addr_c = getenv("REMOTE_ADDR");
  if (!remote_addr_c)
    return "";
  
  return std::string(remote_addr_c);  
}


uint32 parse_ipv4_address(const std::string ip_addr)
{
  if (ip_addr == "")
    return 0;
  
  std::string::size_type pos = ip_addr.find(".");
  std::string::size_type old_pos = 0;
  uint32 client_token = 0;
  
  // Try IPv4 address format
  while (pos != std::string::npos)
  {
    client_token = (client_token<<8 |
      atoll(ip_addr.substr(old_pos, pos - old_pos).c_str()));
    old_pos = pos + 1;
    pos = ip_addr.find(".", old_pos);
  }
  client_token = (client_token<<8 | atoll(ip_addr.substr(old_pos).c_str()));
  
  return client_token;  
}


int decode_hex(std::string representation)
{
  int result = 0;
  std::string::size_type pos = 0;
  
  while (pos < representation.size())
  {
    if (representation[pos] >= '0' && representation[pos] <= '9')
      result = (result<<4) | (representation[pos] - '0');
    else if (representation[pos] >= 'a' && representation[pos] <= 'f')
      result = (result<<4) | (representation[pos] - 'a' + 10);
    else if (representation[pos] >= 'A' && representation[pos] <= 'F')
      result = (result<<4) | (representation[pos] - 'A' + 10);
    ++pos;
  }
  return result;
}


std::vector< uint16 > parse_short_ipv6_address(std::string ip_addr)
{
  std::vector< uint16 > ipv6_address;
  
  // Try shortened IPv6 address format
  std::string::size_type upper_end = ip_addr.find("::");
  std::string::size_type pos = ip_addr.find(":");
  std::string::size_type old_pos = 0;
  
  while (pos < upper_end)
  {
    ipv6_address.push_back(decode_hex(ip_addr.substr(old_pos, pos - old_pos).c_str()));
    old_pos = pos + 1;
    pos = ip_addr.find(":", old_pos);
  }
  ipv6_address.push_back(decode_hex(ip_addr.substr(old_pos, upper_end - old_pos).c_str()));
    
  std::vector< uint16 > lower_ipv6_address;
  old_pos = upper_end + 2;
  pos = ip_addr.find(":", old_pos);
  while (pos != std::string::npos)
  {
    lower_ipv6_address.push_back(decode_hex(ip_addr.substr(old_pos, pos - old_pos).c_str()));
    old_pos = pos + 1;
    pos = ip_addr.find(":", old_pos);
  }
  lower_ipv6_address.push_back(decode_hex(ip_addr.substr(old_pos).c_str()));
   
  ipv6_address.resize(8, 0);
  for (std::vector< uint16 >::size_type i = 0; i < lower_ipv6_address.size(); ++i)
    ipv6_address[i + 8 - lower_ipv6_address.size()] = lower_ipv6_address[i];
  
  return ipv6_address;
}


std::vector< uint16 > parse_full_ipv6_address(std::string ip_addr)
{
  std::vector< uint16 > ipv6_address;
  
  std::string::size_type pos = ip_addr.find(":");
  std::string::size_type old_pos = 0;
  
  while (pos != std::string::npos)
  {
      ipv6_address.push_back(decode_hex(ip_addr.substr(old_pos, pos - old_pos).c_str()));
      old_pos = pos + 1;
      pos = ip_addr.find(":", old_pos);
  }
    
  ipv6_address.resize(8, 0);
  
  return ipv6_address;
}


uint32 probe_client_token()
{
  std::string ip_addr = probe_client_identifier();
  if (ip_addr == "")
    return 0;
  
  if (ip_addr.find(".") != std::string::npos)
    return parse_ipv4_address(ip_addr);
  
  std::vector< uint16 > ipv6_address = (ip_addr.find("::") == std::string::npos ?
      parse_full_ipv6_address(ip_addr) :
      parse_short_ipv6_address(ip_addr));
  
  // We only consider the upper 64 bit of an IPv6 address.
  // For the sake of simplicity we xor these bits to get a 32 bit token.
  // This shall be reviewed once we know how IPv6 addresses really are distributed.
  return ((ipv6_address[0] ^ ipv6_address[2])<<16 | (ipv6_address[1] ^ ipv6_address[3]));
}
