#ifndef COORD_QUERY_STATEMENT_DEFINED
#define COORD_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Coord_Query_Statement : public Statement
{
  public:
    Coord_Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "coord-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Coord_Query_Statement() {}
    
  private:
    string output;
    int lat, lon;
};

#endif
