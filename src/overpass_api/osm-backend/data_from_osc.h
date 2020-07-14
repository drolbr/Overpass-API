#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__DATA_FROM_OSC_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__DATA_FROM_OSC_H


#include "basic_updater.h"
#include "new_basic_updater.h"

#include <cstdint>
#include <map>
#include <vector>


struct Node_Pre_Event
{
  Node_Pre_Event(const Data_By_Id< Node_Skeleton >::Entry& entry_) : entry(&entry_), timestamp_end(NOW) {}
  Node_Pre_Event(const Data_By_Id< Node_Skeleton >::Entry& entry_, uint64_t timestamp_end_)
    : entry(&entry_), timestamp_end(timestamp_end_) {}

  const Data_By_Id< Node_Skeleton >::Entry* entry;
  uint64_t timestamp_end;

  bool operator==(const Node_Pre_Event& rhs) const
  { return entry->elem == rhs.entry->elem && entry->meta == rhs.entry->meta
      && timestamp_end == rhs.timestamp_end; }

  bool operator<(const Node_Pre_Event& rhs) const
  { return entry->elem.id < rhs.entry->elem.id || (!(rhs.entry->elem.id < entry->elem.id)
      && (entry->meta.timestamp < rhs.entry->meta.timestamp || (!(rhs.entry->meta.timestamp < entry->meta.timestamp)
      && entry->meta.version < rhs.entry->meta.version))); }
};


struct Pre_Event_List
{
  std::vector< Node_Pre_Event > data;
};


class Data_From_Osc
{
public:
  Data_From_Osc() : default_timestamp(0) {}

  void set_node(const Node& node, const OSM_Element_Metadata* meta = 0);
  void set_node_deleted(Node::Id_Type id, const OSM_Element_Metadata* meta = 0);
  void set_way(const Way& node, const OSM_Element_Metadata* meta = 0);
  void set_way_deleted(Way::Id_Type id, const OSM_Element_Metadata* meta = 0);

  std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > node_id_dates() const;
  Pre_Event_List node_pre_events() const;
  std::map< Uint31_Index, Coord_Dates_Per_Idx > node_coord_dates() const;

private:
  uint64_t default_timestamp;
  Data_By_Id< Node_Skeleton > nodes;
  Data_By_Id< Way_Skeleton > ways;
};


#endif
