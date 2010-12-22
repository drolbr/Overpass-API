#ifndef DE_OSM3S__AREA_QUERY_STATEMENT_DEFINED
#define DE_OSM3S__AREA_QUERY_STATEMENT_DEFINED

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Area_Query_Statement : public Statement
{
  public:
    Area_Query_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "area-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(map< string, Set >& maps);
    virtual ~Area_Query_Statement() {}
        
    void get_ranges
      (set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       set< Uint31_Index >& area_block_req);
    void collect_nodes
      (const set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       const set< Uint31_Index >& req,
       vector< uint32 >* ids,
       map< Uint32_Index, vector< Node_Skeleton > >& nodes,
       Stopwatch& stopwatch);

  private:
    string output;
    unsigned int area_id;
};

#endif
