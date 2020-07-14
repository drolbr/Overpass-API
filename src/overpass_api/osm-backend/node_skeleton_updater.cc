#include "node_skeleton_updater.h"


void Node_Skeleton_Updater::collect_relevant_coords_current(
    const Id_Dates_Per_Idx& id_dates,
    const std::vector< Node_Skeleton >& current,
    Coord_Dates_Per_Idx& result)
{
  for (const auto& i : current)
  {
    for (const auto& j : id_dates)
    {
      if (i.id == j.first)
        result.push_back(Attic< Uint32 >(i.ll_lower, j.second));
    }
  }

  keep_oldest_per_coord(result);
}


// Adds to the result all coordinates from nodes whose ids appear in id_dates
void Node_Skeleton_Updater::collect_relevant_coords_attic(
    const Id_Dates_Per_Idx& id_dates,
    const std::vector< Attic< Node_Skeleton > >& attic,
    Coord_Dates_Per_Idx& result)
{
  for (const auto& i : attic)
  {
    for (const auto& j : id_dates)
    {
      if (i.id == j.first && j.second < i.timestamp)
        result.push_back(Attic< Uint32 >(i.ll_lower, j.second));
    }
  }

  keep_oldest_per_coord(result);
}


// Keeps from all the objects in the idx only those that are relevant for the event list
std::vector< Node_Skeleton > Node_Skeleton_Updater::extract_relevant_current(
    const Id_Dates_Per_Idx& id_dates,
    Id_Dates_Per_Idx& coord_sharing_ids,
    const Coord_Dates_Per_Idx& coord_dates_per_idx,
    const std::vector< Node_Skeleton >& current)
{
  std::vector< Node_Skeleton > result;

  for (const auto& i : current)
  {
    bool found = false;
    for (const auto& j : id_dates)
    {
      if (i.id == j.first)
      {
        result.push_back(i);
        found = true;
      }
    }
    if (found)
      continue;

    for (const auto& j : coord_dates_per_idx)
    {
      if (i.ll_lower == j.val())
      {
        result.push_back(i);
        coord_sharing_ids.push_back(std::make_pair(i.id, j.timestamp));
      }
    }
  }

  keep_oldest_per_first(coord_sharing_ids);
  return result;
}


// Keeps from all the objects in the idx only those that are relevant for the event list
std::vector< Attic< Node_Skeleton > > Node_Skeleton_Updater::extract_relevant_attic(
    const Id_Dates_Per_Idx& id_dates,
    Id_Dates_Per_Idx& coord_sharing_ids,
    const Coord_Dates_Per_Idx& coord_dates_per_idx,
    const std::vector< Attic< Node_Skeleton > >& attic)
{
  std::vector< Attic< Node_Skeleton > > result;

  for (const auto& i : attic)
  {
    bool found = false;
    for (const auto& j : id_dates)
    {
      if (i.id == j.first && j.second < i.timestamp)
      {
        result.push_back(i);
        found = true;
      }
    }
    if (found)
      continue;

    for (const auto& j : coord_dates_per_idx)
    {
      if (i.ll_lower == j.val() && j.timestamp < i.timestamp)
      {
        result.push_back(i);
        coord_sharing_ids.push_back(std::make_pair(i.id, j.timestamp));
      }
    }
  }

  keep_oldest_per_first(coord_sharing_ids);
  return result;
}
