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
#include "../expat/expat_justparse_interface.h"
#include "user_interface.h"

using namespace std;

string get_xml_raw(Error_Output* error_output, uint32 max_input_size)
{
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
    int cgi_error(0);
    input = decode_cgi_to_plain(input, cgi_error);
    
    // no 'data=' found
    if (cgi_error)
    {
      if (error_output)
	error_output->add_encoding_error("Your input can neither be interpreted verbatim nor does it contain the string \"data=\".");
      return "";
    }
    
    pos = 0;
    line_number = 1;
    while ((pos < input.size()) && (isspace(input[pos])))
    {
      if (input[pos] == '\n')
	++line_number;
      ++pos;
    }
    
    // the input contains at most whitespace
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
      error_output->add_encoding_remark("The first non-whitespace character is '<'. Thus, your input will be interpreted verbatim.");
  }
  
  // assert length restriction
  if (input.size() > max_input_size)
  {
    ostringstream temp;
    temp<<"Input too long (length: "<<input.size()<<", max. allowed: "<<max_input_size<<')';
    if (error_output)
      error_output->add_encoding_error(temp.str());
    return input;
  }
  
  // pos again points at the first non-whitespace character
  if (input.substr(pos, 11) == "<osm-script")
    // add a header line and remove trailing whitespace
  {
    ostringstream temp;
    temp<<"Your input starts with a 'osm-script' tag. Thus, a line with the\n"
	<<"datatype declaration is added. This shifts line numbering by "
	<<line_number - 2<<" line(s).";
    if (error_output)
      error_output->add_encoding_remark(temp.str());
      
    input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	+ input.substr(pos);
  }
  else if (input.substr(pos, 2) != "<?")
    // add a header line, the root tag 'osm-script' and remove trailing whitespace
  {
    ostringstream temp;
    temp<<"Your input starts with a tag but not the root tag. Thus, a line with the\n"
	<<"datatype declaration and a line with the root tag 'osm-script' is\n"
	<<"added. This shifts line numbering by "<<line_number-3<<" line(s).";
    if (error_output)
      error_output->add_encoding_remark(temp.str());
      
    input = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-script>\n"
	+ input.substr(pos) + "\n</osm-script>\n";
  }

  return input;
}

string escape_xml(const string& s)
{
  string result;
  for (int i(0); i < s.length(); ++i)
  {
    if (s[i] == '&')
      result += "&amp;";
    else if (s[i] == '\"')
      result += "&quot;";
    else if (s[i] == '<')
      result += "&lt;";
    else if (s[i] == '>')
      result += "&gt;";
    else if ((uint8)s[i] < 32)
      result += '?';
    else
      result += s[i];
  }
  return result;
}
