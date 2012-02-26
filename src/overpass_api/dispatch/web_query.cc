/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "resource_manager.h"
#include "scripting_core.h"
#include "../expat/expat_justparse_interface.h"
#include "../frontend/web_output.h"
#include "../frontend/user_interface.h"
#include "../statements/osm_script.h"
#include "../statements/statement.h"
#include "../../template_db/dispatcher.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char *argv[])
{
  Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(&error_output);
  
  try
  {
    string url = "http://www.openstreetmap.org/browse/{{{type}}}/{{{id}}}";
    string node_template_name = "default.node";
    string way_template_name = "default.way";
    string relation_template_name = "default.relation";
    bool redirect = true;
    string xml_raw(get_xml_cgi(&error_output, 1048576, url, redirect,
			       node_template_name, way_template_name, relation_template_name));
    
    if (error_output.display_encoding_errors())
      return 0;
    
    Statement::Factory stmt_factory;
    if (!parse_and_validate(stmt_factory, xml_raw, &error_output, parser_execute))
      return 0;
    
    // open read transaction and log this.
    int area_level = 0;
    Dispatcher_Stub dispatcher("", &error_output, xml_raw, area_level);
    
    // set limits - short circuited until forecast gets effective
    dispatcher.set_limits();
    
    Osm_Script_Statement* osm_script = 0;
    if (!get_statement_stack()->empty())
      osm_script = dynamic_cast< Osm_Script_Statement* >(get_statement_stack()->front());
    if (!osm_script || osm_script->get_type() == "xml")
      error_output.write_xml_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "");
    else if (osm_script->get_type() == "json")
      error_output.write_json_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "");
    else
    {
      for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	   it != get_statement_stack()->end(); ++it)
        if (dynamic_cast< Osm_Script_Statement* >(*it))
	  dynamic_cast< Osm_Script_Statement* >(*it)->set_template_names
	      (node_template_name, way_template_name, relation_template_name);
    }

    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it)
      (*it)->execute(dispatcher.resource_manager());

    if (osm_script && osm_script->get_type() == "custom")
    {
      uint32 count = osm_script->get_written_elements_count();
      if (count == 0)
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
	     area_level > 0 ? dispatcher.get_area_timestamp() : "");
        cout<<'\n'<<osm_script->get_output();
	error_output.write_footer();
      }
    }
    else
      error_output.write_footer();
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin.substr(e.origin.size()-9) == "::timeout")
      temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin
          <<". Probably the server is overcrowded.\n";
    else
      temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
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
  catch(Exit_Error e) {}

  return 0;
}
