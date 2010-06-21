#include <sstream>
#include <string>
#include <vector>

#include "../expat/expat_justparse_interface.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../statements/statement.h"

using namespace std;

// const char* LOGFILE = "/opt/osm_why_api/dispatcher.log";
// static int output_mode(NOTHING);

vector< Statement* > statement_stack;
vector< string > text_stack;
Script_Parser xml_parser;

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
    ++argpos;
  }
  
  Error_Output* error_output(new Console_Output());
  Statement::set_error_output(error_output);
  
  string xml_raw(get_xml_raw(error_output));
  if (error_output->display_encoding_errors())
    return 0;
  
  try
  {
    xml_parser.parse(xml_raw, start, end);
  }
  catch(Parse_Error parse_error)
  {
    error_output->add_parse_error(parse_error.message,
				  xml_parser.current_line_number());
  }
  catch(File_Error e)
  {
    ostringstream temp;
    temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
    error_output->runtime_error(temp.str());
    
    return 0;
  }
  if (error_output->display_parse_errors())
    return 0;
  if (error_output->display_static_errors())
    return 0;
  
/*  string current_db(detect_active_database());
  db_subdir = current_db;
  if ((db_subdir.size() > 0) && (db_subdir[db_subdir.size()-1] != '/'))
    db_subdir += '/';
  set_meta_db(current_db);*/
  
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
    
/*    out_header(output_mode);*/
    
    map< string, Set > maps;
    for (vector< Statement* >::const_iterator it(statement_stack.begin());
	 it != statement_stack.end(); ++it)
      (*it)->execute(maps);
  
/*    out_footer(output_mode);*/
  }
  catch(File_Error e)
  {
    ostringstream temp;
    temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
    error_output->runtime_error(temp.str());
  }

  return 0;
}
