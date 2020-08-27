#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_SKELETON_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_SKELETON_UPDATER_H


#include "moved_coords.h"
#include "new_basic_updater.h"

#include <vector>


struct Way_Implicit_Pre_Event
{
  Way_Skeleton::Id_Type id;
  uint64_t begin;
  std::vector< Quad_Coord > geometry;
  std::vector< Node::Id_Type > nds;
};


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
      std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      const std::vector< const Way_Skeleton* >& current,
      const std::vector< const Attic< Way_Skeleton >* >& attic,
      std::vector< Way_Skeleton >& current_result,
      std::vector< Attic< Way_Skeleton > >& attic_result);
  /* Assertions:
   * - an object w is contained in current_result if and only
   *   if an e in pre_event_refs exist with w.id == e.id or
   *   for an object v with v.id == w.id at least one of the coordinates or nds of v appears in moved_coords
   * - an object w is contained in attic_result if and only
   *   if an e in pre_event_refs exist with w.id == e.id or
   *   for an object v with v.id == w.id at least one of the coordinates or nds of v appears in moved_coords
   * - an object p is contained in implicit_pre_events if and only
   *   if there exists an object w in current_result or attic_result such that
   *   p is the result of applying all events from moved_coords up to p.begin
   *   and for p.begin there exists a relevant object m in moved_coords with m.timestamp == p.begin
   *   or p.begin == v.end for an object v in attic_result with v.id == p.id
   */
}


#endif
