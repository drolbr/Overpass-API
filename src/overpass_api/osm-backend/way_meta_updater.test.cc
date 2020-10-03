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

  return 0;
}
