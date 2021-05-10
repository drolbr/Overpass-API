#include "way_skeleton_updater.h"
#include "way_tag_updater.h"
#include "test_tools.h"


/*namespace Way_Tag_Updater
{
  bool operator==(const Tags_Per_Id_Onetime& lhs, const Tags_Per_Id_Onetime& rhs)
  {
    return lhs.ref == rhs.ref && lhs.before == rhs.before && lhs.tags == rhs.tags;
  }


  bool operator==(const Tags_Per_Id_Timespan& lhs, const Tags_Per_Id_Timespan& rhs)
  {
    return lhs.ref == rhs.ref && lhs.not_before == rhs.not_before && lhs.before == rhs.before && lhs.tags == rhs.tags;
  }
}


bool operator==(const Way_Tag_Updater::Tagdata_By_Idx_Id& lhs, const Way_Tag_Updater::Tagdata_By_Idx_Id& rhs)
{
  return lhs.tags_at_last_unchanged == rhs.tags_at_last_unchanged && lhs.new_tags == rhs.new_tags;
}


void test_merge_values()
{
  {
    std::cerr<<"\nTest empty index:\n";

    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >{}, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (tags_by_id);
  }
  {
    std::cerr<<"\nTest one object with no tags:\n";

    std::map< Uint31_Index, std::vector< Way_Event_With_Tags > > way_events = {
        { ll_upper_(51.25, 7.15), {
          { Way{ 496u }, Data_By_Id< Way_Skeleton >::Entry::Tag_Container{}, {}, 1000, NOW }
        } }
      };
    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        way_events, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 496u, 1000, NOW, {} }
          } })
        (tags_by_id);
  }
  {
    std::cerr<<"\nTest one object with one tag:\n";

    std::map< Uint31_Index, std::vector< Way_Event_With_Tags > > way_events = {
        { ll_upper_(51.25, 7.15), {
          { Way{ 496u }, {
              { "foo", "bar" }
            }, {}, 1000, NOW }
        } }
      };
    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        way_events, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 496u, 1000, NOW, {
                { "foo", "bar" }
              } }
          } })
        (tags_by_id);
  }
  {
    std::cerr<<"\nTest one object with multiple tags:\n";

    std::map< Uint31_Index, std::vector< Way_Event_With_Tags > > way_events = {
        { ll_upper_(51.25, 7.15), {
          { Way{ 496u }, {
              { "foo", "bar" },
              { "goo", "baz" }
            }, {}, 1000, NOW }
        } }
      };
    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        way_events, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 496u, 1000, NOW, {
                { "foo", "bar" },
                { "goo", "baz" }
              } }
          } })
        (tags_by_id);
  }
  {
    std::cerr<<"\nTest multiple objects each of with one tag:\n";

    std::map< Uint31_Index, std::vector< Way_Event_With_Tags > > way_events = {
        { ll_upper_(51.25, 7.15), {
          { Way{ 494u }, {
              { "foo", "bar" }
            }, {}, 2000, NOW },
          { Way{ 495u }, {
              { "goo", "baz" }
            }, {}, 1000, NOW },
          { Way{ 496u }, {
              { "foo", "bar" }
            }, {}, 1000, NOW }
        } }
      };
    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        way_events, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 494u, 2000, NOW, {
                { "foo", "bar" }
              } },
            { 495u, 1000, NOW, {
                { "goo", "baz" }
              } },
            { 496u, 1000, NOW, {
                { "foo", "bar" }
              } }
          } })
        (tags_by_id);
  }
  {
    std::cerr<<"\nTest multiple indices each of with one object:\n";

    std::map< Uint31_Index, std::vector< Way_Event_With_Tags > > way_events = {
        { ll_upper_(51.25, 7.15), {
          { Way{ 496u }, {
              { "foo", "bar496" }
            }, {}, 1000, NOW }
        } },
        { ll_upper_(51.25, 7.16), {
          { Way{ 495u }, {
              { "foo", "bar495" }
            }, {}, 1000, NOW }
        } },
        { ll_upper_(51.25, 12.15), {
          { Way{ 494u }, {
              { "foo", "bar494" }
            }, {}, 1000, NOW }
        } }
      };
    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        way_events, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 496u, 1000, NOW, {
                { "foo", "bar496" }
              } },
            { 495u, 1000, NOW, {
                { "foo", "bar495" }
              } }
          } })
        (ll_upper_(51.25, 12.15) & 0x7fffff00, { {}, {
            { 494u, 1000, NOW, {
                { "foo", "bar494" }
              } }
          } })
        (tags_by_id);
  }
  {
    std::cerr<<"\nTest that preexisting objects in tags_by_id are preserved:\n";

    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id = {
        { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, {
                { "foo", "until2000" }
              } }
          }, {
            { 496u, 2000, NOW, {
                { "foo", "since2000" }
              } }
          } } }
      };
    Way_Tag_Updater::merge_values(
        std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >{}, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        ( ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, {
                { "foo", "until2000" }
              } }
          }, {
            { 496u, 2000, NOW, {
                { "foo", "since2000" }
              } }
          } })
        (tags_by_id);
  }
}*/


