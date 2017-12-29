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


struct Four_Field_Index
{
  Four_Field_Index() : base_lat(0), base_lon(2000000000), base_significant_bits(0) {}
  
  void add_point(double lat, double lon);
  void add_segment(double lhs_lat, double lhs_lon, double rhs_lat, double rhs_lon);
  
  // Only for testing the structure
  std::string to_string() const;

private:
  Four_Field_Entry& make_available(uint32 base_lat, int32 base_lon, int significant_bits);
  
  std::vector< Four_Field_Entry > tree;
  uint32 base_lat;
  int32 base_lon;
  int base_significant_bits;
};


#endif
