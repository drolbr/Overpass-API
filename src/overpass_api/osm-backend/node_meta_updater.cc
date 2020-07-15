#include "node_meta_updater.h"


void Node_Meta_Updater::adapt_pre_event_list(
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
    Pre_Event_List& pre_events)
{
  auto j = pre_events.data.begin();
  for (const auto& i : meta)
  {
    while (j != pre_events.data.end() &&
        (j->entry->elem.id < i.ref || (!(i.ref < j->entry->elem.id) && j->entry->meta.timestamp < i.timestamp)))
      ++j;
    if (j != pre_events.data.begin())
    {
      --j;
      if (j->entry->elem.id == i.ref && (j->timestamp_end == 0 || i.timestamp < j->timestamp_end))
        j->timestamp_end = i.timestamp;
      ++j;
    }
  }
}


void Node_Meta_Updater::collect_current_meta_to_move(
    const Pre_Event_List& pre_events,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current_meta,
    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& to_move)
{
  to_move.clear();

  auto j = pre_events.data.begin();
  for (const auto& i : current_meta)
  {
    while (j != pre_events.data.end() &&
        (j->entry->elem.id < i.ref || (!(i.ref < j->entry->elem.id) && j->entry->meta.timestamp < i.timestamp)))
      ++j;
    if (j != pre_events.data.end() && j->entry->elem.id == i.ref)
      to_move.insert(i);
  }
}
