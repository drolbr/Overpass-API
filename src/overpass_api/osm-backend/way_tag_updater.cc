#include "way_tag_updater.h"


/*namespace
{
  Uint31_Index idx_for_tags(Uint31_Index arg)
  {
    return { arg.val() & 0x7fffff00 };
  }


  std::vector< Way_Tag_Updater::KV_Tag > make_tags(const Data_By_Id< Way_Skeleton >::Entry::Tag_Container& tags)
  {
    std::vector< Way_Tag_Updater::KV_Tag > result;
    for (const auto& i : tags)
      result.push_back({ i.first, i.second });
    return result;
  }
}*/


const std::string& Way_Tag_Updater::invalid_value()
{
  static std::string val = "\xff";
  return val;
}


/*void Way_Tag_Updater::eval_tags(
    const std::vector< Attic< Way_Skeleton::Id_Type > >& unchanged_before,
    const std::vector< Way_Event >& proto_events,
    const std::vector< Id_Timestamp_Tag >& tags_by_id_timestamp,
    Uint31_Index target_idx,
    std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id,
    std::map< Tag_Index_Local, std::vector< Way_Skeleton::Id_Type > >& existing_current,
    std::map< Tag_Index_Local, std::vector< Attic< Way_Skeleton::Id_Type > > >& existing_attic)
{
  auto i_unchanged_before = unchanged_before.begin();
  auto i_events = proto_events.begin();

  Tagdata_By_Idx_Id& tags_by_id_target = tags_by_id[idx_for_tags(target_idx)];
  Tags_Per_Id_Onetime onetime{ 0u, NOW, {} };
  std::vector< Tags_Per_Id_Onetime > timeline;
  std::vector< Tags_Per_Id_Onetime > last_timeline;
  auto i_last_timeline = last_timeline.begin();

  for (const auto& i : tags_by_id_timestamp)
  {
    if (onetime.ref.val() && onetime.ref < i.ref)
    {
      if (!onetime.tags.empty() && onetime.tags.back().value == invalid_value())
        onetime.tags.pop_back();
      tags_by_id_target.tags_at_last_unchanged.push_back(onetime);
      onetime.ref = 0u;
    }

    if (!timeline.empty() && timeline.front().ref < i.ref)
    {
      while (i_last_timeline != last_timeline.end())
      {
        timeline.push_back({ i.ref, i_last_timeline->before, std::move(i_last_timeline->tags) });
        ++i_last_timeline;
      }
      last_timeline.clear();

      //TODO

      timeline.clear();
    }

    while (i_unchanged_before != unchanged_before.end() && Way_Skeleton::Id_Type(*i_unchanged_before) < i.ref)
      ++i_unchanged_before;
    while (i_events != proto_events.end() && i_events->meta.ref < i.ref)
      ++i_events;

    bool relevant = false;

    if (i_unchanged_before != unchanged_before.end() && Way_Skeleton::Id_Type(*i_unchanged_before) == i.ref
        && i_unchanged_before->timestamp <= i.before)
    {
      relevant = true;
      onetime.ref = i.ref;
      onetime.before = i.before;
      if (onetime.tags.empty())
        onetime.tags.push_back({ i.tag->key, i.tag->value });
      else if (onetime.tags.back().key != i.tag->key)
      {
        if (onetime.tags.back().value == invalid_value())
          onetime.tags.pop_back();
        onetime.tags.push_back({ i.tag->key, i.tag->value });
      }
    }

    if (i_events != proto_events.end() && i_events->meta.ref == i.ref && i_events->not_before < i.before)
    {
      if (!timeline.empty() && timeline.back().tags.back().key != i.tag->key)
      {
        while (i_last_timeline != last_timeline.end())
        {
          timeline.push_back({ i.ref, i_last_timeline->before, std::move(i_last_timeline->tags) });
          ++i_last_timeline;
        }

        last_timeline.swap(timeline);
        i_last_timeline = last_timeline.begin();
        timeline.clear();
      }

      while (i_last_timeline != last_timeline.end() && i_last_timeline->before <= i.before)
      {
        timeline.push_back({ i.ref, i_last_timeline->before, std::move(i_last_timeline->tags) });
        ++i_last_timeline;
      }
      if (timeline.back().before < i.before)
        timeline.push_back({ i.ref, i.before,
            i_last_timeline == last_timeline.end() ? std::vector< KV_Tag >() : i_last_timeline->tags });
      timeline.back().tags.push_back({ i.tag->key, i.tag->value });

      relevant = true;
    }

    if (relevant)
    {
      if (i.before == NOW)
        existing_current[*i.tag].push_back(i.ref);
      else
        existing_attic[*i.tag].push_back({ i.ref, i.before });
    }
  }
}



void Way_Tag_Updater::merge_values(
    const std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >& changes_per_idx,
    std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id)
{
  for (const auto& i : changes_per_idx)
  {
    auto& sink = tags_by_id[idx_for_tags(i.first)].new_tags;
    for (const auto& j : i.second)
      sink.push_back(Tags_Per_Id_Timespan{ j.skel.id, j.not_before, j.before, make_tags(j.tags) });
  }

  // NB: Do not sort on write because we have multiple writes but only one read
//   for (auto& i : tags_by_id)
//     std::sort(i.second.new_tags.begin(), i.second.new_tags.end());
}


namespace
{
  struct Single_Tag
  {
    std::string key;
    std::string value;
    Way_Skeleton::Id_Type ref;
  };


  bool operator<(const Single_Tag& lhs, const Single_Tag& rhs)
  {
    return lhs.key < rhs.key || (lhs.key == rhs.key &&
        (lhs.value < rhs.value || (lhs.value == rhs.value &&
        lhs.ref < rhs.ref)));
  }


  struct KV_Ordered_Delta
  {
    void process_unmatched_unchanged(const Way_Tag_Updater::Tags_Per_Id_Onetime& unchanged_tags);
    void compare_first_version(
        const Way_Tag_Updater::Tags_Per_Id_Onetime* unchanged_tags, const Way_Tag_Updater::Tags_Per_Id_Timespan& next);
    void compare_versions(
        const Way_Tag_Updater::Tags_Per_Id_Timespan& prev, const Way_Tag_Updater::Tags_Per_Id_Timespan& next);
    void process_last_version(const Way_Tag_Updater::Tags_Per_Id_Timespan& arg);

    std::vector< Attic< Single_Tag > > new_attic_entries;
    std::vector< Single_Tag > new_current_entries;
  };
}


void KV_Ordered_Delta::compare_first_version(
    const Way_Tag_Updater::Tags_Per_Id_Onetime* unchanged_tags, const Way_Tag_Updater::Tags_Per_Id_Timespan& next)
{
  if (!unchanged_tags)
    return;

  auto i_prev = unchanged_tags->tags.begin();
  for (const auto& i_next : next.tags)
  {
    while (i_prev != unchanged_tags->tags.end() && i_prev->key < i_next.key)
    {
      new_attic_entries.push_back({ { i_prev->key, i_prev->value, next.ref }, unchanged_tags->before });
      ++i_prev;
    }
    if (i_prev == unchanged_tags->tags.end() || i_next.key < i_prev->key)
      new_attic_entries.push_back({ { i_next.key, Way_Tag_Updater::invalid_value(), next.ref }, unchanged_tags->before });
    else
    {
      if (i_prev->value != i_next.value)
        new_attic_entries.push_back({ { i_next.key, i_prev->value, next.ref }, unchanged_tags->before });
      ++i_prev;
    }
  }
  while (i_prev != unchanged_tags->tags.end())
  {
    new_attic_entries.push_back({ { i_prev->key, i_prev->value, next.ref }, unchanged_tags->before });
    ++i_prev;
  }
}


void KV_Ordered_Delta::process_unmatched_unchanged(
    const Way_Tag_Updater::Tags_Per_Id_Onetime& unchanged_tags)
{
  for (const auto& i : unchanged_tags.tags)
    new_attic_entries.push_back({ { i.key, i.value, unchanged_tags.ref }, unchanged_tags.before });
}


void KV_Ordered_Delta::compare_versions(
    const Way_Tag_Updater::Tags_Per_Id_Timespan& prev, const Way_Tag_Updater::Tags_Per_Id_Timespan& next)
{
  auto i_prev = prev.tags.begin();
  for (const auto& i_next : next.tags)
  {
    while (i_prev != prev.tags.end() && i_prev->key < i_next.key)
    {
      new_attic_entries.push_back({ { i_prev->key, i_prev->value, prev.ref }, prev.before });
      ++i_prev;
    }
    if (i_prev == prev.tags.end() || i_next.key < i_prev->key)
      new_attic_entries.push_back({ { i_next.key, Way_Tag_Updater::invalid_value(), prev.ref }, prev.before });
    else
    {
      if (i_prev->value != i_next.value)
        new_attic_entries.push_back({ { i_next.key, i_prev->value, prev.ref }, prev.before });
      ++i_prev;
    }
  }
  while (i_prev != prev.tags.end())
  {
    new_attic_entries.push_back({ { i_prev->key, i_prev->value, prev.ref }, prev.before });
    ++i_prev;
  }
}


void KV_Ordered_Delta::process_last_version(const Way_Tag_Updater::Tags_Per_Id_Timespan& arg)
{
  if (arg.before == NOW)
  {
    for (const auto& i : arg.tags)
      new_current_entries.push_back({ i.key, i.value, arg.ref });
  }
  else
  {
    for (const auto& i : arg.tags)
      new_attic_entries.push_back({ { i.key, i.value, arg.ref }, arg.before });
  }
}*/


