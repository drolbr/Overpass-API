#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../frontend/web_output.h"
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

// const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";
// static int output_mode(NOTHING);

int main(int argc, char *argv[])
{ 
  // read command line arguments
  string db_dir;
  uint log_level = Error_Output::ASSISTING;
  Debug_Level debug_level = parser_execute;
  int area_level = 0;
  
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
      area_level = 2;
    else if (!(strcmp(argv[argpos], "--dump-xml")))
      debug_level = parser_dump_xml;
    else if (!(strcmp(argv[argpos], "--dump-pretty-map-ql")))
      debug_level = parser_dump_pretty_map_ql;
    else if (!(strcmp(argv[argpos], "--dump-compact-map-ql")))
      debug_level = parser_dump_compact_map_ql;
    ++argpos;
  }
  
  Error_Output* error_output(new Console_Output(log_level));
  Statement::set_error_output(error_output);
  
  // connect to dispatcher and get database dir
  try
  {
    string xml_raw(get_xml_console(error_output));
    
    if ((error_output) && (error_output->display_encoding_errors()))
      return 0;
    
    if (!parse_and_validate(xml_raw, error_output, debug_level))
      return 0;
    if (debug_level != parser_execute)
      return 0;
    
    // open read transaction and log this.
    Dispatcher_Stub dispatcher(db_dir, error_output, xml_raw, area_level);
    
    // set limits - short circuited until forecast gets effective
    dispatcher.set_limits();
  
    Web_Output web_output(Error_Output::QUIET);
    Osm_Script_Statement* osm_script = 0;
    if (!get_statement_stack()->empty())
      osm_script = dynamic_cast< Osm_Script_Statement* >(get_statement_stack()->front());
    if (!osm_script || osm_script->get_type() == "xml")
      web_output.write_xml_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "", false);
    else
      web_output.write_json_header
          (dispatcher.get_timestamp(),
	   area_level > 0 ? dispatcher.get_area_timestamp() : "", false);
    
//     cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
//       "<osm version=\"0.6\" generator=\"Overpass API\">\n"
//       "<note>The data included in this document is from www.openstreetmap.org. "
//       "It has there been collected by a large group of contributors. For individual "
//       "attribution of each item please refer to "
//       "http://www.openstreetmap.org/api/0.6/[node|way|relation]/#id/history </note>\n";
//     cout<<"<meta osm_base=\""<<dispatcher.get_timestamp()<<'\"';
//     if (area_level > 0)
//       cout<<" areas=\""<<dispatcher.get_area_timestamp()<<"\"";
//     cout<<"/>\n\n";

    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it)
      (*it)->execute(dispatcher.resource_manager());
    
    web_output.write_footer();
//     cout<<"\n</osm>\n";

    return 0;
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin != "Dispatcher_Stub::Dispatcher_Stub::1")
    {
      temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
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
