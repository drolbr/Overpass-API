#include "test_tools.h"
#include "update_events_preparer.h"


int main(int argc, char* args[])
{
  {
    std::cerr<<"Test empty input:\n";
    bool all_ok = true;
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >("extract_first_appearance")
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs(), Id_Dates(),
            std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(),
            std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
    all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("extract_relevant_undeleted")
        (Update_Events_Preparer::extract_relevant_undeleted(
            Pre_Event_Refs(), Id_Dates(), std::vector< Attic< Node_Skeleton::Id_Type > >()));
  }
  {
    std::cerr<<"\nTest empty results for extract_first_appearance:\n";
    bool all_ok = true;
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("id_dates_and_coord_sharing_ids")
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1000, 0 } }),
            Id_Dates({ std::make_pair(Uint64(497ull), 1000) }),
            std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >(),
            std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("current_and_attic")
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs(), Id_Dates(),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1100) }));
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("no_match")
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1000, 0 } }),
            Id_Dates({ std::make_pair(Uint64(497ull), 1000) }),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(494ull, 1200) },
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1100) }));
  }
  {
    std::cerr<<"\nTest whether extract_first_appearance finds the earliest result:\n";
    bool all_ok = true;
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("single_item")
        (std::make_pair(Uint64(496ull), 1200))
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1300, 0 } }), Id_Dates(),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
            std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("single_item_multiple_timestamps_1")
        (std::make_pair(Uint64(496ull), 1100))
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1300, 0 } }), Id_Dates(),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1100) }));
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("single_item_multiple_timestamps_2")
        (std::make_pair(Uint64(496ull), 1050))
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1300, 0 } }), Id_Dates(),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1200) },
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1050),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1100) }));
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("multiple_items")
        (std::make_pair(Uint64(495ull), 1055))
        (std::make_pair(Uint64(496ull), 1206))
        (std::make_pair(Uint64(497ull), 1057))
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs(
                { Pre_Event_Ref{ Uint64(495ull), 1300, 0 },
                  Pre_Event_Ref{ Uint64(496ull), 1300, 0 },
                  Pre_Event_Ref{ Uint64(497ull), 1300, 0 } }),
            Id_Dates(),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1205),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1206),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1207) },
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1055),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1105),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1057),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1107) }));
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("id_dates_and_coord_sharing_ids")
        (std::make_pair(Uint64(495ull), 1205))
        (std::make_pair(Uint64(496ull), 1206))
        (std::make_pair(Uint64(497ull), 1207))
        (Update_Events_Preparer::extract_first_appearance(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1000, 0 } }),
            Id_Dates({ std::make_pair(Uint64(495ull), 1000), std::make_pair(Uint64(497ull), 1000) }),
            { OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495ull, 1205),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496ull, 1206),
              OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497ull, 1207) },
            std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >()));
  }
  {
    std::cerr<<"\nTest extract_relevant_undeleted:\n";
    bool all_ok = true;
    all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("id_dates")
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006))
        (Update_Events_Preparer::extract_relevant_undeleted(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(493ull), 1300, 0 }, Pre_Event_Ref{ Uint64(496ull), 1300, 0 } }),
            Id_Dates(),
            { Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004),
              Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005),
              Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006) } ));
    all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("coord_sharing_node_ids")
        (Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005))
        (Update_Events_Preparer::extract_relevant_undeleted(
            Pre_Event_Refs(), Id_Dates({ std::make_pair(Uint64(495ull), 1300) }),
            { Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004),
              Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005),
              Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006) } ));
    all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("both")
        (Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004))
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006))
        (Update_Events_Preparer::extract_relevant_undeleted(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1300, 0 } }),
            Id_Dates({ std::make_pair(Uint64(494ull), 1300) }),
            { Attic< Node_Skeleton::Id_Type >(Uint64(494ull), 1004),
              Attic< Node_Skeleton::Id_Type >(Uint64(495ull), 1005),
              Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1006) } ));
    all_ok &= Compare_Vector< Attic< Node_Skeleton::Id_Type > >("multiple_undeletes_per_id")
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1100))
        (Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1200))
        (Update_Events_Preparer::extract_relevant_undeleted(
            Pre_Event_Refs({ Pre_Event_Ref{ Uint64(496ull), 1300, 0 } }), Id_Dates(),
            { Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1100),
              Attic< Node_Skeleton::Id_Type >(Uint64(496ull), 1200) } ));
  }

  return 0;
}
