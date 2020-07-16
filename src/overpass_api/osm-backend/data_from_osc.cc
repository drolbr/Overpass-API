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
      (Uint31_Index(0u), Node_Skeleton(0ull, 0u),
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
      (Uint31_Index(0u), Way_Skeleton(0ull),
      meta ? OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, *meta)
          : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, default_timestamp)));
}


std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > Data_From_Osc::node_id_dates() const
{
  std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > result;

  for (const auto& i : nodes.data)
    result.push_back(std::make_pair(i.meta.ref, i.meta.timestamp));

  for (const auto& i : ways.data)
  {
    for (const auto& j : i.elem.nds)
      result.push_back(std::make_pair(j, i.meta.timestamp));
  }

  keep_oldest_per_first(result);

  return result;
}


Pre_Event_List Data_From_Osc::node_pre_events()
{
  Pre_Event_List result;

  for (auto& i : nodes.data)
    result.data.push_back(Node_Pre_Event(i));

  std::sort(result.data.begin(), result.data.end(), [](const Node_Pre_Event& lhs, const Node_Pre_Event& rhs)
      { return lhs.entry->meta.ref < rhs.entry->meta.ref || (!(rhs.entry->meta.ref < lhs.entry->meta.ref)
            && (lhs.entry->meta.timestamp < rhs.entry->meta.timestamp || (!(rhs.entry->meta.timestamp < lhs.entry->meta.timestamp)
            && lhs.entry->meta.version < rhs.entry->meta.version))); } );

  for (decltype(result.data.size()) i = 0; i+1 < result.data.size(); ++i)
  {
    if (result.data[i+1].entry->meta.ref == result.data[i].entry->meta.ref)
    {
      result.data[i].timestamp_end = result.data[i+1].entry->meta.timestamp;
      if (!result.data[i+1].entry->elem.id.val())
        result.data[i+1].entry->idx = result.data[i].entry->idx;
    }
  }

  for (auto& i : result.data)
  {
    if (!i.entry->elem.id.val())
      result.timestamp_last_not_deleted.push_back(Node_Pre_Event(*i.entry, 0ull));
  }

  return result;
}


std::map< Uint31_Index, Coord_Dates_Per_Idx > Data_From_Osc::node_coord_dates() const
{
  std::map< Uint31_Index, Coord_Dates_Per_Idx > result;

  for (const auto& i : nodes.data)
  {
    if (i.elem.id.val())
      result[i.idx].push_back(Attic< Uint32 >(i.elem.ll_lower, i.meta.timestamp));
  }

  for (auto& i : result)
    keep_oldest_per_coord(i.second);

  return result;
}
