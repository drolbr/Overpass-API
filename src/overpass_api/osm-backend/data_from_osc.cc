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
      (Uint31_Index(0u), Way_Skeleton::Id_Type(0u),
      meta ? OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, *meta)
          : OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(id, default_timestamp)));
}


namespace
{
  template< typename Skeleton >
  Pre_Event_List< Skeleton > pre_events(Data_By_Id< Skeleton >& skels)
  {
    Pre_Event_List< Skeleton > result;

    for (auto& i : skels.data)
      result.data.push_back(Pre_Event< Skeleton >(i));

    std::sort(result.data.begin(), result.data.end(), [](const Pre_Event< Skeleton >& lhs, const Pre_Event< Skeleton >& rhs)
        { return lhs.entry->meta.ref < rhs.entry->meta.ref || (!(rhs.entry->meta.ref < lhs.entry->meta.ref)
              && (lhs.entry->meta.timestamp < rhs.entry->meta.timestamp || (!(rhs.entry->meta.timestamp < lhs.entry->meta.timestamp)
              && lhs.entry->meta.version < rhs.entry->meta.version))); } );

    if (!result.data.empty() && !result.data[0].entry->elem.id.val())
      result.timestamp_last_not_deleted.push_back(Pre_Event< Skeleton >(*result.data[0].entry, 0ull));
    for (decltype(result.data.size()) i = 0; i+1 < result.data.size(); ++i)
    {
      if (result.data[i+1].entry->meta.ref == result.data[i].entry->meta.ref)
      {
        result.data[i].timestamp_end = result.data[i+1].entry->meta.timestamp;
        if (!result.data[i+1].entry->elem.id.val())
        {
          result.data[i+1].entry->idx = result.data[i].entry->idx;
          result.timestamp_last_not_deleted.push_back(
              Pre_Event< Skeleton >(*result.data[i+1].entry, result.data[i].entry->meta.timestamp));
        }
      }
      else if (!result.data[i+1].entry->elem.id.val())
        result.timestamp_last_not_deleted.push_back(Pre_Event< Skeleton >(*result.data[i+1].entry, 0ull));
    }

    return result;
  }
}


Pre_Event_List< Node_Skeleton > Data_From_Osc::node_pre_events()
{
  return pre_events< Node_Skeleton >(nodes);
}


Pre_Event_List< Way_Skeleton > Data_From_Osc::way_pre_events()
{
  return pre_events< Way_Skeleton >(ways);
}


namespace
{
  void keep_oldest_timestamp(std::vector< Pre_Event_Ref< Node_Skeleton::Id_Type > >& arg, const Pre_Event_List< Node_Skeleton >& events)
  {
    std::sort(arg.begin(), arg.end(),
        [](const Pre_Event_Ref< Node_Skeleton::Id_Type >& lhs, const Pre_Event_Ref< Node_Skeleton::Id_Type >& rhs)
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


Node_Pre_Event_Refs Data_From_Osc::node_pre_event_refs(Pre_Event_List< Node_Skeleton >& events) const
{
  Node_Pre_Event_Refs result;

  if (!events.data.empty())
    result.push_back(Pre_Event_Ref< Node_Skeleton::Id_Type >{ events.data[0].entry->meta.ref, events.data[0].entry->meta.timestamp, 0 });
  for (unsigned int i = 1; i < events.data.size(); ++i)
  {
    if (!(events.data[i-1].entry->meta.ref == events.data[i].entry->meta.ref))
      result.push_back(Pre_Event_Ref< Node_Skeleton::Id_Type >{ events.data[i].entry->meta.ref, events.data[i].entry->meta.timestamp, i });
  }

  for (const auto& i : ways.data)
  {
    for (const auto& j : i.elem.nds)
      result.push_back(Pre_Event_Ref< Node_Skeleton::Id_Type >{ j, i.meta.timestamp, (unsigned int)events.data.size() });
  }

  keep_oldest_timestamp(result, events);

  return result;
}


std::map< Uint31_Index, Node_Pre_Event_Refs > Data_From_Osc::pre_event_refs_by_idx(Pre_Event_List< Node_Skeleton >& events)
{
  std::map< Uint31_Index, Node_Pre_Event_Refs > result;

  unsigned int last_id_begin = 0;
  for (unsigned int i = 0; i < events.data.size(); ++i)
  {
    if (i > 0 && !(events.data[i-1].entry->meta.ref == events.data[i].entry->meta.ref))
      last_id_begin = i;

    if (events.data[i].entry->idx.val())
    {
      Node_Pre_Event_Refs& target = result[events.data[i].entry->idx];
      if (target.empty() || !(target.back().offset == last_id_begin))
        target.push_back(
            Pre_Event_Ref< Node_Skeleton::Id_Type >{
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
