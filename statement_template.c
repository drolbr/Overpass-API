#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_queries.h"
#include "script_tools.h"
#include "$(filename)_statement.h"

#include <mysql.h>

using namespace std;

void $(Capitalize)_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["from"] = "_";
  attributes["into"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  input = attributes["into"];
  output = attributes["into"];
}

void $(Capitalize)_Statement::add_statement(Statement* statement)
{
  substatement_error(get_name(), statement);
}

void $(Capitalize)_Statement::execute(MYSQL* mysql, map< string, Set >& maps)
{
}
