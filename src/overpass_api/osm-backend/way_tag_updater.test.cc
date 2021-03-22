#include "way_tag_updater.h"
#include "test_tools.h"


void test_way_delta()
{
  {
    std::cerr<<"\nTest single new current:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {
          { ll_upper_(51.25, 7.15), { { 496u, 1000, NOW, { {"foo", "bar"} } } } }
        }, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest single new attic:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {
          { ll_upper_(51.25, 7.15), { { 496u, 1000, 2000, { {"foo", "bar"} } } } }
        }, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { { 496u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest single existing current:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {}, {
          { { ll_upper_(51.25, 7.15), "foo", "bar" }, { 496u } }
        }, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest single existing attic:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {}, {}, {
          { { ll_upper_(51.25, 7.15), "foo", "bar" }, { { 496u, 1000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { { 496u, 1000 } })
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest existing current-attic pair:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {}, {
          { { ll_upper_(51.25, 7.15), "foo", "new" }, { 496u } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "old" }, { { 496u, 1000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "new" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "old" }, { { 496u, 1000 } })
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to a single existing current, values are equal:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { { 496u, 2000, NOW, { {"foo", "bar"} } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "bar" }, { 496u } }
        }, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to a single existing current, values differ:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 1000, 2000, { {"foo", "old"} } },
            { 496u, 2000, NOW, { {"foo", "new"} } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "old" }, { 496u } }
        }, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "new" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "old" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "old" }, { { 496u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to a single existing current, keys differ:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {}, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 1000, 2000, { {"old", "bar"} } },
            { 496u, 2000, NOW, { {"new", "bar"} } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "old", "bar" }, { 496u } }
        }, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "new", "bar" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "old", "bar" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "new", Way_Tag_Updater::invalid_value() }, { { 496u, 2000 } })
        ({ ll_upper_(51.25, 7.15), "old", "bar" }, { { 496u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to a single existing attic, values are equal:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { {"foo", "bar"} } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { { 496u, 3000, NOW, { {"foo", "bar"} } } } }
        }, {}, {
          { { ll_upper_(51.25, 7.15), "foo", "bar" }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { { 496u, 2000 } })
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to a single existing attic, values differ:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { {"foo", "old"} } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { { 496u, 3000, NOW, { {"foo", "new"} } } } }
        }, {}, {
          { { ll_upper_(51.25, 7.15), "foo", "old" }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "new" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to a single existing attic, keys differ:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { {"old", "bar"} } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { { 496u, 3000, NOW, { {"new", "bar"} } } } }
        }, {}, {
          { { ll_upper_(51.25, 7.15), "old", "bar" }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "new", "bar" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "new", Way_Tag_Updater::invalid_value() }, { { 496u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to an existing current-attic pair, values are equal:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { {"foo", "attic"} } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, 3000, { {"foo", "existing"} } },
            { 496u, 3000, NOW, { {"foo", "existing"} } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "existing" }, { 496u } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "attic" }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to an existing current-attic pair, values differ:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { {"foo", "attic"} } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, 3000, { {"foo", "existing"} } },
            { 496u, 3000, NOW, { {"foo", "new"} } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "existing" }, { 496u } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "attic" }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "new" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "existing" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "existing" }, { { 496u, 3000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to an existing current-attic pair, keys differ:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { { "attic", "bar" } } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, 3000, { { "existing", "bar" } } },
            { 496u, 3000, NOW, { { "new", "bar" } } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "existing", "bar" }, { 496u } }
        }, {
          { { ll_upper_(51.25, 7.15), "attic", "bar" }, { { 496u, 2000 } } },
          { { ll_upper_(51.25, 7.15), "existing", Way_Tag_Updater::invalid_value() }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "new", "bar" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "existing", "bar" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "existing", "bar" }, { { 496u, 3000 } })
        ({ ll_upper_(51.25, 7.15), "new", Way_Tag_Updater::invalid_value() }, { { 496u, 3000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest adding a new current to an existing current-attic pair, a key comes back:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, { { "attic", "bar" } } } } }
        }, {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, {
            { 496u, 2000, 3000, { { "existing", "bar" } } },
            { 496u, 3000, NOW, { { "attic", "bar" } } } } }
        }, {
          { { ll_upper_(51.25, 7.15), "existing", "bar" }, { 496u } }
        }, {
          { { ll_upper_(51.25, 7.15), "attic", "bar" }, { { 496u, 2000 } } },
          { { ll_upper_(51.25, 7.15), "existing", Way_Tag_Updater::invalid_value() }, { { 496u, 2000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "attic", "bar" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "existing", "bar" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "existing", "bar" }, { { 496u, 3000 } })
        ({ ll_upper_(51.25, 7.15), "attic", Way_Tag_Updater::invalid_value() }, { { 496u, 3000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";

//     std::vector< Attic< Way_Skeleton::Id_Type > > undeleted =
//         Way_Skeleton_Updater::extract_relevant_undeleted({}, {});
// 
//     std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
//     Way_Skeleton_Updater::adjust_implicit_events({}, implicit_pre_events);
// 
//     std::vector< Way_Event > events_for_this_idx;
//     std::map< Uint31_Index, std::vector< Way_Event > > arrived_objects;
//     Way_Skeleton_Updater::resolve_coord_events(
//         ll_upper_(52.15, 7.15), std::vector< Proto_Way >{},
//         events_for_this_idx, arrived_objects);
// 
//     std::map< Uint31_Index, std::vector< Way_Event > > changes_per_idx;
//     std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > deletions;
//     Way_Skeleton_Updater::resolve_coord_events(
//         Pre_Event_List< Way_Skeleton >(), Moved_Coords{},
//         changes_per_idx, deletions);
// 
//     Way_Skeleton_Updater::Way_Skeleton_Delta skel_delta(
//         std::vector< Way_Event >{},
//         std::vector< const Way_Skeleton* >{}, std::vector< const Attic< Way_Skeleton >* >{});
//     Way_Skeleton_Updater::Way_Undelete_Delta undel_delta({}, {}, {});

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }

  test_way_delta();

  return 0;
}
