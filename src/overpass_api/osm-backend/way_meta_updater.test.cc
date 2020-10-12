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


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
        std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
        ll_upper_(51.25, 7.15),
        std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >(),
        Pre_Event_List< Way_Skeleton >(),
        std::vector< Way_Implicit_Pre_Event >(),
        std::vector< Attic< Way_Skeleton::Id_Type > >(),
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest all but meta filled:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 2000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete =
        { Attic< Way_Skeleton::Id_Type >(Way_Skeleton::Id_Type(496u), 3000) };

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
        std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether current is moved by pre_events:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 1003),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1004),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1005),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1006),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(497ull, 1007),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 1008) };

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(493u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 2003)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(495u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 2005)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2006)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(498u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 2008)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(493u), 2003ull, 0 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(495u), 2005ull, 1 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 2006ull, 2 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 2008ull, 3 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current,
        std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 1003))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1005))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1006))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 1008))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether attic is not moved by pre_events:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current;
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 1003),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1004),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1005),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1006),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(497ull, 1007),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 1008) };

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(493u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 2003)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(495u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 2005)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2006)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(498u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 2008)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(493u), 2003ull, 0 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(495u), 2005ull, 1 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 2006ull, 2 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 2008ull, 3 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest current plus attic plus pre_events:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 1503),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1504),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1505),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1506),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(497ull, 1507),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 1508) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 1003),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1004),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1005),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(497ull, 1007),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 1008) };

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(493u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 2003)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(495u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 2005)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2006)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(498u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 2008)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(493u), 2003ull, 0 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(495u), 2005ull, 1 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 2006ull, 2 },
          Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(498u), 2008ull, 3 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(493ull, 1503))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1505))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1506))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(498ull, 1508))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether no change is found if the implicit event has the same index than current:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether an index change per implicit event moves the current meta:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether a double idx change per implicit makes one attic and moves the current:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 6.15), ll_lower(51.25, 6.150001) },
            Quad_Coord{ ll_upper_(51.25, 6.15), ll_lower(51.25, 6.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (ll_upper_(51.25, 6.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1500) })
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (attic_to_add);
  }
  {
    std::cerr<<"\nDoes an index change per later implicit event moves the current meta and adds new current:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether the current stays if the newest implicit gets back to working_idx:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest whether the current moves if the newest implicit is on a different idx:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest with a current, an implicit event on the same idx, and a pre_event elsewhere:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 3000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = false;//true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest with a current, an implicit event on a different idx, and a pre_event elsewhere:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 3000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = false;//true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest with a current, two implicit events on a different idxs, and a pre_event elsewhere:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 3000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 6.15), ll_lower(51.25, 6.150001) },
            Quad_Coord{ ll_upper_(51.25, 6.15), ll_lower(51.25, 6.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = false;//true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 6.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest with two implicit events on a different and then the working_idx, and a pre_event elsewhere:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 3000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = false;//true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (attic_to_add);
  }
  {
    std::cerr<<"\nTest with two implicit events on the working_idx and then a different idx, and a pre_event elsewhere:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic;

    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(496u)),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
    pre_events.data[0].timestamp_end = NOW;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs =
        { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32(496u), 3000ull, 0 } };

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 496ull, 1000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2000, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = false;//true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000))
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000) })
        (attic_to_add);
  }
  {
    std::cerr<<"\nNo idxs spill if the timestamps are aligned:\n";

    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > to_move;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_to_delete;
    std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_to_delete;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > current_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > > attic_to_add;

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 3004),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 3005),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3006) };
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic =
        { OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1004),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 2004),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 1005),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 2005),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1006),
          OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2006) };

    Pre_Event_List< Way_Skeleton > pre_events;
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs;

    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ 494ull, 1004, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 4.15), ll_lower(51.25, 4.150001) },
            Quad_Coord{ ll_upper_(51.25, 4.15), ll_lower(51.25, 4.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 494ull, 2004, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 5.15), ll_lower(51.25, 5.150001) },
            Quad_Coord{ ll_upper_(51.25, 5.15), ll_lower(51.25, 5.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 494ull, 3004, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 6.15), ll_lower(51.25, 6.150001) },
            Quad_Coord{ ll_upper_(51.25, 6.15), ll_lower(51.25, 6.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 495ull, 2005, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 495ull, 3005, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 1006, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 2006, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001) },
            Quad_Coord{ ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002) } }, std::vector< Node::Id_Type >() },
          Way_Implicit_Pre_Event{ 496ull, 3006, std::vector< Quad_Coord >{
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150001) },
            Quad_Coord{ ll_upper_(51.25, 8.15), ll_lower(51.25, 8.150002) } }, std::vector< Node::Id_Type >() } };
    std::vector< Attic< Way_Skeleton::Id_Type > > undelete;

    bool all_ok = false;//true;
    Way_Meta_Updater::collect_meta_to_move(
        current, attic,
        ll_upper_(51.25, 7.15), pre_event_refs, pre_events, implicit_pre_events, undelete,
        to_move, current_to_delete, attic_to_delete, current_to_add, attic_to_add);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::to_move")
        (to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::current_to_delete")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 3004))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3006))
        (current_to_delete);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >
        ("collect_current_meta_to_move::attic_to_delete")
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1004))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 2004))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 2005))
        (OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1006))
        (attic_to_delete);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::current_to_add")
        (ll_upper_(51.25, 6.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 3004) })
        (ll_upper_(51.25, 8.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3006) })
        (current_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
        ("collect_current_meta_to_move::attic_to_add")
        (ll_upper_(51.25, 4.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1004) })
        (ll_upper_(51.25, 5.15), { OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 2004) })
        (ll_upper_(51.25, 8.15), {
            OSM_Element_Metadata_Skeleton< Uint32_Index >(495ull, 2005),
            OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1006) })
        (attic_to_add);
  }

// weitere Testfälle:
// - attic neg + pos: attic + current, attic + attic, attic leer, attic aus eher
// - attic pos + neg: attic + current, attic + attic, attic leer, attic aus eher

  return 0;
}
