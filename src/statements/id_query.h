#ifndef ID_QUERY_STATEMENT_DEFINED
#define ID_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Id_Query_Statement : public Statement
{
  public:
    Id_Query_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "id-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Id_Query_Statement() {}
    
  private:
    string output;
    int type;
    unsigned int ref;
};

#endif
