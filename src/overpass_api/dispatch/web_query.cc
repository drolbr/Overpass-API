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
  Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(&error_output);
  
  try
  {
    string url = "http://www.openstreetmap.org/browse/{{{type}}}/{{{id}}}";
    string template_name = "default.wiki";
    bool redirect = true;
    string xml_raw(get_xml_cgi(&error_output, 16*1024*1024, url, redirect, template_name,
	error_output.http_method, error_output.allow_headers, error_output.has_origin));
    
    if (error_output.display_encoding_errors())
      return 0;
    
    Statement::Factory stmt_factory;
    if (!parse_and_validate(stmt_factory, xml_raw, &error_output, parser_execute))
      return 0;
    
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
      Osm_Script_Statement temp(0, map< string, string >(), 0);
      max_allowed_time = temp.get_max_allowed_time();
      max_allowed_space = temp.get_max_allowed_space();
    }

    if (error_output.http_method == error_output.http_options
        || error_output.http_method == error_output.http_head)
    {
      if (!osm_script || osm_script->get_type() == "xml")
        error_output.write_xml_header("", "");
      else if (osm_script->get_type() == "json")
        error_output.write_json_header("", "");
      else if (osm_script->get_type() == "csv")
        error_output.write_csv_header("", "");
      else
        osm_script->set_template_name(template_name);
    }
    else
    {
      // open read transaction and log this.
      int area_level = determine_area_level(&error_output, 0);
      Dispatcher_Stub dispatcher("", &error_output, xml_raw,
			         get_uses_meta_data(), area_level, max_allowed_time, max_allowed_space);
      if (osm_script && osm_script->get_desired_timestamp())
        dispatcher.resource_manager().set_desired_timestamp(osm_script->get_desired_timestamp());
    
      if (!osm_script || osm_script->get_type() == "xml")
        error_output.write_xml_header
            (dispatcher.get_timestamp(),
	     area_level > 0 ? dispatcher.get_area_timestamp() : "");
      else if (osm_script->get_type() == "json")
        error_output.write_json_header
            (dispatcher.get_timestamp(),
	     area_level > 0 ? dispatcher.get_area_timestamp() : "");
      else if (osm_script->get_type() == "csv")
        error_output.write_csv_header
            (dispatcher.get_timestamp(),
	     area_level > 0 ? dispatcher.get_area_timestamp() : "");
      else
        osm_script->set_template_name(template_name);
      
      for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	   it != get_statement_stack()->end(); ++it)
        (*it)->execute(dispatcher.resource_manager());

      if (osm_script && osm_script->get_type() == "custom")
      {
        uint32 count = osm_script->get_written_elements_count();
        if (count == 0 && redirect)
        {
          error_output.write_html_header
              (dispatcher.get_timestamp(),
	       area_level > 0 ? dispatcher.get_area_timestamp() : "");
	  cout<<"<p>No results found.</p>\n";
	  error_output.write_footer();
        }
        else if (count == 1 && redirect)
        {
	  cout<<"Status: 302 Moved\n";
	  cout<<"Location: "
	      <<osm_script->adapt_url(url)
	      <<"\n\n";
        }
        else
        {
          error_output.write_html_header
              (dispatcher.get_timestamp(),
	       area_level > 0 ? dispatcher.get_area_timestamp() : "", 200,
	       osm_script->template_contains_js());
	  osm_script->write_output();
	  error_output.write_footer();
        }
      }
      else if (osm_script && osm_script->get_type() == "popup")
      {
        error_output.write_html_header
            (dispatcher.get_timestamp(),
	     area_level > 0 ? dispatcher.get_area_timestamp() : "", 200,
	     osm_script->template_contains_js(), false);
        osm_script->write_output();
        error_output.write_footer();
      }
      else
        error_output.write_footer();
    }
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin.substr(e.origin.size()-9) == "::timeout")
    {
      error_output.write_html_header("", "", 504, false);
      if (error_output.http_method == error_output.http_get
          || error_output.http_method == error_output.http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Probably the server is overcrowded.\n";
    }
    else if (e.origin.substr(e.origin.size()-14) == "::rate_limited")
    {
      error_output.write_html_header("", "", 429, false);
      if (error_output.http_method == error_output.http_get
          || error_output.http_method == error_output.http_post)
        temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin
            <<". Another request from your IP is still running.\n";
    }
    else
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
    error_output.runtime_error(temp.str());
  }
  catch(Resource_Error e)
  {
    ostringstream temp;
    if (e.timed_out)
      temp<<"Query timed out in \""<<e.stmt_name<<"\" at line "<<e.line_number
          <<" after "<<e.runtime<<" seconds.";
    else
      temp<<"Query run out of memory in \""<<e.stmt_name<<"\" at line "
          <<e.line_number<<" using about "<<e.size/(1024*1024)<<" MB of RAM.";
    error_output.runtime_error(temp.str());
  }
  catch(std::bad_alloc& e)
  {
    rlimit limit;
    getrlimit(RLIMIT_AS, &limit);
    ostringstream temp;
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
