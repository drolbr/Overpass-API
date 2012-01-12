#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__AROUND_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Around_Statement : public Statement
{
  public:
    Around_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "around"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Around_Statement();
    
    static Generic_Statement_Maker< Around_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    string get_source_name() const { return input; }

    set< pair< Uint32_Index, Uint32_Index > > calc_ranges
        (const map< Uint32_Index, vector< Node_Skeleton > >& input_nodes);
    
    bool is_inside(double lat, double lon) const;
    bool is_inside(double first_lat, double first_lon, double second_lat, double second_lon) const;
    bool is_inside(const Way_Skeleton& way,
		   const vector< pair< Uint32_Index, const Node_Skeleton* > >& way_members_by_id)
		   const;
    
  private:
    string input, output;
    double radius;
    map< Uint32_Index, vector< pair< double, double > > > lat_lons;
    vector< Query_Constraint* > constraints;
};

#endif
