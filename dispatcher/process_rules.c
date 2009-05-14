#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "../expat_justparse_interface.h"
#include "../script_datatypes.h"
#include "../script_queries.h"
#include "../script_tools.h"
#include "../statement_factory.h"
#include "../user_interface.h"
#include "process_rules.h"

#include <mysql.h>

using namespace std;

static int output_mode(HTML);

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

void retrieve_rules(MYSQL* mysql, uint32 max_version,
		    map< string, uint32 >& rules, map< string, string >& bodys)
{
  ostringstream temp;
  temp<<"select rule_bodys.id, rule_names.name, rule_bodys.source from rule_bodys "
      <<"left join rule_names on rule_names.id = rule_bodys.rule where rule_bodys.id > 0 "
      <<"and rule_bodys.id < "<<max_version;
  MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
  if (!result)
    return;
  
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]))
  {
    uint32 version(atoll(row[0]));
    string name(row[1]);
    uint32& prev_version(rules[name]);
    if (version > prev_version)
    {
      prev_version = version;
      bodys[name] = row[2];
    }
    
    row = mysql_fetch_row(result);
  }
  mysql_free_result(result);
}

void execute_rules(MYSQL* mysql, const map< string, uint32 >& rules, const map< string, string >& bodys)
{
  prepare_caches(mysql);
  
  ostringstream temp;
  temp<<"Applying Rules";
  runtime_remark(temp.str(), cout);
    
  for (map< string, uint32 >::const_iterator it(rules.begin()); it != rules.end(); ++it)
  {
    set_debug_mode(VERBOSE);
  
    temp.str("");
    temp<<"Entering Rule "<<it->first;
    runtime_remark(temp.str(), cout);
    
    string xml_raw("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<osm-script>");
    xml_raw += bodys.find(it->first)->second + "</osm-script>\n";
    
    parse_script(xml_raw, start, end);
  
    set_rule(it->second, it->first);
    //Rule execution
    map< string, Set > maps;
    for (vector< Statement* >::const_iterator it2(statement_stack.begin());
	 it2 != statement_stack.end(); ++it2)
      (*it2)->execute(mysql, maps);
  
    statement_stack.clear();
  }
  
  out_footer(cout, output_mode);
}

void process_rules(MYSQL* mysql, const string& current_db, uint32 max_version)
{
  map< string, uint32 > rules;
  map< string, string > bodys;
  
  void_query(mysql, "use osm");
  retrieve_rules(mysql, max_version, rules, bodys);
  void_query(mysql, (string)("use ") + current_db);
  execute_rules(mysql, rules, bodys);
  void_query(mysql, "use osm");
}
