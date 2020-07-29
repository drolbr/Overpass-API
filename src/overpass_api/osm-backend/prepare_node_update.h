#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__PREPARE_NODE_UPDATE_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__PREPARE_NODE_UPDATE_H


#include "node_event_list.h"

#include <set>


namespace Prepare_Node_Update
{
  // Creates the set of deletions and additions for a single index for the current nodes file
  /* No preconditions
  */
  void create_update_for_nodes(
      const Node_Event_List& events, std::set< Node_Skeleton >& to_delete, std::set< Node_Skeleton >& to_insert);
  /* Assertions:
  * - for each latest event n per id i with (n.visible_before, n.coord_before) != (n.visible_after, n.coord_after) there is an entry in to_delete if n.visible_before and an entry in to_insert if n.visible_after
  * - no other entries exist in to_delete or to_insert
  */


  // Creates the set of deletions and additions for a single index for the current nodes file
  /* No preconditions
  */
  void create_update_for_nodes_attic(
      const Node_Event_List& events,
      std::set< Attic< Node_Skeleton > >& to_delete, std::set< Attic< Node_Skeleton > >& to_insert);
  /* Assertions:
  * - for each but the latest event n per id i with m the following event:
  * -- if n.visible_before and (n.visible_before, n.coord_before) != (m.visible_before, m.coord_before) then there is an entry in to_delete
  * -- if n.visible_after and (n.visible_after, n.coord_after) != (m.visible_after, m.coord_after) then there is an entry in to_insert
  * - no other entries exist in to_delete or to_insert
  */


  // Creates the set of deletions and additions for a single index for the current nodes file
  /* No preconditions
  */
  void create_update_for_nodes_undelete(
      const Node_Event_List& events,
      std::set< Attic< Node_Skeleton::Id_Type > >& to_delete,
      std::set< Attic< Node_Skeleton::Id_Type > >& to_insert);
  /* Assertions:
  * - for each but the latest event n per id i with m the following event, !n.visible_before, n.visible_after, m.visible_before, m.visible_after and there exists an earlier event l with l.visible_before there is an entry in to_delete
  * - for each but the latest event n per id i with m the following event, !n.visible_after, m.visible_after and there exists an earlier event l with l.visible_after and (n.visible_before || !m.visible_before) there is an entry in to_insert
  * - no other entries exist in to_delete or to_insert
  */
}


#endif
