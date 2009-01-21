#ifndef SCRIPT_TOOLS
#define SCRIPT_TOOLS

#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "script_datatypes.h"
#include "script_tools.h"

#include <mysql.h>

using namespace std;

class Statement
{
  public:
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() = 0;
    virtual string get_result_name() = 0;
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) = 0;
    virtual ~Statement() {}
};

void eval_cstr_array(string element, map< string, string >& attributes, const char **attr);

void substatement_error(string parent, Statement* child);
void assure_no_text(string text, string name);

int display_static_errors();

const vector< string >& get_role_cache();
void prepare_caches(MYSQL* mysql);

class Root_Statement : public Statement
{
  public:
    Root_Statement() : timeout(0) {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() { return "osm-script"; }
    virtual string get_result_name() { return ""; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Root_Statement() {}
    
  private:
    vector< Statement* > substatements;
    int timeout;
};

#endif
