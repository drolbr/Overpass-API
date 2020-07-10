#include "../../template_db/block_backend.h"
#include "../../template_db/random_file.h"
#include "../core/settings.h"
#include "basic_updater.h"
#include "mapfile_io.h"


std::map< Uint31_Index, Id_Dates_Per_Idx > Mapfile_IO::read_idx_list(
    const std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >& id_dates)
{
  existing_idx_lists.clear();
  std::map< Uint31_Index, Id_Dates_Per_Idx > result;

  Random_File< Node_Skeleton::Id_Type, Uint31_Index > current(transaction.random_index(
      osm_base_settings().NODES));
  for (const auto& i : id_dates)
  {
    Uint31_Index idx = current.get(i.first.val());
    if (idx.val() > 0)
      result[idx].push_back(i);
  }

  Random_File< Node_Skeleton::Id_Type, Uint31_Index > attic(transaction.random_index(
      attic_settings().NODES));
  std::set< Node_Skeleton::Id_Type > idx_list_req;
  for (const auto& i : id_dates)
  {
    Uint31_Index idx = attic.get(i.first.val());
    if (idx.val() == 0xff)
      idx_list_req.insert(i.first);
    else if (idx.val() > 0)
    {
      result[idx].push_back(i);
      existing_attic_idxs.push_back(std::make_pair(i.first, idx));
    }
  }

  Block_Backend< Node_Skeleton::Id_Type, Uint31_Index > db(transaction.data_index(attic_settings().NODE_IDX_LIST));
  for (typename Block_Backend< Node_Skeleton::Id_Type, Uint31_Index >::Discrete_Iterator
      it = db.discrete_begin(idx_list_req.begin(), idx_list_req.end()); !(it == db.discrete_end()); ++it)
    existing_idx_lists[it.index()].insert(it.object());

  auto it = id_dates.begin();
  for (const auto& i : existing_idx_lists)
  {
    while (it != id_dates.end() && it->first < i.first)
      ++it;
    if (it == id_dates.end() || !(it->first == i.first))
      continue;
    for (auto j : i.second)
      result[j].push_back(*it);
  }

  for (auto& i : result)
  {
    std::sort(i.second.begin(), i.second.end(),
        [](const decltype(id_dates.front())& lhs, const decltype(id_dates.front())& rhs)
        { return lhs.first < rhs.first; });
    i.second.erase(std::unique(i.second.begin(), i.second.end()), i.second.end());
  }

  return result;
}


void Mapfile_IO::compute_and_write_idx_lists(
    const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& nodes_meta_to_move_to_attic,
    const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& nodes_meta_to_add,
    const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >& nodes_attic_meta_to_add)
{
  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_current_map_positions;
  for (const auto& per_idx : nodes_meta_to_add)
  {
    for (const auto& entry : per_idx.second)
    {
      if (new_current_map_positions.empty() || !(new_current_map_positions.back().first == entry.ref)
          || !(new_current_map_positions.back().second == per_idx.first))
        new_current_map_positions.push_back(std::make_pair(entry.ref, per_idx.first));
    }
  }
  std::sort(
      new_current_map_positions.begin(), new_current_map_positions.end(),
      [](
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& lhs,
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& rhs)
      { return lhs.first < rhs.first || (!(rhs.first < lhs.first) && lhs.second < rhs.second); } );

  update_map_positions(new_current_map_positions, transaction, *osm_base_settings().NODES);

  std::vector< std::pair< Node_Skeleton::Id_Type, Uint31_Index > > new_attic_map_positions;
  for (const auto& per_idx : nodes_meta_to_move_to_attic)
  {
    for (const auto& entry : per_idx.second)
    {
      if (new_attic_map_positions.empty() || !(new_attic_map_positions.back().first == entry.ref)
          || !(new_attic_map_positions.back().second == per_idx.first))
        new_attic_map_positions.push_back(std::make_pair(entry.ref, per_idx.first));
    }
  }
  for (const auto& per_idx : nodes_attic_meta_to_add)
  {
    for (const auto& entry : per_idx.second)
    {
      if (new_attic_map_positions.empty() || !(new_attic_map_positions.back().first == entry.ref)
          || !(new_attic_map_positions.back().second == per_idx.first))
        new_attic_map_positions.push_back(std::make_pair(entry.ref, per_idx.first));
    }
  }
  std::sort(
      new_attic_map_positions.begin(), new_attic_map_positions.end(),
      [](
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& lhs,
          const std::pair< Node_Skeleton::Id_Type, Uint31_Index >& rhs)
      { return lhs.first < rhs.first || (!(rhs.first < lhs.first) && lhs.second < rhs.second); } );

  std::map< Node_Skeleton::Id_Type, std::set< Uint31_Index > > new_attic_idx_lists;
  {
    auto from = new_attic_map_positions.begin();
    auto to = new_attic_map_positions.begin();
    auto existing_from = existing_attic_idxs.begin();

    while (from != new_attic_map_positions.end())
    {
      auto i_exist = existing_idx_lists.find(from->first);
      if (i_exist != existing_idx_lists.end())
      {
        auto id = from->first;
        decltype(i_exist->second) idx_list;
        while (from != new_attic_map_positions.end() && from->first == id)
        {
          if (i_exist->second.find(from->second) == i_exist->second.end())
            idx_list.insert(from->second);
          ++from;
        }
        if (!idx_list.empty())
          new_attic_idx_lists.insert(std::make_pair(id, idx_list));
      }
      else
      {
        std::set< Uint31_Index > idx_list;
        while (existing_from != existing_attic_idxs.end() && existing_from->first < from->first)
          ++existing_from;
        if (existing_from != existing_attic_idxs.end() && existing_from->first == from->first)
          idx_list.insert(existing_from->second);

        auto first = from;
        while (from != new_attic_map_positions.end() && from->first == first->first)
        {
          idx_list.insert(from->second);
          ++from;
        }
        if (idx_list.size() <= 1)
          *to = *first;
        else
        {
          new_attic_idx_lists.insert(std::make_pair(first->first, idx_list));
          *to = std::make_pair(first->first, 0xff);
        }
        ++to;
      }
    }
    new_attic_map_positions.erase(to, new_attic_map_positions.end());
  }

  update_map_positions(new_attic_map_positions, transaction, *attic_settings().NODES);
  update_elements(
      decltype(existing_idx_lists)(), new_attic_idx_lists, transaction, *attic_settings().NODE_IDX_LIST);
}
