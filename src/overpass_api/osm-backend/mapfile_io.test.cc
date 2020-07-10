#include "mapfile_io.h"
#include "test_tools.h"


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" db_dir\n";
    return 0;
  }
  std::string db_dir = args[1];

  remove((db_dir + "/nodes.map").c_str());
  remove((db_dir + "/nodes.map.idx").c_str());
  remove((db_dir + "/nodes_attic.map").c_str());
  remove((db_dir + "/nodes_attic.map.idx").c_str());
  remove((db_dir + "/node_attic_indexes.bin").c_str());
  remove((db_dir + "/node_attic_indexes.bin.idx").c_str());

  {
    std::cerr<<"\nPrepare some data.\n";
    Nonsynced_Transaction transaction(true, false, db_dir, "");
    Mapfile_IO mapfile_io(transaction);

    std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > id_dates;
    id_dates.push_back(std::make_pair(Uint64(494ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(495ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(496ull), 1200));
 
    Compare_Map< Uint31_Index, Id_Dates_Per_Idx >("read_idx_list")
        (mapfile_io.read_idx_list(id_dates));

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_current;
    new_current[ll_upper_(51.25, 7.45)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(494, 1104));
    new_current[ll_upper_(51.25, 7.55)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495, 1105));
    new_current[ll_upper_(51.25, 7.65)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496, 1106));

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_attic;
    new_attic[ll_upper_(51.25, 7.55)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495, 1005));
    new_attic[ll_upper_(51.35, 7.65)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496, 1006));

    mapfile_io.compute_and_write_idx_lists(
        std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >(),
        new_current, new_attic);
  }

  {
    std::cerr<<"\nTest single current and attic indices:\n";
    Nonsynced_Transaction transaction(true, false, db_dir, "");
    Mapfile_IO mapfile_io(transaction);

    std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > id_dates;
    id_dates.push_back(std::make_pair(Uint64(493ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(494ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(495ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(496ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(497ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(498ull), 1200));

    Compare_Map< Uint31_Index, Id_Dates_Per_Idx >("read_idx_list")
        (ll_upper_(51.25, 7.45), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(494ull), 1200)))
        (ll_upper_(51.25, 7.55), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(495ull), 1200)))
        (ll_upper_(51.25, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.35, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (mapfile_io.read_idx_list(id_dates));

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > moved;
    moved[ll_upper_(51.25, 7.55)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(495, 1105));
    moved[ll_upper_(51.25, 7.65)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496, 1106));

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_current;
    new_current[ll_upper_(51.15, 7.65)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496, 1206));
    new_current[ll_upper_(51.25, 7.75)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497, 1207));
    new_current[ll_upper_(51.25, 7.85)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(498, 1208));

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_attic;
    new_attic[ll_upper_(51.35, 7.75)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497, 1207));
    new_attic[ll_upper_(51.45, 7.75)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497, 1207));
    new_attic[ll_upper_(51.35, 7.85)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(498, 1208));
    new_attic[ll_upper_(51.45, 7.85)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(498, 1208));

    mapfile_io.compute_and_write_idx_lists(
        moved, new_current, new_attic);
  }

  {
    std::cerr<<"\nTest index list creation:\n";
    Nonsynced_Transaction transaction(true, false, db_dir, "");
    Mapfile_IO mapfile_io(transaction);

    std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > id_dates;
    id_dates.push_back(std::make_pair(Uint64(495ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(496ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(497ull), 1200));

    Compare_Map< Uint31_Index, Id_Dates_Per_Idx >("read_idx_list")
        (ll_upper_(51.25, 7.55), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(495ull), 1200)))
        (ll_upper_(51.15, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.25, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.35, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.25, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (ll_upper_(51.35, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (ll_upper_(51.45, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (mapfile_io.read_idx_list(id_dates));

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > moved;
    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_current;

    std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > > new_attic;
    new_attic[ll_upper_(51.25, 7.65)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496, 1306));
    new_attic[ll_upper_(51.35, 7.65)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(496, 1306));
    new_attic[ll_upper_(51.35, 7.75)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497, 1307));
    new_attic[ll_upper_(51.55, 7.75)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(497, 1307));
    new_attic[ll_upper_(51.55, 7.85)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(498, 1308));
    new_attic[ll_upper_(51.65, 7.85)].insert(
        OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type >(498, 1308));

    mapfile_io.compute_and_write_idx_lists(
        moved, new_current, new_attic);
  }

  {
    std::cerr<<"\nTest index list updates:\n";
    Nonsynced_Transaction transaction(true, false, db_dir, "");
    Mapfile_IO mapfile_io(transaction);

    std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > > id_dates;
    id_dates.push_back(std::make_pair(Uint64(496ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(497ull), 1200));
    id_dates.push_back(std::make_pair(Uint64(498ull), 1200));

    Compare_Map< Uint31_Index, Id_Dates_Per_Idx >("read_idx_list")
        (ll_upper_(51.15, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.25, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.35, 7.65), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(496ull), 1200)))
        (ll_upper_(51.25, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (ll_upper_(51.35, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (ll_upper_(51.45, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (ll_upper_(51.55, 7.75), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(497ull), 1200)))
        (ll_upper_(51.25, 7.85), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(498ull), 1200)))
        (ll_upper_(51.35, 7.85), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(498ull), 1200)))
        (ll_upper_(51.45, 7.85), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(498ull), 1200)))
        (ll_upper_(51.55, 7.85), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(498ull), 1200)))
        (ll_upper_(51.65, 7.85), std::vector< std::pair< Node_Skeleton::Id_Type, uint64_t > >(1,
            std::make_pair(Uint64(498ull), 1200)))
        (mapfile_io.read_idx_list(id_dates));
  }

  return 0;
}
