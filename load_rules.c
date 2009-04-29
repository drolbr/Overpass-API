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

int main(int argc, char *argv[])
{
  string xml_total_raw(get_xml_raw());
  if (display_encoding_errors(cout))
    return 0;
  if (display_parse_errors(cout, xml_total_raw))
    return 0;
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    runtime_error("Connection to database failed.\n", cout);
    return 0;
  }
  
  out_header(cout, output_mode);
  
  map< string, int > versions;
  map< string, string > bodys;
  
  string::size_type pos(0);
  pos = xml_total_raw.find("<osm-script");
  while ((pos != string::npos) && (pos < xml_total_raw.size()))
  {
    string xml_raw("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    string::size_type endpos(xml_total_raw.find("</osm-script", pos+1));
    if ((endpos != string::npos) && (endpos < xml_total_raw.find("<osm-script", pos+1)))
      xml_raw += xml_total_raw.substr(pos, endpos - pos + 13);
    else
      xml_raw += xml_total_raw.substr(pos, xml_raw.find('>', pos+1) - pos + 1);
    
    parse_script(xml_raw, start, end);
    if (display_parse_errors(cout, xml_raw))
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
      add_static_error("Loading a rule requires the name of the rule.");
    if (rule_replace)
      add_static_error("Providing which version to replace while loading rules is not allowed.");
  
    xml_raw = xml_raw.substr(xml_raw.find("<osm-script"));
    xml_raw = xml_raw.substr(xml_raw.find('>') + 1);
    if (xml_raw.find("</osm-script>") != string::npos)
      xml_raw = xml_raw.substr(0, xml_raw.find("</osm-script>"));
    else
      xml_raw = "";
    
    if (display_static_errors(cout, xml_raw))
      return 0;
  
    ostringstream temp;
    temp<<"select id from rule_names where name = '";
    escape_insert(temp, rule_name);
    temp<<'\'';
    int name_id(int_query(mysql, temp.str()));
  
    if (!name_id)
    {
      name_id = int_query(mysql, "select max(id) from rule_names") + 1;
      temp.str("");
      temp<<"insert rule_names values ("<<name_id<<", '";
      escape_insert(temp, rule_name);
      temp<<"')";
      mysql_query(mysql, temp.str().c_str());
    }
  
    int body_id(rule_version);

    temp.str("");
    temp<<"insert rule_bodys values ("<<body_id<<", "<<name_id<<", '";
    escape_insert(temp, xml_raw);
    temp<<"')";
    mysql_query(mysql, temp.str().c_str());
  
    temp.str("");
    temp<<"Rule '"<<rule_name<<"' version "<<body_id<<" successfully updated.";
    runtime_remark(temp.str(), cout);
  
    if (versions[rule_name] < rule_version)
    {
      versions[rule_name] = rule_version;
      bodys[rule_name] = xml_raw;
    }
  
    statement_stack.clear();
    pos = xml_total_raw.find("<osm-script", ++pos);
  }
  
  for (map< string, int >::const_iterator it(versions.begin()); it != versions.end(); ++it)
  {
    set_debug_mode(VERBOSE);
  
    //TEMP
    ostringstream temp;
    temp<<"Entering Rule "<<it->first;
    runtime_remark(temp.str(), cout);
    
    string xml_raw("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-script>");
    xml_raw += bodys[it->first] + "</osm-script>\n";
    
    parse_script(xml_raw, start, end);
  
    set_rule(it->second, it->first);
    //Rule execution
    prepare_caches(mysql);
  
    map< string, Set > maps;
    for (vector< Statement* >::const_iterator it2(statement_stack.begin());
	 it2 != statement_stack.end(); ++it2)
      (*it2)->execute(mysql, maps);
  
    statement_stack.clear();
  }
  
  out_footer(cout, output_mode);
  
  mysql_close(mysql);
    
  return 0;
}