void test_way_delta()
{
  {
    std::cerr<<"\nTest whether an existing unmatched id is ignored:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, { { 496u, "foo", {
        { NOW, "bar" }
      } } } }
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
    std::cerr<<"\nTest whether an existing id with one tag is deleted:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, "foo", { { NOW, "bar" } } }
      } }
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, NOW } }, {} }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ( { ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 1000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether an existing id with multiple tags is deleted:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, "foo", { { NOW, "bar" } } },
        { 496u, "goo", { { NOW, "baz" } } }
      } }
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, NOW } }, {} }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { 496u })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "goo", "baz" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 1000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "goo", "baz" }, { { 496u, 1000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether an existing id with multiple events is deleted:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, "foo", { { 2000, "bar" }, { NOW, "baz" } } },
      } }
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, NOW } }, {} }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "baz" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 1000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 2000 } })
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether a new object is added:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      {}
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, NOW } }, {
          { "foo", { { 1000, NOW, "bar" } } }
        } }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { 496u })
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
    std::cerr<<"\nTest whether the value of a key is updated:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, "foo", { { NOW, "old" } } }
      } }
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, NOW } }, {
          { "foo", { { 1000, NOW, "new" } } }
        } }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "new" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "old" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "old" }, { { 496u, 1000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether a single tag for multiple detached versions is added:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      {}
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 2000, 3000 }, { 4000, 5000 } }, {
          { "foo", { { 1000, NOW, "bar" } } }
        } }
      } }
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
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 3000 }, { 496u, 5000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether a single tag for multiple touching versions is added:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      {}
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 2000, 3000 }, { 3000, 4000 } }, {
          { "foo", { { 1000, NOW, "bar" } } }
        } }
      } }
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
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 4000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether a tag and the invalid marker are added for a single longtime version:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      {}
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, NOW } }, {
          { "foo", { { 2000, 3000, "bar" }, { 4000, 5000, "bar" } } }
        } }
      } }
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
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 3000 }, { 496u, 5000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", Way_Tag_Updater::invalid_value() }, { { 496u, 2000 }, { 496u, 4000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest whether a tag and the invalid marker are added for multiple shorttime versions:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      {}
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 1000, 2000 }, { 2000, 3000 }, { 3000, 4000 }, { 4000, 5000 }, { 5000, NOW } }, {
          { "foo", { { 2000, 3000, "bar" }, { 4000, 5000, "bar" } } }
        } }
      } }
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
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar" }, { { 496u, 3000 }, { 496u, 5000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", Way_Tag_Updater::invalid_value() }, { { 496u, 2000 }, { 496u, 4000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest with a complex existing value timeline:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, "foo", { { 2000, "bar_2000" }, { 4000, "bar_4000" }, { 6000, "bar_6000" } } }
      } }
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 4000, 5000 }, { 6000, NOW } }, {
          { "foo", { { 4000, 5000, "bar_4000" }, { 6000, NOW, "final" } } }
        } }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "final" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar_4000" }, { { 496u, 5000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar_4000" }, { { 496u, 4000 } })
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest with a complex new value timeline:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 3000, 5000 }, { 6000, NOW } }, {
          { "foo", { { 1000, 2000, "bar_2000" }, { 4000, 6000, "bar_4000" }, { 6000, NOW, "final" } } }
        } }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "final" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "bar_4000" }, { { 496u, 5000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", Way_Tag_Updater::invalid_value() }, { { 496u, 4000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest with multiple keys and ids:\n";

    Way_Tag_Updater::Way_Tag_Delta delta({
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 495u, "foo", { { 2000, "bar_2000" }, { NOW, "final_495" } } },
        { 496u, "bar", { { NOW, "final_bar_496" } } },
        { 496u, "baz", { { NOW, "final_baz_496" } } },
        { 496u, "foo", { { NOW, "final_foo_496" } } },
        { 497u, "foo", { { NOW, "final_497" } } }
      } }
    }, {
      { ll_upper_(51.25, 7.15) & 0x7fffff00, {
        { 496u, { { 2000, NOW } }, {
          { "baz", { { 2000, NOW, "final_baz" } } },
          { "doo", { { 2000, NOW, "final_doo" } } },
          { "goo", { { 2000, NOW, "final_goo" } } }
        } }
      } }
    });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "baz", "final_baz" }, { 496u })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "doo", "final_doo" }, { 496u })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "goo", "final_goo" }, { 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "bar", "final_bar_496" }, { 496u })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "baz", "final_baz_496" }, { 496u })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "final_foo_496" }, { 496u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "bar", "final_bar_496" }, { { 496u, 2000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "baz", "final_baz_496" }, { { 496u, 2000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "final_foo_496" }, { { 496u, 2000 } })
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

//     std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
//     Way_Tag_Updater::merge_values(
//         std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >{}, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {});

    bool all_ok = true;
//     all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
//         ("merge_values::tags_by_id")
//         (tags_by_id);
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

  //test_merge_values();
  test_way_delta();

  return 0;
}
