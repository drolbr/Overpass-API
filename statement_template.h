#ifndef $(UPPERCASE)_STATEMENT_DEFINED
#define $(UPPERCASE)_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

// places where to register a new statement
// * statement_factory.h (3x)
// * script_tools.c (1x)
// * Makefile (1x)
// * foreach, union etc. (several files)

using namespace std;

class $(Capitalize)_Statement : public Statement
{
  public:
    $(Capitalize)_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() const { return "$(lowercase)"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~$(Capitalize)_Statement() {}
    
  private:
    //string input, output;
};

#endif
