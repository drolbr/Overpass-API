#ifndef SCRIPT_TOOLS
#define SCRIPT_TOOLS

#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <vector>
#include "expat_justparse_interface.h"
#include "script_datatypes.h"
#include "script_tools.h"

#include <mysql.h>

using namespace std;

class Statement
{
  public:
    Statement() : line_number(current_line_number()) {}
    
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() const = 0;
    virtual string get_result_name() const = 0;
    virtual int get_line_number() const { return line_number; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) = 0;
    virtual ~Statement() {}
    
  private:
    int line_number;
};

const int NODE = 1;
const int WAY = 2;
const int RELATION = 3;
const int AREA = 4;
extern const char* types_lowercase[];
extern const char* types_uppercase[];

void set_rule(int rule_id, string rule_name);
int get_rule_id();
string get_rule_name();
void push_stack(int type, int id);
void pop_stack();
const vector< pair< int, int > >& get_stack();

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
    virtual string get_name() const { return "osm-script"; }
    virtual string get_result_name() const { return ""; }
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Root_Statement() {}
    
    string get_rule_name() { return name; }
    int get_rule_replace() { return replace; }
    int get_rule_version() { return version; }
    
  private:
    vector< Statement* > substatements;
    int timeout;
    string name;
    int replace, version;
};

#endif
