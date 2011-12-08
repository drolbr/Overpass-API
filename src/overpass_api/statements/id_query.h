#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__ID_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__ID_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Id_Query_Statement : public Statement
{
  public:
    Id_Query_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "id-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Id_Query_Statement() {}
    
  private:
    string output;
    int type;
    unsigned int ref, lower, upper;
};

#endif
