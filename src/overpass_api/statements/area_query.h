#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AREA_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AREA_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Area_Query_Statement : public Statement
{
  public:
    Area_Query_Statement(int line_number_) : Statement(line_number_)
        { is_used_ = true; }
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "area-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Area_Query_Statement() {}
        
    void get_ranges
      (set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       set< Uint31_Index >& area_block_req,
       Resource_Manager& rman);
    void collect_nodes
      (const set< pair< Uint32_Index, Uint32_Index > >& nodes_req,
       const set< Uint31_Index >& req,
       vector< uint32 >* ids,
       map< Uint32_Index, vector< Node_Skeleton > >& nodes,
       Stopwatch& stopwatch,
       Resource_Manager& rman);

       static bool is_used() { return is_used_; }
  
  private:
    string output;
    unsigned int area_id;
    
    static bool is_used_;
};

#endif
