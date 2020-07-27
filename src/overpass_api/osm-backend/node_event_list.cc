#include "node_event_list.h"


namespace
{
  template< typename Data >
  struct Event_Builder
  {
    void push_event(const Node_Skeleton& skel, uint64_t last_timestamp);
    void push_event(Node_Skeleton::Id_Type id, uint64_t last_timestamp);

    uint64_t process_first_appearance(Node_Skeleton::Id_Type id, uint64_t maybe_result);
    void finalize_first_appearance();

    std::vector< Node_Event >& data;
    std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >::const_iterator i_first;
    std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >::const_iterator i_first_end;
  };


  template< typename Data >
  uint64_t Event_Builder< Data >::process_first_appearance(Node_Skeleton::Id_Type id, uint64_t maybe_result)
  {
    if (data.empty() || !(data.back().id == id))
    {
      while (i_first != i_first_end && !data.empty() && !(data.back().id < i_first->first))
        ++i_first;
      while (i_first != i_first_end && i_first->first < id)
      {
        data.push_back(Node_Event{ i_first->first, i_first->second, false, 0, false, 0, false });
        ++i_first;
      }
      if (i_first != i_first_end && i_first->first == id)
        maybe_result = i_first->second;
    }
    return maybe_result;
  }


  template< typename Data >
  void Event_Builder< Data >::finalize_first_appearance()
  {
    while (i_first != i_first_end && !data.empty() && !(data.back().id < i_first->first))
      ++i_first;
    while (i_first != i_first_end)
    {
      data.push_back(Node_Event{ i_first->first, i_first->second, false, 0, false, 0, false });
      ++i_first;
    }
  }


  template< typename Data >
  void Event_Builder< Data >::push_event(const Node_Skeleton& skel, uint64_t last_timestamp)
  {
    last_timestamp = process_first_appearance(skel.id, last_timestamp);
    data.push_back(Node_Event{
        skel.id, last_timestamp, true, skel.ll_lower, true, skel.ll_lower, false });
  }


  template< typename Data >
  void Event_Builder< Data >::push_event(Node_Skeleton::Id_Type id, uint64_t last_timestamp)
  {
    last_timestamp = process_first_appearance(id, last_timestamp);
    data.push_back(Node_Event{ id, last_timestamp, false, 0, false, 0, false });
  }


  template< typename Iterator, typename Data >
  void process_current(
      Iterator& i_current, Iterator i_end, Event_Builder< Data >& builder,
      Node_Skeleton::Id_Type comp_id, uint64_t last_timestamp)
  {
    if (!builder.data.empty() && builder.data.back().id < comp_id
        && (i_current == i_end || builder.data.back().id < i_current->id))
      builder.push_event(builder.data.back().id, last_timestamp);
    while (i_current != i_end && i_current->id < comp_id)
    {
      builder.push_event(*i_current, last_timestamp);
      ++i_current;
    }
  }


  template< typename Iterator, typename Data >
  void process_current(
      Iterator& i_current, Iterator i_end, Event_Builder< Data >& builder, uint64_t last_timestamp)
  {
    if (!builder.data.empty() && (i_current == i_end || builder.data.back().id < i_current->id))
      builder.push_event(builder.data.back().id, last_timestamp);
    while (i_current != i_end)
    {
      builder.push_event(*i_current, last_timestamp);
      ++i_current;
    }
  }


  struct Lower_Coord_Usage
  {
    uint32 ll_lower;
    uint64_t timestamp;
    bool is_begin;

    bool operator<(const Lower_Coord_Usage& rhs) const
    { return ll_lower < rhs.ll_lower || (!(rhs.ll_lower < ll_lower)
        && (timestamp < rhs.timestamp || (!(rhs.timestamp < timestamp) && is_begin < rhs.is_begin))); }
  };


