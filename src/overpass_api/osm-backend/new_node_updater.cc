#include <map>
#include <set>
#include <vector>


//TODO: Read files nodes, nodes_attic, nodes_meta, nodes_meta_attic


// Creates the set of deletions and additions for a single index for the current nodes file
/* No preconditions
 */
void create_update_for_nodes(
    const Event_List& events, std::set< Node_Skeleton >& to_delete, std::set< Node_Skeleton >& to_insert);
/* Assertions:
 * - for each latest event n per id i with (n.visible_before, n.coord_before) != (n.visible_after, n.coord_after) there is an entry in to_delete if n.visible_before and an entry in to_insert if n.visible_after
 * - no other entries exist in to_delete or to_insert
 */


// Creates the set of deletions and additions for a single index for the current nodes file
/* No preconditions
 */
void create_update_for_nodes_attic(
    const Event_List& events, std::set< Attic< Node_Skeleton > >& to_delete, std::set< Attic< Node_Skeleton > >& to_insert);
/* Assertions:
 * - for each but the latest event n per id i with m the following event, (n.visible_before, n.coord_before) != (n.visible_after, n.coord_after):
 * -- if n.visible_before and (n.visible_before, n.coord_before) != (m.visible_before, m.coord_before) then there is an entry in to_delete
 * -- if n.visible_after and (n.visible_after, n.coord_after) != (m.visible_after, m.coord_after) then there is an entry in to_insert
 * - no other entries exist in to_delete or to_insert
 */


// Creates the set of deletions and additions for a single index for the current nodes file
/* No preconditions
 */
void create_update_for_nodes_undelete(
    const Event_List& events, std::set< Attic< Node_Skeleton::Id_Type > >& to_delete, std::set< Attic< Node_Skeleton::Id_Type > >& to_insert);
/* Assertions:
 * - for each but the latest event n per id i with m the following event, !n.visible_before, n.visible_after, m.visible_before, m.visible_after and there exists an earlier event l with l.visible_before there is an entry in to_delete
 * - for each but the latest event n per id i with m the following event, !n.visible_after, m.visible_after and there exists an earlier event l with l.visible_after and (n.visible_before || !m.visible_before) there is an entry in to_insert
 * - no other entries exist in to_delete or to_insert
 */


// Nodes_Delta nodes_delta(Event_List)
// is feasible from the event list, using event_date, id, old_cord, new_coord, and new_mult

// void amend_nodes_dict(Event_List, Nodes_Dict&)
// is feasible from the event list, using event_date, id, new_coord, and new_mult


