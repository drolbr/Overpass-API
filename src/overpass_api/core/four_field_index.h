#ifndef DE__OSM3S___OVERPASS_API__CORE__FOUR_FIELD_INDEX_H
#define DE__OSM3S___OVERPASS_API__CORE__FOUR_FIELD_INDEX_H


#include "basic_types.h"

#include <string>
#include <vector>


struct Four_Field_Entry
{
  Four_Field_Entry() : sw(0), se(0), nw(0), ne(0) {}
  
  int32 sw;
  int32 se;
  int32 nw;
  int32 ne;
};


class Area_Oracle
{
public:
  typedef char point_status;
  
  virtual void build_area(bool sw_corner_inside, int32 value, bool* se_corner_inside, bool* nw_corner_inside) = 0;
  virtual point_status get_point_status(int32 value, double lat, double lon) = 0;
};


struct Four_Field_Index
{
  Four_Field_Index(Area_Oracle* area_oracle_);
  
  int32 add_point(double lat, double lon, int32 val = 1);
  Four_Field_Entry add_segment(double lhs_lat, double lhs_lon, double rhs_lat, double rhs_lon, int32 val = 1);
  
  void compute_inside_parts();
  Area_Oracle::point_status get_point_status(double lat, double lon);
  
  // Only for testing the structure
  std::string to_string() const;

private:
  Four_Field_Entry& make_available(uint32 base_lat, int32 base_lon, int significant_bits);
  void compute_inside_parts(uint32 lat, int32 lon, int base_significant_bits, unsigned int pos,
      bool sw, bool* se, bool* nw);
  
  std::vector< Four_Field_Entry > tree;
  uint32 base_lat;
  int32 base_lon;
  int base_significant_bits;
  Area_Oracle* area_oracle;
  
  uint32 min_lat;
  uint32 max_lat;
  int32 min_lon;
  int32 max_lon;
};


#endif
