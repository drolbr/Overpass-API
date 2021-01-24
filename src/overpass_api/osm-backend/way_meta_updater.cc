
#include "way_meta_updater.h"


namespace
{
  void adjust_i_impl(
      Way_Skeleton::Id_Type id, uint64_t meta_timestamp,
      const std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >& pre_event_refs,
      std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >::const_iterator& i_pref,
      const Pre_Event_List< Way_Skeleton >& pre_events,
      const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
      std::vector< Way_Implicit_Pre_Event >::const_iterator& i_impl)
  {
//     // NB: The assertion that no implicit event can be before the earliest meta makes this loop inaccesible.
//     // It is only a mitigation measure for a potential bug.
//     while (i_impl != implicit_pre_events.end() && i_impl->id == id && i_impl->begin < meta_timestamp)
//       ++i_impl;
// 
//     if (i_impl == implicit_pre_events.end() || meta_timestamp < i_impl->begin)
//     {
//       --i_impl;
//       if (i_pref != pre_event_refs.end() && i_pref->ref == id)
//       {
//         decltype(pre_events.data.size()) j = i_pref->offset;
//         while (j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == id
//             && pre_events.data[j].entry->meta.timestamp < meta_timestamp)
//         {
//           if (i_impl->begin <= pre_events.data[j].entry->meta.timestamp)
//           {
//             ++i_impl;
//             break;
//           }
//           ++j;
//         }
//       }
//     }
  }


  bool undeleted_until_first_impl(
      Way_Skeleton::Id_Type id, uint64_t first_impl_timestamp,
      const std::vector< Attic< Way_Skeleton::Id_Type > >& undeleted,
      std::vector< Attic< Way_Skeleton::Id_Type > >::const_iterator& i_undel)
  {
    while (i_undel != undeleted.end() && Way_Skeleton::Id_Type(*i_undel) == id
        && i_undel->timestamp < first_impl_timestamp)
      ++i_undel;
    return (
        i_undel != undeleted.end() && Way_Skeleton::Id_Type(*i_undel) == id
        && i_undel->timestamp == first_impl_timestamp);
  }
}


