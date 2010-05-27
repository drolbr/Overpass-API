#ifndef PRINT_STATEMENT_DEFINED
#define PRINT_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Print_Statement : public Statement
{
  public:
    Print_Statement(int line_number_, int stmt_id_) : Statement(line_number_, stmt_id_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "print"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Print_Statement() {}
    
  private:
    string input;
    unsigned int mode;
    unsigned int order;
};

#endif
