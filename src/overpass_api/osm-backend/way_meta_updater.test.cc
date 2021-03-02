#include "way_meta_updater.h"
#include "test_tools.h"


template< typename Object >
std::vector< const Object* > refs_of(const std::vector< Object >& arg)
{
  std::vector< const Object* > result;
  for (decltype(arg.size()) i = 0; i < arg.size(); ++i)
    result.push_back(&arg[i]);
  return result;
}


bool operator==(const Way_Implicit_Pre_Event& lhs, const Way_Implicit_Pre_Event& rhs)
{
  return lhs.base.id == rhs.base.id && lhs.base.nds == rhs.base.nds && lhs.base.geometry == rhs.base.geometry
      && lhs.not_before == rhs.not_before && lhs.before == rhs.before && lhs.pos_events == rhs.pos_events;
}


bool operator==(const Proto_Way& lhs, const Proto_Way& rhs)
{
  return lhs.base.id == rhs.base.id && lhs.base.nds == rhs.base.nds && lhs.base.geometry == rhs.base.geometry
      && lhs.meta == rhs.meta
      && lhs.not_before == rhs.not_before && lhs.before == rhs.before && lhs.pos_events == rhs.pos_events;
}


OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > make_way_meta(
    Way_Skeleton::Id_Type id, uint32 version, uint64 timestamp, uint32 changeset, uint32 user_id)
{
    OSM_Element_Metadata meta;
    meta.version = version;
    meta.timestamp = timestamp;
    meta.changeset = changeset;
    meta.user_id = user_id;
    return OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >{ id, meta };
}


Way_Event make_way_event(
    Way_Skeleton::Id_Type id, uint64_t not_before, uint64_t before,
    uint32 version, uint64 timestamp, uint32 changeset, uint32 user_id)
{
    return Way_Event{
        Way_Skeleton{ id },
        make_way_meta(id, version, timestamp, changeset, user_id),
        not_before, before };
}


void test_prune_first_skeletons()
{
  {
    std::cerr<<"\nTest whether a single current meta has empty result:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    Way_Meta_Updater::prune_first_skeletons(
        { make_way_meta(496u, 1, 1000, 8128, 28) }, {}, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest whether a single attic meta has empty result:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    Way_Meta_Updater::prune_first_skeletons(
        {}, { make_way_meta(496u, 1, 1000, 8128, 28) }, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest whether a single implicit_pre_event stays unchanged:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events = {
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } };
    Way_Meta_Updater::prune_first_skeletons(
        {}, {}, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest whether a single implicit_pre_event with not_before set stays unchanged:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events = {
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } };
    Way_Meta_Updater::prune_first_skeletons(
        { make_way_meta(496u, 2, 3000, 8128, 28) }, { make_way_meta(496u, 1, 2000, 8128, 28) }, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest whether a single implicit_pre_event without not_before set is changed:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events = {
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 0, NOW, {} } };
    Way_Meta_Updater::prune_first_skeletons(
        { make_way_meta(496u, 2, 2000, 8128, 28) }, { make_way_meta(496u, 1, 1000, 8128, 28) }, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest whether a single implicit_pre_event without not_before set is changed based on current:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events = {
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 0, NOW, {} } };
    Way_Meta_Updater::prune_first_skeletons(
        { make_way_meta(496u, 1, 1000, 8128, 28) }, {}, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest multiple implicit_pre_events for a single id:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events = {
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 0, 2000, {} },
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } };
    Way_Meta_Updater::prune_first_skeletons(
        { make_way_meta(496u, 2, 2000, 8128, 28) }, { make_way_meta(496u, 1, 1000, 8128, 28) }, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} })
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest multiple implicit_pre_events for multiple ids:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events = {
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(492u), 0, NOW, {} },
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(494u), 0, NOW, {} },
        Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 0, NOW, {} } };
    Way_Meta_Updater::prune_first_skeletons(
        { make_way_meta(493u, 2, 2000, 8128, 28),
          make_way_meta(494u, 1, 3000, 8128, 28),
          make_way_meta(496u, 2, 2000, 8128, 28) },
        { make_way_meta(493u, 1, 1000, 8128, 28),
          make_way_meta(495u, 1, 1000, 8128, 28),
          make_way_meta(496u, 1, 1000, 8128, 28) }, implicit_pre_events);

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(492u), 0, NOW, {} })
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(494u), 3000, NOW, {} })
        (Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} })
        (implicit_pre_events);
  }
}


