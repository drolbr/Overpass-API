#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_TAG_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__WAY_TAG_UPDATER_H


#include "data_from_osc.h"
#include "way_skeleton_updater.h"

#include <map>
#include <vector>


namespace Way_Tag_Updater
{
  struct KV_Tag
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


  void merge_values(
      const std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >& changes_per_idx,
      std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id);


  struct Way_Tag_Delta
  {
    Way_Tag_Delta(
        const std::map< Uint31_Index, Tagdata_By_Idx_Id >& tags_by_id,
        const std::map< Tag_Index_Local, std::vector< Way_Skeleton::Id_Type > >& existing_current,
        const std::map< Tag_Index_Local, std::vector< Attic< Way_Skeleton::Id_Type > > >& existing_attic);
    /* Assertions:
     * ...
     */

    std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > current_to_add;
    std::map< Tag_Index_Local, std::set< Way_Skeleton::Id_Type > > current_to_delete;
    std::map< Tag_Index_Local, std::set< Attic< Way_Skeleton::Id_Type > > > attic_to_add;
    std::map< Tag_Index_Local, std::set< Attic< Way_Skeleton::Id_Type > > > attic_to_delete;
  };


  const std::string& invalid_value();
}


#endif