Way_Tag_Updater::Way_Tag_Delta::Per_Key_Collector::Per_Key_Collector(
    Way_Tag_Delta& parent_, Uint31_Index idx_, const Value_Timeline_Per_Id_Key& existing_)
    : parent(parent_), idx(idx_), existing(existing_), i_existing(existing.timeline.begin()) {}


void Way_Tag_Updater::Way_Tag_Delta::Per_Key_Collector::set(
    uint64_t not_before, uint64_t before, const std::string& value)
{
  if (!to_add.empty() && to_add.back().before == not_before && to_add.back().value == value)
    to_add.pop_back();
  else
  {
    while (i_existing != existing.timeline.end() && i_existing->before < not_before)
      ++i_existing;
    if (i_existing != existing.timeline.end())
    {
      if (i_existing->before == not_before)
      {
        if (i_existing->value == value)
          to_delete.push_back(*i_existing);
        ++i_existing;
      }
      else if (i_existing->value != value)
        to_add.push_back({ not_before, i_existing->value });
    }
  }

  while (i_existing != existing.timeline.end() && i_existing->before < before)
  {
    to_delete.push_back(*i_existing);
    ++i_existing;
  }

  if (i_existing != existing.timeline.end() && i_existing->before == before)
  {
    if (i_existing->value != value)
    {
      to_delete.push_back(*i_existing);
      to_add.push_back({ before, value });
      ++i_existing;
    }
  }
  else
    to_add.push_back({ before, value });
}


