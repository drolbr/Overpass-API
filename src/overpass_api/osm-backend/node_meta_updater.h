#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_META_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_META_UPDATER_H


#include "data_from_osc.h"

#include <vector>


namespace Node_Meta_Updater
{
  // Changes the entry of end in the pre_event_list where applicable.
  /* Preconditions:
   * meta is sorted by id and timestamp
   * pre_events is sorted by id and timestamp in both data and timestamp_last_not_deleted
   */
  void adapt_pre_event_list(
      Uint31_Index working_idx, const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
      const Pre_Event_Refs& pre_event_refs, Pre_Event_List& pre_events);
  /* Assertions:
   * The elements in pre_events are the same before and after the call except
   * - their value for end
   * - their idx if the node is deleted
   * - updated states in timestamp_last_not_deleted
   * For every entry e in pre_events the value of e.end
   * is the minimum of e.end before the call and the begin dates of all entries m in meta
   * with m.id == e.id and e.begin < m.time
   */
}


namespace Node_Meta_Updater
{
  // Collects all elements from current_meta that become outdated due to pre_events
  /* Preconditions:
   * current_meta is sorted by id
   * pre_events.data is sorted by id and timestamp
   */
  void collect_current_meta_to_move(
      const Pre_Event_Refs& pre_event_refs, const Pre_Event_List& pre_events,
      const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current_meta,
      std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& to_move);
  /* Assertions:
   * to_move contains only elements from current_meta
   * An element m is contained in to_move
   * if there is an element e in pre_events with e.id == m.id and m.time < e.begin
   */
}


namespace Node_Meta_Updater
{
  // Creates from pre_events and to_move all new current and attic meat elements.
  /* Preconditions:
   * pre_events.data is sorted by id and timestamp
   * current_to_add and attic_to_add are empty
   */
  void create_update_for_nodes_meta(
      const Pre_Event_List& pre_events,
      const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& to_move,
      std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& current_to_add,
      std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& attic_to_add);
}


#endif
