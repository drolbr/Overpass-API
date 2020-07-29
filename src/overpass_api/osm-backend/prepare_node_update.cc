
#include "prepare_node_update.h"


void Prepare_Node_Update::create_update_for_nodes(
    const Node_Event_List& events, std::set< Node_Skeleton >& to_delete, std::set< Node_Skeleton >& to_insert)
{
  for (decltype(events.data.size()) i = 0; i < events.data.size(); ++i)
  {
    if (i+1 >= events.data.size() || !(events.data[i].id == events.data[i+1].id))
    {
      const auto& i_data = events.data[i];
      if (i_data.visible_before && (!i_data.visible_after || i_data.ll_lower_before != i_data.ll_lower_after))
        to_delete.insert(Node_Skeleton(i_data.id, i_data.ll_lower_before));
      if (i_data.visible_after && (!i_data.visible_before || i_data.ll_lower_before != i_data.ll_lower_after))
        to_insert.insert(Node_Skeleton(i_data.id, i_data.ll_lower_after));
    }
  }
}


void Prepare_Node_Update::create_update_for_nodes_attic(
    const Node_Event_List& events, std::set< Attic< Node_Skeleton > >& to_delete, std::set< Attic< Node_Skeleton > >& to_insert)
{
  for (decltype(events.data.size()) i = 0; i < events.data.size(); ++i)
  {
    if (i+1 < events.data.size() && events.data[i].id == events.data[i+1].id)
    {
      const auto& i_data = events.data[i];
      const auto& i_next = events.data[i+1];
      bool before_void = (!i_data.visible_before ||
          (i_next.visible_before && i_data.ll_lower_before == i_next.ll_lower_before));
      bool after_void = (!i_data.visible_after ||
          (i_next.visible_after && i_data.ll_lower_after == i_next.ll_lower_after));
      if (!before_void && (after_void || !i_data.visible_after ||
          (i_data.visible_before && i_data.ll_lower_before != i_data.ll_lower_after)))
        to_delete.insert(Attic< Node_Skeleton >(
            Node_Skeleton(i_data.id, i_data.ll_lower_before), i_next.timestamp));
      if (!after_void && (before_void || !i_data.visible_before ||
          (i_data.visible_after && i_data.ll_lower_before != i_data.ll_lower_after)))
        to_insert.insert(Attic< Node_Skeleton >(
            Node_Skeleton(i_data.id, i_data.ll_lower_after), i_next.timestamp));
    }
  }
}


void Prepare_Node_Update::create_update_for_nodes_undelete(
    const Node_Event_List& events, std::set< Attic< Node_Skeleton::Id_Type > >& to_delete, std::set< Attic< Node_Skeleton::Id_Type > >& to_insert)
{
  for (decltype(events.data.size()) i = 0; i < events.data.size(); ++i)
  {
    if (i+1 < events.data.size() && events.data[i].id == events.data[i+1].id)
    {
      const auto& i_data = events.data[i];
      const auto& i_next = events.data[i+1];
      if (!i_data.visible_before && i_next.visible_before && (i_data.visible_after || !i_next.visible_after))
        to_delete.insert(Attic< Node_Skeleton::Id_Type >(i_data.id, i_next.timestamp));
      if (!i_data.visible_after && i_next.visible_after && (i_data.visible_before || !i_next.visible_before))
        to_insert.insert(Attic< Node_Skeleton::Id_Type >(i_data.id, i_next.timestamp));
    }
  }
}
