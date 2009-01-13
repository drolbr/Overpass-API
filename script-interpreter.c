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
#include "union_statement.h"
#include "query_statement.h"
#include "id_query_statement.h"
#include "recurse_statement.h"
#include "foreach_statement.h"
#include "item_statement.h"
#include "make_area_statement.h"
#include "coord_query_statement.h"
#include "print_statement.h"
#include "conflict_statement.h"
#include "detect_odd_nodes_statement.h"

#include <mysql.h>

using namespace std;

MYSQL* mysql(NULL);

//-----------------------------------------------------------------------------

Statement* generate_statement(string element)
{
  if (element == "osm-script")
    return new Root_Statement();
  if (element == "union")
    return new Union_Statement();
  else if (element == "id-query")
    return new Id_Query_Statement();
  else if (element == "query")
    return new Query_Statement();
  else if (element == "has-kv")
    return new Has_Key_Value_Statement();
  else if (element == "recurse")
    return new Recurse_Statement();
  else if (element == "foreach")
    return new Foreach_Statement();
  else if (element == "item")
    return new Item_Statement();
  else if (element == "make-area")
    return new Make_Area_Statement();
  else if (element == "coord-query")
    return new Coord_Query_Statement();
  else if (element == "print")
    return new Print_Statement();
  else if (element == "conflict")
    return new Conflict_Statement();
  else if (element == "detect-odd-nodes")
    return new Detect_Odd_Nodes_Statement();
  
  ostringstream temp;
  temp<<"Unknown tag \""<<element<<"\" in line "<<current_line_number()<<'!';
  add_static_error(Error(temp.str(), current_line_number()));
  
  return 0;
}

bool is_known_element(string element)
{
  if ((element == "osm-script") ||
       (element == "union") ||
       (element == "id-query") ||
       (element == "query") ||
       (element == "has-kv") ||
       (element == "recurse") ||
       (element == "foreach") ||
       (element == "item") ||
       (element == "make-area") ||
       (element == "coord-query") ||
       (element == "print") ||
       (element == "conflict") ||
       (element == "detect-odd-nodes"))
    return true;
  
  return false;
}

//-----------------------------------------------------------------------------

vector< Statement* > statement_stack;
vector< string > text_stack;

void start(const char *el, const char **attr)
{
  Statement* statement(generate_statement(el));
  if (statement)
  {
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

//TODO: reasonable area counter
int next_area_id(-1);

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
