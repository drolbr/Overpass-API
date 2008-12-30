#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "cgi-helper.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "query_statement.h"
#include "id_query_statement.h"
#include "recurse_statement.h"
#include "print_statement.h"

#include <mysql.h>

using namespace std;

MYSQL* mysql(NULL);

//-----------------------------------------------------------------------------

Statement* generate_statement(string element)
{
  if (element == "osm-script")
    return new Root_Statement();
  else if (element == "id-query")
    return new Id_Query_Statement();
  else if (element == "query")
    return new Query_Statement();
  else if (element == "has-kv")
    return new Has_Key_Value_Statement();
  else if (element == "recurse")
    return new Recurse_Statement();
  else if (element == "print")
    return new Print_Statement();
  
  ostringstream temp;
  temp<<"Unknown tag \""<<element<<"\" in line "<<current_line_number()<<'!';
  add_static_error(Error(temp.str(), current_line_number()));
  
  return 0;
}

bool is_known_element(string element)
{
  if ((element == "osm-script") ||
       (element == "id-query") ||
       (element == "query") ||
       (element == "has-kv") ||
       (element == "recurse") ||
       (element == "print"))
    return true;
  
  return false;
}

//-----------------------------------------------------------------------------

vector< Statement* > statement_stack;

void start(const char *el, const char **attr)
{
  Statement* statement(generate_statement(el));
  if (statement)
  {
    statement->set_attributes(attr);
    statement_stack.push_back(statement);
  }
}

void end(const char *el)
{
  if ((is_known_element(el)) && (statement_stack.size() > 1))
  {
    Statement* statement(statement_stack.back());
    //Include an end-control to catch e.g. empty query-statements?
    statement_stack.pop_back();
    statement_stack.back()->add_statement(statement);
  }
}

int main(int argc, char *argv[])
{
  int line_offset(0);
  string xml_raw(get_xml_raw(line_offset));
  set_line_offset(line_offset);
  
  if (xml_raw.size() > BUFFSIZE-1)
  {
    ostringstream temp;
    temp<<"Input too long (length: "<<xml_raw.size()<<", max. allowed: "<<BUFFSIZE-1<<')';
    return_error(temp.str(), current_line_number());
    return 0;
  }
  
  string parse_status(parse(xml_raw, start, end));
  if (parse_status != "")
  {
    return_error(parse_status, current_line_number());
    return 0;
  }
  
  if (display_static_errors())
    return 0;
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    return_error("Connection to database failed.\n");
    return 0;
  }
  
  //Sanity-Check
  
  cout<<"Content-type: application/xml\n\n"
      <<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not-osm>\n\n";
  
  prepare_caches(mysql);
  
  map< string, Set > maps;
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
       it != statement_stack.end(); ++it)
    (*it)->execute(mysql, maps);
  
  cout<<"\n</not-osm>"<<'\n';
  
  mysql_close(mysql);
  
  return 0;
}
