#ifndef BBOX_QUERY_STATEMENT_DEFINED
#define BBOX_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

// places where to register a new statement
// * foreach, union etc. (several files)

using namespace std;

class Bbox_Query_Statement : public Statement
{
  public:
    Bbox_Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "bbox-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Bbox_Query_Statement() {}
    
  private:
    string output;
    int south, north, west, east;
};

#endif
