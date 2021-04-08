#include "way_skeleton_updater.h"
#include "way_tag_updater.h"
#include "test_tools.h"


namespace Way_Tag_Updater
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
}


void test_way_delta()
{
  {
    std::cerr<<"\nTest single new current:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15), { {}, { { 496u, 1000, NOW, { {"foo", "bar"} } } } } }
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
        {
          { ll_upper_(51.25, 7.15), { {}, { { 496u, 1000, 2000, { {"foo", "bar"} } } } } }
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
        {}, {
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
        {}, {}, {
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
        {}, {
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
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, { { 496u, 2000, NOW, { {"foo", "bar"} } } } } }
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
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 496u, 1000, 2000, { {"foo", "old"} } },
            { 496u, 2000, NOW, { {"foo", "new"} } }
          } } }
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
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 496u, 1000, 2000, { {"old", "bar"} } },
            { 496u, 2000, NOW, { {"new", "bar"} } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { {"foo", "bar"} } }
          }, {
            { 496u, 3000, NOW, { {"foo", "bar"} } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { {"foo", "old"} } }
          }, {
            { 496u, 3000, NOW, { {"foo", "new"} } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { {"old", "bar"} } }
          }, {
            { 496u, 3000, NOW, { {"new", "bar"} } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { {"foo", "attic"} } }
          }, {
            { 496u, 2000, 3000, { {"foo", "existing"} } },
            { 496u, 3000, NOW, { {"foo", "existing"} } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { {"foo", "attic"} } }
          }, {
            { 496u, 2000, 3000, { {"foo", "existing"} } },
            { 496u, 3000, NOW, { {"foo", "new"} } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { { "attic", "bar" } } }
          }, {
            { 496u, 2000, 3000, { { "existing", "bar" } } },
            { 496u, 3000, NOW, { { "new", "bar" } } }
          } } }
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
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { { "attic", "bar" } } }
          }, {
            { 496u, 2000, 3000, { { "existing", "bar" } } },
            { 496u, 3000, NOW, { { "attic", "bar" } } }
          } } }
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
  {
    std::cerr<<"\nTest with only unchanged:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { { "foo", "old" } } }
          }, {} } }
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
        ({ ll_upper_(51.25, 7.15), "foo", "old" }, { { 496u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest with unmatched unchanged:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 495u, 2000, { { "foo", "bar" } } }
          }, {
            { 496u, 2000, NOW, { { "foo", "bar" } } },
          } } }
        }, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { { 496u } })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { { 495u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest that tags for different ids do not cancel out each other:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {}, {
            { 494u, 1000, 2000, { { "foo", "old" } } },
            { 494u, 2000, NOW, { { "foo", "new" } } },
            { 496u, 1000, 2000, { { "foo", "old" } } },
            { 496u, 2000, NOW, { { "foo", "new" } } },
            { 497u, 1000, 2000, { { "foo", "old" } } },
            { 497u, 2000, NOW, { { "foo", "new" } } }
          } } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "new" }, { 495u, 497u } }
        }, {
          { { ll_upper_(51.25, 7.15), "foo", "old" }, { { 495u, 2000 }, { 497u, 3000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "new" }, { 494u, 496u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "new" }, { 495u })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "foo", "old" }, { { 494u, 2000 }, { 496u, 2000 }, { 497u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 7.15), "foo", "old" }, { { 495u, 2000 }, { 497u, 3000 } })
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest that new tags and entries in unchanged align well:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 495u, 2000, { { "goo", "bar" } } },
            { 496u, 2000, { { "foo", "bar" } } }
          }, {
            { 495u, 2000, NOW, { { "extra", "cat" }, { "goo", "bar" } } },
            { 496u, 2000, NOW, { { "extra", "baz" }, { "foo", "bar" } } }
          } } }
        }, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 7.15), "extra", "baz" }, { 496u })
        ({ ll_upper_(51.25, 7.15), "extra", "cat" }, { 495u })
        ({ ll_upper_(51.25, 7.15), "foo", "bar" }, { 496u })
        ({ ll_upper_(51.25, 7.15), "goo", "bar" }, { 495u })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 7.15), "extra", Way_Tag_Updater::invalid_value() }, { { 495u, 2000 }, { 496u, 2000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        (delta.attic_to_delete);
  }
  {
    std::cerr<<"\nTest with multiple indexes:\n";

    Way_Tag_Updater::Way_Tag_Delta delta(
        {
          { ll_upper_(51.25, 7.15) & 0x7fffff00, { {
            { 496u, 2000, { { "foo", "old" } } }
          }, {} } },
          { ll_upper_(51.25, 12.15) & 0x7fffff00, { {
            { 496u, 1000, { { "foo", "old" } } }
          }, {
            { 496u, 2000, 3000, { { "foo", "new" } } },
            { 496u, 3000, NOW, { { "foo", "changed" } } }
          } } }
        }, {
          { { ll_upper_(51.25, 2.15) & 0x7fffff00, "foo", "new" }, { 495u } },
          { { ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "new" }, { 497u } },
          { { ll_upper_(51.25, 12.15) & 0x7fffff00, "foo", "new" }, { 496u } }
        }, {
          { { ll_upper_(51.25, 2.15) & 0x7fffff00, "foo", "old" }, { { 495u, 2000 } } },
          { { ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "old" }, { { 496u, 2000 }, { 497u, 1000 } } },
          { { ll_upper_(51.25, 12.15) & 0x7fffff00, "foo", "old" }, { { 496u, 1000 } } }
        });

    bool all_ok = true;
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_add")
        ({ ll_upper_(51.25, 12.15) & 0x7fffff00, "foo", "changed" }, { { 496u } })
        (delta.current_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Way_Skeleton::Id_Type >
        ("Way_Tag_Delta::current_to_delete")
        ({ ll_upper_(51.25, 2.15) & 0x7fffff00, "foo", "new" }, { { 495u } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "new" }, { { 497u } })
        ({ ll_upper_(51.25, 12.15) & 0x7fffff00, "foo", "new" }, { { 496u } })
        (delta.current_to_delete);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_add")
        ({ ll_upper_(51.25, 12.15) & 0x7fffff00, "foo", "new" }, { { 496u, 3000 } })
        (delta.attic_to_add);
    all_ok &= Compare_Map_Set< Tag_Index_Local, Attic< Way_Skeleton::Id_Type > >
        ("Way_Tag_Delta::attic_to_delete")
        ({ ll_upper_(51.25, 2.15) & 0x7fffff00, "foo", "old" }, { { 495u, 2000 } })
        ({ ll_upper_(51.25, 7.15) & 0x7fffff00, "foo", "old" }, { { 497u, 1000 } })
        (delta.attic_to_delete);
  }
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";

    std::map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id > tags_by_id;
    Way_Tag_Updater::merge_values(
        std::map< Uint31_Index, std::vector< Way_Event_With_Tags > >{}, tags_by_id);

    Way_Tag_Updater::Way_Tag_Delta delta({}, {}, {});

    bool all_ok = true;
    all_ok &= Compare_Map< Uint31_Index, Way_Tag_Updater::Tagdata_By_Idx_Id >
        ("merge_values::tags_by_id")
        (tags_by_id);
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

  test_merge_values();
  test_way_delta();

  return 0;
}
