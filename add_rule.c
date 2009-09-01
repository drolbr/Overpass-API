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

static int output_mode(HTML);

MYSQL* mysql(NULL);

vector< Statement* > statement_stack;
vector< string > text_stack;

void start(const char *el, const char **attr)
{
  Statement* statement(generate_statement(el));
  if (statement)
  {
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

string db_subdir;

int main(int argc, char *argv[])
{
  string xml_raw(get_xml_raw());
  if (display_encoding_errors())
    return 0;
  if (display_parse_errors(xml_raw))
    return 0;
  
  try
  {
    parse_script(xml_raw, start, end);
  }
  catch(Parse_Error parse_error)
  {
    add_parse_error(parse_error.message);
  }
  if (display_parse_errors(xml_raw))
    return 0;
  // getting special information for rules
  string rule_name("");
  int rule_replace(0), rule_version(0);
  Root_Statement* root_statement(dynamic_cast< Root_Statement* >(statement_stack.front()));
  if (root_statement)
  {
    rule_name = root_statement->get_rule_name();
    rule_replace = root_statement->get_rule_replace();
    rule_version = root_statement->get_rule_version();
  }
  if (rule_name == "")
    add_static_error("Adding a rule requires the name of the rule.");
  if (rule_replace)
    add_static_error("Providing which version to replace while adding a rule is not allowed.");
  if (rule_version)
    add_static_error("Providing a version-id while adding a rule is not allowed.");
  
  xml_raw = xml_raw.substr(xml_raw.find("<osm-script"));
  xml_raw = xml_raw.substr(xml_raw.find('>') + 1);
  if (xml_raw.find("</osm-script>") != string::npos)
    xml_raw = xml_raw.substr(0, xml_raw.find("</osm-script>"));
  else
    add_static_error("No content between start and end of the root-tag.");
    
  if (display_static_errors(xml_raw))
    return 0;
  
  //Sanity-Check
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    runtime_error("Connection to database failed.\n");
    return 0;
  }
  
  out_header(output_mode);
  
  int body_id(int_query(mysql, "select max(id) from rule_bodys")+1);
  
  ostringstream temp;
  temp<<"select id from rule_names where name = '";
  escape_insert(temp, rule_name);
  temp<<'\'';
  int name_id(int_query(mysql, temp.str()));
  
  if (name_id)
  {
    temp.str("");
    temp<<"Rule '"<<rule_name<<"' already exists in the database.";
    runtime_error(temp.str());
    out_footer(output_mode);
    return 0;
  }
  
  name_id = int_query(mysql, "select max(id) from rule_names") + 1;
  temp.str("");
  temp<<"insert rule_names values ("<<name_id<<", '";
  escape_insert(temp, rule_name);
  temp<<"')";
  mysql_query(mysql, temp.str().c_str());

  temp.str("");
  temp<<"insert rule_bodys values ("<<body_id<<", "<<name_id<<", '";
  escape_insert(temp, xml_raw);
  temp<<"')";
  mysql_query(mysql, temp.str().c_str());
  
  set_debug_mode(VERBOSE);
  temp.str("");
  temp<<"Rule '"<<rule_name<<"' successfully added with version "
      <<body_id<<".";
  runtime_remark(temp.str());
  
  mysql_close(mysql);
  
  return 0;
}
