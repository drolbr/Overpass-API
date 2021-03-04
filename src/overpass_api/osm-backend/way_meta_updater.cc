
#include "way_meta_updater.h"


std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > Way_Meta_Updater::extract_relevant_meta(
    const std::vector< Attic< Way_Skeleton::Id_Type > >& unchanged_before,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& meta)
{
  std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > result;

  auto it_meta = meta.begin();
  for (const auto& i : unchanged_before)
  {
    while (it_meta != meta.end() && it_meta->ref < Way_Skeleton::Id_Type(i))
      ++it_meta;
    while (it_meta != meta.end() && it_meta->ref == Way_Skeleton::Id_Type(i) && it_meta->timestamp < i.timestamp)
      ++it_meta;

    if (it_meta != meta.begin() &&
        (it_meta == meta.end() || Way_Skeleton::Id_Type(i) < it_meta->ref || i.timestamp < it_meta->timestamp))
    {
      --it_meta;
      if (it_meta->ref < Way_Skeleton::Id_Type(i))
        ++it_meta;
    }

    while (it_meta != meta.end() && it_meta->ref == Way_Skeleton::Id_Type(i))
    {
      result.push_back(*it_meta);
      ++it_meta;
    }
  }

  return result;
}


void Way_Meta_Updater::detect_deletions(
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
    const std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& deletions)
{
  auto it_pre = implicit_pre_events.begin();
  auto it_current = existing_current_meta.begin();

  for (auto it_attic = existing_attic_meta.begin(); it_attic != existing_attic_meta.end(); ++it_attic)
  {
    while (it_pre != implicit_pre_events.end() && it_pre->base.id < it_attic->ref)
      ++it_pre;
    while (it_pre != implicit_pre_events.end() && it_pre->base.id == it_attic->ref &&
        it_pre->before < it_attic->timestamp)
      ++it_pre;

    while (it_current != existing_current_meta.end() && it_current->ref < it_attic->ref)
      ++it_current;

    if (it_pre != implicit_pre_events.end() && it_pre->base.id == it_attic->ref &&
        it_pre->before == it_attic->timestamp)
    {
      auto it_attic_next = it_attic+1;
      if (it_attic_next == existing_attic_meta.end() || !(it_attic->ref == it_attic_next->ref))
      {
        auto it_pre_next = it_pre+1;
        if (it_pre_next == implicit_pre_events.end() || !(it_pre_next->base.id == it_attic->ref))
          deletions.push_back(*it_attic);
        else if (it_current != existing_current_meta.end() && it_current->ref == it_attic->ref &&
            it_current->timestamp <= it_pre_next->not_before)
          deletions.push_back(*it_attic);
      }
      else if (it_attic->timestamp < it_attic_next->timestamp)
      {
        auto it_pre_next = it_pre+1;
        if (it_pre_next == implicit_pre_events.end() || !(it_pre_next->base.id == it_attic->ref) ||
            it_attic_next->timestamp <= it_pre_next->not_before)
          deletions.push_back(*it_attic);
      }
    }
  }
}


void Way_Meta_Updater::prune_first_skeletons(
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
    std::vector< Way_Implicit_Pre_Event >& events)
{
  auto it_attic = existing_attic_meta.begin();
  auto it_current = existing_current_meta.begin();

  for (auto& i : events)
  {
    if (i.not_before > 0)
      continue;

    while (it_attic != existing_attic_meta.end() && it_attic->ref < i.base.id)
      ++it_attic;
    if (it_attic != existing_attic_meta.end() && it_attic->ref == i.base.id)
      i.not_before = it_attic->timestamp;
    else
    {
      while (it_current != existing_current_meta.end() && it_current->ref < i.base.id)
        ++it_current;
      if (it_current != existing_current_meta.end() && it_current->ref == i.base.id)
        i.not_before = it_current->timestamp;
    }
  }
}


