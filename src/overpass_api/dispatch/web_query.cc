#include "resource_manager.h"
#include "scripting_core.h"
#include "../expat/expat_justparse_interface.h"
#include "../frontend/web_output.h"
#include "../frontend/user_interface.h"
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

// const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";
// static int output_mode(NOTHING);

int main(int argc, char *argv[])
{
  Web_Output error_output(Error_Output::ASSISTING);
  Statement::set_error_output(&error_output);
  
  try
  {
    string xml_raw(get_xml_cgi(&error_output));
    
    if (error_output.display_encoding_errors())
      return 0;
    
    if (!parse_and_validate(xml_raw, &error_output))
      return 0;
    
    // open read transaction and log this.
    int area_level = 0;
    Dispatcher_Stub dispatcher("", &error_output, xml_raw, area_level);
    
    // set limits - short circuited until forecast gets effective
    dispatcher.set_limits();
    
    error_output.write_xml_header(dispatcher.get_timestamp());

    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it)
      (*it)->execute(dispatcher.resource_manager());
  
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
  catch(Exit_Error e) {}

  return 0;
}
