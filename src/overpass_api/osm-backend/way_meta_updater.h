#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_META_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_META_UPDATER_H


#include "data_from_osc.h"
#include "way_skeleton_updater.h"

#include <vector>


namespace Way_Meta_Updater
{
  // Collects all elements from current_meta and attic_meta
  // that become outdated due to pre_events or implicit_pre_events
  // Please note that for implicit_pre_events the idxs are evaluated but not for pre_events.
  /* Preconditions:
   * current_meta is sorted by id
   * attic_meta, pre_events.data, implicit_pre_events are sorted by id and timestamp
   */
  void collect_meta_to_move(
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& current_meta,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& attic_meta,
      Uint31_Index working_idx,
      const std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >& pre_event_refs,
      const Pre_Event_List< Way_Skeleton >& pre_events,
      const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      const std::vector< Attic< Way_Skeleton::Id_Type > >& undeleted,
      std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& to_move,
      std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& current_to_delete,
      std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& attic_to_delete,
      std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >&
          current_to_add,
      std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >&
          attic_to_add);
  /* Assertions:
   * to_move, to_delete, current_to_add, and attic_to_add get only elements added, and only those from current_meta.
   * An element m is added to to_move or to_delete
   * if there is an element e in pre_events with e.id == m.id and m.time <= e.begin
   * or if there is an element e in implicit_pre_events with e.id == m.id and m.time <= e.time
   * and for the latest such element the idx is different from working_idx.
   * It goes to to_delete if there is at least one element e in implicit_pre_events with e.id == m.id
   * but no element in implicit_pre_events with e.id == m.id, m.time <= e.time, and idx(e) == working_idx.
   * An element m is added to current_to_add[idx(e)] or attic_to_add[idx(e)]
   * if there is an element e with idx(e) != working_idx, e.id == m.id, and m.time <= e.time.
   * It goes to current_to_add[idx(e)] if this is the latest such element, otherwise to attic_to_add[idx(e)].
   */
}


#endif