void test_assign_meta()
{
  {
    std::cerr<<"\nTest whether a single matching current is assigned:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single matching attic is assigned:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single earlier current is assigned:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single earlier attic is assigned:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, 3000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 2000, 3000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single later current is ignored:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 1, 2000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            {}, 1000, 2000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single later attic is ignored:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 2000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            {}, 1000, 2000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single in-between current splits the timeline:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 1, 2000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            {}, 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a single in-between attic splits the timeline:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 2000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 3000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            {}, 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 2000, 8128, 28), 2000, 3000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether the attic of attic plus current is matched:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 2, 2000, 8128, 28) },
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether the current of attic plus current is matched:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 2, 2000, 8128, 28) },
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 2, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether the former of attic plus attic is matched:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28),
          make_way_meta(496u, 2, 2000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether the latter of attic plus attic is matched:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28),
          make_way_meta(496u, 2, 2000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 2, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether the current of attic plus current splits the timeline:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 2, 2000, 8128, 28) },
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 2, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether the latter of attic plus attic splits the timeline:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28),
          make_way_meta(496u, 2, 2000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 2, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether an attic matches two consecutive entries:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether an attic matches two entries with a gap:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 3000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 3000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a current matches two consecutive entries:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether a current matches two entries with a gap:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 3000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 3000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether two attics match two consecutive entries:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(496u, 1, 1000, 8128, 28),
          make_way_meta(496u, 2, 2000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 2, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest whether attics plus current matches two consecutive entries:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(496u, 2, 2000, 8128, 28) },
        { make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, 2000, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 2000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, 2000, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 2, 2000, 8128, 28), 2000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest currents for multiple ids:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        { make_way_meta(493u, 1, 1000, 8128, 28),
          make_way_meta(494u, 1, 1000, 8128, 28),
          make_way_meta(495u, 1, 1000, 8128, 28),
          make_way_meta(496u, 1, 1000, 8128, 28) },
        {},
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(493u), 1000, NOW, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(495u), 1000, NOW, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(493u),
            make_way_meta(493u, 1, 1000, 8128, 28), 1000, NOW, {} })
        ({ Way_Skeleton::Id_Type(495u),
            make_way_meta(495u, 1, 1000, 8128, 28), 1000, NOW, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, NOW, {} })
        (proto_ways);
  }
  {
    std::cerr<<"\nTest attics for multiple ids:\n";

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta(
        {},
        { make_way_meta(493u, 1, 1000, 8128, 28),
          make_way_meta(494u, 1, 1000, 8128, 28),
          make_way_meta(495u, 1, 1000, 8128, 28),
          make_way_meta(496u, 1, 1000, 8128, 28) },
        { Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(493u), 1000, NOW, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(495u), 1000, NOW, {} },
          Way_Implicit_Pre_Event{ Way_Skeleton::Id_Type(496u), 1000, NOW, {} } });

    bool all_ok = true;
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        ({ Way_Skeleton::Id_Type(493u),
            make_way_meta(493u, 1, 1000, 8128, 28), 1000, NOW, {} })
        ({ Way_Skeleton::Id_Type(495u),
            make_way_meta(495u, 1, 1000, 8128, 28), 1000, NOW, {} })
        ({ Way_Skeleton::Id_Type(496u),
            make_way_meta(496u, 1, 1000, 8128, 28), 1000, NOW, {} })
        (proto_ways);
  }
}


