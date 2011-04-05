#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../expat/expat_justparse_interface.h"
#include "../frontend/web_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"
#include "dispatcher.h"
#include "resource_manager.h"
#include "scripting_core.h"

using namespace std;

// const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";
// static int output_mode(NOTHING);

int main(int argc, char *argv[])
{
  Web_Output error_output(Error_Output::QUIET);
  Statement::set_error_output(&error_output);
  
  try
  {
    // Connect to dispatcher and get database dir.
    // Register process and let choose db 1 or 2.
    Dispatcher_Stub dispatcher("", &error_output);
    
    string xml_raw(get_xml_cgi(&error_output));
    
    // log query
    dispatcher.log_query(xml_raw);
    
    if (error_output.display_encoding_errors())
      return 0;
    
    if (!parse_and_validate(xml_raw, dispatcher, &error_output))
      return 0;
    
    // set limits - short circuited until forecast gets effective
    dispatcher.set_limits();
    
    error_output.write_xml_header(dispatcher.get_timestamp());

    Resource_Manager rman;
    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it)
      (*it)->execute(rman);
  
    error_output.write_xml_footer();
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin != "Dispatcher_Stub::Dispatcher_Stub::1")
    {
      temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
      error_output.runtime_error(temp.str());
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
    error_output.runtime_error(temp.str());
  }

  return 0;
}
