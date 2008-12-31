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

using namespace std;

vector< Error > parsing_errors;
vector< Error > static_errors;

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
      temp<<"In Line "<<current_line_number()
	  <<": Unknown attribute \""<<attr[i]<<"\" in element \""<<element<<"\".";
      static_errors.push_back(Error(temp.str(), current_line_number()));
    }
  }
}

void add_static_error(const Error& e)
{
  static_errors.push_back(e);
}

void substatement_error(string parent, Statement* child)
{
  ostringstream temp;
  temp<<"In Line "<<current_line_number()
      <<": Element \""<<child->get_name()<<"\" cannot be subelement of element \""<<parent<<"\".";
  static_errors.push_back(Error(temp.str(), current_line_number()));
  
  delete child;
}

int display_static_errors()
{
  if (static_errors.size() == 0)
    return 0;
    
  cout<<"Content-type: text/html\n\n";
  
  cout<<"<html>\n<head>\n<title>Static Error(s)!</title>\n</head>\n<body>\n";
  for(vector< Error >::const_iterator it(static_errors.begin());
      it != static_errors.end(); ++it)
  {
    cout<<"<p>"<<it->text<<"</p>\n";
  }
  cout<<"\n</body>\n</html>\n";

  return 1;
}

//-----------------------------------------------------------------------------

void Root_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  eval_cstr_array(get_name(), attributes, attr);
}

void Root_Statement::add_statement(Statement* statement)
{
  if ((statement->get_name() == "id-query") ||
       (statement->get_name() == "query") ||
       (statement->get_name() == "recurse") ||
       (statement->get_name() == "make-area") ||
       (statement->get_name() == "print"))
    substatements.push_back(statement);
  else
    substatement_error(get_name(), statement);
}

void Root_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
  for (vector< Statement* >::iterator it(substatements.begin());
       it != substatements.end(); ++it)
  {
    cout<<"+++ "<<(uintmax_t)time(NULL)<<'\n';
    (*it)->execute(mysql, maps);
  }
  cout<<"+++ "<<(uintmax_t)time(NULL)<<'\n';
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
