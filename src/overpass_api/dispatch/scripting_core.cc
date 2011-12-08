#include "resource_manager.h"
#include "scripting_core.h"
#include "../core/settings.h"
#include "../frontend/console_output.h"
#include "../frontend/map_ql_parser.h"
#include "../frontend/user_interface.h"
#include "../statements/area_query.h"
#include "../statements/coord_query.h"
#include "../statements/make_area.h"
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
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace
{
  vector< Statement* > statement_stack;
  vector< string > text_stack;
  Script_Parser xml_parser;
  bool uses_meta_data;
}

Dispatcher_Stub::Dispatcher_Stub
    (string db_dir_, Error_Output* error_output_, string xml_raw, int& area_level)
    : db_dir(db_dir_), error_output(error_output_),
      dispatcher_client(0), area_dispatcher_client(0),
      transaction(0), area_transaction(0), rman(0), meta(get_uses_meta_data())
{
  if ((area_level < 2) && (Make_Area_Statement::is_used()))
  {
    if (error_output)
    {
      error_output->runtime_error
          ("Specify --rules to execute a rule. make-area can only appear in rules.");
      throw Exit_Error();
    }
  }
  if ((area_level == 0) &&
      ((Coord_Query_Statement::is_used()) || (Area_Query_Statement::is_used())))
    area_level = 1;
  
  if (db_dir == "")
  {
    dispatcher_client = new Dispatcher_Client(osm_base_settings().shared_name);
    Logger logger(dispatcher_client->get_db_dir());
    try
    {
      logger.annotated_log("request_read_and_idx() start");
      dispatcher_client->request_read_and_idx();
      logger.annotated_log("request_read_and_idx() end");
    }
    catch (const File_Error& e)
    {
      ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number;
      logger.annotated_log(out.str());
      throw;
    }
    transaction = new Nonsynced_Transaction
        (false, false, dispatcher_client->get_db_dir(), "");
  
    transaction->data_index(osm_base_settings().NODES);
    transaction->random_index(osm_base_settings().NODES);
    transaction->data_index(osm_base_settings().NODE_TAGS_LOCAL);
    transaction->data_index(osm_base_settings().NODE_TAGS_GLOBAL);
    transaction->data_index(osm_base_settings().WAYS);
    transaction->random_index(osm_base_settings().WAYS);
    transaction->data_index(osm_base_settings().WAY_TAGS_LOCAL);
    transaction->data_index(osm_base_settings().WAY_TAGS_GLOBAL);
    transaction->data_index(osm_base_settings().RELATIONS);
    transaction->random_index(osm_base_settings().RELATIONS);
    transaction->data_index(osm_base_settings().RELATION_ROLES);
    transaction->data_index(osm_base_settings().RELATION_TAGS_LOCAL);
    transaction->data_index(osm_base_settings().RELATION_TAGS_GLOBAL);
    
    if (meta)
    {
      transaction->data_index(meta_settings().NODES_META);
      transaction->data_index(meta_settings().WAYS_META);
      transaction->data_index(meta_settings().RELATIONS_META);
      transaction->data_index(meta_settings().USER_DATA);
      transaction->data_index(meta_settings().USER_INDICES);
    }
    
    {
      ifstream version((dispatcher_client->get_db_dir() + "osm_base_version").c_str());
      getline(version, timestamp);
    }
    try
    {
      logger.annotated_log("read_idx_finished() start");
      dispatcher_client->read_idx_finished();
      logger.annotated_log("read_idx_finished() end");
      logger.annotated_log('\n' + xml_raw);
    }
    catch (const File_Error& e)
    {
      ostringstream out;
      out<<e.origin<<' '<<e.filename<<' '<<e.error_number;
      logger.annotated_log(out.str());
      throw;
    }
    
    if (area_level > 0)
    {
      area_dispatcher_client = new Dispatcher_Client(area_settings().shared_name);
      Logger logger(area_dispatcher_client->get_db_dir());
      
      if (area_level == 1)
      {
	try
	{
          logger.annotated_log("request_read_and_idx() area start");
          area_dispatcher_client->request_read_and_idx();
          logger.annotated_log("request_read_and_idx() area end");
        }
	catch (const File_Error& e)
	{
	  ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number;
	  logger.annotated_log(out.str());
	  throw;
	}
	area_transaction = new Nonsynced_Transaction
            (false, false, area_dispatcher_client->get_db_dir(), "");
	{
	  ifstream version((area_dispatcher_client->get_db_dir() +   
	      "area_version").c_str());
	  getline(version, area_timestamp);
	}
      }
      else if (area_level == 2)
      {
	try
	{
	  logger.annotated_log("write_start() area start");
	  area_dispatcher_client->write_start();
	  logger.annotated_log("write_start() area end");
	}
	catch (const File_Error& e)
	{
	  ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number;
	  logger.annotated_log(out.str());
	  throw;
	}
	area_transaction = new Nonsynced_Transaction
	    (true, true, area_dispatcher_client->get_db_dir(), "");
	{
	  ofstream area_version((area_dispatcher_client->get_db_dir()
	      + "area_version.shadow").c_str());
	  area_version<<timestamp<<'\n';
	  area_timestamp = timestamp;
	}
      }
      
      area_transaction->data_index(area_settings().AREAS);
      area_transaction->data_index(area_settings().AREA_BLOCKS);
      area_transaction->data_index(area_settings().AREA_TAGS_LOCAL);
      area_transaction->data_index(area_settings().AREA_TAGS_GLOBAL);

      if (area_level == 1)
      {
	try
	{
          logger.annotated_log("read_idx_finished() area start");
          area_dispatcher_client->read_idx_finished();
          logger.annotated_log("read_idx_finished() area end");
	}
	catch (const File_Error& e)
	{
	  ostringstream out;
	  out<<e.origin<<' '<<e.filename<<' '<<e.error_number;
	  logger.annotated_log(out.str());
	  throw;
	}
      }

      rman = new Resource_Manager(*transaction, area_level == 2 ? error_output : 0,
				  *area_transaction, this, area_level == 2);
    }
    else
      rman = new Resource_Manager(*transaction, this);
  }
  else
  {
    transaction = new Nonsynced_Transaction(false, false, db_dir, "");
    if (area_level > 0)
    {
      area_transaction = new Nonsynced_Transaction(area_level == 2, false, db_dir, "");
      rman = new Resource_Manager(*transaction, area_level == 2 ? error_output : 0,
				  *area_transaction, this, area_level == 2);
    }
    else
      rman = new Resource_Manager(*transaction, this);
    
    {
      ifstream version((db_dir + "osm_base_version").c_str());
      getline(version, timestamp);
    }
    if (area_level == 1)
    {
      ifstream version((db_dir + "area_version").c_str());
      getline(version, area_timestamp);
    }
    else if (area_level == 2)
    {
      ofstream area_version((db_dir + "area_version").c_str());
      area_version<<timestamp<<'\n';
      area_timestamp = timestamp;
    }
  }
}

