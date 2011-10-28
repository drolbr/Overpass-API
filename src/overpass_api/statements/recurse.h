#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__RECURSE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__RECURSE_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Recurse_Statement : public Statement
{
  public:
    Recurse_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "recurse"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Recurse_Statement() {}
    
  private:
    string input, output;
    unsigned int type;
};

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result);

#endif
