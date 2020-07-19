#include "node_skeleton_updater.h"
#include "test_tools.h"


int main(int argc, char* args[])
{
  {
    std::cerr<<"Test empty input:\n";
    Id_Dates_Per_Idx id_dates;
    Coord_Dates_Per_Idx coord_result;
    bool all_ok = true;

    Node_Skeleton_Updater::collect_relevant_coords_current(
        id_dates, std::vector< Node_Skeleton >(), coord_result);
    all_ok &= Compare_Vector< Attic< Uint32 > >("collect_relevant_coords_current")
        (coord_result);

    Node_Skeleton_Updater::collect_relevant_coords_attic(
        id_dates, std::vector< Attic< Node_Skeleton > >(), coord_result);
    all_ok &= Compare_Vector< Attic< Uint32 > >("collect_relevant_coords_attic")
        (coord_result);

    Id_Dates_Per_Idx coord_sharing_ids;
    auto skel_result = Node_Skeleton_Updater::extract_relevant_current(
        id_dates, coord_sharing_ids, Coord_Dates_Per_Idx(), std::vector< Node_Skeleton >());
    all_ok &= Compare_Vector< Node_Skeleton >("extract_relevant_current")
        (skel_result);
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("extract_relevant_current.coord_sharing_ids")
        (coord_sharing_ids);

    auto attic_result = Node_Skeleton_Updater::extract_relevant_attic(
        id_dates, coord_sharing_ids, Coord_Dates_Per_Idx(), std::vector< Attic< Node_Skeleton > >());
    all_ok &= Compare_Vector< Attic< Node_Skeleton > >("extract_relevant_attic")
        (attic_result);
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("extract_relevant_attic.coord_sharing_ids")
        (coord_sharing_ids);
  }
  {
    std::cerr<<"\nTest single results:\n";
    Id_Dates_Per_Idx id_dates =
        { std::make_pair(Uint64(494ull), 1004ull),
          std::make_pair(Uint64(496ull), 1006ull) };
    Coord_Dates_Per_Idx coord_result =
        { Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15001)), 1001),
          Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15009)), 1009) };
    bool all_ok = true;

    std::vector< Node_Skeleton > current_skels =
        { Node_Skeleton(496ull, ll_lower(51.25, 7.15006)),
          Node_Skeleton(1000496ull, ll_lower(51.25, 7.15006)),
          Node_Skeleton(495ull, ll_lower(51.25, 7.15005)) };
    Node_Skeleton_Updater::collect_relevant_coords_current(
        id_dates, current_skels, coord_result);
    all_ok &= Compare_Vector< Attic< Uint32 > >("collect_relevant_coords_current")
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15001)), 1001))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15006)), 1006))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15009)), 1009))
        (coord_result);

    std::vector< Attic< Node_Skeleton > > attic_skels;
    attic_skels.push_back(Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15004)), 1904));
    attic_skels.push_back(Attic< Node_Skeleton >(Node_Skeleton(1000494ull, ll_lower(51.25, 7.15004)), 1904));
    Node_Skeleton_Updater::collect_relevant_coords_attic(
        id_dates, attic_skels, coord_result);
    all_ok &= Compare_Vector< Attic< Uint32 > >("collect_relevant_coords_attic")
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15001)), 1001))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15004)), 1004))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15006)), 1006))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15009)), 1009))
        (coord_result);

    Id_Dates_Per_Idx coord_sharing_ids;
    auto skel_result = Node_Skeleton_Updater::extract_relevant_current(
        id_dates, coord_sharing_ids, coord_result, current_skels);
    all_ok &= Compare_Vector< Node_Skeleton >("extract_relevant_current")
        (Node_Skeleton(496ull, ll_lower(51.25, 7.15006)))
        (Node_Skeleton(1000496ull, ll_lower(51.25, 7.15006)))
        (skel_result);
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("extract_relevant_current.coord_sharing_ids")
        (std::make_pair(Uint64(1000496ull), 1006ull))
        (coord_sharing_ids);

    auto attic_result = Node_Skeleton_Updater::extract_relevant_attic(
        id_dates, coord_sharing_ids, coord_result, attic_skels);
    all_ok &= Compare_Vector< Attic< Node_Skeleton > >("extract_relevant_attic")
        (Attic< Node_Skeleton >(Node_Skeleton(494ull, ll_lower(51.25, 7.15004)), 1904))
        (Attic< Node_Skeleton >(Node_Skeleton(1000494ull, ll_lower(51.25, 7.15004)), 1904))
        (attic_result);
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("extract_relevant_attic.coord_sharing_ids")
        (std::make_pair(Uint64(1000494ull), 1004ull))
        (std::make_pair(Uint64(1000496ull), 1006ull))
        (coord_sharing_ids);
  }
  {
    std::cerr<<"\nTest timestamp ordering:\n";
    Id_Dates_Per_Idx id_dates =
        { std::make_pair(Uint64(10491ull), 1601ull),
          std::make_pair(Uint64(20491ull), 1501ull),
          std::make_pair(Uint64(10492ull), 1402ull),
          std::make_pair(Uint64(30492ull), 1302ull),
          std::make_pair(Uint64(10493ull), 1403ull),
          std::make_pair(Uint64(20493ull), 1603ull),
          std::make_pair(Uint64(10494ull), 1404ull),
          std::make_pair(Uint64(20494ull), 1604ull) };
    Coord_Dates_Per_Idx coord_result =
        { Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15001)), 1400),
          Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15002)), 1600),
          Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15003)), 1500) };
    bool all_ok = true;

    std::vector< Node_Skeleton > current_skels =
        { Node_Skeleton(10491ull, ll_lower(51.25, 7.15001)),
          Node_Skeleton(20491ull, ll_lower(51.25, 7.15001)),
          Node_Skeleton(10492ull, ll_lower(51.25, 7.15002)),
          Node_Skeleton(20492ull, ll_lower(51.25, 7.15002)),
          Node_Skeleton(10493ull, ll_lower(51.25, 7.15003)),
          Node_Skeleton(20493ull, ll_lower(51.25, 7.15003)),
          Node_Skeleton(10494ull, ll_lower(51.25, 7.15004)),
          Node_Skeleton(20494ull, ll_lower(51.25, 7.15004)) };
    Node_Skeleton_Updater::collect_relevant_coords_current(
        id_dates, current_skels, coord_result);
    all_ok &= Compare_Vector< Attic< Uint32 > >("collect_relevant_coords_current")
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15001)), 1400))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15002)), 1402))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15003)), 1403))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15004)), 1404))
        (coord_result);

    std::vector< Attic< Node_Skeleton > > attic_skels;
    attic_skels.push_back(Attic< Node_Skeleton >(Node_Skeleton(20491ull, ll_lower(51.25, 7.15001)), 1511));
    attic_skels.push_back(Attic< Node_Skeleton >(Node_Skeleton(30492ull, ll_lower(51.25, 7.15002)), 1312));
    Node_Skeleton_Updater::collect_relevant_coords_attic(
        id_dates, attic_skels, coord_result);
    all_ok &= Compare_Vector< Attic< Uint32 > >("collect_relevant_coords_attic")
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15001)), 1400))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15002)), 1302))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15003)), 1403))
        (Attic< Uint32 >(Uint32(ll_lower(51.25, 7.15004)), 1404))
        (coord_result);

    Id_Dates_Per_Idx coord_sharing_ids;
    auto skel_result = Node_Skeleton_Updater::extract_relevant_current(
        id_dates, coord_sharing_ids, coord_result, current_skels);
    all_ok &= Compare_Vector< Node_Skeleton >("extract_relevant_current")
        (Node_Skeleton(10491ull, ll_lower(51.25, 7.15001)))
        (Node_Skeleton(20491ull, ll_lower(51.25, 7.15001)))
        (Node_Skeleton(10492ull, ll_lower(51.25, 7.15002)))
        (Node_Skeleton(20492ull, ll_lower(51.25, 7.15002)))
        (Node_Skeleton(10493ull, ll_lower(51.25, 7.15003)))
        (Node_Skeleton(20493ull, ll_lower(51.25, 7.15003)))
        (Node_Skeleton(10494ull, ll_lower(51.25, 7.15004)))
        (Node_Skeleton(20494ull, ll_lower(51.25, 7.15004)))
        (skel_result);
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("extract_relevant_current.coord_sharing_ids")
        (std::make_pair(Uint64(20492ull), 1302ull))
        (coord_sharing_ids);

    auto attic_result = Node_Skeleton_Updater::extract_relevant_attic(
        id_dates, coord_sharing_ids, coord_result, attic_skels);
    all_ok &= Compare_Vector< Attic< Node_Skeleton > >("extract_relevant_attic")
        (Attic< Node_Skeleton >(Node_Skeleton(20491ull, ll_lower(51.25, 7.15001)), 1511))
        (Attic< Node_Skeleton >(Node_Skeleton(30492ull, ll_lower(51.25, 7.15002)), 1312))
        (attic_result);
    all_ok &= Compare_Vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >
        ("extract_relevant_attic.coord_sharing_ids")
        (std::make_pair(Uint64(20492ull), 1302ull))
        (coord_sharing_ids);
  }

  return 0;
}
