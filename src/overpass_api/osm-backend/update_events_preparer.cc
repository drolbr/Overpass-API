#include "../core/settings.h"
#include "update_events_preparer.h"


namespace
{
  template< typename Id_Type, typename Id_Dates >
  Id_Dates eval_meta(
      Id_Dates&& result,
      const std::vector< OSM_Element_Metadata_Skeleton< Id_Type > >& current,
      const std::vector< OSM_Element_Metadata_Skeleton< Id_Type > >& attic)
  {
    auto i_cur = current.begin();
    for (auto& i : result)
    {
      while (i_cur != current.end() && i_cur->ref < i.first)
        ++i_cur;
      if (i_cur != current.end() && i_cur->ref == i.first)
        i.second = i_cur->timestamp;
    }

    auto i_attic = attic.begin();
    for (auto& i : result)
    {
      while (i_attic != attic.end() && i_attic->ref < i.first)
        ++i_attic;
      if (i_attic != attic.end() && i_attic->ref == i.first && i_attic->timestamp < i.second)
        i.second = i_attic->timestamp;
    }

    result.erase(std::remove_if(result.begin(), result.end(),
        [](const std::pair< Id_Type, uint64_t >& arg)
        { return arg.second == NOW; }
        ), result.end());

    return result;
  }
}


Node_Id_Dates Update_Events_Preparer::extract_first_appearance(
    const Node_Pre_Event_Refs& pre_event_refs,
    const Node_Id_Dates& coord_sharing_ids,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& attic)
{
  Node_Id_Dates result;
  result.reserve(pre_event_refs.size() + coord_sharing_ids.size());

  auto i_id = pre_event_refs.begin();
  auto i_csi = coord_sharing_ids.begin();
  while (i_id != pre_event_refs.end())
  {
    while (i_csi != coord_sharing_ids.end() && i_csi->first < i_id->ref)
    {
      result.push_back(std::make_pair(i_csi->first, NOW));
      ++i_csi;
    }
    result.push_back(std::make_pair(i_id->ref, NOW));
    ++i_id;
  }
  while (i_csi != coord_sharing_ids.end())
  {
    result.push_back(std::make_pair(i_csi->first, NOW));
    ++i_csi;
  }

  return eval_meta< Node_Skeleton::Id_Type, Node_Id_Dates >(std::move(result), current, attic);
}


Way_Id_Dates Update_Events_Preparer::extract_first_appearance(
    const Way_Pre_Event_Refs& pre_event_refs,
    const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& current,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& attic)
{
//   Way_Id_Dates result;
//   result.reserve(pre_event_refs.size() + implicit_pre_events.size());
// 
//   auto i_id = pre_event_refs.begin();
//   auto i_ipe = implicit_pre_events.begin();
//   while (i_id != pre_event_refs.end())
//   {
//     while (i_ipe != implicit_pre_events.end() && i_ipe->id < i_id->ref)
//     {
//       if (result.empty() || !(result.back().first == i_ipe->id))
//         result.push_back(std::make_pair(i_ipe->id, NOW));
//       ++i_ipe;
//     }
//     result.push_back(std::make_pair(i_id->ref, NOW));
//     ++i_id;
//   }
//   while (i_ipe != implicit_pre_events.end())
//   {
//     if (result.empty() || !(result.back().first == i_ipe->id))
//       result.push_back(std::make_pair(i_ipe->id, NOW));
//     ++i_ipe;
//   }
// 
//   return eval_meta< Way_Skeleton::Id_Type, Way_Id_Dates >(std::move(result), current, attic);
}


std::vector< Attic< Node_Skeleton::Id_Type > > Update_Events_Preparer::extract_relevant_undeleted(
    const Node_Pre_Event_Refs& pre_event_refs, const Node_Id_Dates& coord_sharing_ids,
    const std::vector< Attic< Node_Skeleton::Id_Type > >& undeletes)
{
  std::vector< Attic< Node_Skeleton::Id_Type > > result;

  auto i_id = pre_event_refs.begin();
  auto i_csi = coord_sharing_ids.begin();
  for (const auto& i : undeletes)
  {
    while (i_id != pre_event_refs.end() && i_id->ref < i)
      ++i_id;
    while (i_csi != coord_sharing_ids.end() && i_csi->first < i)
      ++i_csi;
    if ((i_id != pre_event_refs.end() && i_id->ref == i)
        || (i_csi != coord_sharing_ids.end() && i_csi->first == i))
      result.push_back(i);
  }

  return result;
}


std::vector< Attic< Way_Skeleton::Id_Type > > Update_Events_Preparer::extract_relevant_undeleted(
    const Way_Pre_Event_Refs& pre_event_refs, const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< Attic< Way_Skeleton::Id_Type > >& undeletes)
{
//   std::vector< Attic< Way_Skeleton::Id_Type > > result;
// 
//   auto i_id = pre_event_refs.begin();
//   auto i_ipe = implicit_pre_events.begin();
//   for (const auto& i : undeletes)
//   {
//     while (i_id != pre_event_refs.end() && i_id->ref < i)
//       ++i_id;
//     while (i_ipe != implicit_pre_events.end() && i_ipe->id < i)
//       ++i_ipe;
//     if ((i_id != pre_event_refs.end() && i_id->ref == i)
//         || (i_ipe != implicit_pre_events.end() && i_ipe->id == i))
//       result.push_back(i);
//   }
// 
//   return result;
}


void Update_Events_Preparer::prune_nonexistant_events(
    const std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >& pre_event_refs,
    const Pre_Event_List< Way_Skeleton >& pre_events,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events)
{
  if (implicit_pre_events.empty())
    return;

  std::vector< Way_Implicit_Pre_Event > result;
  result.reserve(implicit_pre_events.size());

  auto it = implicit_pre_events.begin();
  auto i_pre = pre_event_refs.begin();
  while (it != implicit_pre_events.end())
  {
    while (i_pre != pre_event_refs.end() && i_pre->ref < it->base.id)
      ++i_pre;

    if (i_pre != pre_event_refs.end() && i_pre->ref == it->base.id)
    {
      auto j = i_pre->offset;
      while (j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == i_pre->ref
          && pre_events.data[j].timestamp_end <= it->not_before)
        ++j;

      if (!(j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == i_pre->ref)
          || it->before <= pre_events.data[j].entry->meta.timestamp)
      {
        result.push_back(*it);
        ++it;
      }
      else
      {
        if (it->not_before < pre_events.data[j].entry->meta.timestamp)
          result.push_back({ it->base, it->not_before, pre_events.data[j].entry->meta.timestamp, it->pos_events });

        if (pre_events.data[j].timestamp_end < it->before)
          it->not_before = pre_events.data[j].timestamp_end;
        else
          ++it;
      }
    }
    else
    {
      result.push_back(*it);
      ++it;
    }
  }

  result.swap(implicit_pre_events);
}
