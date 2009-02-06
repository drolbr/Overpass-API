#ifndef DETECT_ODD_NODES_STATEMENT_DEFINED
#define DETECT_ODD_NODES_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

// places where to register a new statement
// * Makefile (1x)

using namespace std;

class Detect_Odd_Nodes_Statement : public Statement
{
  public:
    Detect_Odd_Nodes_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "detect-odd-nodes"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Detect_Odd_Nodes_Statement() {}
    
  private:
    string input, output;
};

#endif
