#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_META_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_META_UPDATER_H


#include "data_from_osc.h"

#include <vector>


namespace Node_Meta_Updater
{
  // Changes the entry of end in the pre_event_list where applicable.
  /* Preconditions:
   * meta is sorted by id and timestamp
   * pre_events is sorted by id and timestamp
   */
  void adapt_pre_event_list(
      const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
      Pre_Event_List& pre_events);
  /* Assertions:
   * The of elements in pre_events are the same before and after the call except their value for end.
   * For every entry e in pre_events the value of e.end
   * is the minimum of e.end before the call and the begin dates of all entries m in meta
   * with m.id == e.id and e.begin < m.time
   */
}


namespace Node_Meta_Updater
{
  // Collects all elements from current_meta that become outdated due to pre_events
  void collect_current_meta_to_move(
      const Pre_Event_List& pre_events,
      const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current_meta,
      std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& to_move);
}


#endif
