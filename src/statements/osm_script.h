#ifndef OSM_SCRIPT_STATEMENT_DEFINED
#define OSM_SCRIPT_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Osm_Script_Statement : public Statement
{
  public:
    Osm_Script_Statement(int line_number_)
    : Statement(line_number_)/*, timeout(0), database_id(0)*/ {}
    virtual void set_attributes(const char **attr);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "osm-script"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Osm_Script_Statement() {}
    
/*    string get_rule_name() { return name; }
    int get_rule_replace() { return replace; }
    int get_rule_version() { return version; }
    void set_database_id(uint database_id_) { database_id = database_id_; }*/
    
  private:
    vector< Statement* > substatements;
/*    int timeout, elem_limit;
    string name;
    int replace, version;
    uint database_id;*/
};

#endif
