#ifndef PRINT_STATEMENT_DEFINED
#define PRINT_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Print_Statement : public Statement
{
  public:
    Print_Statement() {}
    
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "print"; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Print_Statement() {}
    
  private:
    string input;
    unsigned int mode;
};

#endif
