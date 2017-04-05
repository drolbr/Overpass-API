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


#ifdef HAVE_CONFIG_H
#include <config.h>
#undef VERSION
#endif

#ifdef HAVE_FASTCGI
#include "fcgio.h"
#endif

#include "resource_manager.h"
#include "scripting_core.h"
#include "../frontend/web_output.h"
#include "../frontend/user_interface.h"
#include "../statements/osm_script.h"
#include "../statements/statement.h"
#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Maximum bytes
const unsigned long STDIN_MAX = 1000000;


int handle_request(const std::string & content, bool is_cgi, Index_Cache* ic)
{
  Parsed_Query global_settings;
  Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(&error_output);

  try
  {
    global_settings.set_input_params(
	get_xml_cgi(content, &error_output, 16*1024*1024,
	error_output.http_method, error_output.allow_headers, error_output.has_origin, is_cgi));

    if (error_output.display_encoding_errors())
      return 0;

    Statement::Factory stmt_factory(global_settings);
    if (!parse_and_validate(stmt_factory, global_settings, global_settings.get_input_params().find("data")->second,
        &error_output, parser_execute))
      return 0;

    error_output.set_output_handler(global_settings.get_output_handler());

    Osm_Script_Statement* osm_script = 0;
    if (!get_statement_stack()->empty())
      osm_script = dynamic_cast< Osm_Script_Statement* >(get_statement_stack()->front());

    uint32 max_allowed_time = 0;
    uint64 max_allowed_space = 0;
    if (osm_script)
    {
      max_allowed_time = osm_script->get_max_allowed_time();
      max_allowed_space = osm_script->get_max_allowed_space();
    }
    else
    {
      Osm_Script_Statement temp(0, std::map< std::string, std::string >(), global_settings);
      max_allowed_time = temp.get_max_allowed_time();
      max_allowed_space = temp.get_max_allowed_space();
    }

    if (error_output.http_method == http_options
        || error_output.http_method == http_head)
      error_output.write_payload_header("", "", "", true);
    else
    {
      // open read transaction and log this.
      int area_level = determine_area_level(&error_output, 0);
      Dispatcher_Stub dispatcher("", &error_output, global_settings.get_input_params().find("data")->second,
			         get_uses_meta_data(), area_level,
				 max_allowed_time, max_allowed_space, global_settings, ic);
      if (osm_script && osm_script->get_desired_timestamp())
        dispatcher.resource_manager().set_desired_timestamp(osm_script->get_desired_timestamp());

      error_output.write_payload_header(dispatcher.get_db_dir(), dispatcher.get_timestamp(),
 	  area_level > 0 ? dispatcher.get_area_timestamp() : "", true);

      dispatcher.resource_manager().start_cpu_timer(0);
      for (std::vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	   it != get_statement_stack()->end(); ++it)
        (*it)->execute(dispatcher.resource_manager());
      dispatcher.resource_manager().stop_cpu_timer(0);

    //TODO
//       if (osm_script && osm_script->get_type() == "popup")
//       {
//         error_output.write_html_header
//             (dispatcher.get_timestamp(),
// 	     area_level > 0 ? dispatcher.get_area_timestamp() : "", 200,
// 	     osm_script->template_contains_js(), false);
//         osm_script->write_output();
//         error_output.write_footer();
//       }
//       else
//         error_output.write_footer();
    }
  }
  catch(File_Error e)
  {
    std::ostringstream temp;
    if (e.origin.substr(e.origin.size()-9) == "::timeout")
    {
      error_output.write_html_header("", "", 504, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". The server is probably too busy to handle your request.\n";
    }
    else if (e.origin.substr(e.origin.size()-14) == "::rate_limited")
    {
      error_output.write_html_header("", "", 429, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Please check /api/status for the quota of your IP address.\n";
    }
    else
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
    error_output.runtime_error(temp.str());
  }
  catch(Resource_Error e)
  {
    std::ostringstream temp;
    if (e.timed_out)
      temp<<"Query timed out in \""<<e.stmt_name<<"\" at line "<<e.line_number
          <<" after "<<e.runtime<<" seconds.";
    else
      temp<<"Query ran out of memory in \""<<e.stmt_name<<"\" at line "
          <<e.line_number<<". It would need at least "<<e.size/(1024*1024)<<" MB of RAM to continue.";
    error_output.runtime_error(temp.str());
  }
  catch(std::bad_alloc& e)
  {
    rlimit limit;
    getrlimit(RLIMIT_AS, &limit);
    std::ostringstream temp;
    temp<<"Query run out of memory using about "<<limit.rlim_cur/(1024*1024)<<" MB of RAM.";
    error_output.runtime_error(temp.str());
  }
  catch(std::exception& e)
  {
    error_output.runtime_error(std::string("Query failed with the exception: ") + e.what());
  }
  catch(Exit_Error e) {}

  return 0;
}

#ifdef HAVE_FASTCGI
std::string get_request_content(const FCGX_Request & request) {
    char * content_length_str = FCGX_GetParam("CONTENT_LENGTH", request.envp);
    unsigned long content_length = STDIN_MAX;

    if (content_length_str) {
        content_length = strtol(content_length_str, &content_length_str, 10);
        if (*content_length_str) {
          std::cerr << "Can't Parse 'CONTENT_LENGTH='"
                 << FCGX_GetParam("CONTENT_LENGTH", request.envp)
                 << "'. Consuming stdin up to " << STDIN_MAX << std::endl;
        }

        if (content_length > STDIN_MAX) {
            content_length = STDIN_MAX;
        }
    } else {
        // Do not read from stdin if CONTENT_LENGTH is missing
        content_length = 0;
    }

    char * content_buffer = new char[content_length];
    std::cin.read(content_buffer, content_length);
    content_length = std::cin.gcount();

    // Chew up any remaining stdin - this shouldn't be necessary
    // but is because mod_fastcgi doesn't handle it correctly.

    // ignore() doesn't set the eof bit in some versions of glibc++
    // so use gcount() instead of eof()...
    do std::cin.ignore(1024); while (std::cin.gcount() == 1024);

    std::string content(content_buffer, content_length);
    delete [] content_buffer;
    return content;
}
#endif


int main(int argc, char *argv[])
{
  Index_Cache ic;

#ifdef HAVE_FASTCGI

  if (FCGX_IsCGI())
  {

#endif

    int ret = handle_request("", true, &ic);
    return (ret);

#ifdef HAVE_FASTCGI

  }
  else
  {
    // Backup the stdio streambufs
    std::streambuf * cin_streambuf  = std::cin.rdbuf();
    std::streambuf * cout_streambuf = std::cout.rdbuf();
    std::streambuf * cerr_streambuf = std::cerr.rdbuf();

    FCGX_Request request;

    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);

    while (FCGX_Accept_r(&request) == 0) {
      fcgi_streambuf cin_fcgi_streambuf(request.in);
      fcgi_streambuf cout_fcgi_streambuf(request.out);
      fcgi_streambuf cerr_fcgi_streambuf(request.err);

      std::cin.rdbuf(&cin_fcgi_streambuf);
      std::cout.rdbuf(&cout_fcgi_streambuf);
      std::cerr.rdbuf(&cerr_fcgi_streambuf);

      std::string content = get_request_content(request);

      char* request_method = FCGX_GetParam("REQUEST_METHOD", request.envp);
      char* access_control_headers = FCGX_GetParam("HTTP_ACCESS_CONTROL_REQUEST_HEADERS", request.envp);
      char* http_origin = FCGX_GetParam("HTTP_ORIGIN", request.envp);
      char* remote_addr = FCGX_GetParam("REMOTE_ADDR", request.envp);
      char* query_string = FCGX_GetParam("QUERY_STRING", request.envp);

      setenv("REQUEST_METHOD", request_method != NULL ? request_method : "", true);
      setenv("HTTP_ACCESS_CONTROL_REQUEST_HEADERS", access_control_headers != NULL ? access_control_headers : "" , true);
      setenv("HTTP_ORIGIN", http_origin != NULL ? http_origin : "", true);
      setenv("REMOTE_ADDR", remote_addr != NULL ? remote_addr : "", true);
      setenv("QUERY_STRING", query_string != NULL ? query_string : "", true);

      initialize();

      int ret = handle_request(content, FCGX_IsCGI(), &ic);

    }

    // restore stdio streambufs
    std::cin.rdbuf(cin_streambuf);
    std::cout.rdbuf(cout_streambuf);
    std::cerr.rdbuf(cerr_streambuf);

  }
#endif

  return 0;
}

