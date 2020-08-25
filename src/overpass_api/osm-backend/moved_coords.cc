#include "moved_coords.h"


void Moved_Coords::record(Uint31_Index working_idx, const std::vector< Node_Event >& events)
{
  if (hash_built)
    data.clear();
  hash_built = false;

  for (const auto& i : events)
    data.push_back(Move_Coord_Event{
        i.visible_before ? working_idx : Uint31_Index(0u), i.ll_lower_before, i.timestamp, i.id,
        working_idx, i.ll_lower_after, i.visible_after, i.multiple_after });
}


namespace
{
  uint32_t hash_by_id_(Node_Skeleton::Id_Type node_id)
  {
    return ((node_id.val()>>48)^(node_id.val()>>32)
        ^(node_id.val()>>16)^(node_id.val()))&0xffff;
  }

  uint32_t hash_by_coord_(Uint31_Index idx, uint32_t ll_lower)
  {
    return ((idx.val()>>16)^(idx.val())^(ll_lower>>16)^ll_lower)&0xffff;
  }
};


void Moved_Coords::build_hash()
{
  hash_by_id.clear();
  hash_by_id.resize(0x10000);
  hash_by_coord.clear();
  hash_by_coord.resize(0x10000);

  for (decltype(data.size()) i = 0; i < data.size(); ++i)
    hash_by_id[hash_by_id_(data[i].node_id)].push_back(&data[i]);

  for (auto& i : hash_by_id)
  {
    std::sort(i.begin(), i.end(), [](const Move_Coord_Event* lhs, const Move_Coord_Event* rhs)
        { return lhs->node_id < rhs->node_id || (!(rhs->node_id < lhs->node_id) &&
            (lhs->timestamp < rhs->timestamp || (!(rhs->timestamp < lhs->timestamp) &&
            lhs->idx_after < rhs->idx_after))); });

    auto i_last = i.begin();
    auto i_to = i.begin();
    auto i_from = i.begin();
    while (i_from != i.end())
    {
      Uint31_Index idx_after = 0u;
      Uint31_Index idx_before = 0u;
      *i_to = *i_from;
      if (i_to != i.begin() && (*i_to)->node_id == (*i_last)->node_id)
      {
        idx_after = (*i_last)->idx_after;
        idx_before = (*i_last)->idx;
        if (!((*i_from)->idx.val() || (*i_from)->idx_after == idx_before))
        {
          (*i_to)->idx = (*i_last)->idx;
          (*i_to)->ll_lower = (*i_last)->ll_lower;
        }
        if (!((*i_from)->visible_after || (*i_from)->idx_after == idx_after))
        {
          (*i_to)->idx_after = (*i_last)->idx_after;
          (*i_to)->ll_lower_after = (*i_last)->ll_lower_after;
          (*i_to)->visible_after = (*i_last)->visible_after;
          (*i_to)->multiple_after = (*i_last)->multiple_after;
        }
      }
      if (i_from != i.end())
        ++i_from;
      while (i_from != i.end() &&
          (*i_to)->node_id == (*i_from)->node_id && (*i_to)->timestamp == (*i_from)->timestamp)
      {
        if ((*i_from)->idx.val() || (*i_from)->idx_after == idx_before)
        {
          (*i_to)->idx = (*i_from)->idx;
          (*i_to)->ll_lower = (*i_from)->ll_lower;
        }
        if ((*i_from)->visible_after || (*i_from)->idx_after == idx_after)
        {
          (*i_to)->idx_after = (*i_from)->idx_after;
          (*i_to)->ll_lower_after = (*i_from)->ll_lower_after;
          (*i_to)->visible_after = (*i_from)->visible_after;
          (*i_to)->multiple_after = (*i_from)->multiple_after;
        }
        ++i_from;
      }
      i_last = i_to;
      ++i_to;
    }
    i.erase(i_to, i.end());
  }

  for (const auto& i : hash_by_id)
  {
    for (decltype(i.size()) j = 0; j+1 < i.size(); ++j)
    {
      auto cur_hash = hash_by_coord_(i[j]->idx, (i[j]->ll_lower));
      hash_by_coord[cur_hash].push_back(i[j]);
      if (i[j]->node_id == i[j+1]->node_id)
      {
        auto next_hash = hash_by_coord_(i[j+1]->idx, (i[j+1]->ll_lower));
        if (cur_hash != next_hash)
          hash_by_coord[cur_hash].push_back(i[j+1]);
      }
    }
    if (!i.empty() && i.back()->idx.val())
      hash_by_coord[hash_by_coord_(i.back()->idx, i.back()->ll_lower)].push_back(i.back());
  }

  hash_built = true;
}


std::vector< const Move_Coord_Event* > Moved_Coords::get_id(Node_Skeleton::Id_Type node_id) const
{
  if (!hash_built)
    throw std::logic_error("Moved_Coords::get_id(..) called before building the hash");

  const auto& bucket = hash_by_id[hash_by_id_(node_id)];
  std::vector< const Move_Coord_Event* > result;

  for (auto i : bucket)
  {
    if (i->node_id == node_id)
      result.push_back(i);
  }

  return result;
}


std::vector< const Move_Coord_Event* > Moved_Coords::get_coord(Uint31_Index idx, uint32_t ll_lower) const
{
  if (!hash_built)
    throw std::logic_error("Moved_Coords::get_coord(..) called before building the hash");

  const auto& bucket = hash_by_coord[hash_by_coord_(idx, ll_lower)];
  std::vector< const Move_Coord_Event* > result;

  decltype(bucket.front()->node_id) relevant_node_id = 0ull;
  for (auto i : bucket)
  {
    if (i->idx == idx && i->ll_lower == ll_lower)
    {
      result.push_back(i);
      relevant_node_id = i->node_id;
    }
    else if (i->node_id == relevant_node_id)
    {
      result.push_back(i);
      relevant_node_id = 0ull;
    }
  }

  return result;
}
