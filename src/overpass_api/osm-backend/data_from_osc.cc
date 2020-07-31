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


namespace
{
  void keep_oldest_timestamp(std::vector< Pre_Event_Ref >& arg, const Pre_Event_List& events)
  {
    std::sort(arg.begin(), arg.end(),
        [](const Pre_Event_Ref& lhs, const Pre_Event_Ref& rhs)
        { return lhs.ref < rhs.ref || (!(rhs.ref < lhs.ref) && lhs.timestamp < rhs.timestamp); });
    auto i_to = arg.begin();
    Node_Skeleton::Id_Type last_id(0ull); 
    for (auto i_from = arg.begin(); i_from != arg.end(); ++i_from)
    {
      if (i_from == arg.begin() || !(last_id == i_from->ref))
        *(i_to++) = *i_from;
      else if (i_from->offset < (i_to-1)->offset)
        (i_to-1)->offset = i_from->offset;

      last_id = i_from->ref;
    }
    arg.erase(i_to, arg.end());
  }
}


Pre_Event_Refs Data_From_Osc::node_pre_event_refs(Pre_Event_List& events) const
{
  Pre_Event_Refs result;

  if (!events.data.empty())
    result.push_back(Pre_Event_Ref{ events.data[0].entry->meta.ref, events.data[0].entry->meta.timestamp, 0 });
  for (unsigned int i = 1; i < events.data.size(); ++i)
  {
    if (!(events.data[i-1].entry->meta.ref == events.data[i].entry->meta.ref))
      result.push_back(Pre_Event_Ref{ events.data[i].entry->meta.ref, events.data[i].entry->meta.timestamp, i });
  }

  for (const auto& i : ways.data)
  {
    for (const auto& j : i.elem.nds)
      result.push_back(Pre_Event_Ref{ j, i.meta.timestamp, (unsigned int)events.data.size() });
  }

  keep_oldest_timestamp(result, events);

  return result;
}


std::map< Uint31_Index, Pre_Event_Refs > Data_From_Osc::pre_event_refs_by_idx(Pre_Event_List& events) const
{
  std::map< Uint31_Index, Pre_Event_Refs > result;

  unsigned int last_id_begin = 0;
  for (unsigned int i = 0; i < events.data.size(); ++i)
  {
    if (i > 0 && !(events.data[i-1].entry->meta.ref == events.data[i].entry->meta.ref))
      last_id_begin = i;

    if (events.data[i].entry->idx.val())
    {
      Pre_Event_Refs& target = result[events.data[i].entry->idx];
      if (target.empty() || !(target.back().offset == last_id_begin))
        target.push_back(
            Pre_Event_Ref{
                events.data[i].entry->meta.ref, events.data[last_id_begin].entry->meta.timestamp, last_id_begin });
    }
  }

  for (auto& i : result)
    keep_oldest_timestamp(i.second, events);

  return result;
}


std::map< Uint31_Index, Coord_Dates > Data_From_Osc::node_coord_dates() const
{
  std::map< Uint31_Index, Coord_Dates > result;

  for (const auto& i : nodes.data)
  {
    if (i.elem.id.val())
      result[i.idx].push_back(Attic< Uint32 >(i.elem.ll_lower, i.meta.timestamp));
  }

  for (auto& i : result)
    keep_oldest_per_coord(i.second);

  return result;
}
