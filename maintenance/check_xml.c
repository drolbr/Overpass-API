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

static int output_mode(MIXED_XML);

MYSQL* mysql(NULL);

vector< Statement* > statement_stack;
vector< string > text_stack;

void start(const char *el, const char **attr)
{
}

void end(const char *el)
{
}

string db_subdir;

int main(int argc, char *argv[])
{
  set_output_cout();
  
  string xml_raw(get_xml_raw(512*1024*1024));
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
  
  return 0;
}
