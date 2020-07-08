#include "data_from_osc.h"


void Data_From_Osc::set_node(const Node& node, const OSM_Element_Metadata* meta)
{
  nodes.data.push_back(Data_By_Id< Node_Skeleton >::Entry
      (Uint31_Index(node.index), Node_Skeleton(node),
      meta ? OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(node.id, *meta)
          : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(node.id, default_timestamp),
          node.tags));
}


void Data_From_Osc::set_node_deleted(Node::Id_Type id, const OSM_Element_Metadata* meta)
{
  nodes.data.push_back(Data_By_Id< Node_Skeleton >::Entry
      (Uint31_Index(0u), Node_Skeleton(id, 0u),
      meta ? OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(id, *meta)
          : OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(id, default_timestamp)));
}


void Data_From_Osc::set_way(const Way& way, const OSM_Element_Metadata* meta)
{
  ways.data.push_back(Data_By_Id< Way_Skeleton >::Entry
      (Uint31_Index(0xff), Way_Skeleton(way),
      meta ? OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(way.id, *meta)
          : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(way.id, default_timestamp),
          way.tags));
}


void Data_From_Osc::set_way_deleted(Way::Id_Type id, const OSM_Element_Metadata* meta)
{
  ways.data.push_back(Data_By_Id< Way_Skeleton >::Entry
      (Uint31_Index(0u), Way_Skeleton(id),
      meta ? OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, *meta)
          : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, default_timestamp)));
}


std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > Data_From_Osc::node_id_dates() const
{
  std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > result;

  for (const auto& i : nodes.data)
    result.push_back(std::make_pair(i.elem.id, i.meta.timestamp));

  for (const auto& i : ways.data)
  {
    for (const auto& j : i.elem.nds)
      result.push_back(std::make_pair(j, i.meta.timestamp));
  }

  std::sort(result.begin(), result.end(),
      [](const std::pair< Node_Skeleton::Id_Type, uint64_t >& lhs, const std::pair< Node_Skeleton::Id_Type, uint64_t >& rhs)
      { return lhs.first.val() < rhs.first.val() || (!(rhs.first.val() < lhs.first.val())
          && lhs.second < rhs.second); });
  auto i_to = result.begin();
  Node_Skeleton::Id_Type last_id(0ull); 
  for (auto i_from = result.begin(); i_from != result.end(); ++i_from)
  {
    if (i_from == result.begin() || !(last_id == i_from->first))
      *(i_to++) = *i_from;
    last_id = i_from->first;
  }
  result.erase(i_to, result.end());

  return result;
}


Pre_Event_List Data_From_Osc::node_pre_events() const
{
  Pre_Event_List result;

  for (const auto& i : nodes.data)
    result.data.push_back(Node_Pre_Event(i));

  std::sort(result.data.begin(), result.data.end(), [](const Node_Pre_Event& lhs, const Node_Pre_Event& rhs)
      { return lhs.entry->elem.id < rhs.entry->elem.id || (!(rhs.entry->elem.id < lhs.entry->elem.id)
            && (lhs.entry->meta.timestamp < rhs.entry->meta.timestamp || (!(rhs.entry->meta.timestamp < lhs.entry->meta.timestamp)
            && lhs.entry->meta.version < rhs.entry->meta.version))); } );

  for (decltype(result.data.size()) i = 0; i+1 < result.data.size(); ++i)
  {
    if (result.data[i+1].entry->elem.id == result.data[i].entry->elem.id)
      result.data[i].timestamp_end = result.data[i+1].entry->meta.timestamp;
  }

  return result;
}


std::map< Uint31_Index, Coord_Dates_Per_Idx > Data_From_Osc::node_coord_dates() const
{
  std::map< Uint31_Index, Coord_Dates_Per_Idx > result;

  for (const auto& i : nodes.data)
  {
    if (!(i.idx == Uint31_Index(0u)))
      result[i.idx].push_back(Attic< Uint32 >(i.elem.ll_lower, i.meta.timestamp));
  }

  for (auto& i : result)
  {
    std::sort(i.second.begin(), i.second.end(), [](const Attic< Uint32 >& lhs, const Attic< Uint32 >& rhs)
        { return lhs.val() < rhs.val() || (!(rhs.val() < lhs.val()) && lhs.timestamp < rhs.timestamp); });
    auto i_to = i.second.begin();
    Uint32 last_coord(0u);
    for (auto i_from = i.second.begin(); i_from != i.second.end(); ++i_from)
    {
      if (i_from == i.second.begin() || !(last_coord == i_from->val()))
        *(i_to++) = *i_from;
      last_coord = i_from->val();
    }
    i.second.erase(i_to, i.second.end());
  }

  return result;
}
