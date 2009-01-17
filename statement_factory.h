#ifndef STATEMENT_FACTORY_DEFINED
#define STATEMENT_FACTORY_DEFINED

#include <sstream>
#include <string>
#include "expat_justparse_interface.h"
#include "script_tools.h"
#include "user_interface.h"
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
#include "report_statement.h"
#include "detect_odd_nodes_statement.h"

using namespace std;

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
  else if (element == "report")
    return new Report_Statement();
  else if (element == "detect-odd-nodes")
    return new Detect_Odd_Nodes_Statement();
  
  ostringstream temp;
  temp<<"Unknown tag \""<<element<<"\" in line "<<current_line_number()<<'!';
  add_static_error(temp.str());
  
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
       (element == "report") ||
       (element == "detect-odd-nodes"))
    return true;
  
  return false;
}

#endif
