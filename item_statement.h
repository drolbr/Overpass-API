#ifndef ITEM_STATEMENT_DEFINED
#define ITEM_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Item_Statement : public Statement
{
  public:
    Item_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "item"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) {}
    virtual ~Item_Statement() {}
    
  private:
    string output;
};

#endif