Way_Tag_Updater::Way_Tag_Delta::Per_Key_Collector::~Per_Key_Collector()
{
  if (i_existing == existing.timeline.end() && !to_add.empty() && to_add.back().value == invalid_value())
    // Save space in the database - the key is not yet set later on
    to_add.pop_back();

  for (const auto& i : to_delete)
  {
    if (i.before == NOW)
      parent.current_to_delete[Tag_Index_Local(idx, existing.key, i.value)].insert(existing.id);
    else
      parent.attic_to_delete[Tag_Index_Local(idx, existing.key, i.value)].insert({ existing.id, i.before });
  }
  for (const auto& i : to_add)
  {
    if (i.before == NOW)
      parent.current_to_add[Tag_Index_Local(idx, existing.key, i.value)].insert(existing.id);
    else
      parent.attic_to_add[Tag_Index_Local(idx, existing.key, i.value)].insert({ existing.id, i.before });
  }
}


void Way_Tag_Updater::Way_Tag_Delta::process_key(
    Uint31_Index idx,
    const Value_Timeline_Per_Id_Key& existing,
    const std::vector< Timespan >& active,
    const Value_Timeline_Per_Key& to_apply)
{
  Per_Key_Collector collector(*this, idx, existing);
  auto i_to_apply = to_apply.timeline.begin();
  for (const auto& i : active)
  {
    while (i_to_apply != to_apply.timeline.end() && i_to_apply->before <= i.not_before)
      ++i_to_apply;

    uint64_t last_before = i.not_before;
    while (i_to_apply != to_apply.timeline.end() && i_to_apply->before <= i.before)
    {
      uint64_t not_before = std::max(i_to_apply->not_before, i.not_before);
      if (last_before < not_before)
        collector.set(last_before, not_before, invalid_value());
      collector.set(not_before, i_to_apply->before, i_to_apply->value);
      last_before = i_to_apply->before;
      ++i_to_apply;
    }

    if (i_to_apply != to_apply.timeline.end() && i_to_apply->not_before < i.before)
    {
      uint64_t not_before = std::max(i_to_apply->not_before, i.not_before);
      if (last_before < not_before)
        collector.set(last_before, not_before, invalid_value());
      collector.set(not_before, i.before, i_to_apply->value);
    }
    else if (last_before < i.before)
      collector.set(last_before, i.before, invalid_value());
  }
}


