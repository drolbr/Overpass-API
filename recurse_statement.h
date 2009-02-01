#ifndef RECURSE_STATEMENT_DEFINED
#define RECURSE_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Recurse_Statement : public Statement
{
  public:
    Recurse_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "recurse"; }
    virtual string get_result_name() const { return output; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Recurse_Statement() {}
    
  private:
    string input, output;
    unsigned int type;
};

#endif
