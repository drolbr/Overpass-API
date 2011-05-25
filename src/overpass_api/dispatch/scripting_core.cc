#include "resource_manager.h"
#include "scripting_core.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"
#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../../template_db/types.h"

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

namespace
{
  vector< Statement* > statement_stack;
  vector< string > text_stack;
  Script_Parser xml_parser;
}

Dispatcher_Stub::Dispatcher_Stub(string db_dir_, Error_Output* error_output_) : db_dir(db_dir_), shm_ptr(0), error_output(error_output_)
{
  pid = getpid();
  if (db_dir_ != "")
    return;
  
  shm_fd = shm_open(shared_name.c_str(), O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
  if (shm_fd < 0)
  {
    if (error_output)
    {
      error_output->runtime_error
          ((string)"Can't open shared memory " + shared_name + '\n');
    }
    throw File_Error(errno, shared_name, "Dispatcher_Stub::Dispatcher_Stub::1");
  }
  shm_ptr = (uint8*)
      mmap(0, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
  
  db_dir = (char*)(shm_ptr + SHM_SIZE);
  
  register_process();
}

void Dispatcher_Stub::register_process()
{
  if (shm_ptr != 0)
  {
    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)(shm_ptr + 8) = ++msg_id;
    *(uint32*)shm_ptr = REGISTER_PID;
    
    while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
      (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
    {
      //sleep for a second
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 10000;
      select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
      
      *(uint32*)(shm_ptr + 8) = msg_id;
      *(uint32*)(shm_ptr + 4) = pid;
      *(uint32*)shm_ptr = REGISTER_PID;
    }
    if (*(uint32*)(shm_ptr + OFFSET_BACK + 8) == 1)
      set_basedir(db_dir + "1/");
    else if (*(uint32*)(shm_ptr + OFFSET_BACK + 8) == 2)
      set_basedir(db_dir + "2/");
    else
    {
      if (error_output)
	error_output->runtime_error("Both databases are updating.");
      return;
    }
    if (error_output)
      error_output->runtime_remark("Successfully registered");
  }
}

void Dispatcher_Stub::set_limits()
{
  if (shm_ptr != 0)
  {
    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)(shm_ptr + 8) = ++msg_id;
    *(uint32*)(shm_ptr + 12) = 512;
    *(uint32*)(shm_ptr + 16) = 3600;
    *(uint32*)shm_ptr = SET_LIMITS;
    
    while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
      (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
    {
      //sleep for a second
      struct timeval timeout_;
      timeout_.tv_sec = 0;
      timeout_.tv_usec = 10000;
      select (FD_SETSIZE, NULL, NULL, NULL, &timeout_);
      
      *(uint32*)(shm_ptr + 4) = pid;
      *(uint32*)(shm_ptr + 8) = msg_id;
      *(uint32*)(shm_ptr + 12) = 512;
      *(uint32*)(shm_ptr + 16) = 3600;
      *(uint32*)shm_ptr = SET_LIMITS;
    }
    if (*(uint32*)(shm_ptr + OFFSET_BACK + 8) != SET_LIMITS)
    {
      if (error_output)
	error_output->runtime_error("We are sorry, the server is overcrowded. "
      "Please try again later.");
      
      unregister_process();
      return;
    }
    if (error_output)
      error_output->runtime_remark("Successfully set limits");
  }
}

void Dispatcher_Stub::unregister_process()
{
  uint32 pid(getpid());
  // unregister process
  if (shm_ptr != 0)
  {
    *(uint32*)(shm_ptr + 8) = ++msg_id;
    *(uint32*)(shm_ptr + 4) = pid;
    *(uint32*)shm_ptr = UNREGISTER_PID;
    
    while ((*(uint32*)(shm_ptr + OFFSET_BACK) != pid) ||
      (*(uint32*)(shm_ptr + OFFSET_BACK + 4) != msg_id))
    {
      //sleep for a second
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 10000;
      select (FD_SETSIZE, NULL, NULL, NULL, &timeout);
      
      *(uint32*)(shm_ptr + 8) = msg_id;
      *(uint32*)(shm_ptr + 4) = pid;
      *(uint32*)shm_ptr = UNREGISTER_PID;
    }
    if (error_output)
      error_output->runtime_remark("Successfully unregistered");
  }
}

void Dispatcher_Stub::log_query(string xml_raw)
{
  if (shm_ptr != 0)
  {
    ostringstream temp;
    temp<<db_dir<<"query_logs/"<<time(NULL)<<".txt";
    ofstream query_log(temp.str().c_str());
    query_log<<xml_raw;
  }
}

Dispatcher_Stub::~Dispatcher_Stub()
{
  unregister_process();  
}

void start(const char *el, const char **attr)
{
  Statement* statement(Statement::create_statement
      (el, xml_parser.current_line_number()));
  if (statement)
  {
/*    statement->set_startpos(get_tag_start());
    statement->set_tagendpos(get_tag_end());*/
    statement->set_attributes(attr);
  }
  statement_stack.push_back(statement);
  text_stack.push_back(xml_parser.get_parsed_text());
  xml_parser.reset_parsed_text();
}

void end(const char *el)
{
  if (statement_stack.size() > 1)
  {
    Statement* statement(statement_stack.back());
    
    if (statement)
    {
      statement->add_final_text(xml_parser.get_parsed_text());
      xml_parser.reset_parsed_text();
/*      statement->set_endpos(get_tag_end());*/
    }
    
    statement_stack.pop_back();
    if ((statement_stack.back()) && (statement))
      statement_stack.back()->add_statement(statement, text_stack.back());
    text_stack.pop_back();
  }
  else if ((statement_stack.size() == 1) && (statement_stack.front()))
    statement_stack.front()->add_final_text(xml_parser.get_parsed_text());
}

bool parse_and_validate
    (const string& xml_raw,
     Dispatcher_Stub& dispatcher, Error_Output* error_output)
{
  if ((error_output) && (error_output->display_encoding_errors()))
    return false;
  
  try
  {
    xml_parser.parse(xml_raw, start, end);
  }
  catch(Parse_Error parse_error)
  {
    if (error_output)
      error_output->add_parse_error(parse_error.message,
				    xml_parser.current_line_number());
  }
  catch(File_Error e)
  {
    ostringstream temp;
    temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
    if (error_output)
      error_output->runtime_error(temp.str());
    
    return false;
  }
  if ((error_output) && (error_output->display_parse_errors()))
  {
    return false;
  }
  if ((error_output) && (error_output->display_static_errors()))
  {
    return false;
  }
  
  return true;
}

vector< Statement* >* get_statement_stack()
{
  return &statement_stack;
}
