#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_TAG_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_TAG_UPDATER_H


#include "data_from_osc.h"
#include "way_skeleton_updater.h"

#include <map>
#include <vector>


namespace Way_Tag_Updater
{
/*  struct KV_Tag
  {
    std::string key;
    std::string value;

    bool operator==(const KV_Tag& rhs) const
    { return key == rhs.key && value == rhs.value; }
  };


  struct Tags_Per_Id_Onetime
  {
    Way_Skeleton::Id_Type ref;
    uint64_t before;
    std::vector< KV_Tag > tags;
  };


  struct Tags_Per_Id_Timespan
  {
    Way_Skeleton::Id_Type ref;
    uint64_t not_before;
    uint64_t before;
    std::vector< KV_Tag > tags;

    bool operator<(const Tags_Per_Id_Timespan& rhs)
    {
      return ref < rhs.ref || (ref == rhs.ref && before < rhs.before);
    }
  };


  struct Tagdata_By_Idx_Id
  {
    std::vector< Tags_Per_Id_Onetime > tags_at_last_unchanged;
    std::vector< Tags_Per_Id_Timespan > new_tags;
  };


  struct Id_Timestamp_Tag
  {
    Way_Skeleton::Id_Type ref;
    uint64_t before;
    const Tag_Index_Local* tag;
  };


  void eval_tags(
      const std::vector< Attic< Way_Skeleton::Id_Type > >& unchanged_before,
      const std::vector< Way_Event >& proto_events,
      const std::vector< Id_Timestamp_Tag >& tags_by_id_timestamp,
      Uint31_Index target_idx,
      std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id,
      std::map< Tag_Index_Local, std::vector< Way_Skeleton::Id_Type > >& existing_current,
      std::map< Tag_Index_Local, std::vector< Attic< Way_Skeleton::Id_Type > > >& existing_attic);


  void merge_values(
      const std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >& changes_per_idx,
      std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id);*/


  struct Timespan
  {
    uint64_t not_before;
    uint64_t before;
  };


  struct Value_Onetime
  {
    uint64_t before;
    std::string value;
  };


  struct Value_Timespan
  {
    uint64_t not_before;
    uint64_t before;
    std::string value;
  };


  struct Value_Timeline_Per_Id_Key
  {
    Way_Skeleton::Id_Type id;
    std::string key;
    std::vector< Value_Onetime > timeline;
  };


  struct Value_Timeline_Per_Key
  {
    std::string key;
    std::vector< Value_Timespan > timeline;
  };


  struct Skel_KV_Timeline_Per_Id
  {
    Way_Skeleton::Id_Type id;
    std::vector< Timespan > active;
    std::vector< Value_Timeline_Per_Key > keys;
  };


  struct Way_Tag_Delta
  {
    Way_Tag_Delta(
        const std::map< Uint31_Index, std::vector< Value_Timeline_Per_Id_Key > >& existing,
        const std::map< Uint31_Index, std::vector< Skel_KV_Timeline_Per_Id > >& to_apply);
    /* Assertions:
     * ...
     */

    std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > current_to_add;
    std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > current_to_delete;
    std::map< Tag_Index_Local, std::set< Attic< Way_Skeleton::Id_Type > > > attic_to_add;
    std::map< Tag_Index_Local, std::set< Attic< Way_Skeleton::Id_Type > > > attic_to_delete;

  private:
    struct Per_Key_Collector
    {
      Per_Key_Collector(Way_Tag_Delta& parent, Uint31_Index idx, const Value_Timeline_Per_Id_Key& existing);
      void set(uint64_t not_before, uint64_t before, const std::string& value);
      ~Per_Key_Collector();

    private:
      Way_Tag_Delta& parent;
      Uint31_Index idx;
      const Value_Timeline_Per_Id_Key& existing;
      std::vector< Value_Onetime >::const_iterator i_existing;
      std::vector< Value_Onetime > to_delete;
      std::vector< Value_Onetime > to_add;
    };

    void process_idx(
        Uint31_Index idx,
        const std::vector< Value_Timeline_Per_Id_Key >& existing,
        const std::vector< Skel_KV_Timeline_Per_Id >& to_apply);
    void process_key(
        Uint31_Index idx,
        const Value_Timeline_Per_Id_Key& existing,
        const std::vector< Timespan >& active,
        const Value_Timeline_Per_Key& to_apply);
  };


  const std::string& invalid_value();
}


#endif
