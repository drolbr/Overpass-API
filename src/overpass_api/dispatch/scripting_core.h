#ifndef DE__OSM3S___OVERPASS_API__DISPATCH__SCRIPTING_CORE_H
#define DE__OSM3S___OVERPASS_API__DISPATCH__SCRIPTING_CORE_H

#include "resource_manager.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"
#include "../../expat/expat_justparse_interface.h"
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

struct Exit_Error {};

class Dispatcher_Stub
{
  public:
    // Opens the connection to the database, sets db_dir accordingly
    // and registers the process. error_output_ must remain valid over the
    // entire lifetime of this object.
    Dispatcher_Stub(string db_dir_, Error_Output* error_output_, string xml_raw,
		    int& area_level);
    
    void register_process();
    void set_limits();

    ~Dispatcher_Stub();
    
    string get_timestamp() { return timestamp; }
    string get_area_timestamp() { return area_timestamp; }
    Resource_Manager& resource_manager() { return *rman; }
    
  private:
    string db_dir, timestamp, area_timestamp;
    
    Error_Output* error_output;
    Dispatcher_Client* dispatcher_client;
    Dispatcher_Client* area_dispatcher_client;
    Nonsynced_Transaction* transaction;
    Nonsynced_Transaction* area_transaction;
    Resource_Manager* rman;

    void unregister_process();
};

bool parse_and_validate
    (const string& xml_raw, Error_Output* error_output);

vector< Statement* >* get_statement_stack();
     
#endif
