#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__UPDATE_EVENTS_PREPARER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__UPDATE_EVENTS_PREPARER_H


#include "new_basic_updater.h"
#include "way_skeleton_updater.h"

#include <vector>


namespace Update_Events_Preparer
{
  // Collects the relevant first appearance
  /* Preconditions:
   * All four arguments are sorted by id and timestamp.
   */
  Node_Id_Dates extract_first_appearance(
      const Node_Pre_Event_Refs& id_dates,
      const Node_Id_Dates& coord_sharing_ids,
      const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current,
      const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& attic);
  /* Assertions:
   * An object r is in the result if and only if
   * there is an entry e in id_dates or coord_sharing_ids such that e.id == r.id and
   * from all elements E={f|f in current or attic, f.id == r.id} it is r.date = min_{f\in E}(f.date)
   */
}


namespace Update_Events_Preparer
{
  // Collects the relevant first appearance
  /* Preconditions:
   * All four arguments are sorted by id and timestamp.
   */
  Way_Id_Dates extract_first_appearance(
      const Way_Pre_Event_Refs& id_dates,
      const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& current,
      const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& attic);
  /* Assertions:
   * An object r is in the result if and only if
   * there is an entry e in id_dates or implicit_pre_events such that e.id == r.id and
   * from all elements E={f|f in current or attic, f.id == r.id} it is r.date = min_{f\in E}(f.date)
   */
}


namespace Update_Events_Preparer
{
  // Collects the relevant undelete entries
  /* Preconditions:
   * All four arguments are sorted by id and timestamp.
   */
  std::vector< Attic< Node_Skeleton::Id_Type > > extract_relevant_undeleted(
      const Node_Pre_Event_Refs& id_dates, const Node_Id_Dates& coord_sharing_ids,
      const std::vector< Attic< Node_Skeleton::Id_Type > >& undeletes);
  /* Assertions:
   * An object r is in the result if and only if
   * there is an entry e in id_dates or coord_sharing_id_dates such that e.id == r.id and e.min_date < r.date
   */
}


#endif
