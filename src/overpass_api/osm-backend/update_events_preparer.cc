#include "../core/settings.h"
#include "update_events_preparer.h"


Id_Dates Update_Events_Preparer::extract_first_appearance(
    const Pre_Event_Refs& pre_event_refs,
    const Id_Dates& coord_sharing_ids,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& current,
    const std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >& attic)
{
  Id_Dates result;
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
      [](const std::pair< Node_Skeleton::Id_Type, uint64_t >& arg)
      { return arg.second == NOW; }
      ), result.end());

  return result;
}


std::vector< Attic< Node_Skeleton::Id_Type > > Update_Events_Preparer::extract_relevant_undeleted(
    const Pre_Event_Refs& pre_event_refs, const Id_Dates& coord_sharing_ids,
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
