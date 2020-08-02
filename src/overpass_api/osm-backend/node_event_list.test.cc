#include "node_event_list.h"
#include "test_tools.h"


bool operator==(const Node_Event& lhs, const Node_Event& rhs)
{
  return lhs.id == rhs.id && lhs.timestamp == rhs.timestamp
      && lhs.visible_before == rhs.visible_before && lhs.ll_lower_before == rhs.ll_lower_before
      && lhs.visible_after == rhs.visible_after && lhs.ll_lower_after == rhs.ll_lower_after
      && lhs.multiple_after == rhs.multiple_after;
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"Test empty input:\n";

    Node_Event_List events(
        ll_upper_(51.25, 7.15), Node_Skeletons_Per_Idx(), Pre_Event_Refs(), Pre_Event_List());
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        (events.data);
  }
  {
    std::cerr<<"\nTest single current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, Pre_Event_Refs(), Pre_Event_List());
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15)), 2000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, Pre_Event_Refs(), Pre_Event_List());
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest undeleted for current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.undeleted = { Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 2000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, Pre_Event_Refs(), Pre_Event_List());
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest undeleted for attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15)), 3000) };
    skels.undeleted = { Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 2000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, Pre_Event_Refs(), Pre_Event_List());
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 3000, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest mixed undeleted, attic, and current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(492ull, ll_lower(51.25, 7.15002)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15006)),
        Node_Skeleton(498ull, ll_lower(51.25, 7.15008)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(492ull, ll_lower(51.25, 7.15002)), 2002),
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15004)), 2004),
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15004)), 4004),
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15006)), 2006),
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15006)), 4006),
        Attic< Node_Skeleton >(Node_Skeleton(498ull, ll_lower(51.25, 7.15008)), 2008),
        Attic< Node_Skeleton >(Node_Skeleton(498ull, ll_lower(51.25, 7.150084)), 4008),
        Attic< Node_Skeleton >(Node_Skeleton(498ull, ll_lower(51.25, 7.15008)), 5008),
        Attic< Node_Skeleton >(Node_Skeleton(498ull, ll_lower(51.25, 7.150087)), 7008) };
    skels.undeleted = {
        Attic< Node_Skeleton::Id_Type >(Uint64(492ull), 3002),
        Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 3004),
        Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 3006),
        Attic< Node_Skeleton::Id_Type >(Uint64(498ull), 3008),
        Attic< Node_Skeleton::Id_Type >(Uint64(498ull), 6008) };
    skels.first_appearance = {
        std::make_pair(Uint64(491ull), 1001),
        std::make_pair(Uint64(492ull), 1002),
        std::make_pair(Uint64(493ull), 1003),
        std::make_pair(Uint64(494ull), 1004),
        std::make_pair(Uint64(495ull), 1005),
        std::make_pair(Uint64(496ull), 1006),
        std::make_pair(Uint64(497ull), 1007),
        std::make_pair(Uint64(498ull), 1008),
        std::make_pair(Uint64(499ull), 1009) };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, Pre_Event_Refs(), Pre_Event_List());
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 491ull, 1001, false, 0u, false, 0u, false } })
        ({ Node_Event{ 492ull, 1002, true, ll_lower(51.25, 7.15002), true, ll_lower(51.25, 7.15002), false } })
        ({ Node_Event{ 492ull, 2002, false, 0u, false, 0u, false } })
        ({ Node_Event{ 492ull, 3002, true, ll_lower(51.25, 7.15002), true, ll_lower(51.25, 7.15002), false } })
        ({ Node_Event{ 493ull, 1003, false, 0u, false, 0u, false } })
        ({ Node_Event{ 494ull, 1004, true, ll_lower(51.25, 7.15004), true, ll_lower(51.25, 7.15004), false } })
        ({ Node_Event{ 494ull, 2004, false, 0u, false, 0u, false } })
        ({ Node_Event{ 494ull, 3004, true, ll_lower(51.25, 7.15004), true, ll_lower(51.25, 7.15004), false } })
        ({ Node_Event{ 494ull, 4004, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 1005, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 1006, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false } })
        ({ Node_Event{ 496ull, 2006, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 3006, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false } })
        ({ Node_Event{ 496ull, 4006, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false } })
        ({ Node_Event{ 497ull, 1007, false, 0u, false, 0u, false } })
        ({ Node_Event{ 498ull, 1008, true, ll_lower(51.25, 7.15008), true, ll_lower(51.25, 7.15008), false } })
        ({ Node_Event{ 498ull, 2008, false, 0u, false, 0u, false } })
        ({ Node_Event{ 498ull, 3008, true, ll_lower(51.25, 7.150084), true, ll_lower(51.25, 7.150084), false } })
        ({ Node_Event{ 498ull, 4008, true, ll_lower(51.25, 7.15008), true, ll_lower(51.25, 7.15008), false } })
        ({ Node_Event{ 498ull, 5008, false, 0u, false, 0u, false } })
        ({ Node_Event{ 498ull, 6008, true, ll_lower(51.25, 7.150087), true, ll_lower(51.25, 7.150087), false } })
        ({ Node_Event{ 498ull, 7008, true, ll_lower(51.25, 7.15008), true, ll_lower(51.25, 7.15008), false } })
        ({ Node_Event{ 499ull, 1009, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event for new object:\n";
    Node_Skeletons_Per_Idx skels;
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull, ll_lower(51.25, 7.15)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 1000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, true, ll_lower(51.25, 7.15), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event for formerly existing object:\n";
    Node_Skeletons_Per_Idx skels;
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), Node_Skeleton(496ull, ll_lower(51.25, 7.15)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, true, ll_lower(51.25, 7.15), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event on different index for formerly existing object:\n";
    Node_Skeletons_Per_Idx skels;
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.55), Node_Skeleton(496ull, ll_lower(51.25, 7.55)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event after current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15006), Node_Skeleton(496ull, ll_lower(51.25, 7.15006)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15006), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event on different index after current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.55), Node_Skeleton(496ull, ll_lower(51.25, 7.55)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event after attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15)), 2000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.15006), Node_Skeleton(496ull, ll_lower(51.25, 7.15006)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 3000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 3000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 3000, false, 0u, true, ll_lower(51.25, 7.15006), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest multiple pre_events after current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150063), Node_Skeleton(496ull, ll_lower(51.25, 7.150063)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 3000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        0u, Node_Skeleton(0ull, 0),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 4000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1]));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[2]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.150063), false } })
        ({ Node_Event{ 496ull, 4000, true, ll_lower(51.25, 7.15), false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest pre_events on multiple objects after current:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(494ull, ll_lower(51.25, 7.15004)),
        Node_Skeleton(495ull, ll_lower(51.25, 7.15005)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15006)) };
    skels.first_appearance = {
        std::make_pair(Uint64(494ull), 1004),
        std::make_pair(Uint64(495ull), 1005),
        std::make_pair(Uint64(496ull), 1006) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150042), Node_Skeleton(494ull, ll_lower(51.25, 7.150042)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(494ull, 2004)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150052), Node_Skeleton(495ull, ll_lower(51.25, 7.150052)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 2005)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2006)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0]));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1]));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[2]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 494ull, 2004, 0 },
        Pre_Event_Ref{ 495ull, 2005, 1 },
        Pre_Event_Ref{ 496ull, 2006, 2 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 494ull, 1004, true, ll_lower(51.25, 7.15004), true, ll_lower(51.25, 7.15004), false } })
        ({ Node_Event{ 494ull, 2004, true, ll_lower(51.25, 7.15004), true, ll_lower(51.25, 7.150042), false } })
        ({ Node_Event{ 495ull, 1005, true, ll_lower(51.25, 7.15005), true, ll_lower(51.25, 7.15005), false } })
        ({ Node_Event{ 495ull, 2005, true, ll_lower(51.25, 7.15005), true, ll_lower(51.25, 7.150052), false } })
        ({ Node_Event{ 496ull, 1006, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.15006), false } })
        ({ Node_Event{ 496ull, 2006, true, ll_lower(51.25, 7.15006), true, ll_lower(51.25, 7.150062), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event before current with attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.150063)) };
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150061)), 3000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150061), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.150063), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event before current without attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.150063)) };
    skels.undeleted = { Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 3000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.150063), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event before attic with attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.attic = { 
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150061)), 3000),
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150063)), 4000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150061), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.150063), false } })
        ({ Node_Event{ 496ull, 4000, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event before attic without attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150063)), 4000) };
    skels.undeleted = { Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 3000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.150063), false } })
        ({ Node_Event{ 496ull, 4000, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest single pre_event before first_appearance:\n";
    Node_Skeletons_Per_Idx skels;
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150062)), 3000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 2000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150061)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 2000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 1000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, true, ll_lower(51.25, 7.150061), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, false, 0u, false, 0u, false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest multiple pre_events before current with attic:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.150064)) };
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150061)), 4000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150063)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 3000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1], 4000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 2000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150061), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150063), false } })
        ({ Node_Event{ 496ull, 4000, true, ll_lower(51.25, 7.150064), true, ll_lower(51.25, 7.150064), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest multiple dispersed pre_events:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = { Node_Skeleton(496ull, ll_lower(51.25, 7.150067)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150063)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.150064)), 7000) };
    skels.first_appearance = { std::make_pair(Uint64(496ull), 3000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150061), Node_Skeleton(496ull, ll_lower(51.25, 7.150061)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150065), Node_Skeleton(496ull, ll_lower(51.25, 7.150065)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 5000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150066), Node_Skeleton(496ull, ll_lower(51.25, 7.150066)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 6000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150068), Node_Skeleton(496ull, ll_lower(51.25, 7.150068)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 8000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 2000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1], 3000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[2], 6000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[3], 7000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[4]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 496ull, 1000, 0 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 496ull, 1000, false, 0u, true, ll_lower(51.25, 7.150061), false } })
        ({ Node_Event{ 496ull, 2000, false, 0u, true, ll_lower(51.25, 7.150062), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.150063), true, ll_lower(51.25, 7.150063), false } })
        ({ Node_Event{ 496ull, 4000, true, ll_lower(51.25, 7.150064), true, ll_lower(51.25, 7.150064), false } })
        ({ Node_Event{ 496ull, 5000, true, ll_lower(51.25, 7.150064), true, ll_lower(51.25, 7.150065), false } })
        ({ Node_Event{ 496ull, 6000, true, ll_lower(51.25, 7.150064), true, ll_lower(51.25, 7.150066), false } })
        ({ Node_Event{ 496ull, 7000, true, ll_lower(51.25, 7.150067), true, ll_lower(51.25, 7.150067), false } })
        ({ Node_Event{ 496ull, 8000, true, ll_lower(51.25, 7.150067), true, ll_lower(51.25, 7.150068), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest pre_events on multiple objects:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(493ull, ll_lower(51.25, 7.150032)),
        Node_Skeleton(494ull, ll_lower(51.25, 7.150043)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.150061)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.150041)), 3000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.150051)), 3000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.150053)), 4000) };
    skels.first_appearance = {
        std::make_pair(Uint64(493ull), 2000),
        std::make_pair(Uint64(494ull), 1000),
        std::make_pair(Uint64(495ull), 1000),
        std::make_pair(Uint64(496ull), 1000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150031), Node_Skeleton(493ull, ll_lower(51.25, 7.150031)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(493ull, 1000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150042), Node_Skeleton(494ull, ll_lower(51.25, 7.150042)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(494ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150052), Node_Skeleton(495ull, ll_lower(51.25, 7.150052)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.150062), Node_Skeleton(496ull, ll_lower(51.25, 7.150062)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 2000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 2000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1], 3000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[2], 3000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[3]));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 493ull, 1000, 0 },
        Pre_Event_Ref{ 494ull, 2000, 1 },
        Pre_Event_Ref{ 495ull, 2000, 2 },
        Pre_Event_Ref{ 496ull, 2000, 3 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 493ull, 1000, false, 0u, true, ll_lower(51.25, 7.150031), false } })
        ({ Node_Event{ 493ull, 2000, true, ll_lower(51.25, 7.150032), true, ll_lower(51.25, 7.150032), false } })
        ({ Node_Event{ 494ull, 1000, true, ll_lower(51.25, 7.150041), true, ll_lower(51.25, 7.150041), false } })
        ({ Node_Event{ 494ull, 2000, true, ll_lower(51.25, 7.150041), true, ll_lower(51.25, 7.150042), false } })
        ({ Node_Event{ 494ull, 3000, true, ll_lower(51.25, 7.150043), true, ll_lower(51.25, 7.150043), false } })
        ({ Node_Event{ 495ull, 1000, true, ll_lower(51.25, 7.150051), true, ll_lower(51.25, 7.150051), false } })
        ({ Node_Event{ 495ull, 2000, true, ll_lower(51.25, 7.150051), true, ll_lower(51.25, 7.150052), false } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.150053), true, ll_lower(51.25, 7.150053), false } })
        ({ Node_Event{ 495ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150061), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150061), true, ll_lower(51.25, 7.150062), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest pre_events on unrelated idxs:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(495ull, ll_lower(51.25, 7.150053)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.150062)) };
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.150051)), 3000) };
    skels.first_appearance = {
        std::make_pair(Uint64(495ull), 1000),
        std::make_pair(Uint64(496ull), 2000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.550052), Node_Skeleton(495ull, ll_lower(51.25, 7.550052)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        ll_upper_(51.25, 7.550061), Node_Skeleton(496ull, ll_lower(51.25, 7.550061)),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1], 2000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 495ull, 2000, 0 },
        Pre_Event_Ref{ 496ull, 1000, 1 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 495ull, 1000, true, ll_lower(51.25, 7.150051), true, ll_lower(51.25, 7.150051), false } })
        ({ Node_Event{ 495ull, 2000, true, ll_lower(51.25, 7.150051), false, 0u, false } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.150053), true, ll_lower(51.25, 7.150053), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.150062), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest pre_events for deletion:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(495ull, ll_lower(51.25, 7.150053)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.150062)) };
    skels.attic = { Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.150051)), 3000) };
    skels.first_appearance = {
        std::make_pair(Uint64(495ull), 1000),
        std::make_pair(Uint64(496ull), 2000) };
    Data_By_Id< Node_Skeleton > data_by_id;
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        0u, Node_Skeleton(0ull, 0),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 2000)));
    data_by_id.data.push_back(Data_By_Id< Node_Skeleton >::Entry(
        0u, Node_Skeleton(0ull, 0),
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1000)));
    Pre_Event_List pre_events;
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[0], 3000));
    pre_events.data.push_back(Node_Pre_Event(data_by_id.data[1], 2000));
    Pre_Event_Refs pre_event_refs = {
        Pre_Event_Ref{ 495ull, 2000, 0 },
        Pre_Event_Ref{ 496ull, 1000, 1 } };

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 495ull, 1000, true, ll_lower(51.25, 7.150051), true, ll_lower(51.25, 7.150051), false } })
        ({ Node_Event{ 495ull, 2000, true, ll_lower(51.25, 7.150051), false, 0u, false } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.150053), true, ll_lower(51.25, 7.150053), false } })
        //({ Node_Event{ 496ull, 1000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.150062), true, ll_lower(51.25, 7.150062), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest exact match for multicoords:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(495ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(493ull, ll_lower(51.25, 7.15001)), 2000),
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15001)), 2000) };
    skels.first_appearance = {
        std::make_pair(Uint64(493ull), 1000),
        std::make_pair(Uint64(494ull), 1000),
        std::make_pair(Uint64(495ull), 3000),
        std::make_pair(Uint64(496ull), 3000) };
    Pre_Event_List pre_events;
    Pre_Event_Refs pre_event_refs;

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 493ull, 1000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 493ull, 2000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 494ull, 1000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 494ull, 2000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        (events.data);
  }
  {
    std::cerr<<"\nTest partial overlap for multicoords:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(495ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(492ull, ll_lower(51.25, 7.15)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(493ull, ll_lower(51.25, 7.15001)), 3000),
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15001)), 4000) };
    skels.first_appearance = {
        std::make_pair(Uint64(492ull), 1000),
        std::make_pair(Uint64(493ull), 1000),
        std::make_pair(Uint64(494ull), 2000),
        std::make_pair(Uint64(495ull), 3000),
        std::make_pair(Uint64(496ull), 5000) };
    Pre_Event_List pre_events;
    Pre_Event_Refs pre_event_refs;

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 492ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 492ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 492ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 493ull, 1000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), false } })
        ({ Node_Event{ 493ull, 2000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 493ull, 3000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 494ull, 2000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 494ull, 3000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), false } })
        ({ Node_Event{ 494ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 4000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 495ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        (events.data);
  }
  {
    std::cerr<<"\nTest inclusion for multicoords:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15001)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15001)), 3000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15)), 4000) };
    skels.first_appearance = {
        std::make_pair(Uint64(494ull), 1000),
        std::make_pair(Uint64(495ull), 2000),
        std::make_pair(Uint64(496ull), 2000) };
    Pre_Event_List pre_events;
    Pre_Event_Refs pre_event_refs;

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 494ull, 1000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), false } })
        ({ Node_Event{ 494ull, 2000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 494ull, 3000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), false } })
        ({ Node_Event{ 494ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 2000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 4000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        (events.data);
  }
  {
    std::cerr<<"\nTest gap for multicoords:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(495ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15)), 2000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15)), 6000) };
    skels.undeleted = {
        Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 3000),
        Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 7000) };
    skels.first_appearance = {
        std::make_pair(Uint64(494ull), 1000),
        std::make_pair(Uint64(495ull), 1000),
        std::make_pair(Uint64(496ull), 5000) };
    Pre_Event_List pre_events;
    Pre_Event_Refs pre_event_refs;

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 494ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 494ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 494ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 494ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 2000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 4000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 495ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 6000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 7000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 6000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 7000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        (events.data);
  }
  {
    std::cerr<<"\nTest ladder for multicoords:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(495ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(493ull, ll_lower(51.25, 7.15)), 2000),
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(496ull, ll_lower(51.25, 7.15)), 3000) };
    skels.first_appearance = {
        std::make_pair(Uint64(493ull), 1000),
        std::make_pair(Uint64(494ull), 2000),
        std::make_pair(Uint64(495ull), 4000),
        std::make_pair(Uint64(496ull), 1000) };
    Pre_Event_List pre_events;
    Pre_Event_Refs pre_event_refs;

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 493ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 493ull, 2000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 494ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 494ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 494ull, 4000, false, 0u, false, 0u, false } })
        ({ Node_Event{ 495ull, 4000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 4000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        (events.data);
  }
  {
    std::cerr<<"\nTest multiplicity greater two for multicoords:\n";
    Node_Skeletons_Per_Idx skels;
    skels.current = {
        Node_Skeleton(494ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(495ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(496ull, ll_lower(51.25, 7.15)),
        Node_Skeleton(497ull, ll_lower(51.25, 7.15001)),
        Node_Skeleton(498ull, ll_lower(51.25, 7.15001)) };
    skels.attic = {
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15001)), 5000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15)), 4000),
        Attic< Node_Skeleton >(Node_Skeleton(495ull, ll_lower(51.25, 7.15001)), 5000) };
    skels.first_appearance = {
        std::make_pair(Uint64(494ull), 2000),
        std::make_pair(Uint64(495ull), 3000),
        std::make_pair(Uint64(496ull), 1000),
        std::make_pair(Uint64(497ull), 5000),
        std::make_pair(Uint64(498ull), 1000) };
    Pre_Event_List pre_events;
    Pre_Event_Refs pre_event_refs;

    Node_Event_List events(ll_upper_(51.25, 7.15), skels, pre_event_refs, pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Node_Event >("data")
        ({ Node_Event{ 494ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 494ull, 4000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 494ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 3000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 495ull, 4000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 495ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 1000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 2000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 496ull, 4000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), false } })
        ({ Node_Event{ 496ull, 5000, true, ll_lower(51.25, 7.15), true, ll_lower(51.25, 7.15), true } })
        ({ Node_Event{ 497ull, 5000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 498ull, 1000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), false } })
        ({ Node_Event{ 498ull, 4000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        ({ Node_Event{ 498ull, 5000, true, ll_lower(51.25, 7.15001), true, ll_lower(51.25, 7.15001), true } })
        (events.data);
  }

  return 0;
}
