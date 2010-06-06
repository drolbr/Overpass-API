#ifndef QUERY_STATEMENT_DEFINED
#define QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"
#include "bbox_query.h"

using namespace std;

class Query_Statement : public Statement
{
  public:
    Query_Statement(int line_number_, int stmt_id_)
      : Statement(line_number_, stmt_id_)/*, area_restriction(0)*/,
        bbox_restriction(0) {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Query_Statement() {}
    
  private:
    string output;
    unsigned int type;
    vector< pair< string, string > > key_values;
    //Area_Query_Statement* area_restriction;
    Bbox_Query_Statement* bbox_restriction;
    
    vector< uint32 >* collect_ids
        (const vector< pair< string, string > >& key_values,
	 const File_Properties& file_prop, uint32 stopwatch_account);
};

class Has_Key_Value_Statement : public Statement
{
  public:
    Has_Key_Value_Statement(int line_number_, int stmt_id_)
      : Statement(line_number_, stmt_id_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "has-kv"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps) {}
    virtual ~Has_Key_Value_Statement() {}
    
    string get_key() { return key; }
    string get_value() { return value; }
    
  private:
    string key, value;
};

#endif
