#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__MAPFILE_IO_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__MAPFILE_IO_H


#include "../../template_db/transaction.h"
#include "../core/datatypes.h"
#include "../data/filenames.h"
#include "new_basic_updater.h"

#include <cstdint>
#include <map>
#include <vector>


template< typename Skeleton >
class Mapfile_IO
{
public:
  Mapfile_IO(Transaction& transaction_) : transaction(transaction_) {}

  /* Precondition:
   * id_dates are ordered by id, and for no id exists more than one entry
   */
  void read_idx_list(
      const std::vector< Pre_Event_Ref< typename Skeleton::Id_Type > >& pre_event_refs,
      std::map< Uint31_Index, std::vector< Pre_Event_Ref< typename Skeleton::Id_Type > > >& append_to);

  /* Precondition:
   * read_idx_list must have been called before,
   * and all ids appearing here must have been included there.
   */
  void compute_and_write_idx_lists(
      const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > >& nodes_meta_to_move_to_attic,
      const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > >& nodes_meta_to_add,
      const std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< typename Skeleton::Id_Type > > >& nodes_attic_meta_to_add);

private:
  Transaction& transaction;
  std::map< typename Skeleton::Id_Type, std::set< Uint31_Index > > existing_idx_lists;
  std::vector< std::pair< typename Skeleton::Id_Type, Uint31_Index > > existing_attic_idxs;
};


template class Mapfile_IO< Node_Skeleton >;
template class Mapfile_IO< Way_Skeleton >;


#endif
