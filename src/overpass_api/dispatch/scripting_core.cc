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
#include <ctime>
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

Dispatcher_Stub::Dispatcher_Stub
    (string db_dir_, Error_Output* error_output_, string xml_raw)
    : db_dir(db_dir_), shm_ptr(0), error_output(error_output_),
      dispatcher_client(0), transaction(0), rman(0)
{
  if (db_dir == "")
  {
    dispatcher_client = new Dispatcher_Client(shared_name);
    Logger logger(dispatcher_client->get_db_dir());
    logger.annotated_log("request_read_and_idx() start");
    dispatcher_client->request_read_and_idx();
    logger.annotated_log("request_read_and_idx() end");
    logger.annotated_log('\n' + xml_raw);
    transaction = new Nonsynced_Transaction
        (false, false, dispatcher_client->get_db_dir(), "");
    rman = new Resource_Manager(*transaction);
  
    transaction->data_index(de_osm3s_file_ids::NODES);
    transaction->random_index(de_osm3s_file_ids::NODES);
    transaction->data_index(de_osm3s_file_ids::NODE_TAGS_LOCAL);
    transaction->data_index(de_osm3s_file_ids::NODE_TAGS_GLOBAL);
    transaction->data_index(de_osm3s_file_ids::WAYS);
    transaction->random_index(de_osm3s_file_ids::WAYS);
    transaction->data_index(de_osm3s_file_ids::WAY_TAGS_LOCAL);
    transaction->data_index(de_osm3s_file_ids::WAY_TAGS_GLOBAL);
    transaction->data_index(de_osm3s_file_ids::RELATIONS);
    transaction->random_index(de_osm3s_file_ids::RELATIONS);
    transaction->data_index(de_osm3s_file_ids::RELATION_ROLES);
    transaction->data_index(de_osm3s_file_ids::RELATION_TAGS_LOCAL);
    transaction->data_index(de_osm3s_file_ids::RELATION_TAGS_GLOBAL);
    
    logger.annotated_log("read_idx_finished() start");
    dispatcher_client->read_idx_finished();
    logger.annotated_log("read_idx_finished() end");
  }
  else
  {
    transaction = new Nonsynced_Transaction(false, false, db_dir, "");
    rman = new Resource_Manager(*transaction);
  }
}

void Dispatcher_Stub::set_limits()
{
}

Dispatcher_Stub::~Dispatcher_Stub()
{
  if (dispatcher_client)
  {
    Logger logger(dispatcher_client->get_db_dir() + get_logfile_name());
    logger.annotated_log("read_finished() start");
    dispatcher_client->read_finished();
    logger.annotated_log("read_finished() end");
    delete dispatcher_client;
  }
  delete transaction;
  delete rman;
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
    (const string& xml_raw, Error_Output* error_output)
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
