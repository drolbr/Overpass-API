#include "node_meta_updater.h"
#include "test_tools.h"


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";
    bool all_ok = true;

    Pre_Event_List pre_events;
    std::vector< Pre_Event_Ref > pre_event_refs;
    Node_Meta_Updater::adapt_pre_event_list(
        Uint31_Index(0u), std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(),
        pre_event_refs, pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list")
        (pre_events.data);

    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > to_move;
    Node_Meta_Updater::collect_current_meta_to_move(
        pre_event_refs, pre_events,
        std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(), to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
        ("collect_current_meta_to_move")
        (to_move);

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        global_to_move;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        nodes_meta_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        nodes_attic_meta_to_add;
    Node_Meta_Updater::create_update_for_nodes_meta(
        pre_events, global_to_move, nodes_meta_to_add, nodes_attic_meta_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        ("create_update_for_nodes_meta.current")
        (nodes_meta_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        ("create_update_for_nodes_meta.attic")
        (nodes_attic_meta_to_add);
  }
  {
    std::cerr<<"\nTest whether pre_events respects ids:\n";
    bool all_ok = true;

    std::vector< Data_By_Id< Node_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15004), Node_Skeleton(494ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1004)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15005), Node_Skeleton(495ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1005)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15006), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1006)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15007), Node_Skeleton(497ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(497ull, 1007)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15008), Node_Skeleton(498ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(498ull, 1008)));
    Pre_Event_List pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Node_Pre_Event(i));
    std::vector< Pre_Event_Ref > pre_event_refs = {
        Pre_Event_Ref{ 494ull, 1004, 0 },
        Pre_Event_Ref{ 495ull, 1005, 1 },
        Pre_Event_Ref{ 496ull, 1006, 2 },
        Pre_Event_Ref{ 497ull, 1007, 3 },
        Pre_Event_Ref{ 498ull, 1008, 4 } };

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1104));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(498ull, 1108));
    Node_Meta_Updater::adapt_pre_event_list(ll_upper_(51.25, 7.15006), current_meta, pre_event_refs, pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list")
        (Node_Pre_Event(entries[0], 1104))
        (Node_Pre_Event(entries[1]))
        (Node_Pre_Event(entries[2], 1106))
        (Node_Pre_Event(entries[3]))
        (Node_Pre_Event(entries[4], 1108))
        (pre_events.data);
  }
  {
    std::cerr<<"\nTest whether pre_events respects timestamps:\n";
    bool all_ok = true;

    std::vector< Data_By_Id< Node_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1200)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1300)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1500)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(entries[0], 1300));
    pre_events.data.push_back(Node_Pre_Event(entries[1], 1500));
    pre_events.data.push_back(Node_Pre_Event(entries[2]));
    std::vector< Pre_Event_Ref > pre_event_refs = {
        Pre_Event_Ref{ 496ull, 1200, 0 } };

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1100));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1400));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1600));
    Node_Meta_Updater::adapt_pre_event_list(ll_upper_(51.25, 7.15006), current_meta, pre_event_refs, pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list")
        (Node_Pre_Event(entries[0], 1300))
        (Node_Pre_Event(entries[1], 1400))
        (Node_Pre_Event(entries[2], 1600))
        (pre_events.data);
  }
  {
    std::cerr<<"\nTest whether pre_events processes deleted items:\n";
    bool all_ok = true;

    std::vector< Data_By_Id< Node_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        Uint31_Index(0u), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(493ull, 1203)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        Uint31_Index(0u), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1204)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        Uint31_Index(0u), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1205)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        Uint31_Index(0u), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1206)));
    Pre_Event_List pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Node_Pre_Event(i));
    for (auto& i : entries)
      pre_events.timestamp_last_not_deleted.push_back(Node_Pre_Event(i, 0u));
    std::vector< Pre_Event_Ref > pre_event_refs = {
        Pre_Event_Ref{ 493ull, 1203, 0 },
        Pre_Event_Ref{ 494ull, 1204, 1 },
        Pre_Event_Ref{ 495ull, 1205, 2 },
        Pre_Event_Ref{ 496ull, 1206, 3 } };

    std::vector< Data_By_Id< Node_Skeleton >::Entry > expected;
    expected.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(493ull, 1203)));
    expected.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1204)));
    expected.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1205)));
    expected.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.25), Node_Skeleton(0ull, 0u),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1206)));

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(493ull, 1103));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1004));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1105));
    Node_Meta_Updater::adapt_pre_event_list(ll_upper_(51.25, 7.15), current_meta, pre_event_refs, pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list.first")
        (Node_Pre_Event(expected[0], NOW))
        (Node_Pre_Event(expected[1], NOW))
        (Node_Pre_Event(expected[2], NOW))
        (Node_Pre_Event(entries[3], NOW))
        (pre_events.data);

    expected[1].idx = ll_upper_(51.25, 7.25);
    current_meta.clear();
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(493ull, 1003));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1104));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1006));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106));
    Node_Meta_Updater::adapt_pre_event_list(ll_upper_(51.25, 7.25), current_meta, pre_event_refs, pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list.second")
        (Node_Pre_Event(expected[0], NOW))
        (Node_Pre_Event(expected[1], NOW))
        (Node_Pre_Event(expected[2], NOW))
        (Node_Pre_Event(expected[3], NOW))
        (pre_events.data);
  }
  {
    std::cerr<<"\nTest all cases of collect_current_meta_to_move:\n";
    bool all_ok = true;

    std::vector< Data_By_Id< Node_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15004), Node_Skeleton(494ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1004)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15005), Node_Skeleton(495ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1205)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15006), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1006)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15006), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1206)));
    Pre_Event_List pre_events;
    for (auto& i : entries)
      pre_events.data.push_back(Node_Pre_Event(i));
    std::vector< Pre_Event_Ref > pre_event_refs = {
        Pre_Event_Ref{ 494ull, 1004, 0 },
        Pre_Event_Ref{ 495ull, 1205, 1 },
        Pre_Event_Ref{ 496ull, 1006, 2 } };

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(493ull, 1103));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1104));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1105));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106));

    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > to_move;
    Node_Meta_Updater::collect_current_meta_to_move(pre_event_refs, pre_events, current_meta, to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
        ("collect_current_meta_to_move")
        (OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1105))
        (OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106))
        (to_move);
  }
  {
    std::cerr<<"Test create_update_for_nodes_meta:\n";
    bool all_ok = true;

    std::vector< Data_By_Id< Node_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1000)));
    entries.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull),
        OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1100)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(entries[0], 1100));
    pre_events.data.push_back(Node_Pre_Event(entries[1], NOW));
    std::vector< Pre_Event_Ref > pre_event_refs;

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        global_to_move;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        nodes_meta_to_add;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        nodes_attic_meta_to_add;
    Node_Meta_Updater::create_update_for_nodes_meta(
        pre_events, global_to_move, nodes_meta_to_add, nodes_attic_meta_to_add);
    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_expected;
    current_expected.insert(entries[1].meta);
    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > attic_expected;
    attic_expected.insert(entries[0].meta);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        ("create_update_for_nodes_meta.current")
        (ll_upper_(51.25, 7.15), current_expected)
        (nodes_meta_to_add);
    all_ok &= Compare_Map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
        ("create_update_for_nodes_meta.attic")
        (ll_upper_(51.25, 7.15), attic_expected)
        (nodes_attic_meta_to_add);
  }

  return 0;
}