template< typename Index, typename Object >
File_Handle
{
  File_Handle(const File_Blocks_Index_Base* data_index, const auto& req)
      : db(data_index), it(db.discrete_begin(req.begin(), req.end()), end(db.discrete_end()) {}

  std::vector< Object > obj_with_idx(Index idx);

private:
  Block_Backend< Index, Object, typename std::vector< Index >::const_iterator > db;
  typename Block_Backend< Index, Object >::Discrete_Iterator it;
  typename Block_Backend< Index, Object >::Discrete_Iterator end;
};


void update_nodes(Transaction& transaction, const Data_From_Osc& new_data)
{
  // before the first pass by idx
  Mapfile_IO mapfile_io;
  std::map< Uint31_Index, Id_Dates_Per_Idx > id_dates_by_idx =
      mapfile_io.read_idx_list(new_data.node_id_dates());
  auto req = idx_list(id_dates_by_idx);

  std::map< Uint31_Index, Node_Skeletons_Per_Idx > skels_per_idx;
  Pre_Event_List pre_events = new_data.node_pre_events(); //TODO: deletion idxs vorbesetzen
  std::map< Uint31_Index, Coord_Dates_Per_Idx > coord_dates = new_data.node_coord_dates();

  File_Handle< Uint31_Index, Node_Skeleton > nodes_bin(
      transaction.data_index(osm_base_settings().NODES), req);
  File_Handle< Uint31_Index, Node_Skeleton > nodes_attic_bin(
      transaction.data_index(attic_settings().NODES), req);
  File_Handle< Uint31_Index, Node_Skeleton > nodes_meta_bin(
      transaction.data_index(meta_settings().NODES_META), req);
  File_Handle< Uint31_Index, Node_Skeleton > nodes_meta_bin(
      transaction.data_index(attic_settings().NODES_META), req);
  File_Handle< Uint31_Index, Node_Skeleton > nodes_undeleted_bin(
      transaction.data_index(attic_settings().NODES_UNDELETED), req);

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
      nodes_meta_to_move_to_attic;

  // first pass by idx
  for (auto i_idx : id_dates_by_idx)
  {
    Uint31_Index working_idx = i_idx->first;
    Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];
    Coord_Dates_Per_Idx& coord_dates_per_idx = coord_dates[working_idx];

    std::vector< Node_Skeleton > current_nodes = nodes_bin.obj_with_idx(working_idx);
    std::vector< Attic< Node_Skeleton > > attic_nodes = nodes_attic_bin.obj_with_idx(working_idx);

    collect_relevant_coords_current(i_idx->second, current_nodes, coord_dates_per_idx);
    collect_relevant_coords_attic(i_idx->second, attic_nodes, coord_dates_per_idx);

    Id_Dates_Per_Idx coord_sharing_ids;
    extract_relevant_current(i_idx->second, coord_sharing_ids, coord_dates_per_idx, current_nodes).swap(skels.current);
    extract_relevant_attic(i_idx->second, coord_sharing_ids, coord_dates_per_idx, attic_nodes).swap(skels.attic);

    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > current_meta =
        nodes_meta_bin.obj_with_idx(working_idx);
    std::vector< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > attic_meta =
        nodes_attic_meta_bin.obj_with_idx(working_idx);

    extract_first_appearance(i_idx->second, coord_sharing_ids, current_meta, attic_meta)
        .swap(skels.first_appearance);
    extract_relevant_undeleted(i_idx->second, coord_sharing_ids, nodes_undeleted_bin.obj_with_idx(working_idx))
        .swap(skels.undeleted);

    adapt_pre_event_list(current_meta, pre_events);
    adapt_pre_event_list(attic_meta, pre_events);

    collect_current_meta_to_move(pre_events, current_meta, nodes_meta_to_move_to_attic[working_idx]);
  }

  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
      nodes_meta_to_add;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > >
      nodes_attic_meta_to_add;
  create_update_for_nodes_meta(
      pre_events, nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);

  // compute nodes_map, nodes_attic_map and nodes_idx_lists from id_dates_by_idx, nodes_meta_to_add, and nodes_attic_meta_to_add
  // write meta, nodes_map, nodes_attic_map, nodes_idx_lists
  mapfile_io.compute_and_write_idx_lists(nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);
  update_elements(nodes_meta_to_move_to_attic, nodes_meta_to_add, *transaction, *meta_settings().NODES_META);
  update_elements(
      decltype(nodes_attic_meta_to_add)(), nodes_attic_meta_to_add, *transaction, *attic_settings().NODES_META);

  std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_delete;
  std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_add;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_delete;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_add;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_delete;
  std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_add;

  // second pass by idx
  for (auto i_idx : id_dates_by_idx)
  {
    Uint31_Index working_idx = i_idx->first;
    Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];

    Event_List events(skels, pre_events);
    create_update_for_nodes(
        events, nodes_to_delete[working_idx], nodes_to_add[working_idx]);
    create_update_for_nodes_attic(
        events, nodes_attic_to_delete[working_idx], nodes_attic_to_add[working_idx]);
    create_update_for_nodes_undelete(
        events, nodes_undelete_to_delete[working_idx], nodes_undelete_to_add[working_idx]);
  }

  // write nodes, nodes_attic, nodes_undelete
  update_elements(nodes_to_delete, nodes_to_add, *transaction, *osm_base_settings().NODES);
  update_elements(nodes_attic_to_delete, nodes_attic_to_add, *transaction, *attic_settings().NODES);
  update_elements(
      nodes_undelete_to_delete, nodes_undelete_to_add, *transaction, *attic_settings().NODES_UNDELETED);

  //TODO: tags, nodes_for_ways
}
