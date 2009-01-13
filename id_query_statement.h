#ifndef ID_QUERY_STATEMENT_DEFINED
#define ID_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Id_Query_Statement : public Statement
{
  public:
    Id_Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() { return "id-query"; }
    virtual string get_result_name() { return output; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Id_Query_Statement() {}
    
  private:
    string output;
    unsigned int type;
    unsigned int ref;
};

#endif
