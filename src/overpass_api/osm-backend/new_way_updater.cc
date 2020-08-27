// #include "data_from_osc.h"
// #include "mapfile_io.h"
// #include "new_node_updater.h"
// #include "node_event_list.h"
// #include "node_meta_updater.h"
// #include "node_skeleton_updater.h"
// #include "prepare_node_update.h"
// #include "update_events_preparer.h"
// 
// #include "../../template_db/block_backend.h"
// #include "../../template_db/transaction.h"
// 
// #include <map>
// #include <set>
// #include <vector>
// 
// 
// // Nodes_Delta nodes_delta(Event_List)
// // is feasible from the event list, using event_date, id, old_cord, new_coord, and new_mult
// 
// // void amend_nodes_dict(Event_List, Nodes_Dict&)
// // is feasible from the event list, using event_date, id, new_coord, and new_mult
// 
// 
// template< typename Index, typename Object >
// class File_Handle
// {
// public:
//   File_Handle(File_Blocks_Index_Base* data_index, const std::vector< Index >& req)
//       : db(data_index), it(db.discrete_begin(req.begin(), req.end())), end(db.discrete_end()) {}
// 
//   std::vector< Object > obj_with_idx(Index idx);
// 
// private:
//   Block_Backend< Index, Object, typename std::vector< Index >::const_iterator > db;
//   typename Block_Backend< Index, Object, typename std::vector< Index >::const_iterator >::Discrete_Iterator it;
//   typename Block_Backend< Index, Object, typename std::vector< Index >::const_iterator >::Discrete_Iterator end;
// };
// 
// 
// template< typename Index, typename Object >
// std::vector< Object > File_Handle< Index, Object >::obj_with_idx(Index idx)
// {
//   std::vector< Object > result;
// 
//   while (!(it == end) && it.index() < idx)
//     ++it;
//   while (!(it == end) && !(idx < it.index()))
//   {
//     result.push_back(it.object());
//     ++it;
//   }
// 
//   return result;
// }
// 
// 
// template< typename Index, typename Object >
// std::vector< Index > idx_list(const std::map< Index, Object >& arg)
// {
//   std::vector< Index > result;
//   result.reserve(arg.size());
//   for (const auto& i : arg)
//     result.push_back(i.first);
//   return result;
// }
// 
// 
// class Perflog_Tree
// {
// public:
//   Perflog_Tree(const std::string& name) : starttime(clock())
//   {
//     std::cerr<<"("<<name<<' ';
//   }
// 
//   ~Perflog_Tree()
//   {
//     std::cerr<<(clock() - starttime)/1000<<") ";
//   }
// 
// private:
//   clock_t starttime;
// };


//template Pre_Event_Refs

void update_ways(Transaction& transaction, Data_From_Osc& new_data)
{
  //needs: id |-> (begin, new_pos, new_mult)+
  //TODO: new_data.compute_way_geometry(node_delta);
  //doch nicht, stattdessen parent_geom

//   Perflog_Tree perf("update_nodes");
//   std::cerr<<new_data.nodes.data.size()<<' '<<new_data.ways.data.size()<<'\n';

  // before the first pass by idx
  Mapfile_IO mapfile_io(transaction); //< Way_Skeleton, Pre_Event_Refs depends on, filenames >
//   std::unique_ptr< Perflog_Tree > dyn_perf(new Perflog_Tree("new_data.node_pre_events"));
  //TODO: timestamp_start?
  Pre_Event_List pre_events = new_data.way_pre_events();
//   std::cerr<<pre_events.data.size()<<' '<<pre_events.timestamp_last_not_deleted.size()<<'\n';
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("new_data.pre_event_refs_by_idx"));
  std::map< Uint31_Index, Pre_Event_Refs > pre_event_refs_by_idx;// new_data.parent_geom_for_ways();
//   std::cerr<<pre_event_refs_by_idx.size()<<' ';
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("mapfile_io.read_idx_list"));
  mapfile_io.read_idx_list(new_data.way_pre_event_refs(pre_events), pre_event_refs_by_idx);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("idx_list ff"));
  auto req = idx_list(pre_event_refs_by_idx);
//   std::cerr<<pre_event_refs_by_idx.size()<<' '<<req.size()<<'\n';
// 
//   std::map< Uint31_Index, Node_Skeletons_Per_Idx > skels_per_idx;
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("nodes_bin ff"));
// 
//   File_Handle< Uint31_Index, Way_Skeleton > ways_bin(
//       transaction.data_index(osm_base_settings().NODES), req);
//   File_Handle< Uint31_Index, Attic< Way_Skeleton > > ways_attic_bin(
//       transaction.data_index(attic_settings().NODES), req);
//   File_Handle< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > ways_meta_bin(
//       transaction.data_index(meta_settings().NODES_META), req);
//   File_Handle< Uint31_Index, OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > ways_attic_meta_bin(
//       transaction.data_index(attic_settings().NODES_META), req);
//   File_Handle< Uint31_Index, Attic< Way_Skeleton::Id_Type > > ways_undeleted_bin(
//       transaction.data_index(attic_settings().NODES_UNDELETED), req);
// 
//   std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > >
//       ways_meta_to_move_to_attic;
// 
//   //NB: indirection necessary to convey timestamp_end between different idxs if any.
// 
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("first pass by idx"));
  for (auto i_idx : pre_event_refs_by_idx)
  {
    Uint31_Index working_idx = i_idx.first;
    Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];

    std::vector< Way_Skeleton > current_ways = ways_bin.obj_with_idx(working_idx);
    std::vector< Attic< Way_Skeleton > > attic_ways = ways_attic_bin.obj_with_idx(working_idx);

    std::sort(current_ways.begin(), current_ways.end());
    std::sort(attic_ways.begin(), attic_ways.end());

    //TODO: in: per Idx (begin, id, old_ll_lower, visible_after, coord_after)
    //TODO: out: {(idx, elem, begin, end)}
    //TODO: implizit geänderte Ways, Idx-übergreifend, Meta mitgeben
    //TODO: pre_event-Fiktion? ggf. vorne abschneiden per first_appearance, meta füllen
    //needs: node_pos by old idx: old pos, id, new_pos, multiplicity, begin, end
