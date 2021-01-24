#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_SKELETON_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_SKELETON_UPDATER_H


#include "moved_coords.h"
#include "new_basic_updater.h"

#include <vector>


struct Way_Implicit_Pre_Event
{
  Way_Skeleton base;
  uint64_t not_before;
  uint64_t before;
  std::vector< std::pair< decltype(Way_Skeleton::geometry.size()), const Move_Coord_Event* > > pos_events;
};


struct Proto_Way
{
  Way_Skeleton base;
  OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
  uint64_t not_before;
  uint64_t before;
  std::vector< std::pair< decltype(Way_Skeleton::geometry.size()), const Move_Coord_Event* > > pos_events;
};


struct Way_Event
{
  Way_Skeleton skel;
  OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
  uint64_t not_before;
  uint64_t before;
};


struct Way_Deletion
{
  OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta;
  uint64_t not_before;
  uint64_t before;
};


using Way_Event_Container = std::vector< Way_Event >;


namespace Way_Skeleton_Updater
{
  // Picks up the current and attic nodes based on pre_event_refs by id and based on moved_coords by coord changes
  /* Precondition:
   * pre_event_refs contains each id at most once.
   * current and attic must be sorted by id then timestamp ascending.
   * current_result, attic_result, and implicit_pre_events must be empty.
   */
  void extract_relevant_current_and_attic(
      const Way_Pre_Event_Refs& pre_event_refs,
      const Moved_Coords& moved_coords,
      const std::vector< const Way_Skeleton* >& current,
      const std::vector< const Attic< Way_Skeleton >* >& attic,
      std::vector< Way_Skeleton >& current_result,
      std::vector< Attic< Way_Skeleton > >& attic_result,
      std::vector< Way_Implicit_Pre_Event >& implicit_events);
  /* Assertions:
   * ...
   */


  // Resolves the base way skeletons against their events and distributes them to their proper indexes.
  // Given that most objects are expected to go to the current index this gets a dedicated argument events_for_this_idx.
  /* Precondition:
   * proto_events contains for no id any objects of which the timestamps overlap.
   * For every w in proto_events it is w.meta.timestamp <= w.not_before <= w.not_after.
   * For every w in proto_events it is w.pos_events ordered by timestamp.
   */
  void resolve_coord_events(
      Uint31_Index cur_idx,
      const std::vector< Proto_Way >& proto_events,
      Way_Event_Container& events_for_this_idx,
      std::map< Uint31_Index, Way_Event_Container >& arrived_objects);
  /* Assertions:
   * ...
   */


  // Resolves the way skeletons from pre_events against moved_coords and distributes them to their proper indexes.
  void resolve_coord_events(
      const Pre_Event_List< Way_Skeleton >& pre_events,
      const Moved_Coords& moved_coords,
      std::map< Uint31_Index, Way_Event_Container >& changes_per_idx,
      std::vector< Way_Deletion >& deletions);
  /* Assertions:
   * ...
   */
}


#endif
