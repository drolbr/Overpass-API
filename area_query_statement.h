#ifndef AREA_QUERY_STATEMENT_DEFINED
#define AREA_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Area_Query_Statement : public Statement
{
  public:
    Area_Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "area-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Area_Query_Statement() {}
    
  private:
    string output;
    uint32 area_ref;
};

#endif