//     Id_Dates coord_sharing_ids;
//     Way_Skeleton_Updater::extract_relevant_current(
//         i_idx.second, coord_sharing_ids, coord_dates_per_idx, current_ways).swap(skels.current);
//     Way_Skeleton_Updater::extract_relevant_attic(
//         i_idx.second, coord_sharing_ids, coord_dates_per_idx, attic_ways).swap(skels.attic);

    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > current_meta =
        ways_meta_bin.obj_with_idx(working_idx);
    std::vector< OSM_Element_Metadata_Skeleton< Way_Skeleton::Id_Type > > attic_meta =
        ways_attic_meta_bin.obj_with_idx(working_idx);

    std::sort(current_meta.begin(), current_meta.end());
    std::sort(attic_meta.begin(), attic_meta.end());

    // require sorted implicit_pre_events
    Update_Events_Preparer::extract_and_apply_first_appearance(
        i_idx.second, implicit_pre_events, current_meta, attic_meta).swap(skels.first_appearance);
    Update_Events_Preparer::extract_and_apply_relevant_undeleted(
        i_idx.second, implicit_pre_events, ways_undeleted_bin.obj_with_idx(working_idx)).swap(skels.undeleted);

      //TODO: redistribute found_implicit_pre_events (and corresponding meta) to proper idxs
      //Hinweis: es gibt nur ein Current-Objekt (schon wegen Mapfile)
      //Querverschub in Current und Attic
      //da in Attic unklar, ob Meta schon vorhanden, solche Objekte als Vorsichtslöschung
// 
       //note: not relevant for found_implicit_pre_events
    Way_Meta_Updater::adapt_pre_event_list(working_idx, current_meta, i_idx.second, pre_events);
    Way_Meta_Updater::adapt_pre_event_list(working_idx, attic_meta, i_idx.second, pre_events);

    Way_Meta_Updater::collect_current_meta_to_move(
        i_idx.second, pre_events, current_meta, ways_meta_to_move_to_attic[working_idx]);
  }
//   std::cerr<<"A "<<skels_per_idx.size()<<' '<<coord_dates.size()<<'\n';
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("nodes_meta_to_add ff"));
// 
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
      nodes_meta_to_add;
  std::map< Uint31_Index, std::set< OSM_Element_Metadata_Skeleton< Node_Skeleton::Id_Type > > >
      nodes_attic_meta_to_add;
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("Node_Meta_Updater::create_update_for_nodes_meta"));
  Node_Meta_Updater::create_update_for_nodes_meta(
      pre_events, nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);
// 
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("mapfile_io.compute_and_write_idx_lists"));
  // compute nodes_map, nodes_attic_map and nodes_idx_lists from pre_event_refs_by_idx, nodes_meta_to_add, and nodes_attic_meta_to_add
  // write meta, nodes_map, nodes_attic_map, nodes_idx_lists
  mapfile_io.compute_and_write_idx_lists(nodes_meta_to_move_to_attic, nodes_meta_to_add, nodes_attic_meta_to_add);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(meta)"));
  update_elements(nodes_meta_to_move_to_attic, nodes_meta_to_add, transaction, *meta_settings().NODES_META);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(attic_meta)"));
  update_elements(
      decltype(nodes_attic_meta_to_add)(), nodes_attic_meta_to_add, transaction, *attic_settings().NODES_META);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("nodes_to_delete ff"));
// 
//   std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_delete;
//   std::map< Uint31_Index, std::set< Node_Skeleton > > nodes_to_add;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_delete;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton > > > nodes_attic_to_add;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_delete;
//   std::map< Uint31_Index, std::set< Attic< Node_Skeleton::Id_Type > > > nodes_undelete_to_add;
// 
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("second pass by idx"));
//   for (auto i_idx : pre_event_refs_by_idx)
//   {
//     Uint31_Index working_idx = i_idx.first;
//     Node_Skeletons_Per_Idx& skels = skels_per_idx[working_idx];
// 
//     Node_Event_List events(working_idx, skels, i_idx.second, pre_events);
//     Prepare_Node_Update::create_update_for_nodes(
//         events, nodes_to_delete[working_idx], nodes_to_add[working_idx]);
//     Prepare_Node_Update::create_update_for_nodes_attic(
//         events, nodes_attic_to_delete[working_idx], nodes_attic_to_add[working_idx]);
//     Prepare_Node_Update::create_update_for_nodes_undelete(
//         events, nodes_undelete_to_delete[working_idx], nodes_undelete_to_add[working_idx]);
//   }
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(nodes)"));
// 
//   // write nodes, nodes_attic, nodes_undelete
//   update_elements(nodes_to_delete, nodes_to_add, transaction, *osm_base_settings().NODES);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(nodes_attic)"));
//   update_elements(nodes_attic_to_delete, nodes_attic_to_add, transaction, *attic_settings().NODES);
//   dyn_perf.reset(0);
//   dyn_perf.reset(new Perflog_Tree("update_elements(nodes_undeleted)"));
//   update_elements(
//       nodes_undelete_to_delete, nodes_undelete_to_add, transaction, *attic_settings().NODES_UNDELETED);

  //TODO: tags, nodes_for_ways
}
