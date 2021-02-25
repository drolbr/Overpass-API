
#include "way_skeleton_updater.h"


namespace
{
  std::vector< std::pair< decltype(Way_Skeleton::geometry.size()), const Move_Coord_Event* > > process_way(
      const Way_Skeleton& way, const Moved_Coords& moved_coords, uint64_t before)
  {
    std::vector< std::pair< decltype(way.geometry.size()), const Move_Coord_Event* > > result;
    if (!way.nds.empty() && way.nds.size() != way.geometry.size())
      throw std::logic_error("Way_Skeleton_Updater::process_way: size of nds does not match size of geometry");
    for (decltype(way.geometry.size()) i = 0; i < way.geometry.size(); ++i)
    {
      if (!way.nds.empty() && way.nds[i].val())
      {
        std::vector< const Move_Coord_Event* > events = moved_coords.get_id(way.nds[i]);
        for (auto j : events)
        {
          if (j->timestamp < before)
            result.push_back(std::make_pair(i, j));
        }
      }
      else
      {
        std::vector< const Move_Coord_Event* > events =
            moved_coords.get_coord(way.geometry[i].ll_upper, way.geometry[i].ll_lower);
        for (auto j : events)
        {
          if (j->timestamp < before)
            result.push_back(std::make_pair(i, j));
        }
      }
    }

    std::sort(result.begin(), result.end(),
        [](const decltype(result.front())& lhs, const decltype(result.front())& rhs)
        { return lhs.second->timestamp < rhs.second->timestamp ||
            (!(rhs.second->timestamp < lhs.second->timestamp) && lhs.first < rhs.first); });

    return result;
  }
}


void Way_Skeleton_Updater::extract_relevant_current_and_attic(
    const Way_Pre_Event_Refs& pre_event_refs,
    const Moved_Coords& moved_coords,
    const std::vector< const Way_Skeleton* >& current,
    const std::vector< const Attic< Way_Skeleton >* >& attic,
    std::vector< Way_Skeleton >& current_result,
    std::vector< Attic< Way_Skeleton > >& attic_result,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events)
{
  auto i_cur = current.begin();
  Way_Skeleton::Id_Type last_ref = 0u;
  uint64_t last_timestamp = 0u;
  bool last_found = false;
  for (auto i : attic)
  {
    while (i_cur != current.end() && (*i_cur)->id < i->id)
    {
      bool found = (last_ref == (*i_cur)->id && last_found);
      if (!found)
      {
        for (const auto& j : pre_event_refs)
          found |= ((*i_cur)->id == j.ref);
      }

      auto pos_events = process_way(**i_cur, moved_coords, NOW);
      if (found || !pos_events.empty())
      {
        current_result.push_back(**i_cur);
        implicit_pre_events.push_back(
            Way_Implicit_Pre_Event{ **i_cur, last_ref == (*i_cur)->id ? last_timestamp : 0u, NOW, std::move(pos_events) });
      }

      ++i_cur;
    }

    bool found = (last_ref == i->id && last_found);
    if (!found)
    {
      for (const auto& j : pre_event_refs)
        found |= (i->id == j.ref && j.timestamp <= i->timestamp);
    }

    auto pos_events = process_way(*i, moved_coords, i->timestamp);
    last_found = found || !pos_events.empty();
    if (found || !pos_events.empty())
    {
      attic_result.push_back(*i);
      implicit_pre_events.push_back(
          Way_Implicit_Pre_Event{ *i, last_ref == (*i_cur)->id ? last_timestamp : 0u, i->timestamp, std::move(pos_events) });
    }

    last_ref = i->id;
    last_timestamp = i->timestamp;
  }
  while (i_cur != current.end())
  {
    bool found = (last_ref == (*i_cur)->id && last_found);
    if (!found)
    {
      for (const auto& j : pre_event_refs)
        found |= ((*i_cur)->id == j.ref);
    }

    auto pos_events = process_way(**i_cur, moved_coords, NOW);
    if (found || !pos_events.empty())
    {
      current_result.push_back(**i_cur);
      implicit_pre_events.push_back(
          Way_Implicit_Pre_Event{ **i_cur, last_ref == (*i_cur)->id ? last_timestamp : 0u, NOW, std::move(pos_events) });
    }

    ++i_cur;
  }
}


