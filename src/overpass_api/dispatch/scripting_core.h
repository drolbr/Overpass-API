#ifndef ORG__OVERPASS_API__SCRIPTING_CORE
#define ORG__OVERPASS_API__SCRIPTING_CORE

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

#include "../../expat/expat_justparse_interface.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"
#include "dispatcher.h"
#include "resource_manager.h"

using namespace std;

// const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";
// static int output_mode(NOTHING);

class Dispatcher_Stub
{
  public:
    // Opens the connection to the database, sets db_dir accordingly
    // and registers the process. error_output_ must remain valid over the
    // entire lifetime of this object.
    Dispatcher_Stub(string db_dir_, Error_Output* error_output_);
    
    void register_process();
    void log_query(string xml_raw);
    void set_limits();

    ~Dispatcher_Stub();
    
    string get_timestamp()
    {
      return (shm_ptr != 0 ? (const char*)(shm_ptr+OFFSET_DB_1+4) : "unknown");
    }
    
  private:
    string db_dir;
    uint8* shm_ptr;
    uint32 msg_id;
    int shm_fd;
    uint32 pid;
    
    Error_Output* error_output;

    void unregister_process();
};

bool parse_and_validate
    (const string& xml_raw,
     Dispatcher_Stub& dispatcher, Error_Output* error_output);

vector< Statement* >* get_statement_stack();
     
#endif