void Way_Tag_Updater::Way_Tag_Delta::process_idx(
    Uint31_Index idx,
    const std::vector< Value_Timeline_Per_Id_Key >& existing,
    const std::vector< Skel_KV_Timeline_Per_Id >& to_apply)
{
  auto i_existing = existing.begin();
  for (const auto& i : to_apply)
  {
    while (i_existing != existing.end() && i_existing->id < i.id)
      ++i_existing;

    auto i_key = i.keys.begin();

    while (i_existing != existing.end() && i_existing->id == i.id)
    {
      while (i_key != i.keys.end() && i_key->key < i_existing->key)
      {
        process_key(idx, { i.id, i_key->key, {} }, i.active, *i_key);
        ++i_key;
      }

      if (i_key != i.keys.end() && i_key->key == i_existing->key)
      {
        process_key(idx, *i_existing, i.active, *i_key);
        ++i_key;
      }
      else
        process_key(idx, *i_existing, i.active, {});

      ++i_existing;
    }

    while (i_key != i.keys.end())
    {
      process_key(idx, { i.id, i_key->key, {} }, i.active, *i_key);
      ++i_key;
    }
  }
}


Way_Tag_Updater::Way_Tag_Delta::Way_Tag_Delta(
    const std::map< Uint31_Index, std::vector< Value_Timeline_Per_Id_Key > >& existing,
    const std::map< Uint31_Index, std::vector< Skel_KV_Timeline_Per_Id > >& to_apply)
{
  auto i_existing = existing.begin();
  for (const auto& i : to_apply)
  {
    while (i_existing != existing.end() && i_existing->first < i.first)
    {
      process_idx(i_existing->first, i_existing->second, {});
      ++i_existing;
    }
    if (i_existing == existing.end() || i.first < i_existing->first)
      process_idx(i.first, {}, i.second);
    else
    {
      process_idx(i.first, i_existing->second, i.second);
      ++i_existing;
    }
  }
  while (i_existing != existing.end())
  {
    process_idx(i_existing->first, i_existing->second, {});
    ++i_existing;
  }
}
