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

#include <mysql.h>

using namespace std;

static int output_mode(MIXED_XML);

MYSQL* mysql(NULL);

string db_subdir;

int main(int argc, char *argv[])
{
  mysql = mysql_init(NULL);
  
  if (!mysql_real_connect(mysql, "localhost", "osm", "osm", "osm", 0, NULL,
       CLIENT_LOCAL_FILES))
  {
    runtime_error("Connection to database failed.\n");
    out_footer(output_mode);
    return 0;
  }
  
  out_header(output_mode);
  
  ostringstream temp;
  temp<<"select rule_bodys.id, rule_names.name, rule_bodys.source from rule_bodys "
      <<"left join rule_names on rule_names.id = rule_bodys.rule where rule_bodys.id > 0";
  MYSQL_RES* result(mysql_query_wrapper(mysql, temp.str()));
  if (!result)
  {
    out_footer(output_mode);
    return 0;
  }
  
  MYSQL_ROW row(mysql_fetch_row(result));
  while ((row) && (row[0]) && (row[1]) && (row[2]))
  {
    get_output().print("<osm-script name=\"");
    get_output().print(row[1]);
    get_output().print("\" version=\"");
    get_output().print(atoi(row[0]));
    get_output().print("\">");
    get_output().print(row[2]);
    get_output().print("</osm-script>\n\n");
    row = mysql_fetch_row(result);
  }
  mysql_free_result(result);
  
  out_footer(output_mode);
  
  mysql_close(mysql);
  
  return 0;
}