  void set_node_multiplicity(std::vector< Node_Event >& data)
  {
    std::vector< Lower_Coord_Usage > ll_lower_usage;
    ll_lower_usage.reserve(2*data.size());
    for (decltype(data.size()) i = 0; i < data.size(); ++i)
    {
      if (data[i].visible_after)
      {
        ll_lower_usage.push_back(Lower_Coord_Usage{ data[i].ll_lower_after, data[i].timestamp, true });
        if (i+1 < data.size() && data[i+1].id == data[i].id)
          ll_lower_usage.push_back(Lower_Coord_Usage{ data[i].ll_lower_after, data[i+1].timestamp, false });
      }
    }
    std::sort(ll_lower_usage.begin(), ll_lower_usage.end());

    auto i_to = ll_lower_usage.begin();
    int cnt = 0;
    uint32 last_ll = 0u;
    for (auto i_from = ll_lower_usage.begin(); i_from != ll_lower_usage.end(); ++i_from)
    {
      if (last_ll != i_from->ll_lower)
        cnt = 0;
      if (i_from->is_begin)
      {
        ++cnt;
        if (cnt == 2)
        {
          *i_to = *i_from;
          ++i_to;
        }
      }
      else
      {
        --cnt;
        if (cnt == 1)
        {
          *i_to = *i_from;
          ++i_to;
        }
      }
      last_ll = i_from->ll_lower;
    }
    ll_lower_usage.erase(i_to, ll_lower_usage.end());

    if (!ll_lower_usage.empty())
    {
      std::vector< Node_Event > existing_data;
      existing_data.swap(data);
      data.reserve(existing_data.size() + ll_lower_usage.size());

      for (decltype(existing_data.size()) i = 0; i < existing_data.size(); ++i)
      {
        auto j_ll = std::upper_bound(ll_lower_usage.begin(), ll_lower_usage.end(),
            Lower_Coord_Usage{ existing_data[i].ll_lower_after, existing_data[i].timestamp, true } );
        if (j_ll == ll_lower_usage.end() || j_ll->ll_lower != existing_data[i].ll_lower_after)
        {
          data.push_back(existing_data[i]);
          if (j_ll != ll_lower_usage.begin())
          {
            --j_ll;
            if (j_ll->ll_lower == existing_data[i].ll_lower_after)
              data.back().multiple_after = j_ll->is_begin;
          }
        }
        else
        {
          data.push_back(existing_data[i]);
          data.back().multiple_after = !j_ll->is_begin;
          while (j_ll != ll_lower_usage.end() && j_ll->ll_lower == existing_data[i].ll_lower_after
              && (i+1 >= existing_data.size() || !(existing_data[i].id == existing_data[i+1].id)
                  || j_ll->timestamp < existing_data[i+1].timestamp))
          {
            if (data.back().timestamp != j_ll->timestamp)
            {
              data.push_back(existing_data[i]);
              data.back().timestamp = j_ll->timestamp;
            }
            data.back().multiple_after = j_ll->is_begin;
            ++j_ll;
          }
        }
      }
    }
  }
}


Node_Event_List::Node_Event_List(
    Uint31_Index working_idx, const Node_Skeletons_Per_Idx& skels, const Pre_Event_List& pre_events)
{
  std::vector< Node_Event > existing_data;
  existing_data.reserve(skels.first_appearance.size() + skels.attic.size() + skels.undeleted.size());

  auto i_first = skels.first_appearance.begin();
  auto i_current = skels.current.begin();
  auto i_undel = skels.undeleted.begin();
  uint64_t last_timestamp = 0ull;
  Event_Builder< decltype(data) > builder
      { existing_data, skels.first_appearance.begin(), skels.first_appearance.end() };
  for (const auto& i_attic : skels.attic)
  {
    while (i_undel != skels.undeleted.end() && (Node_Skeleton::Id_Type(*i_undel) < i_attic.id ||
        (!(i_attic.id < Node_Skeleton::Id_Type(*i_undel)) && i_undel->timestamp < i_attic.timestamp)))
    {
      process_current(i_current, skels.current.end(), builder, Node_Skeleton::Id_Type(*i_undel), last_timestamp);
      builder.push_event(*i_undel, last_timestamp);
      last_timestamp = i_undel->timestamp;
      ++i_undel;
    }
    process_current(i_current, skels.current.end(), builder, i_attic.id, last_timestamp);
    builder.push_event(i_attic, last_timestamp);
    last_timestamp = i_attic.timestamp;
  }
  while (i_undel != skels.undeleted.end())
  {
    process_current(i_current, skels.current.end(), builder, Node_Skeleton::Id_Type(*i_undel), last_timestamp);
    builder.push_event(*i_undel, last_timestamp);
    last_timestamp = i_undel->timestamp;
    ++i_undel;
  }
  process_current(i_current, skels.current.end(), builder, last_timestamp);
  builder.finalize_first_appearance();

  data.reserve(2*existing_data.size());

  auto i_ex = existing_data.begin();
  for (const auto& i_pre : pre_events.data)
  {
    while (i_ex != existing_data.end() && (i_ex->id < i_pre.entry->meta.ref ||
        (i_ex->id == i_pre.entry->meta.ref && i_ex->timestamp < i_pre.entry->meta.timestamp)))
    {
      data.push_back(*i_ex);
      ++i_ex;
    }
    if (!data.empty() && data.back().id == i_pre.entry->meta.ref)
    {
      if (i_pre.entry->idx == working_idx)
        data.push_back(Node_Event{
            i_pre.entry->meta.ref, i_pre.entry->meta.timestamp,
            data.back().visible_before, data.back().ll_lower_before,
            i_pre.entry->elem.id.val(), i_pre.entry->elem.ll_lower, false });
      else
        data.push_back(Node_Event{
            i_pre.entry->meta.ref, i_pre.entry->meta.timestamp,
            data.back().visible_before, data.back().ll_lower_before,
            false, 0u, false });
    }
    else
    {
      if (i_pre.entry->idx == working_idx)
        data.push_back(Node_Event{
            i_pre.entry->meta.ref, i_pre.entry->meta.timestamp,
            false, 0u, i_pre.entry->elem.id.val(), i_pre.entry->elem.ll_lower, false });
    }
  }
  while (i_ex != existing_data.end())
  {
    data.push_back(*i_ex);
    ++i_ex;
  }

  set_node_multiplicity(data);
}