std::vector< Proto_Way > Way_Meta_Updater::assign_meta(
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
    std::vector< Way_Implicit_Pre_Event >&& implicit_events)
{
  std::vector< Proto_Way > result;

  auto it = implicit_events.begin();
  auto it_attic = existing_attic_meta.begin();
  auto it_current = existing_current_meta.begin();
  const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >* meta_ptr = 0;
  uint64_t meta_until = 0;
  while (it != implicit_events.end())
  {
    if (meta_ptr && meta_ptr->ref < it->base.id)
      meta_ptr = 0;

    // Find youngest attic that is older or equal to it->not_before
    while (it_attic != existing_attic_meta.end() && it_attic->ref < it->base.id)
      ++it_attic;
    while (it_attic != existing_attic_meta.end() && it_attic->ref == it->base.id && it_attic->timestamp <= it->not_before)
    {
      meta_ptr = &*it_attic;
      ++it_attic;
    }

    if (it_attic != existing_attic_meta.end() && it_attic->ref == it->base.id)
      meta_until = it_attic->timestamp;
    else
      // Sort out between youngest attic and current which is applicable
    {
      meta_until = NOW;
      while (it_current != existing_current_meta.end() && it_current->ref < it->base.id)
        ++it_current;
      if (it_current != existing_current_meta.end() && it_current->ref == it->base.id)
      {
        if (it_current->timestamp <= it->not_before)
          meta_ptr = &*it_current;
        else
          meta_until = it_current->timestamp;
      }
    }

    if (it->before <= meta_until)
    {
      result.push_back({ it->base,
          meta_ptr ? *meta_ptr : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{},
          it->not_before, it->before, it->pos_events });
      ++it;
    }
    else
    {
      result.push_back({ it->base,
          meta_ptr ? *meta_ptr : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{},
          it->not_before, meta_until, it->pos_events });
      it->not_before = meta_until;
    }
  }

  return result;
}


namespace
{
  bool way_meta_equal(const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >& lhs, const OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >& rhs)
  {
    return lhs.ref == rhs.ref && lhs.timestamp == rhs.timestamp && lhs.version == rhs.version;
  }


  void sync_and_compare(
      std::vector< Attic< Way_Skeleton::Id_Type > >::const_iterator& unchanged_before_it,
      const std::vector< Attic< Way_Skeleton::Id_Type > >::const_iterator& unchanged_before_end,
      std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >::const_iterator& it,
      std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& to_delete)
  {
    while (unchanged_before_it != unchanged_before_end && Way_Skeleton::Id_Type(*unchanged_before_it) < it->ref)
      ++unchanged_before_it;
    if (unchanged_before_it == unchanged_before_end || it->ref < Way_Skeleton::Id_Type(*unchanged_before_it)
        || unchanged_before_it->timestamp <= it->timestamp)
      to_delete.insert(*it);
    ++it;
  }
}


Way_Meta_Updater::Way_Meta_Delta::Way_Meta_Delta(
    const std::vector< Way_Event >& events_for_this_idx,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_current_meta,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& existing_attic_meta,
    const std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& deletions,
    const std::vector< Attic< Way_Skeleton::Id_Type > >& unchanged_before)
{
  auto attic_it = existing_attic_meta.begin();
  auto current_it = existing_current_meta.begin();
  auto deletion_it = deletions.begin();
  auto unchanged_before_it = unchanged_before.begin();

  for (auto it = events_for_this_idx.begin(); it != events_for_this_idx.end(); ++it)
  {
    if (it+1 != events_for_this_idx.end() && way_meta_equal(it->meta, (it+1)->meta))
      continue;

    while (attic_it != existing_attic_meta.end() && (
        attic_it->ref < it->meta.ref || (
            attic_it->ref == it->meta.ref && attic_it->timestamp < it->meta.timestamp)))
      sync_and_compare(unchanged_before_it, unchanged_before.end(), attic_it, attic_to_delete);
    while (current_it != existing_current_meta.end() && current_it->ref < it->meta.ref)
      sync_and_compare(unchanged_before_it, unchanged_before.end(), current_it, current_to_delete);

    if (it->before < NOW)
    {
      if (attic_it != existing_attic_meta.end() && way_meta_equal(*attic_it, it->meta))
        ++attic_it;
      else
      {
        if (current_it != existing_current_meta.end() && way_meta_equal(*current_it, it->meta))
        {
          current_to_delete.insert(*current_it);
          ++current_it;
        }
        attic_to_add.insert(it->meta);
      }

      while (deletion_it != deletions.end() && deletion_it->timestamp < it->before)
        ++deletion_it;
      if (deletion_it != deletions.end() && deletion_it->timestamp == it->before)
      {
        if (attic_it != existing_attic_meta.end() && way_meta_equal(*attic_it, *deletion_it))
          ++attic_it;
        else
          attic_to_add.insert(*deletion_it);
        ++deletion_it;
      }
    }
    else
    {
      if (current_it != existing_current_meta.end() && way_meta_equal(*current_it, it->meta))
        ++current_it;
      else
        current_to_add.insert(it->meta);
    }
  }

  while (attic_it != existing_attic_meta.end())
    sync_and_compare(unchanged_before_it, unchanged_before.end(), attic_it, attic_to_delete);
  while (current_it != existing_current_meta.end())
    sync_and_compare(unchanged_before_it, unchanged_before.end(), current_it, current_to_delete);
}
