#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "statement_factory.h"
#include "user_interface.h"

#include <mysql.h>

using namespace std;

const int NOTHING = 0;
const int HTML = 1;
const int MIXED_XML = 2;
int output_mode(NOTHING);

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
    //Include an end-control to catch e.g. empty query-statements?
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
  
  parse(xml_raw, start, end);
  if (display_parse_errors(cout, xml_raw))
    return 0;
  if (display_static_errors(cout, xml_raw))
    return 0;
  
  return_runtime_error("Gegenwärtig will ich die Datenbank nicht stören.\n", cout);
  return 0;
  
  //Sanity-Check
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    return_runtime_error("Connection to database failed.\n", cout);
    return 0;
  }
  
  if (output_mode == MIXED_XML)
    cout<<"Content-type: application/xml\n\n"
	<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not-osm>\n\n";
  else if  (output_mode == HTML)
    cout<<"Content-type: text/html\n\n"
	<<"<html>\n<head><title>Query Results</title></head>\n<body>\n\n";
  else
    cout<<"Content-type: text/html\n\n"
	<<"<html>\n<head><title>Nothing</title></head>\n<body>\n\n"
	<<"Your query doesn't contain any statement that produces output\n</body>";
  
  prepare_caches(mysql);
  
  map< string, Set > maps;
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
       it != statement_stack.end(); ++it)
    (*it)->execute(mysql, maps);
  
  if (output_mode == MIXED_XML)
    cout<<"\n</not-osm>"<<'\n';
  else
    cout<<"\n</html>"<<'\n';
  
  mysql_close(mysql);
  
  return 0;
}
