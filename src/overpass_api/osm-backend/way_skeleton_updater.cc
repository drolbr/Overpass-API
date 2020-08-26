
#include "way_skeleton_updater.h"


namespace
{
  std::vector< Way_Implicit_Pre_Event > process_way(
      const Way_Skeleton& way, const Moved_Coords& moved_coords, uint64_t not_after)
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
          if (j->timestamp < not_after)
            pos_events.push_back(std::make_pair(i, j));
        }
      }
      else
      {
        std::vector< const Move_Coord_Event* > events =
            moved_coords.get_coord(way.geometry[i].ll_upper, way.geometry[i].ll_lower);
        for (auto j : events)
        {
          if (j->timestamp < not_after)
            pos_events.push_back(std::make_pair(i, j));
        }
      }
    }

    if (pos_events.empty())
      return result;

    std::sort(pos_events.begin(), pos_events.end(),
        [](const decltype(pos_events.front())& lhs, const decltype(pos_events.front())& rhs)
        { return lhs.second->timestamp < rhs.second->timestamp ||
            (!(rhs.second->timestamp < lhs.second->timestamp) && lhs.first < rhs.first); });

    Way_Skeleton cur = way;
    uint64_t timestamp = pos_events.front().second->timestamp;
    for (auto i : pos_events)
    {
      if (i.second->timestamp != timestamp)
        result.push_back(Way_Implicit_Pre_Event{ way.id, timestamp, cur.geometry, cur.nds });

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
    result.push_back(Way_Implicit_Pre_Event{ way.id, timestamp, cur.geometry, cur.nds });

    return result;
  }
}


std::vector< Way_Skeleton > Way_Skeleton_Updater::extract_relevant_current(
    const Way_Pre_Event_Refs& pre_event_refs,
    const Moved_Coords& moved_coords,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< Way_Skeleton >& current)
{
  std::vector< Way_Skeleton > result;

  for (const auto& i : current)
  {
    bool found = false;
    for (const auto& j : pre_event_refs)
    {
      if (i.id == j.ref)
      {
        result.push_back(i);
        found = true;
      }
    }

    std::vector< Way_Implicit_Pre_Event > this_obj_implicit = process_way(i, moved_coords, NOW);
    if (!found && !this_obj_implicit.empty())
      result.push_back(i);
    implicit_pre_events.insert(implicit_pre_events.end(), this_obj_implicit.begin(), this_obj_implicit.end());
  }

  return result;
}


std::vector< Attic< Way_Skeleton > > Way_Skeleton_Updater::extract_relevant_attic(
    const Way_Pre_Event_Refs& pre_event_refs,
    const Moved_Coords& moved_coords,
    std::vector< Way_Implicit_Pre_Event >& implicit_pre_events,
    const std::vector< Attic< Way_Skeleton > >& attic)
{
  std::vector< Attic< Way_Skeleton > > result;

  for (const auto& i : attic)
  {
    bool found = false;
    for (const auto& j : pre_event_refs)
    {
      if (i.id == j.ref && j.timestamp <= i.timestamp)
      {
        result.push_back(i);
        found = true;
      }
    }

    std::vector< Way_Implicit_Pre_Event > this_obj_implicit = process_way(i, moved_coords, i.timestamp);
    if (!found && !this_obj_implicit.empty())
      result.push_back(i);
    implicit_pre_events.insert(implicit_pre_events.end(), this_obj_implicit.begin(), this_obj_implicit.end());
  }

  return result;
}
