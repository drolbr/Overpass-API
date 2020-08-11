#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_EVENT_LIST_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NODE_EVENT_LIST_H


#include "data_from_osc.h"

#include <vector>


/* Invariants:
 * - events are sorted by (id, time)
 * Note: specific for a given working_idx
 */
struct Node_Skeletons_Per_Idx
{
  std::vector< Node_Skeleton > current;
  std::vector< Attic< Node_Skeleton > > attic;
  std::vector< Attic< Node_Skeleton::Id_Type > > undeleted;
  Id_Dates first_appearance;
};


struct Node_Event
{
  Node_Skeleton::Id_Type id;
  uint64_t timestamp;
  bool visible_before;
  uint32 ll_lower_before;
  bool visible_after;
  uint32 ll_lower_after;
  bool multiple_after;
};


/* Invariants:
 * - events are sorted by (id, time)
 * NB: specific for a given working_idx
 */
struct Node_Event_List
{
  Node_Event_List(
      Uint31_Index working_idx, const Node_Skeletons_Per_Idx& skels,
      const Node_Pre_Event_Refs& pre_event_refs, const Pre_Event_List< Node_Skeleton >& pre_events);
  /* Assertions:
   * - for every last entry e
   *   it is e.visible if and only if there is a current node for this id in this idx
   * - for every attic node n with min_date(id) < n.end
   *   there exists a successive pair of entries e, f with f.time = n.end
   * - for every last entry e and current node n with e.id == n.id
   *   it is e.coord_before == n.coord
   * - for each pair of successive entries e, f for the same id
   *   it is (e.coord_before, e.visible_before) != (f.coord_before, f.visible_before)
   *   if and only if there is an attic node n in skels
   *   with n.id == e.id, e.visible_before, e.coord_before == n.coord and n.end = f.time
   *   or an entry u in undelete
   *   with u.id = e.id, !e.visible_before and u.time = f.time
   * - for entry e in pre_events and every time t with e.begin <= t < e.end
   *   there is an entry f with e.id == f.id, e.visible == f.visible_after, e.coord == f.coord_after,
   *   and f is the last entry or f.time <= t < g.time for g the successor of f
   * - for every entry f and every time t with f.time <= t and f is the last entry or t < g.time for the successor g
   *   and f.visible_before != f.visible_after or f.coord_before != f.coord_after
   *   there is an entry e in pre_events with e.id == f.id, e.visible == f.visible_after, and e.coord == f.coord_after
   * - first_elements_date and the first_elements_date from pre_events are identical
   * - no entry e has e.time < first_elements_date
   * - for any timestamp t where two or more entries e_0, .., e_n exist such that all have the same coord_after,
   *   that time is smaller or equal to t and the successive time is later than t
   *   there is also mult_after set for all these entries.
   * - no other entry has mult_after set to true.
   */

  std::vector< Node_Event > data;
};


#endif
