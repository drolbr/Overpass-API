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
#include "user_interface.h"
#include "vigilance_control.h"

#include <mysql.h>

using namespace std;

const char* types_lowercase[] = { "", "node", "way", "relation", "area" };
const char* types_uppercase[] = { "", "Node", "Way", "Relation", "Area" };

static string rule_name_;
static int rule_id_;
static vector< pair< int, int > > stack;

void set_rule(int rule_id, string rule_name)
{
  rule_id_ = rule_id;
  rule_name_ = rule_name;
}

int get_rule_id()
{
  return rule_id_;
}

string get_rule_name()
{
  return rule_name_;
}

void push_stack(int type, int id)
{
  stack.push_back(make_pair< int, int>(type, id));
}

void pop_stack()
{
  stack.pop_back();
}

const vector< pair< int, int > >& get_stack()
{
  return stack;
}

//-----------------------------------------------------------------------------

void eval_cstr_array(string element, map< string, string >& attributes, const char **attr)
{
  for (unsigned int i(0); attr[i]; i += 2)
  {
    map< string, string >::iterator it(attributes.find(attr[i]));
    if (it != attributes.end())
      it->second = attr[i+1];
    else
    {
      ostringstream temp;
      temp<<"Unknown attribute \""<<attr[i]<<"\" in element \""<<element<<"\".";
      add_static_error(temp.str());
    }
  }
}

void substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  add_static_error(temp.str());
  
  delete child;
}

//-----------------------------------------------------------------------------

void assure_no_text(string text, string name)
{
  for (unsigned int i(0); i < text.size(); ++i)
  {
    if (!isspace(text[i]))
    {
      ostringstream temp;
      temp<<"Element \""<<name<<"\" must not contain text.";
      add_static_error(temp.str());
      break;
    }
  }
}

void Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  substatement_error(get_name(), statement);
}

void Statement::add_final_text(string text)
{
  assure_no_text(text, this->get_name());
}

//-----------------------------------------------------------------------------

void Root_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["timeout"] = "0";
  attributes["name"] = "";
  attributes["replace"] = "0";
  attributes["version"] = "0";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  name = attributes["name"];
  replace = atoi(attributes["replace"].c_str());
  timeout = atoi(attributes["timeout"].c_str());
  version = atoi(attributes["version"].c_str());
}

void Root_Statement::add_statement(Statement* statement, string text)
{
  assure_no_text(text, this->get_name());
  
  if ((statement->get_name() == "union") ||
       (statement->get_name() == "id-query") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "recurse") ||
       (statement->get_name() == "foreach") ||
       (statement->get_name() == "make-area") ||
       (statement->get_name() == "coord-query") ||
       (statement->get_name() == "print") ||
       (statement->get_name() == "conflict") ||
       (statement->get_name() == "report") ||
       (statement->get_name() == "detect-odd-nodes"))
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Root_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  if (timeout > 0)
  {
    if (add_timeout(mysql_thread_id(mysql), timeout))
      return;
  }
  
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    (*it)->execute(mysql, maps);
    
    statement_finished(*it);
  }

  if (timeout > 0)
    remove_timeout();
}

//-----------------------------------------------------------------------------

vector< string > role_cache;

const vector< string >& get_role_cache()
{
  return role_cache;
}

void prepare_caches(MYSQL* mysql)
{
  role_cache.push_back("");
  
  MYSQL_RES* result(mysql_query_wrapper(mysql, "select id, role from member_roles"));
  if (!result)
    return;
  
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]))
  {
    unsigned int id((unsigned int)atol(row[0]));
    if (role_cache.size() <= id)
      role_cache.resize(id+64);
    role_cache[id] = row[1];
    row = mysql_fetch_row(result);
  }
}
