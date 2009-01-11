#ifndef ITEM_STATEMENT_DEFINED
#define ITEM_STATEMENT_DEFINED

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

class Item_Statement : public Statement
{
  public:
    Item_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement);
    virtual string get_name() { return "item"; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Item_Statement() {}
    
    virtual string get_result_name() { return output; }

  private:
    string output;
};

#endif
