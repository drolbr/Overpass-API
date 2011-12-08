#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__NEWER_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__NEWER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Newer_Statement : public Statement
{
  public:
    Newer_Statement(int line_number_, const map< string, string >& input_attributes);
    virtual string get_name() const { return "newer"; }
    virtual string get_result_name() const { return ""; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Newer_Statement();

    virtual Query_Constraint* get_query_constraint();
    
    uint64 get_timestamp() const { return than_timestamp; }

  private:
    uint64 than_timestamp;
    vector< Query_Constraint* > constraints;
};

#endif
