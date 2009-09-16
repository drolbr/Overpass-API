#ifndef BBOX_QUERY_STATEMENT_DEFINED
#define BBOX_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "script_datatypes.h"

#include <mysql.h>

using namespace std;

class Bbox_Query_Statement : public Statement
{
  public:
    Bbox_Query_Statement() {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "bbox-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast(MYSQL* mysql);
    virtual void execute(MYSQL* mysql, map< string, Set >& maps);
    virtual ~Bbox_Query_Statement() {}
    
    void get_nodes(MYSQL* mysql, set< Node >& nodes);
    uint32 prepare_split(MYSQL* mysql);
    void get_nodes(MYSQL* mysql, set< Node >& nodes, uint32 part);
    
  private:
    string output;
    int32 south, north, west, east;
    map< uint32, set< Line_Segment > > segments_per_tile;
    set< pair< int32, int32 > > in_inside;
    set< pair< int32, int32 > > in_border;
    vector< set< pair< int32, int32 > > > in_inside_v;
    vector< set< pair< int32, int32 > > > in_border_v;
    
    void indices_of_bbox(int32 south, int32 north, int32 west, int32 east);
    bool is_contained(const Node& node);
};

#endif
