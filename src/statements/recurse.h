#ifndef RECURSE_STATEMENT_DEFINED
#define RECURSE_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Recurse_Statement : public Statement
{
  public:
    Recurse_Statement(int line_number_, int stmt_id_) : Statement(line_number_, stmt_id_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "recurse"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Recurse_Statement() {}
    
  private:
    string input, output;
    unsigned int type;
};

#endif
