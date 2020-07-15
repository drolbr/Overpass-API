#include "node_meta_updater.h"
#include "test_tools.h"


int main(int argc, char* args[])
{
  {
    std::cerr<<"Test empty input:\n";
    Data_From_Osc data_from_osc;
    bool all_ok = true;

    Pre_Event_List pre_events;
    Node_Meta_Updater::adapt_pre_event_list(
        std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(), pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list")
        (pre_events.data);

    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > to_move;
    Node_Meta_Updater::collect_current_meta_to_move(
        pre_events, std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(), to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
        ("collect_current_meta_to_move")
        (to_move);
  }
  {
    std::cerr<<"\nTest whether pre_events respects ids:\n";
    Data_From_Osc data_from_osc;
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
    for (const auto& i : entries)
      pre_events.data.push_back(Node_Pre_Event(i));

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1104));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(498ull, 1108));
    Node_Meta_Updater::adapt_pre_event_list(current_meta, pre_events);
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
    Data_From_Osc data_from_osc;
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

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1100));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1400));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1600));
    Node_Meta_Updater::adapt_pre_event_list(current_meta, pre_events);
    all_ok &= Compare_Vector< Node_Pre_Event >("adapt_pre_event_list")
        (Node_Pre_Event(entries[0], 1300))
        (Node_Pre_Event(entries[1], 1400))
        (Node_Pre_Event(entries[2], 1600))
        (pre_events.data);
  }
  {
    std::cerr<<"\nTest all cases of collect_current_meta_to_move:\n";
    Data_From_Osc data_from_osc;
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
    for (const auto& i : entries)
      pre_events.data.push_back(Node_Pre_Event(i));

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta;
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(493ull, 1103));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(494ull, 1104));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1105));
    current_meta.push_back(OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106));

    std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > to_move;
    Node_Meta_Updater::collect_current_meta_to_move(pre_events, current_meta, to_move);
    all_ok &= Compare_Set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
        ("collect_current_meta_to_move")
        (OSM_Element_Metadata_Skeleton< Uint64 >(495ull, 1105))
        (OSM_Element_Metadata_Skeleton< Uint64 >(496ull, 1106))
        (to_move);
  }

  return 0;
}
