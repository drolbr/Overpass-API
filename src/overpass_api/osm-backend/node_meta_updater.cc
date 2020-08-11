#include "node_meta_updater.h"


void Node_Meta_Updater::adapt_pre_event_list(
    Uint31_Index working_idx, const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
    const Node_Pre_Event_Refs& pre_event_refs, Pre_Event_List< Node_Skeleton >& pre_events)
{
  auto i_meta = meta.begin();
  auto i_deleted = pre_events.timestamp_last_not_deleted.begin();
  for (const auto& i_pre : pre_event_refs)
  {
    while (i_meta != meta.end() && i_meta->ref < i_pre.ref)
      ++i_meta;
    while (i_deleted != pre_events.timestamp_last_not_deleted.end() && i_deleted->entry->meta.ref < i_pre.ref)
      ++i_deleted;
    for (auto j = i_pre.offset;
        j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == i_pre.ref; ++j)
    {
      while (i_meta != meta.end() && i_meta->ref == i_pre.ref
          && i_meta->timestamp <= pre_events.data[j].entry->meta.timestamp)
        ++i_meta;
      if (i_meta != meta.end() && i_meta->ref == i_pre.ref
          && (pre_events.data[j].timestamp_end == 0
              || i_meta->timestamp < pre_events.data[j].timestamp_end))
        pre_events.data[j].timestamp_end = i_meta->timestamp;

      while (i_deleted != pre_events.timestamp_last_not_deleted.end() && i_deleted->entry->meta.ref == i_pre.ref
          && i_deleted->entry->meta.timestamp < pre_events.data[j].entry->meta.timestamp)
        ++i_deleted;
      if (i_deleted != pre_events.timestamp_last_not_deleted.end() && pre_events.data[j].entry == i_deleted->entry)
      {
        if (i_meta != meta.begin())
        {
          --i_meta;
          if (i_deleted->timestamp_end < i_meta->timestamp)
          {
            i_deleted->timestamp_end = i_meta->timestamp;
            pre_events.data[j].entry->idx = working_idx;
          }
          ++i_meta;
        }
      }
    }
  }
}


void Node_Meta_Updater::collect_current_meta_to_move(
    const Node_Pre_Event_Refs& pre_event_refs, const Pre_Event_List< Node_Skeleton >& pre_events,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& meta,
    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& to_move)
{
  to_move.clear();

  auto i_meta = meta.begin();
  for (const auto& i_pre : pre_event_refs)
  {
    while (i_meta != meta.end() && i_meta->ref < i_pre.ref)
      ++i_meta;
    for (auto j = i_pre.offset;
        j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == i_pre.ref; ++j)
    {
      while (i_meta != meta.end() && i_meta->ref == i_pre.ref
          && i_meta->timestamp < pre_events.data[j].entry->meta.timestamp)
      {
        to_move.insert(*i_meta);
        ++i_meta;
      }
    }
  }
}


void Node_Meta_Updater::create_update_for_nodes_meta(
      const Pre_Event_List< Node_Skeleton >& pre_events,
      const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& to_move,
      std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& current_to_add,
      std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& attic_to_add)
{
  attic_to_add = to_move;

  for (const auto& i : pre_events.data)
  {
    if (i.timestamp_end == NOW)
      current_to_add[i.entry->idx].insert(i.entry->meta);
    else
      attic_to_add[i.entry->idx].insert(i.entry->meta);
  }
}
