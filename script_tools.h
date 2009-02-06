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

class Set_Forecast
{
  public:
    Set_Forecast()
      : origin(-1), node_count(0), way_count(0), relation_count(0), area_count(0), depends_on() {}
  
    Set_Forecast(int origin_, int node_count_, int way_count_, int relation_count_, int area_count_)
      : origin(origin_), node_count(node_count_), way_count(way_count_), relation_count(relation_count_),
			    area_count(area_count_), depends_on() {}
			    
    int origin;
    int node_count;
    int way_count;
    int relation_count;
    int area_count;
    vector< pair< string, int > > depends_on;
};

// inline bool operator<(const Set_Forecast& sf_1, const Set_Forecast& sf_2)
// {
//   return (sf_1.name < sf_2.name);
// }

// class Flow_Forecast
// {
//   public:
//     void begin_statement(string name, int line, int stmt_id) {}
//     void end_statement() {}
//     void add_input(string name);
//     void add_in_out(string name, const Set_Forecast& sf) { sets[name] = sf; }
//     void add_output(string name, const Set_Forecast& sf) { sets[name] = sf; }
//     void add_time(int milliseconds) { used_time += milliseconds; }
//     const Set_Forecast& get_set(string name);
//     
//   private:
//     map< string, Set_Forecast > sets;
//     long long used_time;
//     int stack_depth;
// };

int next_stmt_id();

class Statement
{
  public:
    Statement() : line_number(current_line_number()), stmt_id(next_stmt_id()) {}
    
    virtual void set_attributes(const char **attr) = 0;
    virtual void add_statement(Statement* statement, string text);
    virtual void add_final_text(string text);
    virtual string get_name() const = 0;
    virtual string get_result_name() const = 0;
    virtual void forecast() = 0;
    virtual void execute(MYSQL* mysql, map< string, Set >& maps) = 0;
    virtual ~Statement() {}
    
    int get_line_number() const { return line_number; }
    int get_stmt_id() const { return stmt_id; }
    int get_startpos() const { return startpos; }
    void set_startpos(int pos) { startpos = pos; }
    int get_endpos() const { return endpos; }
    void set_endpos(int pos) { endpos = pos; }
    int get_tagendpos() const { return tagendpos; }
    void set_tagendpos(int pos) { tagendpos = pos; }
    
    void display_full();
    void display_starttag();
    
  private:
    int line_number;
    int stmt_id;
    int startpos, endpos, tagendpos;
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
    virtual void forecast();
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

const int READ_FORECAST = 1;
const int WRITE_FORECAST = 2;
const int UNION_FORECAST = 3;

void declare_used_time(int milliseconds);
const Set_Forecast& declare_read_set(string name);
Set_Forecast& declare_write_set(string name);
Set_Forecast& declare_union_set(string name);
void inc_stack();
void dec_stack();
const vector< pair< int, string > >& pending_stack();
int stack_time_offset();
void finish_statement_forecast();
void display_state();

#endif