void Dispatcher_Stub::set_limits()
{
}

void Dispatcher_Stub::ping() const
{
  if (dispatcher_client)
    dispatcher_client->ping();
  if (area_dispatcher_client)
    area_dispatcher_client->ping();
}

Dispatcher_Stub::~Dispatcher_Stub()
{
  bool areas_written = (rman->area_updater() != 0);
  delete rman;
  if (transaction)
    delete transaction;
  if (area_transaction)
    delete area_transaction;
  if (dispatcher_client)
  {
    Logger logger(dispatcher_client->get_db_dir());
    logger.annotated_log("read_finished() start");
    dispatcher_client->read_finished();
    logger.annotated_log("read_finished() end");
    delete dispatcher_client;
  }
  if (area_dispatcher_client)
  {
    if (areas_written)
    {
      Logger logger(area_dispatcher_client->get_db_dir());
      logger.annotated_log("write_commit() area start");
      area_dispatcher_client->write_commit();
      rename((area_dispatcher_client->get_db_dir() + "area_version.shadow").c_str(),
	     (area_dispatcher_client->get_db_dir() + "area_version").c_str());
      logger.annotated_log("write_commit() area end");
    }
    else
    {
      Logger logger(area_dispatcher_client->get_db_dir());
      logger.annotated_log("read_finished() area start");
      area_dispatcher_client->read_finished();
      logger.annotated_log("read_finished() area end");
    }
    delete area_dispatcher_client;
  }
}

void start(const char *el, const char **attr)
{
  Statement* statement(Statement::create_statement
      (el, xml_parser.current_line_number(), convert_c_pairs(attr)));
      
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
    if (statement_stack.back() && statement)
      statement_stack.back()->add_statement(statement, text_stack.back());
    text_stack.pop_back();
  }
  else if ((statement_stack.size() == 1) && (statement_stack.front()))
    statement_stack.front()->add_final_text(xml_parser.get_parsed_text());
}

bool parse_and_validate
    (const string& xml_raw, Error_Output* error_output, Debug_Level debug_level)
{
  if (error_output && error_output->display_encoding_errors())
    return false;
  
  unsigned int pos(0);
  while (pos < xml_raw.size() && isspace(xml_raw[pos]))
    ++pos;
  
  if (pos < xml_raw.size() && xml_raw[pos] == '<')
  {
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
      temp<<"open: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
      if (error_output)
        error_output->runtime_error(temp.str());
    
      return false;
    }
  }
  else
  {
    if (debug_level == parser_execute)
      parse_and_validate_map_ql(xml_raw, error_output);
    else if (debug_level == parser_dump_xml)
      parse_and_dump_xml_from_map_ql(xml_raw, error_output);
    else if (debug_level == parser_dump_compact_map_ql)
      parse_and_dump_compact_from_map_ql(xml_raw, error_output);
    else if (debug_level == parser_dump_pretty_map_ql)
      parse_and_dump_pretty_from_map_ql(xml_raw, error_output);
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

bool get_uses_meta_data()
{
  return true;
}