void Way_Meta_Updater::collect_meta_to_move(
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& current_meta,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& attic_meta,
    Uint31_Index working_idx,
    const std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >& pre_event_refs,
    const Pre_Event_List< Way_Skeleton >& pre_events,
    const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< Attic< Way_Skeleton::Id_Type > >& undeleted,
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& to_move,
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& current_to_delete,
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& attic_to_delete,
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >&
        current_to_add,
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >&
        attic_to_add)
{
//   auto i_cur = current_meta.begin();
//   auto i_attic = attic_meta.begin();
//   auto i_impl = implicit_pre_events.begin();
//   auto i_pref = pre_event_refs.begin();
//   auto i_undel = undeleted.begin();
// 
//   while (i_cur != current_meta.end() || i_attic != attic_meta.end())
//   {
//     Way_Skeleton::Id_Type id(0u);
//     if (i_attic == attic_meta.end() || (i_cur != current_meta.end() && i_cur->ref < i_attic->ref))
//       id = i_cur->ref;
//     else
//       id = i_attic->ref;
// 
//     // NB: The assertion that for every implicit event there is a meta element makes this loop inaccesible.
//     // It is only a mitigation measure for a potential bug.
//     while (i_impl != implicit_pre_events.end() && i_impl->id < id)
//       ++i_impl;
// 
//     while (i_pref != pre_event_refs.end() && i_pref->ref < id)
//       ++i_pref;
// 
//     if (i_impl == implicit_pre_events.end() || id < i_impl->id)
//     {
//       while (i_attic != attic_meta.end() && i_attic->ref == id)
//         ++i_attic;
//       if (i_cur != current_meta.end() && i_cur->ref == id)
//       {
//         if (i_pref != pre_event_refs.end() && i_pref->ref == id)
//         {
//           decltype(pre_events.data.size()) j = i_pref->offset;
//           while (j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == id)
//           {
//             if (pre_events.data[j].timestamp_end == NOW)
//               to_move.insert(*i_cur);
//             ++j;
//           }
//         }
// 
//         ++i_cur;
//       }
//     }
//     else
//     {
//       auto first_impl_timestamp = i_impl->begin;
// 
//       while (i_attic != attic_meta.end() && i_attic->ref == id)
//       {
//         uint64_t meta_valid_until = NOW;
//         auto i_next = i_attic+1;
//         if (i_next != attic_meta.end() && i_next->ref == id)
//           meta_valid_until = i_next->timestamp;
//         else if (i_cur != current_meta.end() && i_cur->ref == id)
//           meta_valid_until = i_cur->timestamp;
// 
//         bool working_idx_found = false;
//         std::vector< Uint31_Index > found_idxs;
// 
//         if (first_impl_timestamp <= i_attic->timestamp)
//           adjust_i_impl(id, i_attic->timestamp, pre_event_refs, i_pref, pre_events, implicit_pre_events, i_impl);
//         else if (first_impl_timestamp < meta_valid_until)
//           //NB: No other meta exists between i_attic->timestamp and first_impl_timestamp
//           //    because otherwise the implicit_pre_event for first_impl_timestamp did not belong to working_idx.
//           //Thus, no pre_event can start before first_impl_timestamp: otherwise,
//           //  it either had displaced the implicit_pre_event or it ends on a time without a meta or successor.
//           working_idx_found |= !undeleted_until_first_impl(
//               id, first_impl_timestamp, undeleted, i_undel);
//         else
//           working_idx_found = true;
// 
//         while (i_impl != implicit_pre_events.end() && i_impl->id == id && i_impl->begin < meta_valid_until)
//         {
//           Uint31_Index idx = calc_index(i_impl->geometry);
//           if (idx == working_idx)
//             working_idx_found = true;
//           else
//             found_idxs.push_back(idx);
// 
//           ++i_impl;
//         }
//         std::sort(found_idxs.begin(), found_idxs.end());
//         found_idxs.erase(std::unique(found_idxs.begin(), found_idxs.end()), found_idxs.end());
// 
//         if (!working_idx_found && !found_idxs.empty())
//           attic_to_delete.insert(*i_attic);
// 
//         for (auto j : found_idxs)
//           attic_to_add[j].insert(*i_attic);
// 
//         ++i_attic;
//       }
//       if (i_cur != current_meta.end() && i_cur->ref == id)
//       {
//         bool working_idx_found = false;
//         std::vector< Uint31_Index > found_idxs;
// 
//         if (first_impl_timestamp <= i_cur->timestamp)
//           adjust_i_impl(id, i_cur->timestamp, pre_event_refs, i_pref, pre_events, implicit_pre_events, i_impl);
//         else
//           working_idx_found |= !undeleted_until_first_impl(
//               id, first_impl_timestamp, undeleted, i_undel);
// 
//         Uint31_Index final_idx = working_idx;
//         while (i_impl != implicit_pre_events.end() && i_impl->id == id)
//         {
//           Uint31_Index idx = calc_index(i_impl->geometry);
//           final_idx = idx;
//           if (idx == working_idx)
//             working_idx_found = true;
//           else
//             found_idxs.push_back(idx);
//  
//           ++i_impl;
//         }
//         std::sort(found_idxs.begin(), found_idxs.end());
//         found_idxs.erase(std::unique(found_idxs.begin(), found_idxs.end()), found_idxs.end());
// 
//         bool pre_event_covers_now = false;
//         if (i_pref != pre_event_refs.end() && i_pref->ref == id)
//         {
//           decltype(pre_events.data.size()) j = i_pref->offset;
//           while (j < pre_events.data.size() && pre_events.data[j].entry->meta.ref == id)
//           {
//             if (pre_events.data[j].timestamp_end == NOW)
//               pre_event_covers_now = true;
//             ++j;
//           }
//         }
// 
//         if (!working_idx_found && !found_idxs.empty())
//           current_to_delete.insert(*i_cur);
//         else if (pre_event_covers_now || (working_idx_found && !(final_idx == working_idx)))
//           to_move.insert(*i_cur);
// 
//         for (auto j : found_idxs)
//         {
//           if (pre_event_covers_now || !(j == final_idx))
//             attic_to_add[j].insert(*i_cur);
//           else
//             current_to_add[j].insert(*i_cur);
//         }
// 
//         ++i_cur;
//       }
//     }
//   }
}
