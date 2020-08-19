#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__MOVED_COORDS_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__MOVED_COORDS_H


#include "node_event_list.h"


struct Move_Coord_Event
{
  Uint31_Index idx;
  uint32_t ll_lower;
  uint64_t timestamp;
  Node_Skeleton::Id_Type node_id;
  Uint31_Index idx_after;
  uint32_t ll_lower_after;
  bool visible_after;
  bool multiple_after;

  bool operator<(const Move_Coord_Event& rhs) const
  {
    return idx < rhs.idx || (!(rhs.idx < idx) &&
        (ll_lower < rhs.ll_lower || (!(rhs.ll_lower < ll_lower) &&
        (timestamp < rhs.timestamp || (!(rhs.timestamp < timestamp) &&
        node_id < rhs.node_id)))));
  }
};


class Moved_Coords
{
public:
  Moved_Coords() : hash_built(false) {}

  void record(Uint31_Index working_idx, const std::vector< Node_Event >& events);
  void build_hash();
  std::vector< const Move_Coord_Event* > get_id(Node_Skeleton::Id_Type node_id);
  std::vector< const Move_Coord_Event* > get_coord(Uint31_Index idx, uint32_t ll_lower);

private:
  std::vector< Move_Coord_Event > data;
  std::vector< std::vector< Move_Coord_Event* > > hash_by_id;
  std::vector< std::vector< Move_Coord_Event* > > hash_by_coord;
  bool hash_built;
};


#endif
