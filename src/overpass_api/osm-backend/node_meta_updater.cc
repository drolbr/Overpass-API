#include "node_meta_updater.h"


void Node_Meta_Updater::adapt_pre_event_list(
    Uint31_Index working_idx, const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
    Pre_Event_List& pre_events)
{
  auto j_data = pre_events.data.begin();
  auto j_deleted = pre_events.timestamp_last_not_deleted.begin();
  for (const auto& i : meta)
  {
    while (j_data != pre_events.data.end() &&
        (j_data->entry->meta.ref < i.ref || (!(i.ref < j_data->entry->meta.ref) &&
        j_data->entry->meta.timestamp < i.timestamp)))
      ++j_data;
    while (j_deleted != pre_events.timestamp_last_not_deleted.end() &&
        (j_deleted->entry->meta.ref < i.ref || (!(i.ref < j_deleted->entry->meta.ref) &&
        j_deleted->entry->meta.timestamp < i.timestamp)))
      ++j_deleted;
    if (j_data != pre_events.data.begin())
    {
      --j_data;
      if (j_data->entry->meta.ref == i.ref && (j_data->timestamp_end == 0 || i.timestamp < j_data->timestamp_end))
        j_data->timestamp_end = i.timestamp;
      ++j_data;
    }
    if (j_deleted != pre_events.timestamp_last_not_deleted.end() &&
        j_data->entry == j_deleted->entry)
    {
      if (j_deleted->timestamp_end < i.timestamp)
      {
        j_deleted->timestamp_end = i.timestamp;
        j_data->entry->idx = working_idx;
      }
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
        (j->entry->meta.ref < i.ref || (!(i.ref < j->entry->meta.ref) && j->entry->meta.timestamp < i.timestamp)))
      ++j;
    if (j != pre_events.data.end() && j->entry->meta.ref == i.ref)
      to_move.insert(i);
  }
}
