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
#include "item_statement.h"

#include <mysql.h>

using namespace std;

void Item_Statement::set_attributes(const char **attr)
{
  map< string, string > attributes;
  
  attributes["set"] = "_";
  
  eval_cstr_array(get_name(), attributes, attr);
  
  output = attributes["set"];
}
