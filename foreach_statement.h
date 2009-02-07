#ifndef FOREACH_STATEMENT_DEFINED
#define FOREACH_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Foreach_Statement : public Statement
{
  public:
    Foreach_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "foreach"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Foreach_Statement() {}
    
  private:
    string input, output;
    vector< Statement* > substatements;
};

#endif
