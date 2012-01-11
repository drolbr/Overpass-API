#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__ITEM_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__ITEM_H

#include <map>
#include <string>
#include <vector>

#include "query.h"
#include "statement.h"

using namespace std;

class Item_Statement : public Statement
{
  public:
    Item_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "item"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman) {}
    virtual ~Item_Statement();
    
    static Generic_Statement_Maker< Item_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
  private:
    string output;
    vector< Query_Constraint* > constraints;
};

#endif
