/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Roland Olbricht et al.
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


int main(int argc, char *argv[])
{
  Parsed_Query global_settings;
  Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(&error_output);

  try
  {
    global_settings.set_input_params(
	get_xml_cgi(&error_output, 16*1024*1024,
	error_output.http_method, error_output.allow_headers, error_output.has_origin));

    if (error_output.display_encoding_errors())
      return 0;

    Statement::Factory stmt_factory(global_settings);
    if (!parse_and_validate(stmt_factory, global_settings, global_settings.get_input_params().find("data")->second,
        &error_output, parser_execute))
      return 0;

    error_output.set_output_handler(global_settings.get_output_handler());
    if (global_settings.get_api_key().empty())
    {
      std::map< std::string, std::string >::const_iterator it = global_settings.get_input_params().find("apikey");
      if (it != global_settings.get_input_params().end())
        global_settings.set_api_key(api_key_from_hex(it->second));
    }

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
          get_uses_meta_data(), area_level, max_allowed_time, max_allowed_space,
          global_settings.get_api_key(), global_settings);
      if (osm_script && osm_script->get_desired_timestamp())
        dispatcher.resource_manager().set_desired_timestamp(osm_script->get_desired_timestamp());

      error_output.write_payload_header(dispatcher.get_db_dir(), dispatcher.get_timestamp(),
 	  area_level > 0 ? dispatcher.get_area_timestamp() : "", true);

      Cpu_Timer cpu(dispatcher.resource_manager(), 0);
      for (std::vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	   it != get_statement_stack()->end(); ++it)
        (*it)->execute(dispatcher.resource_manager());

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
            <<". The server is probably too busy to handle your request.";
    }
    else if (e.origin.substr(e.origin.size()-14) == "::rate_limited")
    {
      error_output.write_html_header("", "", 429, false);
      if (error_output.http_method == http_get
          || error_output.http_method == http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Please check /api/status for the quota of your IP address.";
    }
    else if (e.origin == "Dispatcher_Client::1")
    {
      error_output.write_html_header("", "", 504, false);
      temp<<"The dispatcher (i.e. the database management system) is turned off.";
    }
    else
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
    error_output.runtime_error(temp.str());
  }
  catch(Authorization_Error e)
  {
    error_output.write_html_header("", "", 403, false);
    std::ostringstream temp;
    if (e.cause == Authorization_Error::not_found)
      temp<<"Api key \""<<e.api_key<<"\" not known or expired.";
    else
      temp<<"User related data requested, but api key is \""<<e.api_key<<"\" not authorized for that.";
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
