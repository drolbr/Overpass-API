#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__DATA_FROM_OSC_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__DATA_FROM_OSC_H


#include "basic_updater.h"
#include "new_basic_updater.h"

#include <cstdint>
#include <map>
#include <vector>


struct Node_Pre_Event
{
  Node_Pre_Event(Data_By_Id< Node_Skeleton >::Entry& entry_) : entry(&entry_), timestamp_end(NOW) {}
  Node_Pre_Event(Data_By_Id< Node_Skeleton >::Entry& entry_, uint64_t timestamp_end_)
    : entry(&entry_), timestamp_end(timestamp_end_) {}

  Data_By_Id< Node_Skeleton >::Entry* entry;
  uint64_t timestamp_end;

  bool operator==(const Node_Pre_Event& rhs) const
  { return entry->elem == rhs.entry->elem && entry->meta == rhs.entry->meta && entry->idx == rhs.entry->idx
      && timestamp_end == rhs.timestamp_end; }

  bool operator<(const Node_Pre_Event& rhs) const
  { return entry->elem.id < rhs.entry->elem.id || (!(rhs.entry->elem.id < entry->elem.id)
      && (entry->meta.timestamp < rhs.entry->meta.timestamp || (!(rhs.entry->meta.timestamp < entry->meta.timestamp)
      && entry->meta.version < rhs.entry->meta.version))); }
};


struct Pre_Event_List
{
  std::vector< Node_Pre_Event > data;
  std::vector< Node_Pre_Event > timestamp_last_not_deleted;
};


class Data_From_Osc
{
public:
  Data_From_Osc() : default_timestamp(0) {}

  void set_node(const Node& node, const OSM_Element_Metadata* meta = 0);
  void set_node_deleted(Node::Id_Type id, const OSM_Element_Metadata* meta = 0);
  void set_way(const Way& node, const OSM_Element_Metadata* meta = 0);
  void set_way_deleted(Way::Id_Type id, const OSM_Element_Metadata* meta = 0);

  // not const: Pre_Event_List has pointers to entry and those entries are intended to be mutable
  Pre_Event_List node_pre_events();

  Pre_Event_Refs node_pre_event_refs(Pre_Event_List& events) const;
  std::map< Uint31_Index, Pre_Event_Refs > pre_event_refs_by_idx(Pre_Event_List& events) const;
  std::map< Uint31_Index, Coord_Dates > node_coord_dates() const;

//private:
  uint64_t default_timestamp;
  Data_By_Id< Node_Skeleton > nodes;
  Data_By_Id< Way_Skeleton > ways;
};


#endif