void test_way_meta_delta()
{
  {
    std::cerr<<"\nWay_Meta_Delta: Test one event:\n";

    Way_Event event = make_way_event(496, 1000, NOW, 2, 1000, 8128, 28);
    Way_Meta_Updater::Way_Meta_Delta delta{
        { event },
        {}, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (event.meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test consecutive events with equal meta:\n";

    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 2, 1000, 8128, 28),
        make_way_event(496, 2000, NOW, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[0].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test events with gap with equal meta:\n";

    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28),
        make_way_event(496, 4000, NOW, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[0].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test consecutive events with different meta:\n";

    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28),
        make_way_event(496, 3000, NOW, 2, 3000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[1].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test events with gap with different meta:\n";

    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28),
        make_way_event(496, 4000, NOW, 2, 4000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[1].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test events for multiple ids:\n";

    std::vector< Way_Event > events = {
        make_way_event(494, 1040, NOW, 4, 1004, 8126, 26),
        make_way_event(495, 1050, NOW, 5, 1005, 8127, 27),
        make_way_event(496, 1060, NOW, 6, 1006, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[0].meta)
        (events[1].meta)
        (events[2].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test whether a single existing current is deleted:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    Way_Meta_Updater::Way_Meta_Delta delta{
        {},
        { meta },
        {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (meta)
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test whether a single existing attic is deleted:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    Way_Meta_Updater::Way_Meta_Delta delta{
        {},
        {},
        { meta },
        {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (meta)
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test whether a single existing current with reuse flag is kept:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    Way_Meta_Updater::Way_Meta_Delta delta{
        {},
        { meta },
        {},
        {}, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test whether a single existing attic with reuse flag is kept:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    Way_Meta_Updater::Way_Meta_Delta delta{
        {},
        {},
        { meta },
        {}, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Test whether existing metas with non-matching reuse flags are deleted:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > attic_meta = make_way_meta(496, 2, 1006, 8128, 28);
    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > current_meta = make_way_meta(495, 2, 1005, 8128, 28);
    Way_Meta_Updater::Way_Meta_Delta delta{
        {},
        { current_meta },
        { attic_meta },
        {}, { Attic< Way_Skeleton::Id_Type >{ 495u, 1005 }, Attic< Way_Skeleton::Id_Type >{ 496u, 1006 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (current_meta)
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (attic_meta)
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing current with a current event:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, NOW, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        { meta },
        {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Let a single existing current miss the current event:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, NOW, 2, 2000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        { meta },
        {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (meta)
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[0].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing current with an attic event:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        { meta },
        {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (meta)
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Let a single existing current miss the attic event:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 2000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        { meta },
        {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (meta)
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Mix existing current metas and current events for multiple ids:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > metas =
        { make_way_meta(492, 2, 1002, 8128, 28),
          make_way_meta(494, 4, 1004, 8128, 28),
          make_way_meta(495, 5, 1005, 8128, 28),
          make_way_meta(496, 6, 1006, 8128, 28),
          make_way_meta(497, 7, 1007, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(493, 1997, NOW, 3, 1003, 8128, 28),
        make_way_event(496, 1994, NOW, 6, 1006, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events, metas, {}, {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (metas[0])
        (metas[1])
        (metas[2])
        (metas[4])
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[0].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing attic with an attic event:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {},
        { meta },
        {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Let a single existing attic miss the attic event:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 2000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {},
        { meta },
        {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (meta)
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Mix existing current metas and current events for multiple ids:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > metas =
        { make_way_meta(492, 2, 1002, 8128, 28),
          make_way_meta(494, 4, 1004, 8128, 28),
          make_way_meta(495, 5, 1005, 8128, 28),
          make_way_meta(496, 6, 1006, 8128, 28),
          make_way_meta(497, 7, 1007, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(493, 1997, 2997, 3, 1003, 8128, 28),
        make_way_event(496, 1994, 2994, 6, 1006, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events, {}, metas,
        {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (metas[0])
        (metas[1])
        (metas[2])
        (metas[4])
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Mix existing current metas and current events for multiple ids:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_metas =
        { make_way_meta(494, 3, 4004, 8128, 28),
          make_way_meta(495, 2, 2005, 8128, 28) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_metas =
        { make_way_meta(494, 1, 2004, 8128, 28),
          make_way_meta(494, 2, 3004, 8128, 28),
          make_way_meta(495, 1, 1005, 8128, 28),
          make_way_meta(496, 1, 1006, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(494, 1004, 2004, 4, 1004, 8128, 28),
        make_way_event(494, 2004, 3004, 1, 2004, 8128, 28),
        make_way_event(494, 3004, 4004, 2, 3004, 8128, 28),
        make_way_event(494, 4004, 5004, 3, 4004, 8128, 28),
        make_way_event(494, 5004, 6004, 5, 5004, 8128, 28),
        make_way_event(496, 2006, NOW, 2, 2006, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events, current_metas, attic_metas,
        {}, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (current_metas[0])
        (current_metas[1])
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (attic_metas[2])
        (attic_metas[3])
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[5].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (events[3].meta)
        (events[4].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing current with a current event and set reuse flag:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, NOW, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        { meta },
        {}, {}, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing current with an attic event and set reuse flag:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        { meta },
        {}, {}, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (meta)
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing attic with an attic event and set reuse flag:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {},
        { meta },
        {}, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Match a single existing attic with an attic event, followed by another attic event, and set reuse flag:\n";

    OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > meta = make_way_meta(496, 2, 1000, 8128, 28);
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 2, 1000, 8128, 28),
        make_way_event(496, 3000, 4000, 2, 2000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {},
        { meta },
        {}, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[1].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Ensure that a deletion is stored as attic meta:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 2000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (deletions[0])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Ensure that a deletion is stored as attic meta amid an existing current meta:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 2000, 8128, 28) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_metas = {
        make_way_meta(496, 1, 1000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        current_metas, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (current_metas[0])
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (deletions[0])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Store deletion after two consecutive events:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 3, 3000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28),
        make_way_event(496, 2000, 3000, 2, 2000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (events[1].meta)
        (deletions[0])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Store deletion after two events with a gap:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 3, 4000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28),
        make_way_event(496, 3000, 4000, 2, 3000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (events[1].meta)
        (deletions[0])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Ensure that a deletion is stored as attic meta amid a pre-use flag on current:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 3000, 8128, 28) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_metas = {
        make_way_meta(496, 1, 1000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 1, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        current_metas, {},
        deletions, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (current_metas[0])
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (deletions[0])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Ensure that a deletion is stored as attic meta amid a pre-use flag on attic:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 3000, 8128, 28) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_metas = {
        make_way_meta(496, 1, 1000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 2000, 3000, 1, 1000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, attic_metas,
        deletions, { Attic< Way_Skeleton::Id_Type >{ 496u, 2000 } } };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (deletions[0])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Multiple deletions on multiple objects:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(493, 3, 2003, 8128, 28),
        make_way_meta(495, 5, 2005, 8128, 28),
        make_way_meta(496, 6, 2006, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(492, 1002, NOW, 1, 1002, 8128, 28),
        make_way_event(493, 1003, 2003, 1, 1003, 8128, 28),
        make_way_event(494, 1004, NOW, 1, 1004, 8128, 28),
        make_way_event(496, 1006, 2006, 1, 1006, 8128, 28),
        make_way_event(497, 1007, NOW, 1, 1007, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (events[0].meta)
        (events[2].meta)
        (events[4].meta)
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[1].meta)
        (deletions[0])
        (events[3].meta)
        (deletions[2])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: A deletion within and a deletion at the end of multiple versions of one object:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 2000, 8128, 28),
        make_way_meta(496, 4, 4000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28),
        make_way_event(496, 3000, 4000, 3, 3000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (deletions[0])
        (events[1].meta)
        (deletions[1])
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: Two deletions within multiple versions of one object:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 2000, 8128, 28),
        make_way_meta(496, 4, 4000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28),
        make_way_event(496, 3000, 4000, 3, 3000, 8128, 28),
        make_way_event(496, 5000, 6000, 5, 3000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, {},
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (events[0].meta)
        (deletions[0])
        (events[1].meta)
        (deletions[1])
        (events[2].meta)
        (delta.attic_to_add);
  }
  {
    std::cerr<<"\nWay_Meta_Delta: A deletion within multiple versions of one object amid existing attic:\n";

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions = {
        make_way_meta(496, 2, 2000, 8128, 28) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_metas = {
        make_way_meta(496, 1, 1000, 8128, 28),
        make_way_meta(496, 2, 2000, 8128, 28),
        make_way_meta(496, 3, 3000, 8128, 28) };
    std::vector< Way_Event > events = {
        make_way_event(496, 1000, 2000, 1, 1000, 8128, 28),
        make_way_event(496, 3000, 4000, 3, 3000, 8128, 28) };
    Way_Meta_Updater::Way_Meta_Delta delta{
        events,
        {}, attic_metas,
        deletions, {} };
    bool all_ok = true;
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    Way_Meta_Updater::prune_first_skeletons({}, {}, implicit_pre_events);

    std::vector< Proto_Way > proto_ways = Way_Meta_Updater::assign_meta({}, {}, {});

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;
    Way_Meta_Updater::Way_Meta_Delta delta{ {}, {}, {}, {}, {} };

    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_first_skeletons::implicit_pre_events")
        (implicit_pre_events);
    all_ok &= Compare_Vector< Proto_Way >("assign_meta::proto_ways")
        (proto_ways);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_delete")
        (delta.attic_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("Way_Meta_Delta::attic_to_add")
        (delta.attic_to_add);
  }

  test_prune_first_skeletons();
  test_assign_meta();
  test_way_meta_delta();

  return 0;
}
