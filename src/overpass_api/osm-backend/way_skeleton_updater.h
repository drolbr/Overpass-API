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
  //TODO: Implizite Änderungen finden: teuer ist das Screening, daher sofort Ereignis konstruieren. Da Effekte auf Meta, ist Vertagen nicht möglich
  //Idx speichern?
  //TODO: Ways aufsammeln, die von pre_event_refs betroffen sind. Trotzdem werden alle Zwischenversionen gebraucht, da es übernommene Versionen geben könnte. Direktintegration?
  std::vector< Way_Skeleton > extract_relevant_current(
      const Way_Pre_Event_Refs& pre_event_refs,
      const Moved_Coords& moved_coords,
      std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      const std::vector< Way_Skeleton >& current);
}


namespace Way_Skeleton_Updater
{
  std::vector< Attic< Way_Skeleton > > extract_relevant_attic(
      const Way_Pre_Event_Refs& pre_event_refs,
      const Moved_Coords& moved_coords,
      std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      const std::vector< Attic< Way_Skeleton > >& attic);
}


#endif
