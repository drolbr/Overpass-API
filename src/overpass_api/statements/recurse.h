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
    Recurse_Statement(int line_number_, const map< string, string >& input_attributes);
    virtual string get_name() const { return "recurse"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Recurse_Statement() {}
    
    static Generic_Statement_Maker< Recurse_Statement > statement_maker;
    
  private:
    string input, output;
    unsigned int type;
};

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_begin,
     map< Uint31_Index, vector< Way_Skeleton > >::const_iterator ways_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result);

void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result);
     
void collect_nodes
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint32_Index, vector< Node_Skeleton > >& result,
     const set< pair< Uint32_Index, Uint32_Index > >& node_ranges);
     
void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result);

void collect_ways
    (const Statement& stmt, Resource_Manager& rman,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_begin,
     map< Uint31_Index, vector< Relation_Skeleton > >::const_iterator rels_end,
     map< Uint31_Index, vector< Way_Skeleton > >& result,
     const set< pair< Uint31_Index, Uint31_Index > >& way_ranges);

     
struct Order_By_Node_Id
{
  bool operator() (const pair< Uint32_Index, const Node_Skeleton* >& a,
		   const pair< Uint32_Index, const Node_Skeleton* >& b)
  {
    return (a.second->id < b.second->id);
  }
};

struct Order_By_Way_Id
{
  bool operator() (const pair< Uint31_Index, const Way_Skeleton* >& a,
		   const pair< Uint31_Index, const Way_Skeleton* >& b)
  {
    return (a.second->id < b.second->id);
  }
};


#endif
