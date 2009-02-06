#ifndef REPORT_STATEMENT_DEFINED
#define REPORT_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Report_Statement : public Statement
{
  public:
    Report_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "report"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Report_Statement() {}
    
  private:
    string input;
};

#endif
