#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__OSM_SCRIPT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__OSM_SCRIPT_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Osm_Script_Statement : public Statement
{
  public:
    Osm_Script_Statement(int line_number_, const map< string, string >& input_attributes);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "osm-script"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Osm_Script_Statement();
    
    virtual void set_output_handle(Output_Handle* output);
    
    /*    string get_rule_name() { return name; }
    int get_rule_replace() { return replace; }
    int get_rule_version() { return version; }
    void set_database_id(uint database_id_) { database_id = database_id_; }*/
    
  private:
    vector< Statement* > substatements;
    uint32 max_allowed_time;
    uint64 max_allowed_space;
    /* string name;
    int replace, version;
    uint database_id;*/
};

#endif
