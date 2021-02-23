#include "test_tools.h"
#include "update_events_preparer.h"


bool operator==(const Way_Implicit_Pre_Event& lhs, const Way_Implicit_Pre_Event& rhs)
{
  return lhs.base.id == rhs.base.id && lhs.base.nds == rhs.base.nds && lhs.base.geometry == rhs.base.geometry
      && lhs.not_before == rhs.not_before && lhs.before == rhs.before && lhs.pos_events == rhs.pos_events;
}


Quad_Coord make_quad(double lat, double lon)
{
  return { ll_upper_(lat, lon), ll_lower(lat, lon) };
}


Way_Skeleton way_496(double lon_offset_1 = 0)
{
  return Way_Skeleton{ 496u, { 496001ull, 0ull },
      { make_quad(51.25, 7.150001 + lon_offset_1), make_quad(51.25, 7.150002) } };
}


Way_Skeleton way_49x(Way_Skeleton::Id_Type id)
{
  return Way_Skeleton{ id, { 496001ull, 0ull }, { make_quad(51.25, 7.150001), make_quad(51.25, 7.150002) } };
}


Move_Coord_Event make_moved_coord_event_496002(uint64_t timestamp)
{
  return {
      ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002),
      timestamp, 496002ull,
      ll_upper_(51.25, 7.15), ll_lower(51.25, 7.1500021),
      true, false };
}


int main(int argc, char* args[])
{
  {
    std::cerr<<"\nTest empty input:\n";
    bool all_ok = true;
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >("extract_first_appearance(Node ..)")
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs(), Node_Id_Dates(),
//             std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(),
//             std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >("extract_first_appearance(Way ..)")
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs(), std::vector< Way_Implicit_Pre_Event >(),
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("extract_relevant_undeleted")
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Node_Pre_Event_Refs(), Node_Id_Dates(), std::vector< Attic< Node_Skeleton::Id_Type > >()));
// 
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events;
    Update_Events_Preparer::prune_nonexistant_events(
        std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >{}, Pre_Event_List< Way_Skeleton >{},
        implicit_pre_events);
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (implicit_pre_events);
  }
