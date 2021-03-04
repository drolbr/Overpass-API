#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_META_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_META_UPDATER_H


#include "data_from_osc.h"
#include "way_skeleton_updater.h"

#include <vector>


namespace Way_Meta_Updater
{
  /* NB: If no skel exists then also no meta exists
   * NB: independend for attic and current */
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > extract_relevant_meta(
      const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& meta);
  /* Assertions:
    * ...
    */


  void detect_deletions(
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
      const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& deletions);
  /* Assertions:
    * ...
    */


  void prune_first_skeletons(
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
      std::vector< Way_Implicit_Pre_Event >& implicit_pre_events);
  /* Assertions:
    * ...
    */


  std::vector< Proto_Way > assign_meta(
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
      std::vector< Way_Implicit_Pre_Event >&& implicit_events);
  /* Assertions:
    * ...
    */


  struct Way_Meta_Delta
  {
    Way_Meta_Delta(
        const std::vector< Way_Event >& events_for_this_idx,
        const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
        const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
        const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& deletions,
        const std::vector< Attic< Way_Skeleton::Id_Type > >& unchanged_before);
    /* Assertions:
     * ...
     */

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_add;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_add;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
  };
}


#endif
