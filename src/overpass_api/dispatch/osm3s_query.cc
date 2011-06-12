#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
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
  
  int argpos(1);
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
    else if (!(strcmp(argv[argpos], "--verbose")))
      log_level = Error_Output::VERBOSE;
    ++argpos;
  }
  
  Error_Output* error_output(new Console_Output(log_level));
  Statement::set_error_output(error_output);
  
  // connect to dispatcher and get database dir
  try
  {
    string xml_raw(get_xml_console(error_output));
    
    // open read transaction and log this.
    Dispatcher_Stub dispatcher(db_dir, error_output, xml_raw);
    
    if ((error_output) && (error_output->display_encoding_errors()))
      return 0;

    if (!parse_and_validate(xml_raw, error_output))
      return 0;
    
    // set limits - short circuited until forecast gets effective
    dispatcher.set_limits();
  
    cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-derived>\n"
      "<note>The data included in this document is from www.openstreetmap.org. "
      "It has there been collected by a large group of contributors. For individual "
      "attribution of each item please refer to "
      "http://www.openstreetmap.org/api/0.6/[node|way|relation]/#id/history </note>\n";
    cout<<"<meta data_included_until=\""
        <<dispatcher.get_timestamp()
	<<"\" last_rule_applied=\""<<0<<"\"/>\n"
      "\n";

    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it)
      (*it)->execute(dispatcher.resource_manager());
    
    cout<<"\n</osm-derived>\n";
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
  }
  
  return 0;
}
