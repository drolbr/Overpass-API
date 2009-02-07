#ifndef CONFLICT_STATEMENT_DEFINED
#define CONFLICT_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Conflict_Statement : public Statement
{
  public:
    Conflict_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() const { return "conflict"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Conflict_Statement() {}
    
  private:
    string input;
    vector< string > message;
    vector< string > items;
};

#endif
