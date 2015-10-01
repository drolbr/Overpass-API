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

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../frontend/web_output.h"
#include "../osm-backend/clone_database.h"
#include "../statements/osm_script.h"
#include "../statements/statement.h"
#include "resource_manager.h"
#include "scripting_core.h"

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
  // read command line arguments
  string db_dir = "";
  string clone_db_dir = "";
  uint log_level = Error_Output::ASSISTING;
  Debug_Level debug_level = parser_execute;
  int area_level = 0;
  bool respect_timeout = true;
  
  int argpos = 1;
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
    }
    else if (!(strcmp(argv[argpos], "--quiet")))
      log_level = Error_Output::QUIET;
    else if (!(strcmp(argv[argpos], "--concise")))
      log_level = Error_Output::CONCISE;
    else if (!(strcmp(argv[argpos], "--progress")))
      log_level = Error_Output::PROGRESS;
    else if (!(strcmp(argv[argpos], "--verbose")))
      log_level = Error_Output::VERBOSE;
    else if (!(strcmp(argv[argpos], "--rules")))
    {
      area_level = 2;
      respect_timeout = false;
    }
    else if (!(strcmp(argv[argpos], "--dump-xml")))
      debug_level = parser_dump_xml;
    else if (!(strcmp(argv[argpos], "--dump-pretty-map-ql")))
      debug_level = parser_dump_pretty_map_ql;
    else if (!(strcmp(argv[argpos], "--dump-compact-map-ql")))
      debug_level = parser_dump_compact_map_ql;
    else if (!(strcmp(argv[argpos], "--dump-bbox-map-ql")))
      debug_level = parser_dump_bbox_map_ql;
    else if (!(strncmp(argv[argpos], "--clone=", 8)))
    {
      clone_db_dir = ((string)argv[argpos]).substr(8);
      if ((clone_db_dir.size() > 0) && (clone_db_dir[clone_db_dir.size()-1] != '/'))
	clone_db_dir += '/';
    }
    else
    {
      cout<<"Unknown argument: "<<argv[argpos]<<"\n\n"
      "Accepted arguments are:\n"
      "  --db-dir=$DB_DIR: The directory where the database resides. If you set this parameter\n"
      "        then osm3s_query will read from the database without using the dispatcher management.\n"
      "  --dump-xml: Don't execute the query but only dump the query in XML format.\n"
      "  --dump-pretty-map-ql: Don't execute the query but only dump the query in pretty QL format.\n"
      "  --dump-compact-map-ql: Don't execute the query but only dump the query in compact QL format.\n"
      "  --dump-bbox-map-ql: Don't execute the query but only dump the query in a suitable form\n"
      "        for an OpenLayers slippy map.\n"
      "  --clone=$TARGET_DIR: Write a consistent copy of the entire database to the given $TARGET_DIR.\n"
      "  --rules: Ignore all time limits and allow area creation by this query.\n"
      "  --quiet: Don't print anything on stderr.\n"
      "  --concise: Print concise information on stderr.\n"
      "  --progress: Print also progress information on stderr.\n"
      "  --verbose: Print everything that happens on stderr.\n";
      
      return 0;
    }
    ++argpos;
  }
  
  Error_Output* error_output(new Console_Output(log_level));
  Statement::set_error_output(error_output);
  
  // connect to dispatcher and get database dir
  try
  {
    if (clone_db_dir != "")
    {
      // open read transaction and log this.
      area_level = determine_area_level(error_output, area_level);
      Dispatcher_Stub dispatcher(db_dir, error_output, "-- clone database --",
				 get_uses_meta_data(), area_level, 24*60*60, 1024*1024*1024);
      copy_file(dispatcher.resource_manager().get_transaction()->get_db_dir() + "/replicate_id",
		clone_db_dir + "/replicate_id");
      
      clone_database(*dispatcher.resource_manager().get_transaction(), clone_db_dir);
      return 0;
    }
    
    string xml_raw(get_xml_console(error_output));
    
    if ((error_output) && (error_output->display_encoding_errors()))
      return 0;
    
    Statement::Factory stmt_factory;
    if (!parse_and_validate(stmt_factory, xml_raw, error_output, debug_level))
      return 0;
    if (debug_level != parser_execute)
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
    
    // Allow rules to run for unlimited time
    if (!respect_timeout)
      max_allowed_time = 0;
    
    // open read transaction and log this.
    area_level = determine_area_level(error_output, area_level);
    Dispatcher_Stub dispatcher(db_dir, error_output, xml_raw,
			       get_uses_meta_data(), area_level, max_allowed_time, max_allowed_space);
    if (osm_script && osm_script->get_desired_timestamp())
      dispatcher.resource_manager().set_desired_timestamp(osm_script->get_desired_timestamp());
 
    Web_Output web_output(log_level);
    if (!osm_script || osm_script->get_type() == "xml")
      web_output.write_xml_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "", false);
    else if (osm_script->get_type() == "json")
      web_output.write_json_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "", false);
    else
      ;
    
//   double timecounter = 0;
//   timeval tv;
//   gettimeofday(&tv, 0);
//   timecounter -= tv.tv_sec + double(tv.tv_usec)/1000000;
  
    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it)
      (*it)->execute(dispatcher.resource_manager());
    
//   gettimeofday(&tv, 0);
//   timecounter += tv.tv_sec + double(tv.tv_usec)/1000000;
//   std::cerr<<timecounter<<'\n';
    
    if (osm_script && osm_script->get_type() == "custom")
    {
      uint32 count = osm_script->get_written_elements_count();
      if (count == 0)
      {
        web_output.write_html_header
            (dispatcher.get_timestamp(),
	     area_level > 0 ? dispatcher.get_area_timestamp() : "");
	cout<<"<p>No results found.</p>\n";
	web_output.write_footer();
      }
      else if (count == 1)
      {
	cout<<"Status: 302 Moved\n";
	cout<<"Location: "
	    <<osm_script->adapt_url("http://www.openstreetmap.org/browse/{{{type}}}/{{{id}}}")
	    <<"\n\n";
      }
      else
      {
        web_output.write_html_header
            (dispatcher.get_timestamp(),
	     area_level > 0 ? dispatcher.get_area_timestamp() : "");
        osm_script->write_output();
	web_output.write_footer();
      }
    }
    else if (osm_script && osm_script->get_type() == "popup")
    {
      web_output.write_html_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "", 200,
	   osm_script->template_contains_js(), false);
      osm_script->write_output();
      web_output.write_footer();
    }
    else
      web_output.write_footer();
    
    return 0;
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin != "Dispatcher_Stub::Dispatcher_Stub::1")
    {
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
      if (error_output)
        error_output->runtime_error(temp.str());
    }
    
    return 1;
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
    if (error_output)
      error_output->runtime_error(temp.str());
    
    return 2;
  }
  catch(Exit_Error e)
  {
    return 3;
  }
}
