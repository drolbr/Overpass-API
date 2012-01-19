#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__BBOX_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__BBOX_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Bbox_Query_Statement : public Statement
{
  public:
    Bbox_Query_Statement(int line_number_, const map< string, string >& attributes);
    virtual string get_name() const { return "bbox-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Bbox_Query_Statement();
    
    static Generic_Statement_Maker< Bbox_Query_Statement > statement_maker;
    
    virtual Query_Constraint* get_query_constraint();
    
    vector< pair< uint32, uint32 > > calc_ranges()
    {
      return ::calc_ranges(south, north, west, east);
    }
    
    double get_south() const { return south; }
    double get_north() const { return north; }
    double get_west() const { return west; }
    double get_east() const { return east; }

  private:
    string output;
    unsigned int type;
    double south, north, west, east;
    vector< Query_Constraint* > constraints;
};

#endif
