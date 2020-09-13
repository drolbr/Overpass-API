
#include "way_skeleton_updater.h"


namespace
{
  std::vector< Way_Implicit_Pre_Event > process_way(
      const Way_Skeleton& way, const Moved_Coords& moved_coords, uint64_t not_before, uint64_t before,
      bool add_initial_element)
  {
    std::vector< Way_Implicit_Pre_Event > result;

    std::vector< std::pair< decltype(way.geometry.size()), const Move_Coord_Event* > > pos_events;
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
            pos_events.push_back(std::make_pair(i, j));
        }
      }
      else
      {
        std::vector< const Move_Coord_Event* > events =
            moved_coords.get_coord(way.geometry[i].ll_upper, way.geometry[i].ll_lower);
        for (auto j : events)
        {
          if (j->timestamp < before)
            pos_events.push_back(std::make_pair(i, j));
        }
      }
    }

    if (pos_events.empty())
    {
      if (add_initial_element)
        result.push_back(Way_Implicit_Pre_Event{ way.id, not_before, way.geometry, way.nds });
      return result;
    }

    std::sort(pos_events.begin(), pos_events.end(),
        [](const decltype(pos_events.front())& lhs, const decltype(pos_events.front())& rhs)
        { return lhs.second->timestamp < rhs.second->timestamp ||
            (!(rhs.second->timestamp < lhs.second->timestamp) && lhs.first < rhs.first); });

    Way_Skeleton cur = way;
    uint64_t timestamp = pos_events.front().second->timestamp;
    if (add_initial_element && not_before < timestamp)
      timestamp = not_before;
    for (auto i : pos_events)
    {
      if (i.second->timestamp != timestamp)
      {
        if (not_before < timestamp)
          result.push_back(Way_Implicit_Pre_Event{ way.id, timestamp, cur.geometry, cur.nds });
        else if (not_before < i.second->timestamp)
          result.push_back(Way_Implicit_Pre_Event{ way.id, not_before, cur.geometry, cur.nds });
      }

      if (i.second->visible_after)
      {
        cur.geometry[i.first].ll_upper = i.second->idx_after.val();
        cur.geometry[i.first].ll_lower = i.second->ll_lower_after;
      }
      else
      {
        cur.geometry[i.first].ll_upper = 0u;
        cur.geometry[i.first].ll_lower = 0u;
      }

      if (i.second->multiple_after || !i.second->visible_after)
      {
        if (cur.nds.empty())
          cur.nds.resize(cur.geometry.size(), Node_Skeleton::Id_Type(0ull));
        cur.nds[i.first] = i.second->node_id;
      }
      else if (!cur.nds.empty())
        cur.nds[i.first] = 0ull;

      timestamp = i.second->timestamp;
    }
    if (not_before < timestamp)
      result.push_back(Way_Implicit_Pre_Event{ way.id, timestamp, cur.geometry, cur.nds });
    else
      result.push_back(Way_Implicit_Pre_Event{ way.id, not_before, cur.geometry, cur.nds });

    return result;
  }
}


void Way_Skeleton_Updater::extract_relevant_current_and_attic(
    const Way_Pre_Event_Refs& pre_event_refs,
    const Moved_Coords& moved_coords,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< const Way_Skeleton* >& current,
    const std::vector< const Attic< Way_Skeleton >* >& attic,
    std::vector< Way_Skeleton >& current_result,
    std::vector< Attic< Way_Skeleton > >& attic_result)
{
  auto i_cur = current.begin();
  Way_Skeleton::Id_Type last_ref = 0u;
  uint64_t last_timestamp = 0u;
  bool last_implicitly_moved = false;
  bool last_found = false;
  for (auto i : attic)
  {
    while (i_cur != current.end() && (*i_cur)->id < i->id)
    {
      bool found = false;
      for (const auto& j : pre_event_refs)
      {
        if ((*i_cur)->id == j.ref)
        {
          current_result.push_back(**i_cur);
          found = true;
        }
      }

      std::vector< Way_Implicit_Pre_Event > this_obj_implicit =
          process_way(**i_cur, moved_coords, last_ref == (*i_cur)->id ? last_timestamp : 0u, NOW,
              last_ref == (*i_cur)->id && last_implicitly_moved);
      if (!found && !this_obj_implicit.empty())
        current_result.push_back(**i_cur);
      std::move(this_obj_implicit.begin(), this_obj_implicit.end(), std::back_inserter(implicit_pre_events));

      ++i_cur;
    }

    bool found = false;
    if (last_ref == i->id && last_found)
    {
      attic_result.push_back(*i);
      found = true;
    }
    else
    {
      for (const auto& j : pre_event_refs)
      {
        if (i->id == j.ref && j.timestamp <= i->timestamp)
        {
          attic_result.push_back(*i);
          found = true;
        }
      }
    }

    std::vector< Way_Implicit_Pre_Event > this_obj_implicit =
        process_way(*i, moved_coords, last_ref == i->id ? last_timestamp : 0u, i->timestamp,
            last_ref == i->id && last_implicitly_moved);
    if (!found && !this_obj_implicit.empty())
      attic_result.push_back(*i);
    std::move(this_obj_implicit.begin(), this_obj_implicit.end(), std::back_inserter(implicit_pre_events));

    last_ref = i->id;
    last_timestamp = i->timestamp;
    last_implicitly_moved = !this_obj_implicit.empty();
    last_found = found || !this_obj_implicit.empty();
  }
  while (i_cur != current.end())
  {
    bool found = false;
    for (const auto& j : pre_event_refs)
    {
      if ((*i_cur)->id == j.ref)
      {
        current_result.push_back(**i_cur);
        found = true;
      }
    }

    std::vector< Way_Implicit_Pre_Event > this_obj_implicit =
        process_way(**i_cur, moved_coords, last_ref == (*i_cur)->id ? last_timestamp : 0u, NOW,
            last_ref == (*i_cur)->id && last_implicitly_moved);
    if (!found && !this_obj_implicit.empty())
      current_result.push_back(**i_cur);
    std::move(this_obj_implicit.begin(), this_obj_implicit.end(), std::back_inserter(implicit_pre_events));

    ++i_cur;
  }
}
