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
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Print_Statement() {}
    
  private:
    string input;
    unsigned int mode;
};

#endif
