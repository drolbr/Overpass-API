#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "raw_file_db.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "statement_factory.h"
#include "user_interface.h"

#include <mysql.h>

using namespace std;

static int output_mode(NOTHING);

MYSQL* mysql(NULL);

vector< Statement* > statement_stack;
vector< string > text_stack;

void start(const char *el, const char **attr)
{
  Statement* statement(generate_statement(el));
  if (statement)
  {
    if (!strcmp(el, "print"))
      output_mode = MIXED_XML;
    else if ((!strcmp(el, "report")) && (output_mode == NOTHING))
      output_mode = HTML;
    statement->set_startpos(get_tag_start());
    statement->set_tagendpos(get_tag_end());
    statement->set_attributes(attr);
    statement_stack.push_back(statement);
    text_stack.push_back(get_parsed_text());
    reset_parsed_text();
  }
}

void end(const char *el)
{
  if ((is_known_element(el)) && (statement_stack.size() > 1))
  {
    Statement* statement(statement_stack.back());
    
    statement->add_final_text(get_parsed_text());
    reset_parsed_text();
    statement->set_endpos(get_tag_end());
    
    statement_stack.pop_back();
    statement_stack.back()->add_statement(statement, text_stack.back());
    text_stack.pop_back();
  }
  else if ((is_known_element(el)) && (statement_stack.size() == 1))
    statement_stack.front()->add_final_text(get_parsed_text());
}

int main(int argc, char *argv[])
{
  string xml_raw(get_xml_raw());
  if (display_encoding_errors(cout))
    return 0;
  if (display_parse_errors(cout, xml_raw))
    return 0;
  
  parse_script(xml_raw, start, end);
  if (display_parse_errors(cout, xml_raw))
    return 0;
  if (display_static_errors(cout, xml_raw))
    return 0;
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    runtime_error("Connection to database failed.\n", cout);
    out_footer(cout, output_mode);
    return 0;
  }
  
  try
  {
    //Sanity-Check
    inc_stack();
    for (vector< Statement* >::const_iterator it(statement_stack.begin());
	 it != statement_stack.end(); ++it)
      (*it)->forecast(mysql);
    if (display_sanity_errors(cout, xml_raw))
      return 0;
    dec_stack();
  
    if (get_debug_mode() == STATIC_ANALYSIS)
    {
      static_analysis(cout, xml_raw);
      return 0;
    }
  
    out_header(cout, output_mode);
  
    prepare_caches(mysql);
  
    map< string, Set > maps;
    for (vector< Statement* >::const_iterator it(statement_stack.begin());
	 it != statement_stack.end(); ++it)
      (*it)->execute(mysql, maps);
  
    out_footer(cout, output_mode);
  
    mysql_close(mysql);
  }
  catch(File_Error e)
  {
    ostringstream temp;
    temp<<"open64: "<<e.error_number<<' '<<e.filename<<' '<<e.origin;
    runtime_error(temp.str(), cout);
  }
  
  return 0;
}
