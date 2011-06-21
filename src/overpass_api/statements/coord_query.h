#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__COORD_QUERY_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__COORD_QUERY_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"

using namespace std;

class Coord_Query_Statement : public Statement
{
  public:
    Coord_Query_Statement(int line_number_) : Statement(line_number_) {}
    virtual void set_attributes(const char **attr);
    virtual string get_name() const { return "coord-query"; }
    virtual string get_result_name() const { return output; }
    virtual void forecast();
    virtual void execute(Resource_Manager& rman);
    virtual ~Coord_Query_Statement() {}
        
    static int check_segment
        (uint32 a_lat, int32 a_lon, uint32 b_lat, int32 b_lon,
         uint32 coord_lat, int32 coord_lon);
    static uint32 shifted_lat(uint32 ll_index, uint64 coord);
    static int32 lon_(uint32 ll_index, uint64 coord);
    static int check_area_block
        (uint32 ll_index, const Area_Block& area_block,
	 uint32 coord_lat, int32 coord_lon);
    
    const static int TOGGLE = 1;
    const static int HIT = 2;
    
  private:
    string output;
    double lat, lon;
};

#endif
