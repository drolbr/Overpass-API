#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__FOREACH_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__FOREACH_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"

using namespace std;

class Foreach_Statement : public Statement
{
  public:
    Foreach_Statement(int line_number_, const map< string, string >& attributes);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "foreach"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Foreach_Statement() {}
    
  private:
    string input, output;
    vector< Statement* > substatements;
};

#endif