//   {
//     std::cerr<<"\nTest empty results for extract_first_appearance(Node ..):\n";
//     bool all_ok = true;
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("id_dates_and_coord_sharing_ids")
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1000, 0 } }),
//             Node_Id_Dates({ std::make_pair(Uint64(497ull), 1000) }),
//             std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(),
//             std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("current_and_attic")
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs(), Node_Id_Dates(),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1100) }));
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("no_match")
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1000, 0 } }),
//             Node_Id_Dates({ std::make_pair(Uint64(497ull), 1000) }),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(494ull, 1200) },
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1100) }));
//   }
//   {
//     std::cerr<<"\nTest empty results for extract_first_appearance(Way ..):\n";
//     bool all_ok = true;
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("id_dates_and_coord_sharing_ids")
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496u), 1000, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >{
//                 Way_Implicit_Pre_Event{
//                     497u, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } },
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >(),
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("current_and_attic")
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs(), std::vector< Way_Implicit_Pre_Event >(),
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496u, 1200) },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496u, 1100) }));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("no_match")
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496u), 1000, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >{
//                 Way_Implicit_Pre_Event{
//                     497u, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(494u, 1200) },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(495u, 1100) }));
//   }
//   {
//     std::cerr<<"\nTest whether extract_first_appearance(Node ..) finds the earliest result:\n";
//     bool all_ok = true;
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("single_item")
//         (std::make_pair(Uint64(496ull), 1200))
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 } }), Node_Id_Dates(),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
//             std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("single_item_multiple_timestamps_1")
//         (std::make_pair(Uint64(496ull), 1100))
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 } }), Node_Id_Dates(),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1100) }));
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("single_item_multiple_timestamps_2")
//         (std::make_pair(Uint64(496ull), 1050))
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 } }), Node_Id_Dates(),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1050),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1100) }));
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("multiple_items")
//         (std::make_pair(Uint64(495ull), 1055))
//         (std::make_pair(Uint64(496ull), 1206))
//         (std::make_pair(Uint64(497ull), 1057))
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs(
//                 { Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(495ull), 1300, 0 },
//                   Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 },
//                   Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(497ull), 1300, 0 } }),
//             Node_Id_Dates(),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1205),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1206),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1207) },
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1055),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1105),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1057),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1107) }));
//     all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
//         ("id_dates_and_coord_sharing_ids")
//         (std::make_pair(Uint64(495ull), 1205))
//         (std::make_pair(Uint64(496ull), 1206))
//         (std::make_pair(Uint64(497ull), 1207))
//         (Update_Events_Preparer::extract_first_appearance(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1000, 0 } }),
//             Node_Id_Dates({ std::make_pair(Uint64(495ull), 1000), std::make_pair(Uint64(497ull), 1000) }),
//             { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1205),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1206),
//               OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1207) },
//             std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
//   }
//   {
//     std::cerr<<"\nTest whether extract_first_appearance(Way ..) finds the earliest result:\n";
//     bool all_ok = true;
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("single_item")
//         (std::make_pair(Uint32_Index(496ull), 1200))
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496ull), 1300, 0 } }), std::vector< Way_Implicit_Pre_Event >(),
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1200) },
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("single_item_multiple_timestamps_1")
//         (std::make_pair(Uint32_Index(496ull), 1100))
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496ull), 1300, 0 } }), std::vector< Way_Implicit_Pre_Event >(),
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1200) },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1100) }));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("single_item_multiple_timestamps_2")
//         (std::make_pair(Uint32_Index(496ull), 1050))
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496ull), 1300, 0 } }), std::vector< Way_Implicit_Pre_Event >(),
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1200) },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1050),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1100) }));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("multiple_items")
//         (std::make_pair(Uint32_Index(495ull), 1055))
//         (std::make_pair(Uint32_Index(496ull), 1206))
//         (std::make_pair(Uint32_Index(497ull), 1057))
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs(
//                 { Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(495ull), 1300, 0 },
//                   Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496ull), 1300, 0 },
//                   Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(497ull), 1300, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >(),
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(495ull, 1205),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1206),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(497ull, 1207) },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(495ull, 1055),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(495ull, 1105),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(497ull, 1057),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(497ull, 1107) }));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("id_dates_and_implicit_pre_events")
//         (std::make_pair(Uint32_Index(495ull), 1205))
//         (std::make_pair(Uint32_Index(496ull), 1206))
//         (std::make_pair(Uint32_Index(497ull), 1207))
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496ull), 1000, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >{
//                 Way_Implicit_Pre_Event{
//                     495ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() },
//                 Way_Implicit_Pre_Event{
//                     497ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(495ull, 1205),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1206),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(497ull, 1207) },
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >()));
//     all_ok &= Compare_Vector< std::pair< Way_Skeleton::Id_Type, uint64_t > >
//         ("multiple entries in implicit_pre_events")
//         (std::make_pair(Uint32_Index(493ull), 1203))
//         (std::make_pair(Uint32_Index(494ull), 1204))
//         (std::make_pair(Uint32_Index(495ull), 1205))
//         (std::make_pair(Uint32_Index(496ull), 1206))
//         (std::make_pair(Uint32_Index(497ull), 1207))
//         (Update_Events_Preparer::extract_first_appearance(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(495ull), 1000, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >{
//                 Way_Implicit_Pre_Event{
//                     493ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() },
//                 Way_Implicit_Pre_Event{
//                     493ull, 1100, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() },
//                 Way_Implicit_Pre_Event{
//                     494ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() },
//                 Way_Implicit_Pre_Event{
//                     496ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() },
//                 Way_Implicit_Pre_Event{
//                     496ull, 1100, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() },
//                 Way_Implicit_Pre_Event{
//                     497ull, 1000, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } },
//             { OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(493ull, 1203),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(494ull, 1204),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(495ull, 1205),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(496ull, 1206),
//               OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type >(497ull, 1207) },
//             std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > >()));
//   }
//   {
//     std::cerr<<"\nTest extract_relevant_undeleted(Node ..):\n";
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("id_dates")
//         (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(493ull), 1300, 0 }, Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 } }),
//             Node_Id_Dates(),
//             { Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004),
//               Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005),
//               Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006) } ));
//     all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("coord_sharing_node_ids")
//         (Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Node_Pre_Event_Refs(), Node_Id_Dates({ std::make_pair(Uint64(495ull), 1300) }),
//             { Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004),
//               Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005),
//               Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006) } ));
//     all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("both")
//         (Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004))
//         (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 } }),
//             Node_Id_Dates({ std::make_pair(Uint64(494ull), 1300) }),
//             { Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004),
//               Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005),
//               Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006) } ));
//     all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("multiple_undeletes_per_id")
//         (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1100))
//         (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1200))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Node_Pre_Event_Refs({ Pre_Event_Ref< Node_Skeleton::Id_Type >{ Uint64(496ull), 1300, 0 } }), Node_Id_Dates(),
//             { Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1100),
//               Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1200) } ));
//   }
//   {
//     std::cerr<<"\nTest extract_relevant_undeleted(Way ..):\n";
//     bool all_ok = true;
//     all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("id_dates")
//         (Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1006))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(493u), 1300, 0 }, Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496u), 1300, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >(),
//             { Attic< Way_Skeleton::Id_Type >(Uint32_Index(494u), 1004),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(495u), 1005),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1006) } ));
//     all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("coord_sharing_node_ids")
//         (Attic< Way_Skeleton::Id_Type >(Uint32_Index(495u), 1005))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Way_Pre_Event_Refs(),
//             std::vector< Way_Implicit_Pre_Event >{
//                 Way_Implicit_Pre_Event{
//                     495ull, 1300, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } },
//             { Attic< Way_Skeleton::Id_Type >(Uint32_Index(494u), 1004),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(495u), 1005),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1006) } ));
//     all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("both")
//         (Attic< Way_Skeleton::Id_Type >(Uint32_Index(494u), 1004))
//         (Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1006))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496u), 1300, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >{
//                 Way_Implicit_Pre_Event{
//                     494ull, 1300, std::vector< Quad_Coord >(), std::vector< Node::Id_Type >() } },
//             { Attic< Way_Skeleton::Id_Type >(Uint32_Index(494u), 1004),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(495u), 1005),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1006) } ));
//     all_ok &= Compare_Vector< Attic< Way_Skeleton::Id_Type > >("multiple_undeletes_per_id")
//         (Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1100))
//         (Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1200))
//         (Update_Events_Preparer::extract_relevant_undeleted(
//             Way_Pre_Event_Refs({ Pre_Event_Ref< Way_Skeleton::Id_Type >{ Uint32_Index(496u), 1300, 0 } }),
//             std::vector< Way_Implicit_Pre_Event >(),
//             { Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1100),
//               Attic< Way_Skeleton::Id_Type >(Uint32_Index(496u), 1200) } ));
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events without undel or pre_events:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >(),
//         Way_Id_Dates{ std::make_pair(Uint32_Index(496u), 1000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >(), test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (base_vector[0])
//         (test_vector);
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >(), Pre_Event_List< Way_Skeleton >(),
//         test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (base_vector[0])
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with irrelevant undel and pre_event:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 3000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >(),
//         Way_Id_Dates{ std::make_pair(Uint32_Index(496u), 1000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >{
//             Attic< Way_Skeleton::Id_Type >(way_496(), 2000) }, test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (base_vector[0])
//         (test_vector);
// 
//     std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 4000)));
//     Pre_Event_List< Way_Skeleton > pre_events;
//     for (auto& i : entries)
//       pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
//     std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 4000, 0 } };
//     Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (base_vector[0])
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with relevant undel:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >(),
//         Way_Id_Dates{ std::make_pair(Uint32_Index(496u), 1000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >{
//             Attic< Way_Skeleton::Id_Type >(way_496(), 3000) }, test_vector);
//     Way_Implicit_Pre_Event result = base_vector[0];
//     result.begin = 3000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (result)
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with relevant meta limit:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >(),
//         Way_Id_Dates{ std::make_pair(Uint32_Index(496u), 3000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >(), test_vector);
//     Way_Implicit_Pre_Event result = base_vector[0];
//     result.begin = 3000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (result)
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with event elimination:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 3000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(way_496(), 2000) };
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >{ &attic[0] },
//         Way_Id_Dates{ std::make_pair(Uint32_Index(496u), 1000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >{
//             Attic< Way_Skeleton::Id_Type >(way_496(), 3000) }, test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (base_vector[1])
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with long undel sequence:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 1000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 3000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 4000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 6000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 7000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 9000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150009)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 11000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150001)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 13000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 15000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150005)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 17000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 18000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150008)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     std::vector< Attic< Way_Skeleton > > attic = {
//         Attic< Way_Skeleton >(way_496(), 5000),
//         Attic< Way_Skeleton >(way_496(), 10000),
//         Attic< Way_Skeleton >(way_496(), 12000) };
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >{ &attic[0], &attic[1], &attic[2] },
//         Way_Id_Dates{ std::make_pair(Uint32_Index(496u), 2000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >{
//             Attic< Way_Skeleton::Id_Type >(way_496(), 8000),
//             Attic< Way_Skeleton::Id_Type >(way_496(), 14000),
//             Attic< Way_Skeleton::Id_Type >(way_496(), 16000) }, test_vector);
//     Way_Implicit_Pre_Event result_2 = base_vector[0];
//     result_2.begin = 2000;
//     Way_Implicit_Pre_Event result_8 = base_vector[4];
//     result_8.begin = 8000;
//     Way_Implicit_Pre_Event result_16 = base_vector[8];
//     result_16.begin = 16000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (result_2)
//         (base_vector[1])
//         (base_vector[2])
//         (result_8)
//         (base_vector[5])
//         (base_vector[6])
//         (result_16)
//         (base_vector[9])
//         (base_vector[10])
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events on multiple ids:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             493ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             494ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             495ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150005)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
//     Update_Events_Preparer::prune_nonexistant_events(
//         std::vector< const Attic< Way_Skeleton >* >(),
//         Way_Id_Dates{
//             std::make_pair(Uint32_Index(493u), 1000ull),
//             std::make_pair(Uint32_Index(494u), 3000ull),
//             std::make_pair(Uint32_Index(495u), 1000ull),
//             std::make_pair(Uint32_Index(496u), 1000ull) },
//         std::vector< Attic< Way_Skeleton::Id_Type > >{
//             Attic< Way_Skeleton::Id_Type >(way_496(), 3000) }, test_vector);
//     Way_Implicit_Pre_Event result_4 = base_vector[1];
//     result_4.begin = 3000;
//     Way_Implicit_Pre_Event result_6 = base_vector[3];
//     result_6.begin = 3000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::undel_meta")
//         (base_vector[0])
//         (result_4)
//         (base_vector[2])
//         (result_6)
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with relevant open end pre_event:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 3000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
// 
//     std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
//     Pre_Event_List< Way_Skeleton > pre_events;
//     for (auto& i : entries)
//       pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
//     std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };
//     Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with relevant bounded pre_event:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 3000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
// 
//     std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
//     Pre_Event_List< Way_Skeleton > pre_events;
//     for (auto& i : entries)
//       pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
//     pre_events.data[0].timestamp_end = 4000;
//     std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };
//     Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, test_vector);
//     Way_Implicit_Pre_Event result = base_vector[0];
//     result.begin = 4000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (result)
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with event elimination:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 3000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
// 
//     std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000)));
//     Pre_Event_List< Way_Skeleton > pre_events;
//     for (auto& i : entries)
//       pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
//     pre_events.data[0].timestamp_end = 3000;
//     std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 1000, 0 } };
//     Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, test_vector);
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (base_vector[1])
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events with long pre_event sequence:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 7000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150007)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 9000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150009)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 10000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.15)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 12000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150002)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 14000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
// 
//     std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000)));
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 5000)));
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 6000)));
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 11000)));
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 13000)));
//     Pre_Event_List< Way_Skeleton > pre_events;
//     for (auto& i : entries)
//       pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
//     pre_events.data[0].timestamp_end = 3000;
//     pre_events.data[1].timestamp_end = 4000;
//     pre_events.data[2].timestamp_end = 6000;
//     pre_events.data[3].timestamp_end = 8000;
//     pre_events.data[4].timestamp_end = 13000;
//     std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 1000, 0 } };
//     Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, test_vector);
//     Way_Implicit_Pre_Event result_4 = base_vector[0];
//     result_4.begin = 4000;
//     Way_Implicit_Pre_Event result_8 = base_vector[1];
//     result_8.begin = 8000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (result_4)
//         (result_8)
//         (base_vector[2])
//         (base_vector[3])
//         (test_vector);
//   }
//   {
//     std::cerr<<"\nTest implicit_pre_events for multiple ids:\n";
//     bool all_ok = true;
//     std::vector< Way_Implicit_Pre_Event > base_vector = {
//         Way_Implicit_Pre_Event{
//             493ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150003)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             494ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150004)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             495ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150005)) }, std::vector< Node::Id_Type >() },
//         Way_Implicit_Pre_Event{
//             496ull, 2000, std::vector< Quad_Coord >{
//             Quad_Coord(ll_upper_(51.25, 7.15), ll_lower(51.25, 7.150006)) }, std::vector< Node::Id_Type >() } };
//     std::vector< Way_Implicit_Pre_Event > test_vector = base_vector;
// 
//     std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), Way_Skeleton(Uint32_Index(494u)),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(494ull, 1000)));
//     entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
//         ll_upper_(51.25, 7.15), way_496(),
//         OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000)));
//     Pre_Event_List< Way_Skeleton > pre_events;
//     for (auto& i : entries)
//       pre_events.data.push_back(Pre_Event< Way_Skeleton >(i));
//     pre_events.data[1].timestamp_end = 3000;
//     std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 494ull, 1000, 0 },
//         Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 1000, 1 } };
//     Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, test_vector);
//     Way_Implicit_Pre_Event result = base_vector[3];
//     result.begin = 3000;
//     all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::pre_events")
//         (base_vector[0])
//         (base_vector[2])
//         (result)
//         (test_vector);
//   }
  {
    std::cerr<<"\nTest with empty pre_events:\n";
    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 1000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(
        std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > >{}, Pre_Event_List< Way_Skeleton >{},
        implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 1000, NOW, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one earlier pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], 2000));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 1000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 3000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 3000, NOW, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one early overlapping pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], 3000));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 1000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 2000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 3000, NOW, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one fully overlapping pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 1000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 1000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 2000, 3000, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one late overlapping pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 1000, 3000, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 1000, 2000, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one indefinitely overlapping pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 1000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 1000, 2000, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one later pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 3000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 1000, 2000, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 1000, 2000, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with one in-between overlapping pre_event:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], 3000));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 1000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 1000, 2000, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_496(), 3000, NOW, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest one implicit_pre_event with multiple in-between overlapping pre_events:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 4000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 5000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], 3000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[1], 5000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[2], 6000));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(), 1000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(), 1000, 2000, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_496(), 3000, 4000, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_496(), 6000, NOW, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest multiple implicit_pre_events of the same id with one covering pre_events:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(1e-7), 1000, 3000, { {1, &move_coords[0]} } },
          Way_Implicit_Pre_Event{ way_496(3e-7), 3000, 4000, { {1, &move_coords[0]} } },
          Way_Implicit_Pre_Event{ way_496(7e-7), 7000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(1e-7), 1000, 2000, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest multiple implicit_pre_events of the same id with multiple in-between overlapping pre_events:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 5000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 6000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 8000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], 3000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[1], 6000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[2], 7000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[3], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_496(1e-7), 1000, 3000, { {1, &move_coords[0]} } },
          Way_Implicit_Pre_Event{ way_496(3e-7), 3000, 4000, { {1, &move_coords[0]} } },
          Way_Implicit_Pre_Event{ way_496(7e-7), 7000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_496(1e-7), 1000, 2000, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_496(3e-7), 3000, 4000, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_496(7e-7), 7000, 8000, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }
  {
    std::cerr<<"\nTest implicit_pre_events of multiple ids:\n";
    std::vector< Data_By_Id< Way_Skeleton >::Entry > entries;
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 2000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 3000)));
    entries.push_back(Data_By_Id< Way_Skeleton >::Entry(
        ll_upper_(51.25, 7.15), way_496(),
        OSM_Element_Metadata_Skeleton< Uint32_Index >(496ull, 4000)));
    Pre_Event_List< Way_Skeleton > pre_events;
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[0], 3000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[1], 4000));
    pre_events.data.push_back(Pre_Event< Way_Skeleton >(entries[2], NOW));
    std::vector< Pre_Event_Ref< Way_Skeleton::Id_Type > > pre_event_refs = {
        Pre_Event_Ref< Way_Skeleton::Id_Type >{ 496ull, 2000, 0 } };

    std::vector< Move_Coord_Event > move_coords = {
        make_moved_coord_event_496002(1000) };
    std::vector< Way_Implicit_Pre_Event > implicit_pre_events =
        { Way_Implicit_Pre_Event{ way_49x(495u), 1000, NOW, { {1, &move_coords[0]} } },
          Way_Implicit_Pre_Event{ way_49x(496u), 1000, 3000, { {1, &move_coords[0]} } },
          Way_Implicit_Pre_Event{ way_49x(497u), 3000, NOW, { {1, &move_coords[0]} } } };
    Update_Events_Preparer::prune_nonexistant_events(pre_event_refs, pre_events, implicit_pre_events);
    bool all_ok = true;
    all_ok &= Compare_Vector< Way_Implicit_Pre_Event >("prune_nonexistant_events::implicit_pre_events")
        (Way_Implicit_Pre_Event{ way_49x(495u), 1000, NOW, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_49x(496u), 1000, 2000, { {1, &move_coords[0]} } })
        (Way_Implicit_Pre_Event{ way_49x(497u), 3000, NOW, { {1, &move_coords[0]} } })
        (implicit_pre_events);
  }

  return 0;
}
