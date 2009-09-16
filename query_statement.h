#ifndef QUERY_STATEMENT_DEFINED
#define QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"
#include "area_query_statement.h"
#include "bbox_query_statement.h"

#include <mysql.h>

using namespace std;

class Query_Statement : public Statement
{
  public:
    Query_Statement() : area_restriction(0), bbox_restriction(0) {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Query_Statement() {}
    
  private:
    string output;
    unsigned int type;
    vector< pair< string, string > > key_values;
    Area_Query_Statement* area_restriction;
    Bbox_Query_Statement* bbox_restriction;
};

class Has_Key_Value_Statement : public Statement
{
  public:
    Has_Key_Value_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "has-kv"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) {}
    virtual ~Has_Key_Value_Statement() {}
    
    string get_key() { return key; }
    string get_value() { return value; }
    
  private:
    string key, value;
};

#endif
