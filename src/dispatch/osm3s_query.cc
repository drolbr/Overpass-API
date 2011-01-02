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
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"
#include "dispatcher.h"
#include "resource_manager.h"

using namespace std;

// const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";
// static int output_mode(NOTHING);

vector< Statement* > statement_stack;
vector< string > text_stack;
Script_Parser xml_parser;

void unregister_process(uint8* shm_ptr, uint32& msg_id,
			Error_Output* error_output)
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

// string db_subdir;
// 
// void log_script(const string& xml_body)
// {
//   ofstream log(LOGFILE, ios_base::app);
//   log<<"interpreter@"<<(uintmax_t)time(NULL)<<": execute\n"<<xml_body<<'\n';
//   log.close();
// }

int main(int argc, char *argv[])
{ 
  // read command line arguments
  string db_dir;
  bool quiet(false);
  bool show_mime(true);
  
  int argpos(1);
  while (argpos < argc)
  {
    if (!(strncmp(argv[argpos], "--db-dir=", 9)))
    {
      db_dir = ((string)argv[argpos]).substr(9);
      if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
	db_dir += '/';
      set_basedir(db_dir);
    }
    else if (!(strcmp(argv[argpos], "--quiet")))
      quiet = true;
    else if (!(strcmp(argv[argpos], "--no-mime")))
      show_mime = false;
    ++argpos;
  }
  
  Error_Output* error_output(new Console_Output(quiet));
  Statement::set_error_output(error_output);
  
  // connect to dispatcher and get database dir
  uint8* shm_ptr(0);
  uint32 msg_id;
  uint32 pid(getpid());
  if (db_dir == "")
  {
    int shm_fd(shm_open("/osm3s", O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO));
    if (shm_fd < 0)
    {
      if (error_output)
	error_output->runtime_error("Can't open shared memory /osm3s\n");
      return 0;
    }
    shm_ptr = (uint8*)
        mmap(0, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	
    db_dir = (char*)(shm_ptr + SHM_SIZE);
  }
  
  // register process and choose db 1 or 2
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
      return 0;
    }
    if (error_output)
      error_output->runtime_remark("Successfully registered");
  }
  
  string xml_raw(get_xml_raw(error_output));
  
  // log query
  if (shm_ptr != 0)
  {
    ostringstream temp;
    temp<<db_dir<<"query_logs/"<<time(NULL)<<".txt";
    ofstream query_log(temp.str().c_str());
    query_log<<xml_raw;
  }
  
  if ((error_output) && (error_output->display_encoding_errors()))
    return 0;
  
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
    
    unregister_process(shm_ptr, msg_id, error_output);
    return 0;
  }
  if ((error_output) && (error_output->display_parse_errors()))
  {
    unregister_process(shm_ptr, msg_id, error_output);
    return 0;
  }
  if ((error_output) && (error_output->display_static_errors()))
  {
    unregister_process(shm_ptr, msg_id, error_output);
    return 0;
  }
  
  // set limits - short circuited until forecast gets effective
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
      
      unregister_process(shm_ptr, msg_id, error_output);
      return 0;
    }
    if (error_output)
      error_output->runtime_remark("Successfully set limits");
  }
  
  try
  {
    //Sanity-Check
/*    inc_stack();
    for (vector< Statement* >::const_iterator it(statement_stack.begin());
	 it != statement_stack.end(); ++it)
      (*it)->forecast(mysql);
    if (display_sanity_errors(xml_raw))
      return 0;
    dec_stack();
  
    if (get_debug_mode() == STATIC_ANALYSIS)
    {
      static_analysis(xml_raw);
      return 0;
    }
  
    log_script(xml_raw);
    
    current_db = detect_active_database();
    if (current_db == "")
    {
      out_footer(output_mode);
      return 0;
    }
    
    db_subdir = current_db;
    if ((db_subdir.size() > 0) && (db_subdir[db_subdir.size()-1] != '/'))
      db_subdir += '/';
    void_query(mysql, (string)("use ") + current_db);
    (*dynamic_cast< Root_Statement* >(statement_stack.front()))
	.set_database_id(current_db[current_db.size()-1] - 48);*/
    
    if (show_mime)
      cout<<"Content-type: application/osm3s\n\n";
    cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-derived>\n"
      "<note>The data included in this document is from www.openstreetmap.org. "
      "It has there been collected by a large group of contributors. For individual "
      "attribution of each item please refer to "
      "http://www.openstreetmap.org/api/0.6/[node|way|relation]/#id/history </note>\n";
    cout<<"<meta data_included_until=\""
        <<(shm_ptr != 0 ? (const char*)(shm_ptr+OFFSET_DB_1+4) : "unknown")
	<<"\" last_rule_applied=\""<<0<<"\"/>\n"
      "\n";

    Resource_Manager rman;
    for (vector< Statement* >::const_iterator it(statement_stack.begin());
	 it != statement_stack.end(); ++it)
      (*it)->execute(rman);
  
    cout<<"\n</osm-derived>\n";
  }
  catch(File_Error e)
  {
    ostringstream temp;
    temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
    if (error_output)
      error_output->runtime_error(temp.str());
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
  
  unregister_process(shm_ptr, msg_id, error_output);

  return 0;
}
