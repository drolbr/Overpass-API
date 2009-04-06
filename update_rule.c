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
  string xml_raw(get_xml_raw());
  if (display_encoding_errors(cout))
    return 0;
  if (display_parse_errors(cout, xml_raw))
    return 0;
  
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
    add_static_error("Updating a rule requires the name of the rule.");
  if (!rule_replace)
    add_static_error("Updating a rule requires providing its last version-id.");
  if (rule_version)
    add_static_error("Providing a version-id while updating a rule is not allowed.");
  
  xml_raw = xml_raw.substr(xml_raw.find("<osm-script"));
  xml_raw = xml_raw.substr(xml_raw.find('>') + 1);
  if (xml_raw.find("</osm-script>") != string::npos)
    xml_raw = xml_raw.substr(0, xml_raw.find("</osm-script>"));
  else
    xml_raw = "";
    
  if (display_static_errors(cout, xml_raw))
    return 0;
  
  //Sanity-Check
  
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    runtime_error("Connection to database failed.\n", cout);
    out_footer(cout, output_mode);
    return 0;
  }
  
  out_header(cout, output_mode);
  
  ostringstream temp;
  temp<<"select id from rule_names where name = '";
  escape_insert(temp, rule_name);
  temp<<'\'';
  int name_id(int_query(mysql, temp.str()));
  
  if (!name_id)
  {
    temp.str("");
    temp<<"Rule '"<<rule_name<<"' not found in the database.";
    runtime_error(temp.str(), cout);
    out_footer(cout, output_mode);
    return 0;
  }
  
  temp.str("");
  temp<<"select max(id) from rule_bodys where rule = "<<name_id;
  int last_id(int_query(mysql, temp.str().c_str()));
  if (last_id != rule_replace)
  {
    temp.str("");
    temp<<"A newer version ("<<last_id<<") of rule '"<<rule_name
	<<"' has meanwhile been stored in the database.";
    runtime_error(temp.str(), cout);
    out_footer(cout, output_mode);
    return 0;
  }
  
  int body_id(int_query(mysql, "select max(id) from rule_bodys")+1);

  temp.str("");
  temp<<"insert rule_bodys values ("<<body_id<<", "<<name_id<<", '";
  escape_insert(temp, xml_raw);
  temp<<"')";
  mysql_query(mysql, temp.str().c_str());
  
  temp.str("");
  temp<<"Rule '"<<rule_name<<"' successfully updated.";
  runtime_remark(temp.str(), cout);
  
  //Undo old version of the rule
  
  //conflicts
  temp.str("");
  temp<<"select id from conflicts where rule = "<<rule_replace;
  set< uint32 > attic_conflicts;
  multiint_query(mysql, temp.str(), attic_conflicts);
  multiint_to_null_query(mysql, "delete from node_conflicts where conflict in", "", attic_conflicts);
  multiint_to_null_query(mysql, "delete from way_conflicts where conflict in", "", attic_conflicts);
  multiint_to_null_query(mysql, "delete from relation_conflicts where conflict in", "", attic_conflicts);
  temp.str("");
  temp<<"delete from conflicts where rule = "<<rule_replace;
  mysql_query(mysql, temp.str().c_str());
  
  //areas
  temp.str("");
  temp<<"select id from area_origins where rule = "<<rule_replace;
  set< uint32 > pre_attic_areas, non_attic_areas, attic_areas;
  multiint_query(mysql, temp.str(), pre_attic_areas);
  
  temp.str("");
  temp<<"delete from area_origins where rule = "<<rule_replace;
  mysql_query(mysql, temp.str().c_str());
  
  temp.str("");
  multiint_to_multiint_query(mysql, "select id from area_origins where id in", "",
			     pre_attic_areas, non_attic_areas);
  set< uint32 >::const_iterator pait(pre_attic_areas.begin());
  set< uint32 >::const_iterator nait(non_attic_areas.begin());
  while (pait != pre_attic_areas.end())
  {
    if ((nait != non_attic_areas.end()) && (*nait == *pait))
      ++nait;
    else
      attic_areas.insert(*pait);
    ++pait;
  }

  multiint_to_null_query(mysql, "delete from areas where id in", "", attic_areas);
  multiint_to_null_query(mysql, "delete from area_segments where id in", "", attic_areas);
  multiint_to_null_query(mysql, "delete from area_tags where id in", "", attic_areas);
  multiint_to_null_query(mysql, "delete from area_ways where id in", "", attic_areas);
  
  set_rule(body_id, rule_name);
  //Rule execution
  prepare_caches(mysql);
  
  map< string, Set > maps;
  for (vector< Statement* >::const_iterator it(statement_stack.begin());
       it != statement_stack.end(); ++it)
    (*it)->execute(mysql, maps);
  
  out_footer(cout, output_mode);
  
  mysql_close(mysql);
  
  return 0;
}
