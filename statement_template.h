#ifndef $(UPPERCASE)_STATEMENT_DEFINED
#define $(UPPERCASE)_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

// places where to register a new statement
// * script-interpreter.c (3x)
// * script_tools.c (1x)
// * Makefile (1x)
// * foreach etc. (several files)

using namespace std;

class $(Capitalize)_Statement : public Statement
{
  public:
    $(Capitalize)_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "$(lowercase)"; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~$(Capitalize)_Statement() {}
    
  private:
    //string input, output;
};

#endif