namespace
{
  void clear_if_all_zero(std::vector< Node_Skeleton::Id_Type >& nds)
  {
    for (auto i : nds)
    {
      if (i.val())
        return;
    }
    nds.clear();
  }
}


void Way_Skeleton_Updater::resolve_coord_events(
    Uint31_Index cur_idx,
    const std::vector< Proto_Way >& proto_events,
    std::vector< Way_Event >& events_for_this_idx,
    std::map< Uint31_Index, std::vector< Way_Event > >& arrived_objects)
{
  for (const auto& i : proto_events)
  {
    Way_Skeleton cur = i.base;
    uint64_t timestamp = i.not_before;
    for (auto j : i.pos_events)
    {
      if (timestamp < j.second->timestamp)
      {
        clear_if_all_zero(cur.nds);
        Uint31_Index skel_idx = calc_index(cur.geometry);
        if (skel_idx == cur_idx)
          events_for_this_idx.push_back(Way_Event{ cur, i.meta, timestamp, j.second->timestamp });
        else
          arrived_objects[skel_idx].push_back(Way_Event{ cur, i.meta, timestamp, j.second->timestamp });

        timestamp = j.second->timestamp;
      }

      if (j.second->visible_after)
      {
        cur.geometry[j.first].ll_upper = j.second->idx_after.val();
        cur.geometry[j.first].ll_lower = j.second->ll_lower_after;
      }
      else
      {
        cur.geometry[j.first].ll_upper = 0u;
        cur.geometry[j.first].ll_lower = 0u;
      }

      if (j.second->multiple_after || !j.second->visible_after)
      {
        if (cur.nds.empty())
          cur.nds.resize(cur.geometry.size(), Node_Skeleton::Id_Type(0ull));
        cur.nds[j.first] = j.second->node_id;
      }
      else if (!cur.nds.empty())
        cur.nds[j.first] = 0ull;
    }

    clear_if_all_zero(cur.nds);
    Uint31_Index skel_idx = calc_index(cur.geometry);
    if (skel_idx == cur_idx)
      events_for_this_idx.push_back(Way_Event{ cur, i.meta, timestamp, i.before });
    else
      arrived_objects[skel_idx].push_back(Way_Event{ cur, i.meta, timestamp, i.before });
  }
}


void Way_Skeleton_Updater::resolve_coord_events(
    const Pre_Event_List< Way_Skeleton >& pre_events,
    const Moved_Coords& moved_coords,
    std::map< Uint31_Index, std::vector< Way_Event > >& changes_per_idx,
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >& deletions)
{
  for (const auto& i : pre_events.data)
  {
    Way_Skeleton cur = i.entry->elem;
    uint64_t timestamp = i.entry->meta.timestamp;

    if (cur.id == Way_Skeleton::Id_Type(0u))
      deletions.push_back(i.entry->meta);
    else
    {
      auto pos_events = process_way(cur, moved_coords, i.timestamp_end);
      for (auto j : pos_events)
      {
        if (timestamp < j.second->timestamp)
        {
          clear_if_all_zero(cur.nds);
          Uint31_Index skel_idx = calc_index(cur.geometry);
          changes_per_idx[skel_idx].push_back(Way_Event{ cur, i.entry->meta, timestamp, j.second->timestamp });

          timestamp = j.second->timestamp;
        }

        if (j.second->visible_after)
        {
          cur.geometry[j.first].ll_upper = j.second->idx_after.val();
          cur.geometry[j.first].ll_lower = j.second->ll_lower_after;
        }
        else
        {
          cur.geometry[j.first].ll_upper = 0u;
          cur.geometry[j.first].ll_lower = 0u;
        }

        if (j.second->multiple_after || !j.second->visible_after)
        {
          if (cur.nds.empty())
            cur.nds.resize(cur.geometry.size(), Node_Skeleton::Id_Type(0ull));
          cur.nds[j.first] = j.second->node_id;
        }
        else if (!cur.nds.empty())
          cur.nds[j.first] = 0ull;
      }

      clear_if_all_zero(cur.nds);
      Uint31_Index skel_idx = calc_index(cur.geometry);
      changes_per_idx[skel_idx].push_back(Way_Event{ cur, i.entry->meta, timestamp, i.timestamp_end });
    }
  }
}


