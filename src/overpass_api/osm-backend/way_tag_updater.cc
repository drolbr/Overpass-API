#include "way_tag_updater.h"


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
}


const std::string& Way_Tag_Updater::invalid_value()
{
  static std::string val = "\xff";
  return val;
}


Way_Tag_Updater::Way_Tag_Delta::Way_Tag_Delta(
    const std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id,
    const std::map< Tag_Index_Local, std::vector< Way_Skeleton::Id_Type > >& existing_current,
    const std::map< Tag_Index_Local, std::vector< Attic< Way_Skeleton::Id_Type > > >& existing_attic)
{
  auto it_existing_current = existing_current.begin();
  auto it_existing_attic = existing_attic.begin();

  for (const auto& i_tags_by_id : tags_by_id)
  {
    KV_Ordered_Delta kv_delta;

    Uint31_Index idx = i_tags_by_id.first;
    const auto& new_tags_per_idx = i_tags_by_id.second.new_tags;

    const auto* unchanged_per_idx = &i_tags_by_id.second.tags_at_last_unchanged;
    auto i_unchanged_per_idx = unchanged_per_idx->begin();

    for (auto it_new_tags = new_tags_per_idx.begin(); it_new_tags != new_tags_per_idx.end(); ++it_new_tags)
    {
      const Way_Tag_Updater::Tags_Per_Id_Onetime* unchanged_tags = 0;
      while (i_unchanged_per_idx != unchanged_per_idx->end() && i_unchanged_per_idx->ref < it_new_tags->ref)
      {
        kv_delta.process_unmatched_unchanged(*i_unchanged_per_idx);
        ++i_unchanged_per_idx;
      }
      if (i_unchanged_per_idx != unchanged_per_idx->end() && i_unchanged_per_idx->ref == it_new_tags->ref)
        unchanged_tags = &*i_unchanged_per_idx;

      if (it_new_tags == new_tags_per_idx.begin())
        kv_delta.compare_first_version(unchanged_tags, *it_new_tags);
      else if (!((it_new_tags-1)->ref == it_new_tags->ref))
      {
        kv_delta.process_last_version(*(it_new_tags-1));
        kv_delta.compare_first_version(unchanged_tags, *it_new_tags);
      }
      else
        kv_delta.compare_versions(*(it_new_tags-1), *it_new_tags);

      if (unchanged_tags)
        ++i_unchanged_per_idx;
    }
    if (!new_tags_per_idx.empty())
      kv_delta.process_last_version(new_tags_per_idx.back());
    while (i_unchanged_per_idx != unchanged_per_idx->end())
    {
      kv_delta.process_unmatched_unchanged(*i_unchanged_per_idx);
      ++i_unchanged_per_idx;
    }

    while (it_existing_current != existing_current.end() && it_existing_current->first.index < idx.val())
    {
      current_to_delete[it_existing_current->first].insert(
          it_existing_current->second.begin(), it_existing_current->second.end());
      ++it_existing_current;
    }
    while (it_existing_attic != existing_attic.end() && it_existing_attic->first.index < idx.val())
    {
      attic_to_delete[it_existing_attic->first].insert(
          it_existing_attic->second.begin(), it_existing_attic->second.end());
      ++it_existing_attic;
    }

    std::sort(kv_delta.new_attic_entries.begin(), kv_delta.new_attic_entries.end());
    std::sort(kv_delta.new_current_entries.begin(), kv_delta.new_current_entries.end());

    auto i_new_current = kv_delta.new_current_entries.begin();
    while (it_existing_current != existing_current.end() && it_existing_current->first.index == idx.val())
    {
      while (i_new_current != kv_delta.new_current_entries.end() && i_new_current->key < it_existing_current->first.key)
      {
        current_to_add[Tag_Index_Local{ idx, i_new_current->key, i_new_current->value }].insert(i_new_current->ref);
        ++i_new_current;
      }
      while (i_new_current != kv_delta.new_current_entries.end() && i_new_current->key == it_existing_current->first.key
          && i_new_current->value < it_existing_current->first.value)
      {
        current_to_add[Tag_Index_Local{ idx, i_new_current->key, i_new_current->value }].insert(i_new_current->ref);
        ++i_new_current;
      }

      if (i_new_current != kv_delta.new_current_entries.end() && i_new_current->key == it_existing_current->first.key
          && i_new_current->value == it_existing_current->first.value)
      {
        for (auto i : it_existing_current->second)
        {
          while (i_new_current != kv_delta.new_current_entries.end()
              && i_new_current->key == it_existing_current->first.key
              && i_new_current->value == it_existing_current->first.value && i_new_current->ref < i)
          {
            current_to_add[Tag_Index_Local{ idx, i_new_current->key, i_new_current->value }]
                .insert(i_new_current->ref);
            ++i_new_current;
          }

          if (i_new_current != kv_delta.new_current_entries.end() && i_new_current->key == it_existing_current->first.key
              && i_new_current->value == it_existing_current->first.value && i_new_current->ref == i)
            ++i_new_current;
          else
            current_to_delete[it_existing_current->first].insert(i);
        }
      }
      else
        current_to_delete[it_existing_current->first].insert(
            it_existing_current->second.begin(), it_existing_current->second.end());
      ++it_existing_current;
    }
    while (i_new_current != kv_delta.new_current_entries.end())
    {
      current_to_add[Tag_Index_Local{ idx, i_new_current->key, i_new_current->value }].insert(i_new_current->ref);
      ++i_new_current;
    }

    auto i_new_attic = kv_delta.new_attic_entries.begin();
    while (it_existing_attic != existing_attic.end() && it_existing_attic->first.index == idx.val())
    {
      while (i_new_attic != kv_delta.new_attic_entries.end() && i_new_attic->key < it_existing_attic->first.key)
      {
        attic_to_add[Tag_Index_Local{ idx, i_new_attic->key, i_new_attic->value }]
            .insert({ i_new_attic->ref, i_new_attic->timestamp });
        ++i_new_attic;
      }
      while (i_new_attic != kv_delta.new_attic_entries.end() && i_new_attic->key == it_existing_attic->first.key
          && i_new_attic->value < it_existing_attic->first.value)
      {
        attic_to_add[Tag_Index_Local{ idx, i_new_attic->key, i_new_attic->value }]
            .insert({ i_new_attic->ref, i_new_attic->timestamp });
        ++i_new_attic;
      }

      if (i_new_attic != kv_delta.new_attic_entries.end() && i_new_attic->key == it_existing_attic->first.key
          && i_new_attic->value == it_existing_attic->first.value)
      {
        for (auto i : it_existing_attic->second)
        {
          while (i_new_attic != kv_delta.new_attic_entries.end() && i_new_attic->key == it_existing_attic->first.key
              && i_new_attic->value == it_existing_attic->first.value && i_new_attic->ref < i)
          {
            attic_to_add[Tag_Index_Local{ idx, i_new_attic->key, i_new_attic->value }]
                .insert({ i_new_attic->ref, i_new_attic->timestamp });
            ++i_new_attic;
          }
          while (i_new_attic != kv_delta.new_attic_entries.end() && i_new_attic->key == it_existing_attic->first.key
              && i_new_attic->value == it_existing_attic->first.value && i_new_attic->ref == i
              && i_new_attic->timestamp < i.timestamp)
          {
            attic_to_add[Tag_Index_Local{ idx, i_new_attic->key, i_new_attic->value }]
                .insert({ i_new_attic->ref, i_new_attic->timestamp });
            ++i_new_attic;
          }

          if (i_new_attic != kv_delta.new_attic_entries.end() && i_new_attic->key == it_existing_attic->first.key
              && i_new_attic->value == it_existing_attic->first.value && i_new_attic->ref == i
              && i_new_attic->timestamp == i.timestamp)
            ++i_new_attic;
          else
            attic_to_delete[it_existing_attic->first].insert(i);
        }
      }
      else
        attic_to_delete[it_existing_attic->first].insert(
            it_existing_attic->second.begin(), it_existing_attic->second.end());
      ++it_existing_attic;
    }
    while (i_new_attic != kv_delta.new_attic_entries.end())
    {
      attic_to_add[Tag_Index_Local{ idx, i_new_attic->key, i_new_attic->value }]
          .insert({ i_new_attic->ref, i_new_attic->timestamp });
      ++i_new_attic;
    }
  }

  while (it_existing_current != existing_current.end())
  {
    current_to_delete[it_existing_current->first].insert(
        it_existing_current->second.begin(), it_existing_current->second.end());
    ++it_existing_current;
  }
  while (it_existing_attic != existing_attic.end())
  {
    attic_to_delete[it_existing_attic->first].insert(
        it_existing_attic->second.begin(), it_existing_attic->second.end());
    ++it_existing_attic;
  }
}