Way_Skeleton_Updater::Way_Skeleton_Delta::Way_Skeleton_Delta(
    const std::vector< Way_Event >& events_for_this_idx,
    const std::vector< const Way_Skeleton* >& current,
    const std::vector< const Attic< Way_Skeleton >* >& attic)
{
  auto attic_it = attic.begin();
  auto current_it = current.begin();

  for (auto it = events_for_this_idx.begin(); it != events_for_this_idx.end(); ++it)
  {
    if (it+1 != events_for_this_idx.end()
        && it->skel.id == (it+1)->skel.id
        && it->skel.geometry == (it+1)->skel.geometry
        && it->skel.nds == (it+1)->skel.nds
        && it->before == (it+1)->not_before)
      continue;

    while (attic_it != attic.end() && ((*attic_it)->id < it->skel.id ||
        ((*attic_it)->id == it->skel.id && (*attic_it)->timestamp < it->before)))
    {
      attic_to_delete.push_back(**attic_it);
      ++attic_it;
    }
    while (current_it != current.end() && (*current_it)->id < it->skel.id)
    {
      current_to_delete.push_back(**current_it);
      ++current_it;
    }

    if (it->before < NOW)
    {
      if (attic_it != attic.end() && it->skel.id == (*attic_it)->id && it->before == (*attic_it)->timestamp
          && it->skel.geometry == (*attic_it)->geometry && it->skel.nds == (*attic_it)->nds)
        ++attic_it;
      else
        attic_to_add.push_back(Attic< Way_Skeleton >(it->skel, it->before));
    }
    else
    {
      if (current_it != current.end() && it->skel.id == (*current_it)->id
          && it->skel.geometry == (*current_it)->geometry && it->skel.nds == (*current_it)->nds)
        ++current_it;
      else
        current_to_add.push_back(it->skel);
    }
  }

  while (attic_it != attic.end())
  {
    attic_to_delete.push_back(**attic_it);
    ++attic_it;
  }
  while (current_it != current.end())
  {
    current_to_delete.push_back(**current_it);
    ++current_it;
  }
}


Way_Skeleton_Updater::Way_Undelete_Delta::Way_Undelete_Delta(
    const std::vector< Way_Event >& events_for_this_idx,
    const std::vector< Attic< Way_Skeleton::Id_Type > >& existing)
{
  auto existing_it = existing.begin();
  for (auto it = events_for_this_idx.begin(); it != events_for_this_idx.end(); ++it)
  {
    if ((it != events_for_this_idx.begin() && (it-1)->skel.id == it->skel.id)
          ? (it-1)->before < it->not_before
          : it->meta.timestamp < it->not_before)
    {
      while (existing_it != existing.end() &&
          (Way_Skeleton::Id_Type(*existing_it) < it->skel.id || (Way_Skeleton::Id_Type(*existing_it) == it->skel.id && existing_it->timestamp < it->not_before)))
      {
        undeletes_to_delete.push_back(*existing_it);
        ++existing_it;
      }
      if (existing_it != existing.end() &&
          Way_Skeleton::Id_Type(*existing_it) == it->skel.id && existing_it->timestamp == it->not_before)
        ++existing_it;
      else
        undeletes_to_add.push_back(Attic< Way_Skeleton::Id_Type >{it->skel.id, it->not_before});
    }
  }

  while (existing_it != existing.end())
  {
    undeletes_to_delete.push_back(*existing_it);
    ++existing_it;
  }
}
